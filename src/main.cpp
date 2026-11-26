#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "audio/haptic_click.h"
#include "ble/hid_keyboard.h"
#include "board_pins.h"
#include "drivers/display_spd2010.h"
#include "drivers/tca9554.h"
#include "drivers/touch_spd2010.h"
#include "ui/screen_nav.h"
#include "ui/screen_stage.h"
#include "ui/screen_timer.h"
#include "ui/ui_manager.h"

namespace {

bool s_lastBleConnected = false;

} // namespace

void setup() {
    // Latch battery power immediately on battery boot.
    pinMode(PIN_POWER_HOLD, OUTPUT);
    digitalWrite(PIN_POWER_HOLD, HIGH);

    Serial.begin(115200);
    delay(200);
    Serial.println("[KAIROS] Booting Kairos-remote...");

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ_HZ);
    if (!Tca9554::begin()) {
        Serial.println("[KAIROS] ERROR: TCA9554 init failed");
    }

    if (!DisplaySpd2010::begin()) {
        Serial.println("[KAIROS] ERROR: SPD2010 display init failed");
    }

    TouchSpd2010::begin();
    HapticAudio::begin();
    BleHidKeyboard::begin("Kairos-remote");

    lv_init();
    DisplaySpd2010::registerLvglDisplay();
    TouchSpd2010::registerLvglTouch(28, 14);

    UiManager::begin();
    ScreenNav::create(UiManager::getTile(0));
    ScreenStage::create(UiManager::getTile(1));
    ScreenTimer::create(UiManager::getTile(2));

    s_lastBleConnected = BleHidKeyboard::isConnected();
    UiManager::updateBleStatus(s_lastBleConnected);
    Serial.println("[KAIROS] All 3 views ready");
}

void loop() {
    const bool bleNow = BleHidKeyboard::isConnected();
    if (bleNow != s_lastBleConnected) {
        s_lastBleConnected = bleNow;
        UiManager::updateBleStatus(bleNow);
    }
    lv_timer_handler();
    delay(5);
}
