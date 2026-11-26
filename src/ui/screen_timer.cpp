#include "ui/screen_timer.h"

#include <esp_timer.h>

#include "audio/haptic_click.h"
#include "ui/nav_math.h"

namespace {

constexpr int64_t HOLD_RESET_US = 1500000LL; // 1.5 s long-press to reset
constexpr uint64_t MAX_TIMER_MS = 5999000ULL; // 99:59
constexpr uint64_t DEFAULT_TIMER_MS = 15ULL * 60000ULL; // 15:00 default ready state

lv_obj_t *s_timeLabel = nullptr;
lv_obj_t *s_hintLabel = nullptr;
lv_obj_t *s_playBtn = nullptr;
lv_obj_t *s_playLabel = nullptr;
lv_obj_t *s_stopBtn = nullptr;

uint64_t s_remainingMs = DEFAULT_TIMER_MS;
int64_t s_targetEndUs = 0;
bool s_running = false;
bool s_expired = false;

int64_t s_stopPressStartUs = 0;
bool s_stopResetTriggered = false;

bool isSwiping() {
    lv_indev_t *indev = lv_indev_get_act();
    return indev && lv_indev_get_scroll_obj(indev) != nullptr;
}

uint64_t computeRemainingMs(int64_t nowUs) {
    if (!s_running) {
        return s_remainingMs;
    }
    if (nowUs >= s_targetEndUs) {
        return 0;
    }
    return static_cast<uint64_t>((s_targetEndUs - nowUs) / 1000LL);
}

void refreshTimerUi(int64_t nowUs) {
    const uint64_t remMs = computeRemainingMs(nowUs);
    const uint32_t displaySecs = static_cast<uint32_t>((remMs + 999ULL) / 1000ULL);

    char buf[8] = {};
    KairosMath::formatTimerMMSS(displaySecs, buf, sizeof(buf));
    if (s_timeLabel) {
        lv_label_set_text(s_timeLabel, buf);
    }

    lv_color_t accent = lv_color_hex(0xFFFFFF);
    if (s_expired) {
        const bool blinkOn = ((nowUs / 500000LL) % 2) == 0;
        accent = blinkOn ? lv_color_hex(0xEF4444) : lv_color_hex(0x7F1D1D);
    } else if (remMs > 0 && remMs < 120000ULL) {
        accent = lv_color_hex(0xF59E0B);
    }

    if (s_timeLabel) {
        lv_obj_set_style_text_color(s_timeLabel, accent, 0);
    }

    if (s_playBtn && s_playLabel) {
        lv_obj_set_style_bg_color(
            s_playBtn, lv_color_hex(s_running ? 0x92400E : 0x1A4A5B), 0);
        lv_obj_set_style_border_color(
            s_playBtn, lv_color_hex(s_running ? 0xF59E0B : 0x3B7A8C), 0);
    }
}

void timerTickCb(lv_timer_t *timer) {
    (void)timer;
    const int64_t nowUs = esp_timer_get_time();
    if (s_running && nowUs >= s_targetEndUs) {
        s_running = false;
        s_remainingMs = 0;
        if (!s_expired) {
            s_expired = true;
            HapticAudio::playAlarm();
        }
    }
    refreshTimerUi(nowUs);
}

void onPlayPauseClicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED || isSwiping()) {
        return;
    }
    HapticAudio::playClick();
    const int64_t nowUs = esp_timer_get_time();
    s_stopResetTriggered = false;

    if (s_running) {
        s_remainingMs = computeRemainingMs(nowUs);
        s_running = false;
    } else if (s_remainingMs > 0) {
        s_expired = false;
        s_targetEndUs = nowUs + static_cast<int64_t>(s_remainingMs) * 1000LL;
        s_running = true;
    }
    refreshTimerUi(nowUs);
}

void onStopResetEvent(lv_event_t *e) {
    const lv_event_code_t code = lv_event_get_code(e);
    const int64_t nowUs = esp_timer_get_time();

    if (code == LV_EVENT_PRESSED) {
        s_stopPressStartUs = nowUs;
        s_stopResetTriggered = false;
    } else if (code == LV_EVENT_PRESSING) {
        if (!isSwiping() && !s_stopResetTriggered && (nowUs - s_stopPressStartUs) >= HOLD_RESET_US) {
            s_stopResetTriggered = true;
            s_running = false;
            s_expired = false;
            s_remainingMs = 0;
            HapticAudio::playClick();
            if (s_hintLabel) {
                lv_label_set_text(s_hintLabel, "00:00 RESET");
            }
            refreshTimerUi(nowUs);
        }
    } else if (code == LV_EVENT_SHORT_CLICKED) {
        if (!isSwiping() && !s_stopResetTriggered) {
            HapticAudio::playClick();
            if (s_running) {
                s_remainingMs = computeRemainingMs(nowUs);
                s_running = false;
            }
            s_expired = false;
            if (s_hintLabel) {
                lv_label_set_text(s_hintLabel, "Hold 1.5s: Reset");
            }
            refreshTimerUi(nowUs);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_stopResetTriggered = false;
    }
}

void onAddMinutesClicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED || isSwiping()) {
        return;
    }
    const uint32_t mins = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const uint64_t addMs = static_cast<uint64_t>(mins) * 60000ULL;
    const int64_t nowUs = esp_timer_get_time();

    HapticAudio::playClick();
    s_expired = false;
    s_stopResetTriggered = false;

    uint64_t currentMs = computeRemainingMs(nowUs);
    uint64_t nextMs = currentMs + addMs;
    if (nextMs > MAX_TIMER_MS) {
        nextMs = MAX_TIMER_MS;
    }
    s_remainingMs = nextMs;
    if (s_running) {
        s_targetEndUs = nowUs + static_cast<int64_t>(nextMs) * 1000LL;
    }
    if (s_hintLabel) {
        lv_label_set_text(s_hintLabel, "Swipe " LV_SYMBOL_LEFT);
    }
    refreshTimerUi(nowUs);
}

