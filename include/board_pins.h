#ifndef KAIROS_BOARD_PINS_H
#define KAIROS_BOARD_PINS_H

#ifdef KAIROS_BOARD_WAVESHARE_128

/**
 * Hardware pinout and peripheral constants for Waveshare ESP32-S3-Touch-LCD-1.28.
 */
#define KAIROS_BOARD_NAME      "Waveshare ESP32-S3-Touch-LCD-1.28"
#define KAIROS_HAS_TCA9554     0
#define KAIROS_HAS_AUDIO       0
#define KAIROS_HAS_POWER_HOLD  0

// --- Display SPI (GC9A01A, 240x240 IPS RGB565) ---
#define LCD_WIDTH              240
#define LCD_HEIGHT             240
#define PIN_LCD_SCLK           10
#define PIN_LCD_MOSI           11
#define PIN_LCD_MISO           12
#define PIN_LCD_CS             9
#define PIN_LCD_DC             8
#define PIN_LCD_RST            14
#define PIN_LCD_BL             2
#define LCD_SPI_FREQ_HZ        (40 * 1000 * 1000)

// Backlight LEDC PWM configuration (Arduino-ESP32 v2 API)
#define LCD_BL_PWM_CHANNEL     0
#define LCD_BL_PWM_FREQ_HZ     5000
#define LCD_BL_PWM_RES_BITS    8

// --- Shared I2C Bus (Touch CST816S, QMI8658) ---
#define PIN_I2C_SDA            6
#define PIN_I2C_SCL            7
#define I2C_FREQ_HZ            400000

// --- Capacitive Touch (CST816S on I2C) ---
#define TOUCH_I2C_ADDR         0x15
#define PIN_TOUCH_INT          5
#define PIN_TOUCH_RST          13
#define TOUCH_SCROLL_LIMIT     16
#define TOUCH_SCROLL_THROW     14

// --- Power, Battery ADC, and Physical Buttons ---
#define PIN_BOOT_BTN           0    // Active LOW
#define PIN_WAKE_BTN           0    // Active LOW (BOOT button used for wakeup)
#define PIN_BAT_ADC            1    // ADC1_CH0 via 200k / 100k resistor divider
#define BAT_ADC_DIVIDER        3.0f // V_bat = V_adc * 3.0

#else

/**
 * Hardware pinout and peripheral constants for Waveshare ESP32-S3-Touch-LCD-1.46
 * (also compatible with 1.46B and 1.46C cover-glass variants).
 */
#define KAIROS_BOARD_NAME      "Waveshare ESP32-S3-Touch-LCD-1.46"
#define KAIROS_HAS_TCA9554     1
#define KAIROS_HAS_AUDIO       1
#define KAIROS_HAS_POWER_HOLD  1

// --- Display QSPI (SPD2010, 412x412 IPS RGB565) ---
#define LCD_WIDTH           412
#define LCD_HEIGHT          412
#define PIN_LCD_PCLK        40
#define PIN_LCD_D0          46
#define PIN_LCD_D1          45
#define PIN_LCD_D2          42
#define PIN_LCD_D3          41
#define PIN_LCD_CS          21
#define PIN_LCD_BL          5
#define LCD_QSPI_FREQ_HZ    (40 * 1000 * 1000)

// Backlight LEDC PWM configuration (Arduino-ESP32 v2 API)
#define LCD_BL_PWM_CHANNEL  0
#define LCD_BL_PWM_FREQ_HZ  5000
#define LCD_BL_PWM_RES_BITS 8

// --- Shared I2C Bus (Touch SPD2010, TCA9554, QMI8658, PCF85063) ---
#define PIN_I2C_SDA         11
#define PIN_I2C_SCL         10
#define I2C_FREQ_HZ         400000

// --- Capacitive Touch (SPD2010 on I2C) ---
#define TOUCH_I2C_ADDR      0x53
#define PIN_TOUCH_INT       4
#define TOUCH_SCROLL_LIMIT  28
#define TOUCH_SCROLL_THROW  14

// --- I/O Expander (TCA9554PWR on I2C) ---
#define TCA9554_I2C_ADDR    0x20
#define EXIO_TOUCH_RST_BIT  (1U << 0)  // EXIO1: TP_RST (active low)
#define EXIO_LCD_RST_BIT    (1U << 1)  // EXIO2: LCD_RST (active low)
#define EXIO_SD_CS_BIT      (1U << 2)  // EXIO3: TF Card D3/CS (keep HIGH when unused)
#define TCA9554_OUTPUT_MASK (EXIO_TOUCH_RST_BIT | EXIO_LCD_RST_BIT | EXIO_SD_CS_BIT)
// Bits 3..7 kept as inputs (EXIO4/5 are wired to QMI8658 interrupts)
#define TCA9554_CONFIG_DIR  ((uint8_t)(~TCA9554_OUTPUT_MASK))

// --- Audio DAC / Speaker (PCM5101 via I2S) ---
#define PIN_SPK_BCLK        48
#define PIN_SPK_WS          38
#define PIN_SPK_DOUT        47

// --- Power, Battery ADC, and Physical Buttons ---
#define PIN_BOOT_BTN        0    // Active LOW
#define PIN_PWR_BTN         6    // Active LOW (RTC GPIO for ext0 Deep Sleep wakeup)
#define PIN_WAKE_BTN        PIN_PWR_BTN
#define PIN_POWER_HOLD      7    // Drive HIGH at boot to latch battery power
#define PIN_BAT_ADC         8    // ADC1_CH7 via 200k / 100k resistor divider
#define BAT_ADC_DIVIDER     3.0f // V_bat = V_adc * 3.0

#endif // KAIROS_BOARD_WAVESHARE_128

// --- Power Management Timings ---
#define DIM_TIMEOUT_MS      10000UL    // Dim backlight to 15% after 10 s idle
#define DIM_BRIGHTNESS_PCT  15
#define FULL_BRIGHTNESS_PCT 100
#define SLEEP_TIMEOUT_MS    900000UL   // Deep sleep after 15 min idle + no BLE

#endif // KAIROS_BOARD_PINS_H
