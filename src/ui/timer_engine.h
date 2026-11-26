#ifndef KAIROS_TIMER_ENGINE_H
#define KAIROS_TIMER_ENGINE_H

#include <stdint.h>

namespace Kairos {

/**
 * Pure C++17 countdown timer state machine backed by monotonic microsecond timestamps.
 * Supports live minute increments, 2-minute warning threshold, single-shot expiration
 * notification, and 1.5 s hold-to-reset protection.
 */
class CountdownEngine {
public:
    static constexpr int64_t HOLD_RESET_US = 1500000LL;
    static constexpr uint64_t MAX_TIMER_MS = 5999000ULL;     // 99:59
    static constexpr uint64_t DEFAULT_TIMER_MS = 15ULL * 60000ULL;
    static constexpr uint64_t AMBER_THRESHOLD_MS = 120000ULL;

    explicit CountdownEngine(uint64_t initialMs = DEFAULT_TIMER_MS)
        : m_remainingMs(initialMs > MAX_TIMER_MS ? MAX_TIMER_MS : initialMs) {}

    void setRemainingSeconds(uint32_t seconds) {
        m_running = false;
        m_expired = false;
        uint64_t ms = static_cast<uint64_t>(seconds) * 1000ULL;
        m_remainingMs = (ms > MAX_TIMER_MS) ? MAX_TIMER_MS : ms;
    }

    uint64_t remainingMs(int64_t nowUs) const {
        if (!m_running) {
            return m_remainingMs;
        }
        if (nowUs >= m_targetEndUs) {
            return 0;
        }
        return static_cast<uint64_t>((m_targetEndUs - nowUs) / 1000LL);
    }

    uint32_t displaySeconds(int64_t nowUs) const {
        const uint64_t rem = remainingMs(nowUs);
        return static_cast<uint32_t>((rem + 999ULL) / 1000ULL);
    }

    bool isRunning() const { return m_running; }
    bool isExpired() const { return m_expired; }
    bool isWarningAmber(int64_t nowUs) const {
        const uint64_t rem = remainingMs(nowUs);
        return !m_expired && rem > 0 && rem < AMBER_THRESHOLD_MS;
    }

    void togglePlayPause(int64_t nowUs) {
        m_resetTriggered = false;
        if (m_running) {
            m_remainingMs = remainingMs(nowUs);
            m_running = false;
        } else if (m_remainingMs > 0) {
            m_expired = false;
            m_targetEndUs = nowUs + static_cast<int64_t>(m_remainingMs) * 1000LL;
            m_running = true;
        }
    }

    void addMinutes(uint32_t minutes, int64_t nowUs) {
        const uint64_t addMs = static_cast<uint64_t>(minutes) * 60000ULL;
        m_expired = false;
        m_resetTriggered = false;

        uint64_t nextMs = remainingMs(nowUs) + addMs;
        if (nextMs > MAX_TIMER_MS) {
            nextMs = MAX_TIMER_MS;
        }
        m_remainingMs = nextMs;
        if (m_running) {
            m_targetEndUs = nowUs + static_cast<int64_t>(nextMs) * 1000LL;
        }
    }

    /**
     * Advances timer state. Returns true on the exact transition to 00:00 (expired).
     */
    bool tick(int64_t nowUs) {
        if (m_running && nowUs >= m_targetEndUs) {
            m_running = false;
            m_remainingMs = 0;
            if (!m_expired) {
                m_expired = true;
                return true;
            }
        }
        return false;
    }

    void beginStopHold(int64_t nowUs) {
        m_stopPressStartUs = nowUs;
        m_resetTriggered = false;
    }

    /**
     * Checks hold duration; returns true once when >= 1.5 s threshold triggers reset.
     */
    bool updateStopHold(int64_t nowUs) {
        if (!m_resetTriggered && (nowUs - m_stopPressStartUs) >= HOLD_RESET_US) {
            m_resetTriggered = true;
            m_running = false;
            m_expired = false;
            m_remainingMs = 0;
            return true;
        }
        return false;
    }

    /**
     * Handles short tap on STOP/RESET. Returns true if it paused/cleared instead of already reset.
     */
    bool shortStopTap(int64_t nowUs) {
        if (m_resetTriggered) {
            return false;
        }
        if (m_running) {
            m_remainingMs = remainingMs(nowUs);
            m_running = false;
        }
        m_expired = false;
        return true;
    }

    void clearStopHold() {
        m_resetTriggered = false;
    }

private:
    uint64_t m_remainingMs = DEFAULT_TIMER_MS;
    int64_t m_targetEndUs = 0;
    bool m_running = false;
    bool m_expired = false;
    int64_t m_stopPressStartUs = 0;
    bool m_resetTriggered = false;
};

} // namespace Kairos

#endif // KAIROS_TIMER_ENGINE_H
