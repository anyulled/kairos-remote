#include "ui/ui_manager.h"

#include "audio/haptic_click.h"
#include "board_pins.h"

namespace {

constexpr uint8_t TILE_COUNT = 3;

lv_obj_t *s_tileview = nullptr;
lv_obj_t *s_tiles[TILE_COUNT] = {nullptr, nullptr, nullptr};
lv_obj_t *s_dots[TILE_COUNT] = {nullptr, nullptr, nullptr};

lv_obj_t *s_bleLabel = nullptr;
lv_obj_t *s_muteBtn = nullptr;
lv_obj_t *s_muteLabel = nullptr;
lv_obj_t *s_batLabel = nullptr;

uint8_t s_activeTile = 0;

void styleTile(lv_obj_t *tile) {
    lv_obj_set_size(tile, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(tile, lv_color_hex(0x0B0F17), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tile, 0, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
}

void updatePageDots(uint8_t activeIdx) {
    s_activeTile = activeIdx;
    for (uint8_t i = 0; i < TILE_COUNT; ++i) {
        if (!s_dots[i]) {
            continue;
        }
        if (i == activeIdx) {
            lv_obj_set_size(s_dots[i], 20, 8);
            lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(0x38BDF8), 0);
        } else {
            lv_obj_set_size(s_dots[i], 8, 8);
            lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(0x334155), 0);
        }
    }
}

void onTileChanged(lv_event_t *e) {
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *act = lv_tileview_get_tile_act(tv);
    for (uint8_t i = 0; i < TILE_COUNT; ++i) {
        if (s_tiles[i] == act) {
            updatePageDots(i);
            break;
        }
    }
}

void onMuteClicked(lv_event_t *e) {
    (void)e;
    const bool muted = HapticAudio::toggleMute();
    if (!muted) {
        HapticAudio::playClick();
    }
    UiManager::refreshMuteIcon();
}

void createStatusBar() {
    lv_obj_t *topLayer = lv_layer_top();

    lv_obj_t *pill = lv_obj_create(topLayer);
    lv_obj_set_size(pill, 178, 30);
    lv_obj_align(pill, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_radius(pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pill, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_90, 0);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_border_color(pill, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_pad_hor(pill, 12, 0);
    lv_obj_set_style_pad_ver(pill, 0, 0);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_bleLabel = lv_label_create(pill);
    lv_obj_set_style_text_font(s_bleLabel, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_bleLabel, LV_SYMBOL_BLUETOOTH);

    s_muteBtn = lv_btn_create(pill);
    lv_obj_set_size(s_muteBtn, 34, 24);
    lv_obj_set_style_radius(s_muteBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_muteBtn, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_color(s_muteBtn, lv_color_hex(0x334155), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(s_muteBtn, 0, 0);
    lv_obj_set_style_pad_all(s_muteBtn, 0, 0);
    lv_obj_add_event_cb(s_muteBtn, onMuteClicked, LV_EVENT_CLICKED, nullptr);

    s_muteLabel = lv_label_create(s_muteBtn);
    lv_obj_set_style_text_font(s_muteLabel, &lv_font_montserrat_14, 0);
    lv_obj_center(s_muteLabel);

    s_batLabel = lv_label_create(pill);
    lv_obj_set_style_text_font(s_batLabel, &lv_font_montserrat_14, 0);
}

void createPageIndicator() {
    lv_obj_t *topLayer = lv_layer_top();

    lv_obj_t *row = lv_obj_create(topLayer);
    lv_obj_set_size(row, 84, 16);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);

    for (uint8_t i = 0; i < TILE_COUNT; ++i) {
        s_dots[i] = lv_obj_create(row);
        lv_obj_set_size(s_dots[i], 8, 8);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_dots[i], 0, 0);
        lv_obj_set_style_pad_all(s_dots[i], 0, 0);
        lv_obj_clear_flag(s_dots[i], LV_OBJ_FLAG_CLICKABLE);
    }
}

const char *batterySymbolFor(uint8_t percent, bool charging) {
    if (charging) {
        return LV_SYMBOL_CHARGE;
    }
    if (percent >= 85) {
        return LV_SYMBOL_BATTERY_FULL;
    }
    if (percent >= 60) {
        return LV_SYMBOL_BATTERY_3;
    }
    if (percent >= 35) {
        return LV_SYMBOL_BATTERY_2;
    }
    if (percent >= 15) {
        return LV_SYMBOL_BATTERY_1;
    }
    return LV_SYMBOL_BATTERY_EMPTY;
}

} // namespace

namespace UiManager {

void begin() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B0F17), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    s_tileview = lv_tileview_create(scr);
    lv_obj_set_size(s_tileview, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_center(s_tileview);
    lv_obj_set_style_bg_color(s_tileview, lv_color_hex(0x0B0F17), 0);
    lv_obj_set_style_bg_opa(s_tileview, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(s_tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(s_tileview, onTileChanged, LV_EVENT_VALUE_CHANGED, nullptr);

    s_tiles[0] = lv_tileview_add_tile(s_tileview, 0, 0, LV_DIR_RIGHT);
    s_tiles[1] = lv_tileview_add_tile(s_tileview, 1, 0, LV_DIR_HOR);
    s_tiles[2] = lv_tileview_add_tile(s_tileview, 2, 0, LV_DIR_LEFT);
    for (uint8_t i = 0; i < TILE_COUNT; ++i) {
        styleTile(s_tiles[i]);
    }

    createStatusBar();
    createPageIndicator();
    updatePageDots(0);
    updateBleStatus(false);
    updateBattery(100, false);
    refreshMuteIcon();
}

lv_obj_t *getTile(uint8_t index) {
    return (index < TILE_COUNT) ? s_tiles[index] : nullptr;
}

void setActiveTile(uint8_t index, bool animate) {
    if (!s_tileview || index >= TILE_COUNT) {
        return;
    }
    lv_obj_set_tile_id(s_tileview, index, 0, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    updatePageDots(index);
}

uint8_t getActiveTileIndex() {
    return s_activeTile;
}

void updateBleStatus(bool connected) {
    if (!s_bleLabel) {
        return;
    }
    lv_obj_set_style_text_color(
        s_bleLabel, connected ? lv_color_hex(0x38BDF8) : lv_color_hex(0x64748B), 0);
}

void updateBattery(uint8_t percent, bool charging) {
    if (!s_batLabel) {
        return;
    }
    if (percent > 100) {
        percent = 100;
    }
    lv_color_t color = lv_color_hex(0x22C55E);
    if (!charging && percent <= 15) {
        color = lv_color_hex(0xEF4444);
    } else if (!charging && percent <= 35) {
        color = lv_color_hex(0xF59E0B);
    }
    lv_obj_set_style_text_color(s_batLabel, color, 0);
    lv_label_set_text_fmt(s_batLabel, "%s %u%%", batterySymbolFor(percent, charging), percent);
}

void refreshMuteIcon() {
    if (!s_muteLabel) {
        return;
    }
    const bool muted = HapticAudio::isMuted();
    lv_label_set_text(s_muteLabel, muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_color(
        s_muteLabel, muted ? lv_color_hex(0xF87171) : lv_color_hex(0xE2E8F0), 0);
}

} // namespace UiManager
