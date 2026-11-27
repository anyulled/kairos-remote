#ifndef KAIROS_TOUCH_CST816S_H
#define KAIROS_TOUCH_CST816S_H

#include <lvgl.h>
#include <stdint.h>

/**
 * Capacitive touch driver for the Hynitron CST816S over I2C (0x15) on the
 * Waveshare ESP32-S3-Touch-LCD-1.28, driven by falling-edge IRQ on GPIO 5.
 */
namespace TouchCst816s {

using ActivityCallback = void (*)(int16_t x, int16_t y, bool pressed);

bool begin();
bool readPoint(int16_t &x, int16_t &y);
void setActivityCallback(ActivityCallback cb);
void setSerialLogEnabled(bool enabled);
lv_indev_t *registerLvglTouch(uint8_t scrollLimit = 16, uint8_t scrollThrow = 14);

} // namespace TouchCst816s

#endif // KAIROS_TOUCH_CST816S_H
