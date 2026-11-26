#ifndef KAIROS_POWER_STATE_H
#define KAIROS_POWER_STATE_H

#include <stdint.h>

#include "ui/nav_math.h"

namespace Kairos {

struct PowerStepResult {
    bool batterySampleDue = false;
    bool brightnessChanged = false;
    uint8_t targetBrightnessPct = 100;
    bool enterDeepSleep = false;
};

/**
 * Pure C++17 power policy state machine:
 *  - EMA-filtered battery sampling cadence (5 s)
 *  - Idle PWM dimming to 15% after 10 s, instant restore on touch
 *  - Deep sleep trigger after 15 min idle without active BLE connection
 */
class PowerPolicy {
public:
    static constexpr int64_t BAT_SAMPLE_INTERVAL_US = 5000000LL;
    static constexpr int64_t DIM_TIMEOUT_US = 10000000LL;
    static constexpr int64_t SLEEP_TIMEOUT_US = 900000000LL;
    static constexpr uint8_t FULL_BRIGHTNESS = 100;
    static constexpr uint8_t DIM_BRIGHTNESS = 15;
    static constexpr uint16_t CHARGING_MV_THRESHOLD = 4210U;

    void reset(int64_t nowUs, float initialMv) {
        m_lastActivityUs = nowUs;
        m_lastBatReadUs = nowUs;
        m_filteredMv = (initialMv < 0.0f) ? 0.0f : initialMv;
        m_dimmed = false;
    }

    /**
     * Records user touch activity; returns true if backlight should be restored immediately.
     */
    bool notifyTouch(int64_t nowUs) {
        m_lastActivityUs = nowUs;
        if (m_dimmed) {
            m_dimmed = false;
            return true;
        }
        return false;
    }

    void recordBatterySample(float rawMv) {
        const float clamped = (rawMv < 0.0f) ? 0.0f : rawMv;
        m_filteredMv = (0.80f * m_filteredMv) + (0.20f * clamped);
    }

    PowerStepResult evaluate(int64_t nowUs, bool bleConnected) {
        PowerStepResult res;
        if ((nowUs - m_lastBatReadUs) >= BAT_SAMPLE_INTERVAL_US) {
            m_lastBatReadUs = nowUs;
            res.batterySampleDue = true;
        }

        const int64_t idleUs = nowUs - m_lastActivityUs;
        if (!m_dimmed && idleUs >= DIM_TIMEOUT_US) {
            m_dimmed = true;
            res.brightnessChanged = true;
            res.targetBrightnessPct = DIM_BRIGHTNESS;
        }

        if (!bleConnected && idleUs >= SLEEP_TIMEOUT_US) {
            res.enterDeepSleep = true;
        }
        return res;
    }

    bool isDimmed() const { return m_dimmed; }
    uint16_t batteryMillivolts() const { return static_cast<uint16_t>(m_filteredMv); }
    uint8_t batteryPercent() const { return KairosMath::voltageMvToPercent(batteryMillivolts()); }
    bool isProbableCharging() const { return batteryMillivolts() >= CHARGING_MV_THRESHOLD; }

private:
    float m_filteredMv = 4000.0f;
    int64_t m_lastActivityUs = 0;
    int64_t m_lastBatReadUs = 0;
    bool m_dimmed = false;
};

} // namespace Kairos

#endif // KAIROS_POWER_STATE_H
