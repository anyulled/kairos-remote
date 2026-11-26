#include "power/power_manager.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "ble/hid_keyboard.h"
#include "board_pins.h"
#include "drivers/display_spd2010.h"
#include "ui/nav_math.h"
#include "ui/ui_manager.h"

namespace {

constexpr int64_t BAT_SAMPLE_INTERVAL_US = 5000000LL; // 5 s
constexpr int64_t DIM_TIMEOUT_US = static_cast<int64_t>(DIM_TIMEOUT_MS) * 1000LL;
constexpr int64_t SLEEP_TIMEOUT_US = static_cast<int64_t>(SLEEP_TIMEOUT_MS) * 1000LL;
constexpr int ADC_SAMPLES = 16;

float s_filteredMv = 4000.0f;
uint8_t s_batteryPct = 100;
int64_t s_lastActivityUs = 0;
int64_t s_lastBatReadUs = 0;
bool s_dimmed = false;

float readRawBatteryMv() {
    uint32_t acc = 0;
    for (int i = 0; i < ADC_SAMPLES; ++i) {
        acc += analogReadMilliVolts(PIN_BAT_ADC);
    }
    const float avgAdcMv = static_cast<float>(acc) / static_cast<float>(ADC_SAMPLES);
    return avgAdcMv * BAT_ADC_DIVIDER;
}

void publishBattery(float mv) {
    const uint16_t clampedMv = static_cast<uint16_t>(mv < 0.0f ? 0.0f : mv);
    s_batteryPct = KairosMath::voltageMvToPercent(clampedMv);
    const bool charging = (clampedMv >= 4210U);
    UiManager::updateBattery(s_batteryPct, charging);
    BleHidKeyboard::setBatteryLevel(s_batteryPct);
}

} // namespace

namespace PowerManager {

void begin() {
    pinMode(PIN_POWER_HOLD, OUTPUT);
    digitalWrite(PIN_POWER_HOLD, HIGH);
    pinMode(PIN_PWR_BTN, INPUT_PULLUP);

    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);

    s_filteredMv = readRawBatteryMv();
    publishBattery(s_filteredMv);

    const int64_t nowUs = esp_timer_get_time();
    s_lastActivityUs = nowUs;
    s_lastBatReadUs = nowUs;
    s_dimmed = false;
}

void notifyTouchActivity() {
    s_lastActivityUs = esp_timer_get_time();
    if (s_dimmed) {
        DisplaySpd2010::setBrightness(FULL_BRIGHTNESS_PCT);
        s_dimmed = false;
    }
}

void update(bool bleConnected) {
    const int64_t nowUs = esp_timer_get_time();

    if ((nowUs - s_lastBatReadUs) >= BAT_SAMPLE_INTERVAL_US) {
        s_lastBatReadUs = nowUs;
        const float sampleMv = readRawBatteryMv();
        s_filteredMv = (0.80f * s_filteredMv) + (0.20f * sampleMv);
        publishBattery(s_filteredMv);
    }

    const int64_t idleUs = nowUs - s_lastActivityUs;
    if (!s_dimmed && idleUs >= DIM_TIMEOUT_US) {
        DisplaySpd2010::setBrightness(DIM_BRIGHTNESS_PCT);
        s_dimmed = true;
    }

    if (!bleConnected && idleUs >= SLEEP_TIMEOUT_US) {
        Serial.println("[POWER] Idle timeout reached without BLE; entering Deep Sleep");
        enterDeepSleep();
    }
}

uint8_t getBatteryPercent() {
    return s_batteryPct;
}

uint16_t getBatteryMillivolts() {
    return static_cast<uint16_t>(s_filteredMv);
}

[[noreturn]] void enterDeepSleep() {
    DisplaySpd2010::enterSleep();
    BleHidKeyboard::end();

    // Keep battery power latch enabled across Deep Sleep so PWR button can wake the SoC.
    gpio_hold_en(static_cast<gpio_num_t>(PIN_POWER_HOLD));
    gpio_deep_sleep_hold_en();

    const gpio_num_t wakePin = static_cast<gpio_num_t>(PIN_PWR_BTN);
    rtc_gpio_init(wakePin);
    rtc_gpio_set_direction(wakePin, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(wakePin);
    rtc_gpio_pulldown_dis(wakePin);
    esp_sleep_enable_ext0_wakeup(wakePin, 0); // Active LOW

    Serial.flush();
    esp_deep_sleep_start();
}

} // namespace PowerManager
