#include "ui/screen_stage.h"

#include "audio/haptic_click.h"
#include "ble/hid_keyboard.h"
#include "board_pins.h"

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
    lv_align_t align;
    int16_t contentOffX;
    int16_t contentOffY;
    uint32_t bgIdleHex;
    uint32_t bgPressedHex;
    bool showBlackSquare;
};

#ifdef KAIROS_BOARD_WAVESHARE_128
constexpr lv_coord_t STAGE_QUAD_SIZE = 118;
constexpr lv_coord_t STAGE_SQ_SIZE = 8;
constexpr lv_coord_t STAGE_ENTER_SIZE = 74;
constexpr lv_coord_t STAGE_ENTER_BORDER = 2;
constexpr lv_coord_t STAGE_ENTER_OUTLINE = 4;

constexpr QuadrantSpec kQuadrants[] = {
    {StageAction::ESC_OVERVIEW, "ESC", "(OVERVIEW)",
     LV_ALIGN_TOP_LEFT, 10, 10, 0x1C3F60, 0x2563EB, false},
    {StageAction::BLACK_SCREEN, "B", "(NEGRO)",
     LV_ALIGN_TOP_RIGHT, -10, 10, 0x000000, 0x1E293B, true},
    {StageAction::HOME_START, "HOME", "(INICIO " LV_SYMBOL_HOME ")",
     LV_ALIGN_BOTTOM_LEFT, 10, -10, 0x2D572C, 0x16A34A, false},
    {StageAction::FULLSCREEN, "F", "(PANTALLA\nCOMPLETA)",
     LV_ALIGN_BOTTOM_RIGHT, -10, -10, 0x16535E, 0x0891B2, false},
};
#else
constexpr lv_coord_t STAGE_QUAD_SIZE = 204;
constexpr lv_coord_t STAGE_SQ_SIZE = 12;
constexpr lv_coord_t STAGE_ENTER_SIZE = 126;
constexpr lv_coord_t STAGE_ENTER_BORDER = 3;
constexpr lv_coord_t STAGE_ENTER_OUTLINE = 5;

constexpr QuadrantSpec kQuadrants[] = {
    {StageAction::ESC_OVERVIEW, "ESC", "(OVERVIEW)",
     LV_ALIGN_TOP_LEFT, 16, 18, 0x1C3F60, 0x2563EB, false},
    {StageAction::BLACK_SCREEN, "B", "(NEGRO)",
     LV_ALIGN_TOP_RIGHT, -16, 18, 0x000000, 0x1E293B, true},
    {StageAction::HOME_START, "HOME", "(INICIO " LV_SYMBOL_HOME ")",
     LV_ALIGN_BOTTOM_LEFT, 16, -18, 0x2D572C, 0x16A34A, false},
    {StageAction::FULLSCREEN, "F", "(PANTALLA\nCOMPLETA)",
     LV_ALIGN_BOTTOM_RIGHT, -16, -14, 0x16535E, 0x0891B2, false},
};
#endif

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
    const lv_indev_t *indev = lv_indev_get_act();
    if (indev && lv_indev_get_scroll_obj(indev) != nullptr) {
        return;
    }
    const auto action = static_cast<StageAction>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    dispatchStageAction(action);
}

