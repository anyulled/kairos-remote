#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "audio/haptic_click.h"
#include "ble/hid_keyboard.h"
#include "board_pins.h"
#include "drivers/board_hal.h"
#if KAIROS_HAS_TCA9554
#include "drivers/tca9554.h"
#endif
#include "power/power_manager.h"
#include "ui/screen_nav.h"
#include "ui/screen_stage.h"
#include "ui/screen_timer.h"
#include "ui/ui_manager.h"

namespace {

bool s_lastBleConnected = false;

void onTouchActivity(int16_t x, int16_t y, bool pressed) {
    (void)x;
    (void)y;
    if (pressed) {
        PowerManager::notifyTouchActivity();
    }
}

} // namespace

void setup() {
#if KAIROS_HAS_POWER_HOLD
    // Latch battery power immediately on battery boot.
    pinMode(PIN_POWER_HOLD, OUTPUT);
    digitalWrite(PIN_POWER_HOLD, HIGH);
#endif

    Serial.begin(115200);
    delay(200);
    Serial.println("[KAIROS] Booting Kairos-remote...");
    Serial.printf("[KAIROS] Board: %s | PSRAM: %u bytes\n",
                  KAIROS_BOARD_NAME,
                  static_cast<unsigned>(ESP.getPsramSize()));

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ_HZ);
#if KAIROS_HAS_TCA9554
    if (!Tca9554::begin()) {
        Serial.println("[KAIROS] ERROR: TCA9554 init failed");
    }
#endif

    if (!DisplayHal::begin()) {
        Serial.println("[KAIROS] ERROR: Display init failed");
    }

    TouchHal::begin();
    TouchHal::setActivityCallback(onTouchActivity);
    HapticAudio::begin();
    BleHidKeyboard::begin("Kairos-remote");

    lv_init();
    DisplayHal::registerLvglDisplay();
    TouchHal::registerLvglTouch(TOUCH_SCROLL_LIMIT, TOUCH_SCROLL_THROW);

    UiManager::begin();
    ScreenNav::create(UiManager::getTile(0));
    ScreenStage::create(UiManager::getTile(1));
    ScreenTimer::create(UiManager::getTile(2));

    PowerManager::begin();

    s_lastBleConnected = BleHidKeyboard::isConnected();
    UiManager::updateBleStatus(s_lastBleConnected);
    Serial.println("[KAIROS] Firmware ready");
}

void loop() {
    const bool bleNow = BleHidKeyboard::isConnected();
    if (bleNow != s_lastBleConnected) {
        s_lastBleConnected = bleNow;
        UiManager::updateBleStatus(bleNow);
    }
    PowerManager::update(bleNow);
    lv_timer_handler();
    delay(5);
}
