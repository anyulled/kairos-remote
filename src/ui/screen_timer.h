#ifndef KAIROS_SCREEN_TIMER_H
#define KAIROS_SCREEN_TIMER_H

#include <lvgl.h>

/**
 * Screen 3: Local presentation countdown timer backed by esp_timer_get_time().
 * Continues running across all tiles, supports live +1/+2/+5 min adjustments,
 * amber (<2 min) and blinking red (00:00) alerts, and 1.5s hold-to-reset.
 */
namespace ScreenTimer {

void create(lv_obj_t *parent);
bool isRunning();

} // namespace ScreenTimer

#endif // KAIROS_SCREEN_TIMER_H
