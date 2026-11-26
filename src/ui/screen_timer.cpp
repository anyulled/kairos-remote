#include "ui/screen_timer.h"

#include <esp_timer.h>

#include "audio/haptic_click.h"
#include "ui/nav_math.h"

namespace {

constexpr int64_t HOLD_RESET_US = 1500000LL; // 1.5 s long-press to reset
constexpr uint64_t MAX_TIMER_MS = 5999000ULL; // 99:59

lv_obj_t *s_ringArc = nullptr;
lv_obj_t *s_timeLabel = nullptr;
lv_obj_t *s_statusLabel = nullptr;
lv_obj_t *s_playBtn = nullptr;
lv_obj_t *s_playLabel = nullptr;
lv_obj_t *s_stopBtn = nullptr;

uint64_t s_remainingMs = 0;
uint64_t s_totalConfiguredMs = 0;
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

    lv_color_t accent = lv_color_hex(0xF8FAFC);
    lv_color_t arcColor = lv_color_hex(0x38BDF8);
    if (s_expired) {
        const bool blinkOn = ((nowUs / 500000LL) % 2) == 0;
        accent = blinkOn ? lv_color_hex(0xEF4444) : lv_color_hex(0x7F1D1D);
        arcColor = lv_color_hex(0xEF4444);
    } else if (remMs > 0 && remMs < 120000ULL) {
        accent = lv_color_hex(0xF59E0B);
        arcColor = lv_color_hex(0xF59E0B);
    }

    if (s_timeLabel) {
        lv_obj_set_style_text_color(s_timeLabel, accent, 0);
    }
    if (s_ringArc) {
        lv_obj_set_style_arc_color(s_ringArc, arcColor, LV_PART_INDICATOR);
        int16_t pct = 0;
        if (s_totalConfiguredMs > 0) {
            pct = static_cast<int16_t>((remMs * 100ULL) / s_totalConfiguredMs);
            if (pct > 100) {
                pct = 100;
            }
        }
        lv_arc_set_value(s_ringArc, pct);
    }

    if (s_playBtn && s_playLabel) {
        if (s_running) {
            lv_obj_set_style_bg_color(s_playBtn, lv_color_hex(0xD97706), 0);
            lv_label_set_text(s_playLabel, LV_SYMBOL_PAUSE " PAUSA");
        } else {
            lv_obj_set_style_bg_color(s_playBtn, lv_color_hex(0x059669), 0);
            lv_label_set_text(s_playLabel, LV_SYMBOL_PLAY " PLAY");
        }
    }

    if (s_statusLabel && !s_stopResetTriggered) {
        if (s_expired) {
            lv_label_set_text(s_statusLabel, "TIEMPO AGOTADO");
            lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0xF87171), 0);
        } else if (s_running) {
            lv_label_set_text(s_statusLabel, "EN MARCHA");
            lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0x38BDF8), 0);
        } else if (remMs > 0) {
            lv_label_set_text(s_statusLabel, "PAUSADO");
            lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0xFBBF24), 0);
        } else {
            lv_label_set_text(s_statusLabel, "ANADE TIEMPO");
            lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0x94A3B8), 0);
        }
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
            s_totalConfiguredMs = 0;
            HapticAudio::playClick();
            if (s_statusLabel) {
                lv_label_set_text(s_statusLabel, "REINICIADO");
                lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0x38BDF8), 0);
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
            refreshTimerUi(nowUs);
            if (s_statusLabel && s_remainingMs > 0) {
                lv_label_set_text(s_statusLabel, "MANTEN 1.5s RESET");
                lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0xF87171), 0);
            }
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
    if (nextMs > s_totalConfiguredMs) {
        s_totalConfiguredMs = nextMs;
    }
    refreshTimerUi(nowUs);
}

