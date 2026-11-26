#include "ui/screen_stage.h"

#include "audio/haptic_click.h"
#include "ble/hid_keyboard.h"

namespace {

enum class StageAction : uint8_t {
    ESC_OVERVIEW = 0,
    BLACK_SCREEN,
    HOME_START,
    FULLSCREEN,
    ENTER_CONFIRM,
};

struct QuadrantSpec {
    StageAction action;
    const char *title;
    const char *subtitle;
    int16_t offsetX;
    int16_t offsetY;
    uint32_t accentHex;
};

constexpr QuadrantSpec kQuadrants[] = {
    {StageAction::ESC_OVERVIEW, "ESC",  "Overview", -82, -76, 0x38BDF8},
    {StageAction::BLACK_SCREEN, "B",    "Black",     82, -76, 0xA855F7},
    {StageAction::HOME_START,   "HOME", "Inicio",   -82,  76, 0x10B981},
    {StageAction::FULLSCREEN,   "F",    "Full",      82,  76, 0xF59E0B},
};

void dispatchStageAction(StageAction action) {
    HapticAudio::playClick();
    switch (action) {
        case StageAction::ESC_OVERVIEW:
            BleHidKeyboard::sendKey(KEY_ESC);
            break;
        case StageAction::BLACK_SCREEN:
            BleHidKeyboard::sendChar('b');
            break;
        case StageAction::HOME_START:
            BleHidKeyboard::sendKey(KEY_HOME);
            break;
        case StageAction::FULLSCREEN:
            BleHidKeyboard::sendChar('f');
            break;
        case StageAction::ENTER_CONFIRM:
            BleHidKeyboard::sendKey(KEY_RETURN);
            break;
    }
}

void onStageButtonEvent(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) {
        return;
    }
    lv_indev_t *indev = lv_indev_get_act();
    if (indev && lv_indev_get_scroll_obj(indev) != nullptr) {
        return;
    }
    const auto action = static_cast<StageAction>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    dispatchStageAction(action);
}

lv_obj_t *createQuadrantButton(lv_obj_t *parent, const QuadrantSpec &spec) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 116, 96);
    lv_obj_align(btn, LV_ALIGN_CENTER, spec.offsetX, spec.offsetY);
    lv_obj_set_style_radius(btn, 22, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(spec.accentHex), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(spec.accentHex), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(
        btn, onStageButtonEvent, LV_EVENT_SHORT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(spec.action)));

    lv_obj_t *box = lv_obj_create(btn);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(box);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(box, 4, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(box);
    lv_label_set_text(title, spec.title);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), 0);

    lv_obj_t *sub = lv_label_create(box);
    lv_label_set_text(sub, spec.subtitle);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x94A3B8), 0);

    return btn;
}

} // namespace

namespace ScreenStage {

void create(lv_obj_t *parent) {
    for (const auto &spec : kQuadrants) {
        createQuadrantButton(parent, spec);
    }

    // Center ENTER button rendered on top with a dark halo border for clean separation.
    lv_obj_t *enterBtn = lv_btn_create(parent);
    lv_obj_set_size(enterBtn, 122, 122);
    lv_obj_center(enterBtn);
    lv_obj_set_style_radius(enterBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(enterBtn, lv_color_hex(0x0EA5E9), 0);
    lv_obj_set_style_bg_color(enterBtn, lv_color_hex(0x0369A1), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(enterBtn, 6, 0);
    lv_obj_set_style_border_color(enterBtn, lv_color_hex(0x0B0F17), 0);
    lv_obj_set_style_shadow_width(enterBtn, 0, 0);
    lv_obj_add_flag(enterBtn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(
        enterBtn, onStageButtonEvent, LV_EVENT_SHORT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(StageAction::ENTER_CONFIRM)));

    lv_obj_t *lbl = lv_label_create(enterBtn);
    lv_label_set_text(lbl, "ENTER\nOK");
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl);
}

} // namespace ScreenStage
