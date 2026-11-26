#include "Arduino.h"
#include "esp_timer.h"

#include "audio/haptic_click.h"
#include "ble/hid_keyboard.h"

namespace {
uint32_t g_fakeMillis = 100;
bool g_muted = false;
} // namespace

extern "C" uint32_t millis(void) {
    return g_fakeMillis;
}

extern "C" int64_t esp_timer_get_time(void) {
    return static_cast<int64_t>(g_fakeMillis) * 1000LL;
}

namespace HapticAudio {
bool begin() { return true; }
void playClick() {}
void playAlarm() {}
void setMuted(bool muted) { g_muted = muted; }
bool isMuted() { return g_muted; }
bool toggleMute() {
    g_muted = !g_muted;
    return g_muted;
}
} // namespace HapticAudio

namespace BleHidKeyboard {
bool begin(const char *) { return true; }
void end() {}
bool sendKey(uint8_t, uint8_t) { return true; }
bool sendChar(char) { return true; }
bool isConnected() { return true; }
void setBatteryLevel(uint8_t) {}
} // namespace BleHidKeyboard
