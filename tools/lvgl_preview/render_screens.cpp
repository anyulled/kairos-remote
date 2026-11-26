#include <lvgl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_pins.h"
#include "ui/screen_nav.h"
#include "ui/screen_stage.h"
#include "ui/screen_timer.h"
#include "ui/ui_manager.h"

namespace {

uint16_t s_fb[LCD_WIDTH * LCD_HEIGHT];

void flushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    const uint16_t *src = reinterpret_cast<const uint16_t *>(color_p);
    for (int32_t y = area->y1; y <= area->y2; ++y) {
        for (int32_t x = area->x1; x <= area->x2; ++x) {
            if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
                s_fb[y * LCD_WIDTH + x] = *src;
            }
            ++src;
        }
    }
    lv_disp_flush_ready(drv);
}

void writePpm(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", LCD_WIDTH, LCD_HEIGHT);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; ++i) {
        // LV_COLOR_16_SWAP = 1 swaps bytes in RGB565
        const uint16_t raw = s_fb[i];
        const uint16_t rgb565 = static_cast<uint16_t>((raw >> 8) | (raw << 8));
        const uint8_t r = static_cast<uint8_t>(((rgb565 >> 11) & 0x1F) * 255 / 31);
        const uint8_t g = static_cast<uint8_t>(((rgb565 >> 5) & 0x3F) * 255 / 63);
        const uint8_t b = static_cast<uint8_t>((rgb565 & 0x1F) * 255 / 31);
        fputc(r, f);
        fputc(g, f);
        fputc(b, f);
    }
    fclose(f);
}

void renderTile(uint8_t idx, const char *outPath) {
    UiManager::setActiveTile(idx, false);
    lv_timer_handler();
    lv_refr_now(nullptr);
    writePpm(outPath);
    printf("Rendered tile %u -> %s\n", idx, outPath);
}

} // namespace

int main(int argc, char **argv) {
    const char *outDir = (argc > 1) ? argv[1] : ".";
    lv_init();

    static lv_disp_draw_buf_t drawBuf;
    static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT];
    lv_disp_draw_buf_init(&drawBuf, buf, nullptr, LCD_WIDTH * LCD_HEIGHT);

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = LCD_WIDTH;
    dispDrv.ver_res = LCD_HEIGHT;
    dispDrv.flush_cb = flushCb;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    UiManager::begin();
    ScreenNav::create(UiManager::getTile(0));
    ScreenStage::create(UiManager::getTile(1));
    ScreenTimer::create(UiManager::getTile(2));

    UiManager::updateBleStatus(true);
    UiManager::updateBattery(85, false);
    ScreenTimer::setRemainingSeconds(15 * 60);

    char path[256];
    snprintf(path, sizeof(path), "%s/screen1_nav.ppm", outDir);
    renderTile(0, path);
    snprintf(path, sizeof(path), "%s/screen2_stage.ppm", outDir);
    renderTile(1, path);
    snprintf(path, sizeof(path), "%s/screen3_timer.ppm", outDir);
    renderTile(2, path);

    return 0;
}
