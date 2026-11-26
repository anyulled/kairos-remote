#include "drivers/tca9554.h"

#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"

namespace {

constexpr uint8_t REG_OUTPUT = 0x01;
constexpr uint8_t REG_CONFIG = 0x03;

uint8_t s_outputState = 0xFF;

bool writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(TCA9554_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool readReg(uint8_t reg, uint8_t &value) {
    Wire.beginTransmission(TCA9554_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(static_cast<uint8_t>(TCA9554_I2C_ADDR), static_cast<uint8_t>(1)) != 1) {
        return false;
    }
    value = Wire.read();
    return true;
}

} // namespace

namespace Tca9554 {

bool begin() {
    // Preload outputs HIGH before switching direction so reset/CS lines don't glitch LOW.
    s_outputState = 0xFF;
    if (!writeReg(REG_OUTPUT, s_outputState)) {
        return false;
    }
    if (!writeReg(REG_CONFIG, TCA9554_CONFIG_DIR)) {
        return false;
    }
    readReg(REG_OUTPUT, s_outputState);
    return true;
}

bool setPin(uint8_t bitMask, bool high) {
    if (high) {
        s_outputState |= bitMask;
    } else {
        s_outputState &= static_cast<uint8_t>(~bitMask);
    }
    return writeReg(REG_OUTPUT, s_outputState);
}

bool pulseReset(uint8_t bitMask, uint32_t lowMs, uint32_t highMs) {
    if (!setPin(bitMask, false)) {
        return false;
    }
    delay(lowMs);
    if (!setPin(bitMask, true)) {
        return false;
    }
    delay(highMs);
    return true;
}

} // namespace Tca9554
