#ifndef KAIROS_HID_KEYBOARD_H
#define KAIROS_HID_KEYBOARD_H

#include <stdint.h>

#include "ble/hid_keymap.h"

/**
 * Non-blocking BLE HID Keyboard service built on NimBLE-Arduino.
 * Queues keystrokes to a dedicated FreeRTOS task so LVGL never stalls.
 */
namespace BleHidKeyboard {

bool begin(const char *deviceName = "Kairos-remote");
void end();

bool sendKey(uint8_t keycode, uint8_t modifier = HID_MOD_NONE);
bool sendChar(char c);
bool isConnected();
void setBatteryLevel(uint8_t percent);

} // namespace BleHidKeyboard

#endif // KAIROS_HID_KEYBOARD_H
