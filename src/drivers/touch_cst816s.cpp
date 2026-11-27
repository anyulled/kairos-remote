#include "drivers/touch_cst816s.h"

#ifdef KAIROS_BOARD_WAVESHARE_128

#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>

#include "board_pins.h"

namespace {

constexpr uint32_t TP_IDLE_RELEASE_US = 60000;
constexpr uint8_t REG_GESTURE_ID = 0x01;
constexpr uint8_t REG_CHIP_ID = 0xA7;
constexpr uint8_t REG_FW_VER = 0xA9;

volatile bool s_irqFlag = false;
int16_t s_lastX = LCD_WIDTH / 2;
int16_t s_lastY = LCD_HEIGHT / 2;
int64_t s_lastTouchUs = 0;
bool s_isDown = false;
bool s_logSerial = false;
TouchCst816s::ActivityCallback s_activityCb = nullptr;

void IRAM_ATTR onTouchIrq() {
    s_irqFlag = true;
}

bool readRegs(uint8_t reg, uint8_t *buf, size_t len) {
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(true) != 0) {
        return false;
    }
    const size_t received = Wire.requestFrom(static_cast<uint8_t>(TOUCH_I2C_ADDR), static_cast<uint8_t>(len), static_cast<uint8_t>(true));
    if (received != len) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        buf[i] = static_cast<uint8_t>(Wire.read());
    }
    return true;
}

void lvglTouchReadCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    int16_t x = 0;
    int16_t y = 0;
    const bool pressed = TouchCst816s::readPoint(x, y);
    data->point.x = x;
    data->point.y = y;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

} // namespace

namespace TouchCst816s {

bool begin() {
    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
    pinMode(PIN_TOUCH_RST, OUTPUT);

    digitalWrite(PIN_TOUCH_RST, HIGH);
    delay(20);
    digitalWrite(PIN_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(PIN_TOUCH_RST, HIGH);
    delay(60);

    attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), onTouchIrq, FALLING);

    uint8_t chipId = 0;
    uint8_t fwVer = 0;
    const bool chipOk = readRegs(REG_CHIP_ID, &chipId, 1);
    const bool fwOk = readRegs(REG_FW_VER, &fwVer, 1);
    if (chipOk && fwOk) {
        Serial.printf("[TOUCH] CST816S chip=0x%02X fw=0x%02X\n", chipId, fwVer);
        return true;
    }

    Serial.println("[TOUCH] WARNING: CST816S probe at 0x15 did not ACK yet");
    return false;
}

bool readPoint(int16_t &x, int16_t &y) {
    const int64_t nowUs = esp_timer_get_time();

    if (s_irqFlag || s_isDown || digitalRead(PIN_TOUCH_INT) == LOW) {
        s_irqFlag = false;
        uint8_t raw[6] = {};
        if (readRegs(REG_GESTURE_ID, raw, sizeof(raw))) {
            const uint8_t points = raw[1];
            const uint8_t event = raw[2] >> 6;
            const int16_t rx = static_cast<int16_t>(((raw[2] & 0x0F) << 8) | raw[3]);
            const int16_t ry = static_cast<int16_t>(((raw[4] & 0x0F) << 8) | raw[5]);

            if (points > 0 && event != 1 && rx >= 0 && rx < LCD_WIDTH && ry >= 0 && ry < LCD_HEIGHT) {
                s_lastX = rx;
                s_lastY = ry;
                s_lastTouchUs = nowUs;
                if (!s_isDown && s_logSerial) {
                    Serial.printf("[TOUCH] DOWN x=%d y=%d\n", s_lastX, s_lastY);
                }
                s_isDown = true;
                if (s_activityCb) {
                    s_activityCb(s_lastX, s_lastY, true);
                }
            } else if (s_isDown) {
                s_isDown = false;
                if (s_logSerial) {
                    Serial.printf("[TOUCH] UP x=%d y=%d\n", s_lastX, s_lastY);
                }
                if (s_activityCb) {
                    s_activityCb(s_lastX, s_lastY, false);
                }
            }
        } else if (s_isDown && (nowUs - s_lastTouchUs) >= TP_IDLE_RELEASE_US) {
            s_isDown = false;
            if (s_activityCb) {
                s_activityCb(s_lastX, s_lastY, false);
            }
        }
    } else if (s_isDown && (nowUs - s_lastTouchUs) >= TP_IDLE_RELEASE_US) {
        s_isDown = false;
        if (s_activityCb) {
            s_activityCb(s_lastX, s_lastY, false);
        }
    }

    x = s_lastX;
    y = s_lastY;
    return s_isDown;
}

void setActivityCallback(ActivityCallback cb) {
    s_activityCb = cb;
}

void setSerialLogEnabled(bool enabled) {
    s_logSerial = enabled;
}

lv_indev_t *registerLvglTouch(uint8_t scrollLimit, uint8_t scrollThrow) {
    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = lvglTouchReadCb;
    indevDrv.scroll_limit = scrollLimit;
    indevDrv.scroll_throw = scrollThrow;
    return lv_indev_drv_register(&indevDrv);
}

} // namespace TouchCst816s

#endif // KAIROS_BOARD_WAVESHARE_128
