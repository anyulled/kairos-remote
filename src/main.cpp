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
#include "ui/ui_manager.h"

namespace {

bool s_lastBleConnected = false;

void addPlaceholderLabel(lv_obj_t *parent, const char *text) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(lbl);
}

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
    addPlaceholderLabel(UiManager::getTile(2), "3. Cronometro");

    s_lastBleConnected = BleHidKeyboard::isConnected();
    UiManager::updateBleStatus(s_lastBleConnected);
    Serial.println("[KAIROS] UI Tileview ready");
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
