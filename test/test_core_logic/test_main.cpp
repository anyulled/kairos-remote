#include <unity.h>

#include "hid_keymap.h"
#include "nav_math.h"

using KairosMath::NavZone;

void setUp(void) {}
void tearDown(void) {}

void test_radial_center_space(void) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::CENTER_SPACE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206, 206)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::CENTER_SPACE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(250, 250)));
}

void test_radial_four_arcs(void) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::ARC_UP),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206, 56)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::ARC_DOWN),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206, 356)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::ARC_LEFT),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(56, 206)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::ARC_RIGHT),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(356, 206)));
}

void test_radial_deadbands(void) {
    // Ring gap between center (86) and inner arc (96)
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::NONE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206 + 90, 206)));
    // Diagonal 45 deg separator between adjacent arcs
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::NONE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206 + 105, 206 + 105)));
    // Corner outside circular display radius (206)
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::NONE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(10, 10)));
}

void test_lipo_voltage_to_percent(void) {
    TEST_ASSERT_EQUAL_UINT8(0, KairosMath::voltageMvToPercent(3100));
    TEST_ASSERT_EQUAL_UINT8(0, KairosMath::voltageMvToPercent(3300));
    TEST_ASSERT_EQUAL_UINT8(42, KairosMath::voltageMvToPercent(3700));
    TEST_ASSERT_EQUAL_UINT8(100, KairosMath::voltageMvToPercent(4200));
    TEST_ASSERT_EQUAL_UINT8(100, KairosMath::voltageMvToPercent(4350));
}

void test_timer_formatting(void) {
    char buf[8] = {};
    KairosMath::formatTimerMMSS(0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("00:00", buf);

    KairosMath::formatTimerMMSS(119, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("01:59", buf);

    KairosMath::formatTimerMMSS(900, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("15:00", buf);

    KairosMath::formatTimerMMSS(7000, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("99:59", buf);
}

void test_hid_ascii_keymap(void) {
    uint8_t mod = 0;
    uint8_t code = 0;

    TEST_ASSERT_TRUE(asciiToHidKey('b', mod, code));
    TEST_ASSERT_EQUAL_HEX8(HID_MOD_NONE, mod);
    TEST_ASSERT_EQUAL_HEX8(0x05, code);

    TEST_ASSERT_TRUE(asciiToHidKey('f', mod, code));
    TEST_ASSERT_EQUAL_HEX8(HID_MOD_NONE, mod);
    TEST_ASSERT_EQUAL_HEX8(0x09, code);

    TEST_ASSERT_TRUE(asciiToHidKey(' ', mod, code));
    TEST_ASSERT_EQUAL_HEX8(KEY_SPACE, code);

    TEST_ASSERT_TRUE(asciiToHidKey('A', mod, code));
    TEST_ASSERT_EQUAL_HEX8(HID_MOD_LEFT_SHIFT, mod);
    TEST_ASSERT_EQUAL_HEX8(0x04, code);

    TEST_ASSERT_FALSE(asciiToHidKey('\x01', mod, code));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_radial_center_space);
    RUN_TEST(test_radial_four_arcs);
    RUN_TEST(test_radial_deadbands);
    RUN_TEST(test_lipo_voltage_to_percent);
    RUN_TEST(test_timer_formatting);
    RUN_TEST(test_hid_ascii_keymap);
    return UNITY_END();
}
