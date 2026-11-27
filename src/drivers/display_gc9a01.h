#ifndef KAIROS_DISPLAY_GC9A01_H
#define KAIROS_DISPLAY_GC9A01_H

#include <lvgl.h>
#include <stdint.h>

/**
 * 4-wire SPI display driver for the GalaxyCore GC9A01A 240x240 circular IPS panel
 * on the Waveshare ESP32-S3-Touch-LCD-1.28, with PSRAM/DMA LVGL draw buffers.
 */
namespace DisplayGc9a01 {

bool begin();
lv_disp_t *registerLvglDisplay();
void drawBitmap(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const uint16_t *pixels);
void setBrightness(uint8_t percent);
uint8_t getBrightness();
void enterSleep();
void exitSleep();

} // namespace DisplayGc9a01

#endif // KAIROS_DISPLAY_GC9A01_H
