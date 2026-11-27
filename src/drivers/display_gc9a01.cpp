#include "drivers/display_gc9a01.h"

#ifdef KAIROS_BOARD_WAVESHARE_128

#include <Arduino.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include <string.h>

#include "board_pins.h"

namespace {

constexpr uint8_t CMD_SLPIN = 0x10;
constexpr uint8_t CMD_SLPOUT = 0x11;
constexpr uint8_t CMD_INVON = 0x21;
constexpr uint8_t CMD_DISPOFF = 0x28;
constexpr uint8_t CMD_DISPON = 0x29;
constexpr uint8_t CMD_CASET = 0x2A;
constexpr uint8_t CMD_RASET = 0x2B;
constexpr uint8_t CMD_RAMWR = 0x2C;
constexpr uint8_t CMD_MADCTL = 0x36;
constexpr uint8_t CMD_COLMOD = 0x3A;

constexpr size_t DRAW_BUF_LINES = 40;
constexpr size_t DMA_CHUNK_LINES = 40;
constexpr size_t DMA_CHUNK_BYTES = LCD_WIDTH * DMA_CHUNK_LINES * sizeof(uint16_t);

spi_device_handle_t s_spi = nullptr;
uint8_t *s_dmaBuf = nullptr;
uint8_t s_brightnessPct = FULL_BRIGHTNESS_PCT;

struct InitCmd {
    uint8_t cmd;
    uint8_t data[12];
    uint8_t len;
    uint16_t delayMs;
};

// Official Waveshare GC9A01A 240x240 initialization sequence (RGB565, BGR panel order, INVON).
const InitCmd kInitSequence[] = {
    {0xEF, {}, 0, 0},
    {0xEB, {0x14}, 1, 0},
    {0xFE, {}, 0, 0},
    {0xEF, {}, 0, 0},
    {0xEB, {0x14}, 1, 0},
    {0x84, {0x40}, 1, 0},
    {0x85, {0xFF}, 1, 0},
    {0x86, {0xFF}, 1, 0},
    {0x87, {0xFF}, 1, 0},
    {0x88, {0x0A}, 1, 0},
    {0x89, {0x21}, 1, 0},
    {0x8A, {0x00}, 1, 0},
    {0x8B, {0x80}, 1, 0},
    {0x8C, {0x01}, 1, 0},
    {0x8D, {0x01}, 1, 0},
    {0x8E, {0xFF}, 1, 0},
    {0x8F, {0xFF}, 1, 0},
    {0xB6, {0x00, 0x20}, 2, 0},
    {CMD_MADCTL, {0x08}, 1, 0},
    {CMD_COLMOD, {0x05}, 1, 0},
    {0x90, {0x08, 0x08, 0x08, 0x08}, 4, 0},
    {0xBD, {0x06}, 1, 0},
    {0xBC, {0x00}, 1, 0},
    {0xFF, {0x60, 0x01, 0x04}, 3, 0},
    {0xC3, {0x13}, 1, 0},
    {0xC4, {0x13}, 1, 0},
    {0xC9, {0x22}, 1, 0},
    {0xBE, {0x11}, 1, 0},
    {0xE1, {0x10, 0x0E}, 2, 0},
    {0xDF, {0x21, 0x0C, 0x02}, 3, 0},
    {0xF0, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}, 6, 0},
    {0xF1, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}, 6, 0},
    {0xF2, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}, 6, 0},
    {0xF3, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}, 6, 0},
    {0xED, {0x1B, 0x0B}, 2, 0},
    {0xAE, {0x77}, 1, 0},
    {0xCD, {0x63}, 1, 0},
    {0x70, {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03}, 9, 0},
    {0xE8, {0x34}, 1, 0},
    {0x62, {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70}, 12, 0},
    {0x63, {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70}, 12, 0},
    {0x64, {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07}, 7, 0},
    {0x66, {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00}, 10, 0},
    {0x67, {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98}, 10, 0},
    {0x74, {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00}, 7, 0},
    {0x98, {0x3E, 0x07}, 2, 0},
    {0x35, {}, 0, 0},
    {CMD_INVON, {}, 0, 0},
    {CMD_SLPOUT, {}, 0, 120},
    {CMD_DISPON, {}, 0, 20},
};

