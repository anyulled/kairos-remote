#ifndef KAIROS_HID_KEYMAP_H
#define KAIROS_HID_KEYMAP_H

#include <stdint.h>

// Standard USB HID Usage Page 0x07 (Keyboard/Keypad) scan codes
constexpr uint8_t KAIROS_KEY_RETURN      = 0x28;
constexpr uint8_t KAIROS_KEY_ESC         = 0x29;
constexpr uint8_t KAIROS_KEY_SPACE       = 0x2C;
constexpr uint8_t KAIROS_KEY_HOME        = 0x4A;
constexpr uint8_t KAIROS_KEY_RIGHT_ARROW = 0x4F;
constexpr uint8_t KAIROS_KEY_LEFT_ARROW  = 0x50;
constexpr uint8_t KAIROS_KEY_DOWN_ARROW  = 0x51;
constexpr uint8_t KAIROS_KEY_UP_ARROW    = 0x52;

// Spec aliases for direct readability in UI modules
#ifndef KEY_RETURN
#define KEY_RETURN      KAIROS_KEY_RETURN
#endif
#ifndef KEY_ESC
#define KEY_ESC         KAIROS_KEY_ESC
#endif
#ifndef KEY_SPACE
#define KEY_SPACE       KAIROS_KEY_SPACE
#endif
#ifndef KEY_HOME
#define KEY_HOME        KAIROS_KEY_HOME
#endif
#ifndef KEY_RIGHT_ARROW
#define KEY_RIGHT_ARROW KAIROS_KEY_RIGHT_ARROW
#endif
#ifndef KEY_LEFT_ARROW
#define KEY_LEFT_ARROW  KAIROS_KEY_LEFT_ARROW
#endif
#ifndef KEY_DOWN_ARROW
#define KEY_DOWN_ARROW  KAIROS_KEY_DOWN_ARROW
#endif
#ifndef KEY_UP_ARROW
#define KEY_UP_ARROW    KAIROS_KEY_UP_ARROW
#endif

constexpr uint8_t HID_MOD_NONE       = 0x00;
constexpr uint8_t HID_MOD_LEFT_SHIFT = 0x02;

/**
 * Converts an ASCII character to a USB HID modifier + usage code.
 * Returns true if the character is mapped.
 */
inline bool asciiToHidKey(char c, uint8_t &modifier, uint8_t &keycode) {
    modifier = HID_MOD_NONE;
    if (c >= 'a' && c <= 'z') {
        keycode = static_cast<uint8_t>(0x04 + (c - 'a'));
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        modifier = HID_MOD_LEFT_SHIFT;
        keycode = static_cast<uint8_t>(0x04 + (c - 'A'));
        return true;
    }
    if (c >= '1' && c <= '9') {
        keycode = static_cast<uint8_t>(0x1E + (c - '1'));
        return true;
    }
    switch (c) {
        case '0':  keycode = 0x27; return true;
        case '\n':
        case '\r': keycode = KAIROS_KEY_RETURN; return true;
        case 0x1B: keycode = KAIROS_KEY_ESC; return true;
        case '\t': keycode = 0x2B; return true;
        case ' ':  keycode = KAIROS_KEY_SPACE; return true;
        case '-':  keycode = 0x2D; return true;
        case '=':  keycode = 0x2E; return true;
        case '.':  keycode = 0x37; return true;
        case '/':  keycode = 0x38; return true;
        default:   keycode = 0x00; return false;
    }
}

#endif // KAIROS_HID_KEYMAP_H
