#include "audio/haptic_click.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#include "board_pins.h"

namespace {

constexpr i2s_port_t SPK_I2S_PORT = I2S_NUM_0;
constexpr uint32_t SAMPLE_RATE_HZ = 22050;
constexpr float TWO_PI_F = 6.28318530718f;

constexpr uint32_t EVT_PLAY_CLICK = (1U << 0);
constexpr uint32_t EVT_PLAY_ALARM = (1U << 1);

// 18 ms click (397 stereo frames) + 235 ms double-pulse alarm (5182 stereo frames)
constexpr size_t CLICK_FRAMES = (SAMPLE_RATE_HZ * 18) / 1000;
constexpr size_t ALARM_FRAMES = (SAMPLE_RATE_HZ * 235) / 1000;

int16_t *s_clickBuf = nullptr;
int16_t *s_alarmBuf = nullptr;
TaskHandle_t s_audioTask = nullptr;
volatile bool s_muted = false;
bool s_ready = false;

void writeStereoFrame(int16_t *buf, size_t idx, float sampleNorm) {
    if (sampleNorm > 1.0f) {
        sampleNorm = 1.0f;
    } else if (sampleNorm < -1.0f) {
        sampleNorm = -1.0f;
    }
    const int16_t s = static_cast<int16_t>(sampleNorm * 32767.0f);
    buf[idx * 2] = s;
    buf[idx * 2 + 1] = s;
}

void synthesizeClick() {
    // Crisp mechanical switch profile: 0.5 ms attack + exponential decay (tau = 3.2 ms)
    const size_t attackFrames = (SAMPLE_RATE_HZ * 5) / 10000; // 0.5 ms
    const size_t activeFrames = (SAMPLE_RATE_HZ * 14) / 1000; // 14 ms tone, 4 ms zero tail
    for (size_t i = 0; i < CLICK_FRAMES; ++i) {
        if (i >= activeFrames) {
            writeStereoFrame(s_clickBuf, i, 0.0f);
            continue;
        }
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE_HZ);
        const float envAttack = (i < attackFrames)
                                    ? (0.5f * (1.0f - cosf(3.14159265f * static_cast<float>(i) / static_cast<float>(attackFrames))))
                                    : 1.0f;
        const float envDecay = expf(-t / 0.0032f);
        const float tone = 0.65f * sinf(TWO_PI_F * 1150.0f * t) + 0.35f * sinf(TWO_PI_F * 2600.0f * t);
        writeStereoFrame(s_clickBuf, i, 0.42f * envAttack * envDecay * tone);
    }
}

float renderTonePulse(float tLocal, float durationSec, float freqHz) {
    if (tLocal < 0.0f || tLocal >= durationSec) {
        return 0.0f;
    }
    constexpr float rampSec = 0.006f;
    float env = 1.0f;
    if (tLocal < rampSec) {
        env = tLocal / rampSec;
    } else if (tLocal > durationSec - rampSec) {
        env = (durationSec - tLocal) / rampSec;
    }
    return 0.40f * env * sinf(TWO_PI_F * freqHz * tLocal);
}

void synthesizeAlarm() {
    // Double pulse: 70 ms @ 880 Hz, 70 ms pause, 90 ms @ 1175 Hz, 5 ms tail
    for (size_t i = 0; i < ALARM_FRAMES; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE_HZ);
        float sample = 0.0f;
        if (t < 0.070f) {
            sample = renderTonePulse(t, 0.070f, 880.0f);
        } else if (t >= 0.140f && t < 0.230f) {
            sample = renderTonePulse(t - 0.140f, 0.090f, 1175.0f);
        }
        writeStereoFrame(s_alarmBuf, i, sample);
    }
}

void audioTask(void *arg) {
    (void)arg;
    uint32_t notifyVal = 0;
    for (;;) {
        if (xTaskNotifyWait(0, UINT32_MAX, &notifyVal, portMAX_DELAY) == pdTRUE) {
            if (!s_ready || s_muted) {
                continue;
            }
            size_t written = 0;
            if (notifyVal & EVT_PLAY_ALARM) {
                i2s_write(SPK_I2S_PORT, s_alarmBuf, ALARM_FRAMES * 2 * sizeof(int16_t), &written, portMAX_DELAY);
                i2s_zero_dma_buffer(SPK_I2S_PORT);
            } else if (notifyVal & EVT_PLAY_CLICK) {
                i2s_write(SPK_I2S_PORT, s_clickBuf, CLICK_FRAMES * 2 * sizeof(int16_t), &written, portMAX_DELAY);
                i2s_zero_dma_buffer(SPK_I2S_PORT);
            }
        }
    }
}

} // namespace

namespace HapticAudio {

bool begin() {
    if (s_ready) {
        return true;
    }

    s_clickBuf = static_cast<int16_t *>(malloc(CLICK_FRAMES * 2 * sizeof(int16_t)));
    s_alarmBuf = static_cast<int16_t *>(malloc(ALARM_FRAMES * 2 * sizeof(int16_t)));
    if (!s_clickBuf || !s_alarmBuf) {
        return false;
    }

    synthesizeClick();
    synthesizeAlarm();

    const i2s_config_t i2sCfg = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE_HZ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };

    const i2s_pin_config_t pinCfg = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = PIN_SPK_BCLK,
        .ws_io_num = PIN_SPK_WS,
        .data_out_num = PIN_SPK_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    if (i2s_driver_install(SPK_I2S_PORT, &i2sCfg, 0, nullptr) != ESP_OK) {
        return false;
    }
    if (i2s_set_pin(SPK_I2S_PORT, &pinCfg) != ESP_OK) {
        return false;
    }
    i2s_zero_dma_buffer(SPK_I2S_PORT);

    s_ready = true;
    xTaskCreatePinnedToCore(audioTask, "kairos_audio", 2560, nullptr, 3, &s_audioTask, 0);
    return true;
}

void playClick() {
    if (s_ready && !s_muted && s_audioTask) {
        xTaskNotify(s_audioTask, EVT_PLAY_CLICK, eSetBits);
    }
}

void playAlarm() {
    if (s_ready && !s_muted && s_audioTask) {
        xTaskNotify(s_audioTask, EVT_PLAY_ALARM, eSetBits);
    }
}

void setMuted(bool muted) {
    s_muted = muted;
}

bool isMuted() {
    return s_muted;
}

bool toggleMute() {
    s_muted = !s_muted;
    return s_muted;
}

} // namespace HapticAudio
