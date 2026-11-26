#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "board_pins.h"
#include "ble/hid_keyboard.h"
#include "drivers/display_spd2010.h"
#include "drivers/tca9554.h"
#include "drivers/touch_spd2010.h"

namespace {

lv_obj_t *s_coordLabel = nullptr;

void onTestButtonClicked(lv_event_t *e) {
    (void)e;
    lv_point_t pt = {};
    lv_indev_t *indev = lv_indev_get_act();
    if (indev) {
        lv_indev_get_point(indev, &pt);
    }
    Serial.printf("[TEST_BTN] Clicked at x=%d y=%d\n", static_cast<int>(pt.x), static_cast<int>(pt.y));
    if (BleHidKeyboard::isConnected()) {
        BleHidKeyboard::sendKey(KEY_RIGHT_ARROW);
    }
    if (s_coordLabel) {
        lv_label_set_text_fmt(s_coordLabel, "OK (%d, %d)\n%s",
                              static_cast<int>(pt.x), static_cast<int>(pt.y),
                              BleHidKeyboard::isConnected() ? "BLE OK" : "BLE WAIT");
    }
}

void createHardwareTestUi() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B0F17), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 180, 180);
    lv_obj_center(btn);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0EA5E9), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284C7), LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn, onTestButtonClicked, LV_EVENT_CLICKED, nullptr);

    s_coordLabel = lv_label_create(btn);
    lv_label_set_text(s_coordLabel, "KAIROS\nTOUCH");
    lv_obj_set_style_text_align(s_coordLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_coordLabel, &lv_font_montserrat_24, 0);
    lv_obj_center(s_coordLabel);
}

} // namespace

void setup() {
    // Latch battery power immediately on battery boot.
    pinMode(PIN_POWER_HOLD, OUTPUT);
    digitalWrite(PIN_POWER_HOLD, HIGH);

    Serial.begin(115200);
    delay(200);
    Serial.println("[KAIROS] Booting Issue #1 bring-up...");

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ_HZ);
    if (!Tca9554::begin()) {
        Serial.println("[KAIROS] ERROR: TCA9554 init failed");
    }

    if (!DisplaySpd2010::begin()) {
        Serial.println("[KAIROS] ERROR: SPD2010 display init failed");
    }

    TouchSpd2010::begin();
    TouchSpd2010::setSerialLogEnabled(true);
    BleHidKeyboard::begin("Kairos-remote");

    lv_init();
    DisplaySpd2010::registerLvglDisplay();
    TouchSpd2010::registerLvglTouch();

    createHardwareTestUi();
    Serial.println("[KAIROS] LVGL ready");
}

void loop() {
    lv_timer_handler();
    delay(5);
}
