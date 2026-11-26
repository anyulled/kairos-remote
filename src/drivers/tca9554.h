#ifndef KAIROS_TCA9554_H
#define KAIROS_TCA9554_H

#include <stdint.h>

/**
 * Driver for the onboard TCA9554PWR 8-bit I2C I/O expander (address 0x20).
 * Manages hardware reset lines for the SPD2010 LCD/Touch and TF card CS.
 */
namespace Tca9554 {

bool begin();
bool setPin(uint8_t bitMask, bool high);
bool pulseReset(uint8_t bitMask, uint32_t lowMs = 20, uint32_t highMs = 120);

} // namespace Tca9554

#endif // KAIROS_TCA9554_H
