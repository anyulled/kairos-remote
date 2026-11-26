#include "ui/screen_nav.h"

#include "audio/haptic_click.h"
#include "ble/hid_keyboard.h"
#include "ui/nav_math.h"

namespace {

using KairosMath::NavZone;

struct ArcSpec {
    NavZone zone;
    uint16_t startDeg;
    uint16_t endDeg;
    const char *symbol;
    int16_t iconOffsetX;
    int16_t iconOffsetY;
    uint8_t keycode;
};

constexpr ArcSpec kArcs[] = {
    {NavZone::ARC_UP,    229, 311, LV_SYMBOL_UP,    0,   -142, KEY_UP_ARROW},
    {NavZone::ARC_DOWN,   49, 131, LV_SYMBOL_DOWN,  0,    142, KEY_DOWN_ARROW},
    {NavZone::ARC_LEFT,  139, 221, LV_SYMBOL_LEFT, -150,    0, KEY_LEFT_ARROW},
    {NavZone::ARC_RIGHT, 319,  41, LV_SYMBOL_RIGHT, 150,    0, KEY_RIGHT_ARROW},
};

inline lv_color_t COLOR_ARC_IDLE() { return lv_color_hex(0x1E293B); }
inline lv_color_t COLOR_ARC_ACTIVE() { return lv_color_hex(0x0284C7); }
inline lv_color_t COLOR_CENTER_IDLE() { return lv_color_hex(0x0EA5E9); }
inline lv_color_t COLOR_CENTER_ACTIVE() { return lv_color_hex(0x0369A1); }

lv_obj_t *s_arcObjs[4] = {nullptr, nullptr, nullptr, nullptr};
lv_obj_t *s_centerBtn = nullptr;
NavZone s_pressedZone = NavZone::NONE;

void setZoneHighlight(NavZone zone, bool active) {
    if (zone == NavZone::CENTER_SPACE && s_centerBtn) {
        lv_obj_set_style_bg_color(s_centerBtn, active ? COLOR_CENTER_ACTIVE() : COLOR_CENTER_IDLE(), 0);
        return;
    }
    for (size_t i = 0; i < 4; ++i) {
        if (kArcs[i].zone == zone && s_arcObjs[i]) {
            lv_obj_set_style_arc_color(
                s_arcObjs[i], active ? COLOR_ARC_ACTIVE() : COLOR_ARC_IDLE(), LV_PART_MAIN);
            break;
        }
    }
}

void triggerZoneAction(NavZone zone) {
    if (zone == NavZone::NONE) {
        return;
    }
    HapticAudio::playClick();
    if (zone == NavZone::CENTER_SPACE) {
        BleHidKeyboard::sendKey(KEY_SPACE);
        return;
    }
    for (const auto &spec : kArcs) {
        if (spec.zone == zone) {
            BleHidKeyboard::sendKey(spec.keycode);
            return;
        }
    }
}

void onTileTouchEvent(lv_event_t *e) {
    const lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        lv_point_t pt = {};
        lv_indev_get_point(indev, &pt);
        s_pressedZone = KairosMath::classifyRadialTouch(pt.x, pt.y);
        setZoneHighlight(s_pressedZone, true);
    } else if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_RELEASED) {
        if (s_pressedZone != NavZone::NONE) {
            setZoneHighlight(s_pressedZone, false);
        }
        if (code == LV_EVENT_PRESS_LOST) {
            s_pressedZone = NavZone::NONE;
        }
    } else if (code == LV_EVENT_SHORT_CLICKED) {
        if (lv_indev_get_scroll_obj(indev) == nullptr && s_pressedZone != NavZone::NONE) {
            triggerZoneAction(s_pressedZone);
        }
        setZoneHighlight(s_pressedZone, false);
        s_pressedZone = NavZone::NONE;
    }
}

} // namespace

namespace ScreenNav {

void create(lv_obj_t *parent) {
    for (size_t i = 0; i < 4; ++i) {
        lv_obj_t *arc = lv_arc_create(parent);
        lv_obj_set_size(arc, 392, 392);
        lv_obj_center(arc);
        lv_arc_set_bg_angles(arc, kArcs[i].startDeg, kArcs[i].endDeg);
        lv_arc_set_angles(arc, 0, 0);
        lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
        lv_obj_remove_style(arc, nullptr, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(arc, 92, LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc, COLOR_ARC_IDLE(), LV_PART_MAIN);
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
        s_arcObjs[i] = arc;

        lv_obj_t *icon = lv_label_create(parent);
        lv_label_set_text(icon, kArcs[i].symbol);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(0xF8FAFC), 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, kArcs[i].iconOffsetX, kArcs[i].iconOffsetY);
        lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    }

    s_centerBtn = lv_obj_create(parent);
    lv_obj_set_size(s_centerBtn, 168, 168);
    lv_obj_center(s_centerBtn);
    lv_obj_set_style_radius(s_centerBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_centerBtn, COLOR_CENTER_IDLE(), 0);
    lv_obj_set_style_bg_opa(s_centerBtn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_centerBtn, 3, 0);
    lv_obj_set_style_border_color(s_centerBtn, lv_color_hex(0x7DD3FC), 0);
    lv_obj_clear_flag(s_centerBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_centerBtn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spaceLbl = lv_label_create(s_centerBtn);
    lv_label_set_text(spaceLbl, "SPACE");
    lv_obj_set_style_text_font(spaceLbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(spaceLbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(spaceLbl);

    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(parent, onTileTouchEvent, LV_EVENT_ALL, nullptr);
}

} // namespace ScreenNav
