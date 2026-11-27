#include "drivers/touch_spd2010.h"

#if !defined(KAIROS_BOARD_WAVESHARE_128)

#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>

#include "board_pins.h"
#include "drivers/tca9554.h"

namespace {

constexpr uint32_t TP_LIFT_US = 40000; // Debounce transient empty reads during hold
constexpr size_t SPD2010_MAX_HDP_BYTES = 64;

struct TpStatus {
    bool ptExist = false;
    bool gesture = false;
    bool aux = false;
    bool ticBusy = false;
    bool ticInBios = false;
    bool ticInCpu = false;
    bool tintLow = false;
    bool cpuRun = false;
    uint16_t readLen = 0;
};

int16_t s_lastX = LCD_WIDTH / 2;
int16_t s_lastY = LCD_HEIGHT / 2;
int64_t s_lastTouchUs = 0;
bool s_isDown = false;
bool s_logSerial = false;
TouchSpd2010::ActivityCallback s_activityCb = nullptr;

bool i2cWriteBytes(const uint8_t *data, size_t len) {
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(data, len);
    return Wire.endTransmission() == 0;
}

bool i2cReadBytes(uint8_t *buf, size_t len) {
    const size_t received = Wire.requestFrom(static_cast<uint8_t>(TOUCH_I2C_ADDR), static_cast<uint8_t>(len));
    if (received != len) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        buf[i] = static_cast<uint8_t>(Wire.read());
    }
    return true;
}

bool writeCmd4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    const uint8_t cmd[4] = {b0, b1, b2, b3};
    const bool ok = i2cWriteBytes(cmd, sizeof(cmd));
    delayMicroseconds(200);
    return ok;
}

bool readReg16(uint8_t regHi, uint8_t regLo, uint8_t *out, size_t len) {
    const uint8_t reg[2] = {regHi, regLo};
    if (!i2cWriteBytes(reg, sizeof(reg))) {
        return false;
    }
    delayMicroseconds(200);
    if (!i2cReadBytes(out, len)) {
        return false;
    }
    delayMicroseconds(200);
    return true;
}

bool writeClearInt() { return writeCmd4(0x02, 0x00, 0x01, 0x00); }
bool writeCpuStart() { return writeCmd4(0x04, 0x00, 0x01, 0x00); }
bool writePointMode() { return writeCmd4(0x50, 0x00, 0x00, 0x00); }
bool writeTpStart() { return writeCmd4(0x46, 0x00, 0x00, 0x00); }

bool readStatusLength(TpStatus &st) {
    uint8_t buf[4] = {};
    if (!readReg16(0x20, 0x00, buf, sizeof(buf))) {
        return false;
    }
    st.ptExist = (buf[0] & 0x01) != 0;
    st.gesture = (buf[0] & 0x02) != 0;
    st.aux = (buf[0] & 0x08) != 0;
    st.ticBusy = (buf[1] & 0x80) != 0;
    st.ticInBios = (buf[1] & 0x40) != 0;
    st.ticInCpu = (buf[1] & 0x20) != 0;
    st.tintLow = (buf[1] & 0x10) != 0;
    st.cpuRun = (buf[1] & 0x08) != 0;
    st.readLen = static_cast<uint16_t>((buf[3] << 8) | buf[2]);
    return true;
}

bool drainRemainingHdp() {
    for (int guard = 0; guard < 8; ++guard) {
        uint8_t hdpStatus[8] = {};
        if (!readReg16(0xFC, 0x02, hdpStatus, sizeof(hdpStatus))) {
            return false;
        }
        const uint8_t status = hdpStatus[5];
        const uint16_t nextLen = static_cast<uint16_t>(hdpStatus[2] | (hdpStatus[3] << 8));
        if (status == 0x82) {
            return writeClearInt();
        }
        if (status == 0x00 && nextLen > 0 && nextLen <= SPD2010_MAX_HDP_BYTES) {
            uint8_t scratch[SPD2010_MAX_HDP_BYTES] = {};
            if (!readReg16(0x00, 0x03, scratch, nextLen)) {
                return false;
            }
        } else {
            return writeClearInt();
        }
    }
    return writeClearInt();
}

bool pollRawTouch(int16_t &xOut, int16_t &yOut) {
    TpStatus st;
    if (!readStatusLength(st)) {
        return false;
    }

    if (st.ticInBios) {
        writeClearInt();
        writeCpuStart();
        return false;
    }
    if (st.ticInCpu) {
        writePointMode();
        writeTpStart();
        writeClearInt();
        return false;
    }
    if (st.cpuRun && st.readLen == 0) {
        writeClearInt();
        return false;
    }
    if ((st.ptExist || st.gesture) && st.readLen >= 5 && st.readLen <= SPD2010_MAX_HDP_BYTES) {
        uint8_t buf[SPD2010_MAX_HDP_BYTES] = {};
        bool gotPoint = false;
        if (readReg16(0x00, 0x03, buf, st.readLen)) {
            const uint8_t checkId = buf[4];
            if (checkId <= 0x0A && st.ptExist && ((st.readLen - 4) % 6 == 0)) {
                const uint8_t weight = buf[8];
                if (weight > 0) {
                    const int16_t x = static_cast<int16_t>(((buf[7] & 0xF0) << 4) | buf[5]);
                    const int16_t y = static_cast<int16_t>(((buf[7] & 0x0F) << 8) | buf[6]);
                    if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
                        xOut = x;
                        yOut = y;
                        gotPoint = true;
                    }
                }
            }
        }
        drainRemainingHdp();
        return gotPoint;
    }
    if (st.cpuRun && st.aux) {
        writeClearInt();
    }
    return false;
}

void lvglTouchReadCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    int16_t x = 0;
    int16_t y = 0;
    const bool pressed = TouchSpd2010::readPoint(x, y);
    data->point.x = x;
    data->point.y = y;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

} // namespace

namespace TouchSpd2010 {

bool begin() {
    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
    Tca9554::pulseReset(EXIO_TOUCH_RST_BIT, 20, 120);

    // Trigger initial state transition out of BIOS/CPU standby if needed.
    int16_t x = 0;
    int16_t y = 0;
    for (int i = 0; i < 4; ++i) {
        pollRawTouch(x, y);
        delay(10);
    }
    return true;
}

bool readPoint(int16_t &x, int16_t &y) {
    const int64_t nowUs = esp_timer_get_time();
    int16_t rawX = 0;
    int16_t rawY = 0;

    if (pollRawTouch(rawX, rawY)) {
        s_lastX = rawX;
        s_lastY = rawY;
        s_lastTouchUs = nowUs;
        if (!s_isDown && s_logSerial) {
            Serial.printf("[TOUCH] DOWN x=%d y=%d\n", s_lastX, s_lastY);
        }
        s_isDown = true;
        if (s_activityCb) {
            s_activityCb(s_lastX, s_lastY, true);
        }
    } else if (s_isDown && (nowUs - s_lastTouchUs) >= TP_LIFT_US) {
        s_isDown = false;
        if (s_logSerial) {
            Serial.printf("[TOUCH] UP x=%d y=%d\n", s_lastX, s_lastY);
        }
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

} // namespace TouchSpd2010

#endif // !KAIROS_BOARD_WAVESHARE_128