esp_err_t writeCommandByte(uint8_t cmd) {
    digitalWrite(PIN_LCD_DC, LOW);
    spi_transaction_t t = {};
    t.flags = SPI_TRANS_USE_TXDATA;
    t.length = 8;
    t.tx_data[0] = cmd;
    return spi_device_polling_transmit(s_spi, &t);
}

esp_err_t writeDataBytes(const uint8_t *data, size_t len) {
    if (len == 0) {
        return ESP_OK;
    }
    digitalWrite(PIN_LCD_DC, HIGH);
    spi_transaction_t t = {};
    t.length = len * 8;
    if (len <= 4) {
        t.flags = SPI_TRANS_USE_TXDATA;
        memcpy(t.tx_data, data, len);
    } else {
        t.tx_buffer = data;
    }
    return spi_device_polling_transmit(s_spi, &t);
}

esp_err_t sendCmd(uint8_t cmd, const uint8_t *params, size_t len) {
    const esp_err_t err = writeCommandByte(cmd);
    if (err != ESP_OK || len == 0 || !params) {
        return err;
    }
    if (len <= 4) {
        return writeDataBytes(params, len);
    }
    if (!s_dmaBuf || len > DMA_CHUNK_BYTES) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(s_dmaBuf, params, len);
    return writeDataBytes(s_dmaBuf, len);
}

void lvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    DisplayGc9a01::drawBitmap(area->x1, area->y1, area->x2, area->y2,
                              reinterpret_cast<const uint16_t *>(color_p));
    lv_disp_flush_ready(drv);
}

} // namespace

