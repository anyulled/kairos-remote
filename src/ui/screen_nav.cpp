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
    uint32_t fillIdleHex;
    uint32_t fillActiveHex;
    uint32_t rimHex;
    const char *symbol;
    const char *text;
    int16_t symOffX;
    int16_t symOffY;
    int16_t txtOffX;
    int16_t txtOffY;
    int16_t txtAngleDeciDeg;
    uint8_t keycode;
};

constexpr ArcSpec kArcs[] = {
    {NavZone::ARC_UP,    228, 312, 0x17385B, 0x2563EB, 0x3B82F6,
     LV_SYMBOL_UP, "UP",       0, -148,    0, -120,    0, KEY_UP_ARROW},
    {NavZone::ARC_DOWN,   48, 132, 0x152D46, 0x0D9488, 0x2DD4BF,
     LV_SYMBOL_DOWN, "DOWN",   0,  146,    0,  118,    0, KEY_DOWN_ARROW},
    {NavZone::ARC_LEFT,  138, 222, 0x17385B, 0x2563EB, 0x3B82F6,
     LV_SYMBOL_LEFT, "LEFT", -142, -14, -142,   12,    0, KEY_LEFT_ARROW},
    {NavZone::ARC_RIGHT, 318,  42, 0x123E3B, 0x059669, 0x34D399,
     LV_SYMBOL_RIGHT, "RIGHT", 142, -14,  142,  12,    0, KEY_RIGHT_ARROW},
};

lv_obj_t *s_arcObjs[4] = {nullptr, nullptr, nullptr, nullptr};
lv_obj_t *s_centerBtn = nullptr;
NavZone s_pressedZone = NavZone::NONE;

void setZoneHighlight(NavZone zone, bool active) {
    if (zone == NavZone::CENTER_SPACE && s_centerBtn) {
        lv_obj_set_style_bg_color(
            s_centerBtn, lv_color_hex(active ? 0x0284C7 : 0x215E6E), 0);
        return;
    }
    for (size_t i = 0; i < 4; ++i) {
        if (kArcs[i].zone == zone && s_arcObjs[i]) {
            lv_obj_set_style_arc_color(
                s_arcObjs[i],
                lv_color_hex(active ? kArcs[i].fillActiveHex : kArcs[i].fillIdleHex),
                LV_PART_MAIN);
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

lv_obj_t *createArcLayer(lv_obj_t *parent, uint16_t sizePx, uint16_t widthPx,
                         uint16_t startDeg, uint16_t endDeg, uint32_t colorHex) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, sizePx, sizePx);
    lv_obj_center(arc);
    lv_arc_set_bg_angles(arc, startDeg, endDeg);
    lv_arc_set_angles(arc, 0, 0);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    lv_obj_remove_style(arc, nullptr, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, widthPx, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(colorHex), LV_PART_MAIN);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

} // namespace

namespace ScreenNav {

void create(lv_obj_t *parent) {
    for (size_t i = 0; i < 4; ++i) {
        const auto &spec = kArcs[i];
        // Filled wedge sector + glowing outer rim arc
        s_arcObjs[i] = createArcLayer(parent, 404, 118, spec.startDeg, spec.endDeg, spec.fillIdleHex);
        createArcLayer(parent, 406, 7, spec.startDeg, spec.endDeg, spec.rimHex);

        lv_obj_t *sym = lv_label_create(parent);
        lv_label_set_text(sym, spec.symbol);
        lv_obj_set_style_text_font(sym, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(sym, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(sym, LV_ALIGN_CENTER, spec.symOffX, spec.symOffY);
        lv_obj_clear_flag(sym, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *txt = lv_label_create(parent);
        lv_label_set_text(txt, spec.text);
        lv_obj_set_style_text_font(txt, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(txt, lv_color_hex(0xE2E8F0), 0);
        if (spec.txtAngleDeciDeg != 0) {
            lv_obj_set_style_transform_pivot_x(txt, LV_PCT(50), 0);
            lv_obj_set_style_transform_pivot_y(txt, LV_PCT(50), 0);
            lv_obj_set_style_transform_angle(txt, spec.txtAngleDeciDeg, 0);
        }
        lv_obj_align(txt, LV_ALIGN_CENTER, spec.txtOffX, spec.txtOffY);
        lv_obj_clear_flag(txt, LV_OBJ_FLAG_CLICKABLE);
    }

    // Center SPACE circle with crisp black halo separation
    s_centerBtn = lv_obj_create(parent);
    lv_obj_set_size(s_centerBtn, 154, 154);
    lv_obj_center(s_centerBtn);
    lv_obj_set_style_radius(s_centerBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_centerBtn, lv_color_hex(0x215E6E), 0);
    lv_obj_set_style_bg_opa(s_centerBtn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_centerBtn, 3, 0);
    lv_obj_set_style_border_color(s_centerBtn, lv_color_hex(0x4BA3B5), 0);
    lv_obj_set_style_outline_width(s_centerBtn, 5, 0);
    lv_obj_set_style_outline_color(s_centerBtn, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(s_centerBtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_centerBtn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *centerBox = lv_obj_create(s_centerBtn);
    lv_obj_set_size(centerBox, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(centerBox);
    lv_obj_set_style_bg_opa(centerBox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(centerBox, 0, 0);
    lv_obj_set_style_pad_all(centerBox, 0, 0);
    lv_obj_set_flex_flow(centerBox, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(centerBox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(centerBox, 2, 0);
    lv_obj_clear_flag(centerBox, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(centerBox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spaceLbl = lv_label_create(centerBox);
    lv_label_set_text(spaceLbl, "SPACE");
    lv_obj_set_style_text_font(spaceLbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(spaceLbl, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *subLbl = lv_label_create(centerBox);
    lv_label_set_text(subLbl, "(AVANCE)");
    lv_obj_set_style_text_font(subLbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(subLbl, lv_color_hex(0xE2E8F0), 0);

    // Subtle bottom swipe cue
    lv_obj_t *swipeLbl = lv_label_create(parent);
    lv_label_set_text(swipeLbl, LV_SYMBOL_LEFT " Swipe " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(swipeLbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(swipeLbl, lv_color_hex(0xCBD5E1), 0);
    lv_obj_align(swipeLbl, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_clear_flag(swipeLbl, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(parent, onTileTouchEvent, LV_EVENT_ALL, nullptr);
}

} // namespace ScreenNav
