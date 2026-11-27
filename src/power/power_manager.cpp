#include "power/power_manager.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "ble/hid_keyboard.h"
#include "board_pins.h"
#include "drivers/board_hal.h"
#include "power/power_state.h"
#include "ui/ui_manager.h"

namespace {

constexpr int ADC_SAMPLES = 16;
Kairos::PowerPolicy s_policy;

float readRawBatteryMv() {
    uint32_t acc = 0;
    for (int i = 0; i < ADC_SAMPLES; ++i) {
        acc += analogReadMilliVolts(PIN_BAT_ADC);
    }
    const float avgAdcMv = static_cast<float>(acc) / static_cast<float>(ADC_SAMPLES);
    return avgAdcMv * BAT_ADC_DIVIDER;
}

void publishBattery() {
    const uint8_t pct = s_policy.batteryPercent();
    UiManager::updateBattery(pct, s_policy.isProbableCharging());
    BleHidKeyboard::setBatteryLevel(pct);
}

} // namespace

namespace PowerManager {

void begin() {
#if KAIROS_HAS_POWER_HOLD
    pinMode(PIN_POWER_HOLD, OUTPUT);
    digitalWrite(PIN_POWER_HOLD, HIGH);
#endif
    pinMode(PIN_WAKE_BTN, INPUT_PULLUP);

    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);

    s_policy.reset(esp_timer_get_time(), readRawBatteryMv());
    publishBattery();
}

void notifyTouchActivity() {
    if (s_policy.notifyTouch(esp_timer_get_time())) {
        DisplayHal::setBrightness(FULL_BRIGHTNESS_PCT);
    }
}

void update(bool bleConnected) {
    const Kairos::PowerStepResult step = s_policy.evaluate(esp_timer_get_time(), bleConnected);

    if (step.batterySampleDue) {
        s_policy.recordBatterySample(readRawBatteryMv());
        publishBattery();
    }

    if (step.brightnessChanged) {
        DisplayHal::setBrightness(step.targetBrightnessPct);
    }

    if (step.enterDeepSleep) {
        Serial.println("[POWER] Idle timeout reached without BLE; entering Deep Sleep");
        enterDeepSleep();
    }
}

uint8_t getBatteryPercent() {
    return s_policy.batteryPercent();
}

uint16_t getBatteryMillivolts() {
    return s_policy.batteryMillivolts();
}

[[noreturn]] void enterDeepSleep() {
    DisplayHal::enterSleep();
    BleHidKeyboard::end();

#if KAIROS_HAS_POWER_HOLD
    gpio_hold_en(static_cast<gpio_num_t>(PIN_POWER_HOLD));
    gpio_deep_sleep_hold_en();
#endif

    const gpio_num_t wakePin = static_cast<gpio_num_t>(PIN_WAKE_BTN);
    rtc_gpio_init(wakePin);
    rtc_gpio_set_direction(wakePin, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(wakePin);
    rtc_gpio_pulldown_dis(wakePin);
    esp_sleep_enable_ext0_wakeup(wakePin, 0); // Active LOW

    Serial.flush();
    esp_deep_sleep_start();
}

} // namespace PowerManager