namespace DisplayGc9a01 {

bool begin() {
    ledcSetup(LCD_BL_PWM_CHANNEL, LCD_BL_PWM_FREQ_HZ, LCD_BL_PWM_RES_BITS);
    ledcAttachPin(PIN_LCD_BL, LCD_BL_PWM_CHANNEL);
    setBrightness(0);

    pinMode(PIN_LCD_DC, OUTPUT);
    digitalWrite(PIN_LCD_DC, HIGH);

    pinMode(PIN_LCD_RST, OUTPUT);
    digitalWrite(PIN_LCD_RST, HIGH);
    delay(20);
    digitalWrite(PIN_LCD_RST, LOW);
    delay(20);
    digitalWrite(PIN_LCD_RST, HIGH);
    delay(120);

    spi_bus_config_t busCfg = {};
    busCfg.mosi_io_num = PIN_LCD_MOSI;
    busCfg.miso_io_num = -1;
    busCfg.sclk_io_num = PIN_LCD_SCLK;
    busCfg.quadwp_io_num = -1;
    busCfg.quadhd_io_num = -1;
    busCfg.max_transfer_sz = DMA_CHUNK_BYTES + 16;
    busCfg.flags = SPICOMMON_BUSFLAG_MASTER;

    if (spi_bus_initialize(SPI2_HOST, &busCfg, SPI_DMA_CH_AUTO) != ESP_OK) {
        return false;
    }

    spi_device_interface_config_t devCfg = {};
    devCfg.clock_speed_hz = LCD_SPI_FREQ_HZ;
    devCfg.mode = 0;
    devCfg.spics_io_num = PIN_LCD_CS;
    devCfg.queue_size = 4;

    if (spi_bus_add_device(SPI2_HOST, &devCfg, &s_spi) != ESP_OK) {
        return false;
    }

    s_dmaBuf = static_cast<uint8_t *>(heap_caps_malloc(DMA_CHUNK_BYTES, MALLOC_CAP_DMA));
    if (!s_dmaBuf) {
        return false;
    }

    for (const auto &entry : kInitSequence) {
        if (sendCmd(entry.cmd, entry.data, entry.len) != ESP_OK) {
            return false;
        }
        if (entry.delayMs > 0) {
            delay(entry.delayMs);
        }
    }

    setBrightness(FULL_BRIGHTNESS_PCT);
    Serial.println("[DISPLAY] GC9A01A 240x240 ready");
    return true;
}

void drawBitmap(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const uint16_t *pixels) {
    if (!s_spi || !s_dmaBuf || !pixels || x2 < x1 || y2 < y1) {
        return;
    }

    const uint8_t colData[4] = {
        static_cast<uint8_t>((x1 >> 8) & 0xFF),
        static_cast<uint8_t>(x1 & 0xFF),
        static_cast<uint8_t>((x2 >> 8) & 0xFF),
        static_cast<uint8_t>(x2 & 0xFF),
    };
    sendCmd(CMD_CASET, colData, sizeof(colData));

    const size_t width = static_cast<size_t>(x2 - x1 + 1);
    const size_t height = static_cast<size_t>(y2 - y1 + 1);
    const size_t rowBytes = width * sizeof(uint16_t);
    size_t maxStripRows = DMA_CHUNK_BYTES / rowBytes;
    if (maxStripRows == 0) {
        maxStripRows = 1;
    }

    const uint8_t *src = reinterpret_cast<const uint8_t *>(pixels);
    size_t rowOffset = 0;
    while (rowOffset < height) {
        const size_t remRows = height - rowOffset;
        const size_t stripRows = (remRows < maxStripRows) ? remRows : maxStripRows;
        const int16_t stripY1 = static_cast<int16_t>(y1 + static_cast<int16_t>(rowOffset));
        const int16_t stripY2 = static_cast<int16_t>(stripY1 + static_cast<int16_t>(stripRows) - 1);

        const uint8_t rowData[4] = {
            static_cast<uint8_t>((stripY1 >> 8) & 0xFF),
            static_cast<uint8_t>(stripY1 & 0xFF),
            static_cast<uint8_t>((stripY2 >> 8) & 0xFF),
            static_cast<uint8_t>(stripY2 & 0xFF),
        };
        sendCmd(CMD_RASET, rowData, sizeof(rowData));

        const size_t stripBytes = stripRows * rowBytes;
        memcpy(s_dmaBuf, src + (rowOffset * rowBytes), stripBytes);
        writeCommandByte(CMD_RAMWR);
        writeDataBytes(s_dmaBuf, stripBytes);
        rowOffset += stripRows;
    }
}

lv_disp_t *registerLvglDisplay() {
    static lv_disp_draw_buf_t drawBuf;
    static lv_disp_drv_t dispDrv;

    const size_t bufPixels = LCD_WIDTH * DRAW_BUF_LINES;
    lv_color_t *buf1 = static_cast<lv_color_t *>(
        heap_caps_malloc(bufPixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    lv_color_t *buf2 = static_cast<lv_color_t *>(
        heap_caps_malloc(bufPixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (!buf1) {
        buf1 = static_cast<lv_color_t *>(malloc(bufPixels * sizeof(lv_color_t)));
    }

    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, bufPixels);
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = LCD_WIDTH;
    dispDrv.ver_res = LCD_HEIGHT;
    dispDrv.flush_cb = lvglFlushCb;
    dispDrv.draw_buf = &drawBuf;

    return lv_disp_drv_register(&dispDrv);
}

void setBrightness(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    s_brightnessPct = percent;
    const uint32_t maxDuty = (1U << LCD_BL_PWM_RES_BITS) - 1U;
    const uint32_t duty = (static_cast<uint32_t>(percent) * maxDuty) / 100U;
    ledcWrite(LCD_BL_PWM_CHANNEL, duty);
}

uint8_t getBrightness() {
    return s_brightnessPct;
}

void enterSleep() {
    setBrightness(0);
    if (s_spi) {
        sendCmd(CMD_DISPOFF, nullptr, 0);
        delay(20);
        sendCmd(CMD_SLPIN, nullptr, 0);
        delay(120);
    }
}

void exitSleep() {
    if (s_spi) {
        sendCmd(CMD_SLPOUT, nullptr, 0);
        delay(120);
        sendCmd(CMD_DISPON, nullptr, 0);
        delay(20);
    }
    setBrightness(FULL_BRIGHTNESS_PCT);
}

} // namespace DisplayGc9a01

#endif // KAIROS_BOARD_WAVESHARE_128
