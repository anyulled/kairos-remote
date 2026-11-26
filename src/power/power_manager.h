#ifndef KAIROS_POWER_MANAGER_H
#define KAIROS_POWER_MANAGER_H

#include <stdint.h>

/**
 * Manages LiPo battery ADC sampling, 10 s idle PWM auto-dimming (15%),
 * and 15 min idle + disconnected BLE Deep Sleep with physical PWR button wakeup.
 */
namespace PowerManager {

void begin();
void update(bool bleConnected);
void notifyTouchActivity();
uint8_t getBatteryPercent();
uint16_t getBatteryMillivolts();
[[noreturn]] void enterDeepSleep();

} // namespace PowerManager

#endif // KAIROS_POWER_MANAGER_H
