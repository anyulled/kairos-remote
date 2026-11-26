#ifndef KAIROS_UI_MANAGER_H
#define KAIROS_UI_MANAGER_H

#include <lvgl.h>
#include <stdint.h>

/**
 * Manages the 3-column circular LVGL Tileview container, top status bar
 * (BLE state, mute toggle, battery percentage), and bottom 3-dot indicator.
 */
namespace UiManager {

void begin();
lv_obj_t *getTile(uint8_t index);
void setActiveTile(uint8_t index, bool animate = true);
uint8_t getActiveTileIndex();

void updateBleStatus(bool connected);
void updateBattery(uint8_t percent, bool charging = false);
void refreshMuteIcon();

} // namespace UiManager

#endif // KAIROS_UI_MANAGER_H
