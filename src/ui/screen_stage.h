#ifndef KAIROS_SCREEN_STAGE_H
#define KAIROS_SCREEN_STAGE_H

#include <lvgl.h>

/**
 * Screen 2: Stage control shortcuts for reveal.js.
 * Four symmetric quadrants (ESC Overview, 'b' Blackout, HOME First slide, 'f' Fullscreen)
 * surrounding a central ENTER confirmation button.
 */
namespace ScreenStage {

void create(lv_obj_t *parent);

} // namespace ScreenStage

#endif // KAIROS_SCREEN_STAGE_H
