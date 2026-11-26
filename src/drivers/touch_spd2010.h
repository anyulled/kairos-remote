#ifndef KAIROS_TOUCH_SPD2010_H
#define KAIROS_TOUCH_SPD2010_H

#include <lvgl.h>
#include <stdint.h>

/**
 * Capacitive touch driver for the Solomon Systech SPD2010 over I2C (0x53),
 * with 40 ms release debouncing and optional activity/coordinate callbacks.
 */
namespace TouchSpd2010 {

using ActivityCallback = void (*)(int16_t x, int16_t y, bool pressed);

bool begin();
bool readPoint(int16_t &x, int16_t &y);
void setActivityCallback(ActivityCallback cb);
void setSerialLogEnabled(bool enabled);
lv_indev_t *registerLvglTouch(uint8_t scrollLimit = 28, uint8_t scrollThrow = 14);

} // namespace TouchSpd2010

#endif // KAIROS_TOUCH_SPD2010_H