void createQuadrantSector(lv_obj_t *clipCircle, const QuadrantSpec &spec) {
#ifdef KAIROS_BOARD_WAVESHARE_128
    const lv_font_t *titleFont = &lv_font_montserrat_18;
    const lv_font_t *subFont = &lv_font_montserrat_10;
#else
    const lv_font_t *titleFont = &lv_font_montserrat_28;
    const lv_font_t *subFont = &lv_font_montserrat_14;
#endif

    lv_obj_t *btn = lv_btn_create(clipCircle);
    lv_obj_set_size(btn, STAGE_QUAD_SIZE, STAGE_QUAD_SIZE);
    lv_obj_align(btn, spec.align, 0, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(spec.bgIdleHex), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(spec.bgPressedHex), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(
        btn, onStageButtonEvent, LV_EVENT_SHORT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(spec.action)));

    lv_obj_t *box = lv_obj_create(btn);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(box, LV_ALIGN_CENTER, spec.contentOffX, spec.contentOffY);
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
    lv_obj_set_style_text_font(title, titleFont, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *sub = lv_label_create(box);
    lv_label_set_text(sub, spec.subtitle);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(sub, subFont, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xE2E8F0), 0);

    if (spec.showBlackSquare) {
        lv_obj_t *row = lv_obj_create(box);
        lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 5, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_parent(sub, row);

        lv_obj_t *sq = lv_obj_create(row);
        lv_obj_set_size(sq, STAGE_SQ_SIZE, STAGE_SQ_SIZE);
        lv_obj_set_style_radius(sq, 2, 0);
        lv_obj_set_style_bg_color(sq, lv_color_hex(0x000000), 0);
        lv_obj_set_style_border_width(sq, 1, 0);
        lv_obj_set_style_border_color(sq, lv_color_hex(0x94A3B8), 0);
        lv_obj_clear_flag(sq, LV_OBJ_FLAG_CLICKABLE);
    }
}

} // namespace

namespace ScreenStage {

void create(lv_obj_t *parent) {
#ifdef KAIROS_BOARD_WAVESHARE_128
    const lv_font_t *enterFont = &lv_font_montserrat_14;
    const lv_font_t *okFont = &lv_font_montserrat_10;
#else
    const lv_font_t *enterFont = &lv_font_montserrat_22;
    const lv_font_t *okFont = &lv_font_montserrat_16;
#endif

    // Circular container clips the 4 square quadrants into a disc with 4px cross gap
    lv_obj_t *clipCircle = lv_obj_create(parent);
    lv_obj_set_size(clipCircle, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_center(clipCircle);
    lv_obj_set_style_radius(clipCircle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(clipCircle, true, 0);
    lv_obj_set_style_bg_color(clipCircle, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(clipCircle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(clipCircle, 0, 0);
    lv_obj_set_style_pad_all(clipCircle, 0, 0);
    lv_obj_clear_flag(clipCircle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(clipCircle, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);

    for (const auto &spec : kQuadrants) {
        createQuadrantSector(clipCircle, spec);
    }

    // Center ENTER (OK) circle rendered on top with crisp black halo ring
    lv_obj_t *enterBtn = lv_btn_create(clipCircle);
    lv_obj_set_size(enterBtn, STAGE_ENTER_SIZE, STAGE_ENTER_SIZE);
    lv_obj_center(enterBtn);
    lv_obj_set_style_radius(enterBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(enterBtn, lv_color_hex(0x20586E), 0);
    lv_obj_set_style_bg_color(enterBtn, lv_color_hex(0x0284C7), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(enterBtn, STAGE_ENTER_BORDER, 0);
    lv_obj_set_style_border_color(enterBtn, lv_color_hex(0x4BA3B5), 0);
    lv_obj_set_style_outline_width(enterBtn, STAGE_ENTER_OUTLINE, 0);
    lv_obj_set_style_outline_color(enterBtn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(enterBtn, 0, 0);
    lv_obj_add_flag(enterBtn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_event_cb(
        enterBtn, onStageButtonEvent, LV_EVENT_SHORT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(StageAction::ENTER_CONFIRM)));

    lv_obj_t *box = lv_obj_create(enterBtn);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(box);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(box, 2, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(box);
    lv_label_set_text(lbl, "ENTER");
    lv_obj_set_style_text_font(lbl, enterFont, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *okLbl = lv_label_create(box);
    lv_label_set_text(okLbl, "(OK)");
    lv_obj_set_style_text_font(okLbl, okFont, 0);
    lv_obj_set_style_text_color(okLbl, lv_color_hex(0xE2E8F0), 0);
}

} // namespace ScreenStage
