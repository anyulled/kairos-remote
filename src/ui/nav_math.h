#ifndef KAIROS_NAV_MATH_H
#define KAIROS_NAV_MATH_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/**
 * Pure C++ math and formatting helpers shared between ESP32 firmware and host unit tests.
 */
namespace KairosMath {

enum class NavZone : uint8_t {
    NONE = 0,
    CENTER_SPACE,
    ARC_UP,
    ARC_DOWN,
    ARC_LEFT,
    ARC_RIGHT,
};

constexpr int16_t SCREEN_CENTER = 206;
constexpr int16_t CENTER_BTN_RADIUS = 86;
constexpr int16_t ARC_INNER_RADIUS = 96;
constexpr int16_t ARC_OUTER_RADIUS = 206;
constexpr float RAD_TO_DEG_F = 57.29577951308232f;

/**
 * Classifies a 412x412 touch coordinate into the Screen 1 radial navigation hitboxes.
 * Includes a dead-band ring (86..96 px) and +/-4 deg diagonal gaps between adjacent arcs.
 */
inline NavZone classifyRadialTouch(int16_t x, int16_t y) {
    const int32_t dx = static_cast<int32_t>(x) - SCREEN_CENTER;
    const int32_t dy = static_cast<int32_t>(y) - SCREEN_CENTER;
    const int32_t rSq = dx * dx + dy * dy;

    if (rSq <= static_cast<int32_t>(CENTER_BTN_RADIUS) * CENTER_BTN_RADIUS) {
        return NavZone::CENTER_SPACE;
    }
    if (rSq < static_cast<int32_t>(ARC_INNER_RADIUS) * ARC_INNER_RADIUS ||
        rSq > static_cast<int32_t>(ARC_OUTER_RADIUS) * ARC_OUTER_RADIUS) {
        return NavZone::NONE;
    }

    // Angle in degrees matching LVGL arc convention: 0=Right, 90=Down, 180=Left, 270=Up.
    float angle = atan2f(static_cast<float>(dy), static_cast<float>(dx)) * RAD_TO_DEG_F;
    if (angle < 0.0f) {
        angle += 360.0f;
    }

    if (angle >= 319.0f || angle <= 41.0f) {
        return NavZone::ARC_RIGHT;
    }
    if (angle >= 49.0f && angle <= 131.0f) {
        return NavZone::ARC_DOWN;
    }
    if (angle >= 139.0f && angle <= 221.0f) {
        return NavZone::ARC_LEFT;
    }
    if (angle >= 229.0f && angle <= 311.0f) {
        return NavZone::ARC_UP;
    }
    return NavZone::NONE;
}

/**
 * Converts LiPo cell voltage in millivolts (3300..4200 mV) to percentage (0..100%)
 * using a piecewise-linear LiPo discharge curve.
 */
inline uint8_t voltageMvToPercent(uint16_t mv) {
    struct Point {
        uint16_t mv;
        uint8_t pct;
    };
    static constexpr Point kCurve[] = {
        {4200, 100}, {4100, 92}, {4000, 82}, {3900, 70},
        {3800, 56},  {3700, 42}, {3600, 22}, {3500, 10},
        {3400, 4},   {3300, 0},
    };
    if (mv >= kCurve[0].mv) {
        return 100;
    }
    constexpr size_t count = sizeof(kCurve) / sizeof(kCurve[0]);
    if (mv <= kCurve[count - 1].mv) {
        return 0;
    }
    for (size_t i = 0; i + 1 < count; ++i) {
        if (mv <= kCurve[i].mv && mv >= kCurve[i + 1].mv) {
            const uint32_t spanMv = kCurve[i].mv - kCurve[i + 1].mv;
            const uint32_t spanPct = kCurve[i].pct - kCurve[i + 1].pct;
            const uint32_t deltaMv = mv - kCurve[i + 1].mv;
            return static_cast<uint8_t>(kCurve[i + 1].pct + (deltaMv * spanPct) / spanMv);
        }
    }
    return 0;
}

/**
 * Formats seconds as MM:SS (clamped at 99:59).
 */
inline void formatTimerMMSS(uint32_t totalSeconds, char *buf, size_t bufSize) {
    if (!buf || bufSize < 6) {
        return;
    }
    if (totalSeconds > 5999U) {
        totalSeconds = 5999U;
    }
    const uint32_t mins = totalSeconds / 60U;
    const uint32_t secs = totalSeconds % 60U;
    snprintf(buf, bufSize, "%02lu:%02lu", static_cast<unsigned long>(mins), static_cast<unsigned long>(secs));
}

} // namespace KairosMath

#endif // KAIROS_NAV_MATH_H
