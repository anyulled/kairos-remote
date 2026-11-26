#include <unity.h>

#include "ble/hid_keymap.h"
#include "power/power_state.h"
#include "ui/nav_math.h"
#include "ui/timer_engine.h"

using Kairos::CountdownEngine;
using Kairos::PowerPolicy;
using Kairos::PowerStepResult;
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
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::NONE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206 + 90, 206)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NavZone::NONE),
                            static_cast<uint8_t>(KairosMath::classifyRadialTouch(206 + 105, 206 + 105)));
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

void test_timer_engine_play_pause_and_live_add(void) {
    CountdownEngine timer(120 * 1000ULL); // 2:00
    int64_t now = 1000000LL;

    TEST_ASSERT_FALSE(timer.isRunning());
    TEST_ASSERT_EQUAL_UINT32(120, timer.displaySeconds(now));

    timer.togglePlayPause(now);
    TEST_ASSERT_TRUE(timer.isRunning());

    // Advance 30 s -> 90 s left (under 2 min -> amber warning)
    now += 30000000LL;
    TEST_ASSERT_FALSE(timer.tick(now));
    TEST_ASSERT_EQUAL_UINT32(90, timer.displaySeconds(now));
    TEST_ASSERT_TRUE(timer.isWarningAmber(now));

    // Live +5 min while running -> 390 s left (not amber)
    timer.addMinutes(5, now);
    TEST_ASSERT_TRUE(timer.isRunning());
    TEST_ASSERT_EQUAL_UINT32(390, timer.displaySeconds(now));
    TEST_ASSERT_FALSE(timer.isWarningAmber(now));

    // Pause keeps remaining time frozen
    timer.togglePlayPause(now);
    TEST_ASSERT_FALSE(timer.isRunning());
    now += 60000000LL;
    TEST_ASSERT_EQUAL_UINT32(390, timer.displaySeconds(now));

    // Clamps at 99:59
    timer.addMinutes(120, now);
    TEST_ASSERT_EQUAL_UINT32(5999, timer.displaySeconds(now));
}

void test_timer_engine_expiration_fires_once(void) {
    CountdownEngine timer(5000ULL); // 5 s
    int64_t now = 0;
    timer.togglePlayPause(now);

    now = 5000000LL;
    TEST_ASSERT_TRUE(timer.tick(now));
    TEST_ASSERT_TRUE(timer.isExpired());
    TEST_ASSERT_FALSE(timer.isRunning());
    TEST_ASSERT_EQUAL_UINT32(0, timer.displaySeconds(now));

    // Subsequent ticks must not re-fire alarm
    TEST_ASSERT_FALSE(timer.tick(now + 100000LL));

    // Adding +1 min clears expired state
    timer.addMinutes(1, now);
    TEST_ASSERT_FALSE(timer.isExpired());
    TEST_ASSERT_EQUAL_UINT32(60, timer.displaySeconds(now));
}

void test_timer_engine_hold_reset_1_5s(void) {
    CountdownEngine timer(15 * 60000ULL);
    int64_t now = 1000000LL;
    timer.togglePlayPause(now);

    // Hold < 1.5 s only pauses via shortStopTap and does not reset to 00:00 (1.4 s elapsed -> 899 s left)
    timer.beginStopHold(now);
    TEST_ASSERT_FALSE(timer.updateStopHold(now + 1400000LL));
    TEST_ASSERT_TRUE(timer.shortStopTap(now + 1400000LL));
    TEST_ASSERT_FALSE(timer.isRunning());
    TEST_ASSERT_EQUAL_UINT32(899, timer.displaySeconds(now + 1400000LL));

    // Hold >= 1.5 s resets to 00:00 and swallows release tap
    timer.beginStopHold(now + 2000000LL);
    TEST_ASSERT_TRUE(timer.updateStopHold(now + 3500000LL));
    TEST_ASSERT_EQUAL_UINT32(0, timer.displaySeconds(now + 3500000LL));
    TEST_ASSERT_FALSE(timer.shortStopTap(now + 3550000LL));
    timer.clearStopHold();
}

void test_power_policy_dim_restore_and_sleep(void) {
    PowerPolicy p;
    int64_t now = 0;
    p.reset(now, 4000.0f);

    // At 5 s: battery sample due, not dimmed yet
    PowerStepResult r = p.evaluate(5000000LL, false);
    TEST_ASSERT_TRUE(r.batterySampleDue);
    TEST_ASSERT_FALSE(r.brightnessChanged);
    TEST_ASSERT_FALSE(r.enterDeepSleep);

    p.recordBatterySample(3800.0f); // 0.8*4000 + 0.2*3800 = 3960 mV
    TEST_ASSERT_EQUAL_UINT16(3960, p.batteryMillivolts());
    TEST_ASSERT_FALSE(p.isProbableCharging());

    // At 10 s idle: dims to 15%
    r = p.evaluate(10000000LL, false);
    TEST_ASSERT_TRUE(r.brightnessChanged);
    TEST_ASSERT_EQUAL_UINT8(15, r.targetBrightnessPct);
    TEST_ASSERT_TRUE(p.isDimmed());

    // Touch restores immediately
    TEST_ASSERT_TRUE(p.notifyTouch(11000000LL));
    TEST_ASSERT_FALSE(p.isDimmed());

    // With BLE connected at 15 min idle: dims again, but must NOT deep sleep
    r = p.evaluate(11000000LL + PowerPolicy::SLEEP_TIMEOUT_US, true);
    TEST_ASSERT_TRUE(r.brightnessChanged);
    TEST_ASSERT_FALSE(r.enterDeepSleep);

    // Without BLE connected past 15 min idle: enters Deep Sleep
    r = p.evaluate(11000000LL + PowerPolicy::SLEEP_TIMEOUT_US + 1000LL, false);
    TEST_ASSERT_TRUE(r.enterDeepSleep);
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
    RUN_TEST(test_timer_engine_play_pause_and_live_add);
    RUN_TEST(test_timer_engine_expiration_fires_once);
    RUN_TEST(test_timer_engine_hold_reset_1_5s);
    RUN_TEST(test_power_policy_dim_restore_and_sleep);
    return UNITY_END();
}