void createQuickAddButton(lv_obj_t *parent, uint32_t minutes, const char *text, int16_t offX, int16_t offY) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 68, 68);
    lv_obj_align(btn, LV_ALIGN_CENTER, offX, offY);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284C7), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(
        btn, onAddMinutesClicked, LV_EVENT_SHORT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(minutes)));

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xF8FAFC), 0);
    lv_obj_center(lbl);
}

} // namespace

namespace ScreenTimer {

void create(lv_obj_t *parent) {
    s_ringArc = lv_arc_create(parent);
    lv_obj_set_size(s_ringArc, 396, 396);
    lv_obj_center(s_ringArc);
    lv_arc_set_rotation(s_ringArc, 270);
    lv_arc_set_bg_angles(s_ringArc, 0, 360);
    lv_arc_set_range(s_ringArc, 0, 100);
    lv_arc_set_value(s_ringArc, 0);
    lv_obj_remove_style(s_ringArc, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_ringArc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ringArc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ringArc, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ringArc, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_clear_flag(s_ringArc, LV_OBJ_FLAG_CLICKABLE);

    s_timeLabel = lv_label_create(parent);
    lv_label_set_text(s_timeLabel, "00:00");
    lv_obj_set_style_text_font(s_timeLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_timeLabel, lv_color_hex(0xF8FAFC), 0);
    lv_obj_align(s_timeLabel, LV_ALIGN_CENTER, 0, -78);

    s_statusLabel = lv_label_create(parent);
    lv_label_set_text(s_statusLabel, "ANADE TIEMPO");
    lv_obj_set_style_text_font(s_statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_statusLabel, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(s_statusLabel, LV_ALIGN_CENTER, 0, -34);

    // Main transport controls: PLAY/PAUSE and STOP/HOLD-RESET (1.5 s)
    s_playBtn = lv_btn_create(parent);
    lv_obj_set_size(s_playBtn, 126, 54);
    lv_obj_align(s_playBtn, LV_ALIGN_CENTER, -70, 22);
    lv_obj_set_style_radius(s_playBtn, 27, 0);
    lv_obj_set_style_bg_color(s_playBtn, lv_color_hex(0x059669), 0);
    lv_obj_set_style_shadow_width(s_playBtn, 0, 0);
    lv_obj_add_flag(s_playBtn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(s_playBtn, onPlayPauseClicked, LV_EVENT_SHORT_CLICKED, nullptr);

    s_playLabel = lv_label_create(s_playBtn);
    lv_label_set_text(s_playLabel, LV_SYMBOL_PLAY " PLAY");
    lv_obj_set_style_text_font(s_playLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(s_playLabel);

    s_stopBtn = lv_btn_create(parent);
    lv_obj_set_size(s_stopBtn, 126, 54);
    lv_obj_align(s_stopBtn, LV_ALIGN_CENTER, 70, 22);
    lv_obj_set_style_radius(s_stopBtn, 27, 0);
    lv_obj_set_style_bg_color(s_stopBtn, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_color(s_stopBtn, lv_color_hex(0xDC2626), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(s_stopBtn, 0, 0);
    lv_obj_add_flag(s_stopBtn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(s_stopBtn, onStopResetEvent, LV_EVENT_ALL, nullptr);

    lv_obj_t *stopLbl = lv_label_create(s_stopBtn);
    lv_label_set_text(stopLbl, LV_SYMBOL_STOP " RESET");
    lv_obj_set_style_text_font(stopLbl, &lv_font_montserrat_16, 0);
    lv_obj_center(stopLbl);

    // Quick-increment circular buttons arranged along the lower curve
    createQuickAddButton(parent, 1, "+1m", -86, 106);
    createQuickAddButton(parent, 2, "+2m",   0, 118);
    createQuickAddButton(parent, 5, "+5m",  86, 106);

    lv_timer_create(timerTickCb, 100, nullptr);
    refreshTimerUi(esp_timer_get_time());
}

bool isRunning() {
    return s_running;
}

} // namespace ScreenTimer