void createQuickAddButton(lv_obj_t *parent, uint32_t minutes, const char *text, int16_t offX, int16_t offY) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 80, 80);
    lv_obj_align(btn, LV_ALIGN_CENTER, offX, offY);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A4A5B), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284C7), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x4BA3B5), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(
        btn, onAddMinutesClicked, LV_EVENT_SHORT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(minutes)));

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC), 0);
    lv_obj_center(lbl);
}

} // namespace

namespace ScreenTimer {

void create(lv_obj_t *parent) {
    // Large bold countdown digits in the upper half
    s_timeLabel = lv_label_create(parent);
    lv_label_set_text(s_timeLabel, "15:00");
    lv_obj_set_style_text_font(s_timeLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_letter_space(s_timeLabel, 3, 0);
    lv_obj_set_style_text_color(s_timeLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_timeLabel, LV_ALIGN_CENTER, 0, -96);

    // Main transport pill buttons: PLAY / PAUSA and STOP / RESET (1.5 s hold)
    s_playBtn = lv_btn_create(parent);
    lv_obj_set_size(s_playBtn, 174, 84);
    lv_obj_align(s_playBtn, LV_ALIGN_CENTER, -92, 6);
    lv_obj_set_style_radius(s_playBtn, 42, 0);
    lv_obj_set_style_bg_color(s_playBtn, lv_color_hex(0x1A4A5B), 0);
    lv_obj_set_style_bg_color(s_playBtn, lv_color_hex(0x0284C7), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_playBtn, 2, 0);
    lv_obj_set_style_border_color(s_playBtn, lv_color_hex(0x3B7A8C), 0);
    lv_obj_set_style_shadow_width(s_playBtn, 0, 0);
    lv_obj_add_flag(s_playBtn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(s_playBtn, onPlayPauseClicked, LV_EVENT_SHORT_CLICKED, nullptr);

    s_playLabel = lv_label_create(s_playBtn);
    lv_label_set_text(s_playLabel, "PLAY /\nPAUSA");
    lv_obj_set_style_text_align(s_playLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_playLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_playLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(s_playLabel);

    s_stopBtn = lv_btn_create(parent);
    lv_obj_set_size(s_stopBtn, 174, 84);
    lv_obj_align(s_stopBtn, LV_ALIGN_CENTER, 92, 6);
    lv_obj_set_style_radius(s_stopBtn, 42, 0);
    lv_obj_set_style_bg_color(s_stopBtn, lv_color_hex(0x3A3D40), 0);
    lv_obj_set_style_bg_color(s_stopBtn, lv_color_hex(0xDC2626), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_stopBtn, 2, 0);
    lv_obj_set_style_border_color(s_stopBtn, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_shadow_width(s_stopBtn, 0, 0);
    lv_obj_add_flag(s_stopBtn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(s_stopBtn, onStopResetEvent, LV_EVENT_ALL, nullptr);

    lv_obj_t *stopLbl = lv_label_create(s_stopBtn);
    lv_label_set_text(stopLbl, "STOP /\nRESET");
    lv_obj_set_style_text_align(stopLbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(stopLbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stopLbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(stopLbl);

    // Three circular quick-add buttons
    createQuickAddButton(parent, 1, "+1\nmin", -92, 108);
    createQuickAddButton(parent, 2, "+2\nmin",   0, 108);
    createQuickAddButton(parent, 5, "+5\nmin",  92, 108);

    // Bottom swipe hint
    s_hintLabel = lv_label_create(parent);
    lv_label_set_text(s_hintLabel, "Swipe " LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(s_hintLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_hintLabel, lv_color_hex(0xCBD5E1), 0);
    lv_obj_align(s_hintLabel, LV_ALIGN_BOTTOM_MID, 0, -14);

    lv_timer_create(timerTickCb, 100, nullptr);
    refreshTimerUi(esp_timer_get_time());
}

void setRemainingSeconds(uint32_t seconds) {
    s_running = false;
    s_expired = false;
    s_remainingMs = static_cast<uint64_t>(seconds) * 1000ULL;
    if (s_remainingMs > MAX_TIMER_MS) {
        s_remainingMs = MAX_TIMER_MS;
    }
    refreshTimerUi(esp_timer_get_time());
}

bool isRunning() {
    return s_running;
}

} // namespace ScreenTimer
