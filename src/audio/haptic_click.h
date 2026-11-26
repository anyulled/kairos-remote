#ifndef KAIROS_HAPTIC_CLICK_H
#define KAIROS_HAPTIC_CLICK_H

#include <stdint.h>

/**
 * Non-blocking I2S audio feedback for the onboard PCM5101 DAC + speaker.
 * Synthesizes a crisp 18 ms damped mechanical click and a double-pulse timer alarm.
 */
namespace HapticAudio {

bool begin();
void playClick();
void playAlarm();

void setMuted(bool muted);
bool isMuted();
bool toggleMute();

} // namespace HapticAudio

#endif // KAIROS_HAPTIC_CLICK_H
