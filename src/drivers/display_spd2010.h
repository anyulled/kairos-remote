#ifndef KAIROS_DISPLAY_SPD2010_H
#define KAIROS_DISPLAY_SPD2010_H

#include <lvgl.h>
#include <stdint.h>

/**
 * QSPI display driver for the Solomon Systech SPD2010 412x412 circular IPS panel
 * and LVGL display registration with 4-pixel X alignment and PSRAM draw buffers.
 */
namespace DisplaySpd2010 {

bool begin();
lv_disp_t *registerLvglDisplay();
void drawBitmap(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const uint16_t *pixels);
void setBrightness(uint8_t percent);
uint8_t getBrightness();
void enterSleep();
void exitSleep();

} // namespace DisplaySpd2010

#endif // KAIROS_DISPLAY_SPD2010_H
