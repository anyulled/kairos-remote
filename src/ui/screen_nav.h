#ifndef KAIROS_SCREEN_NAV_H
#define KAIROS_SCREEN_NAV_H

#include <lvgl.h>

/**
 * Screen 1: Radial slide navigation for reveal.js
 * Four 90-degree outer arcs (Up, Down, Left, Right) + central SPACE circle.
 */
namespace ScreenNav {

void create(lv_obj_t *parent);

} // namespace ScreenNav

#endif // KAIROS_SCREEN_NAV_H
