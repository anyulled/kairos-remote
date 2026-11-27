#ifndef KAIROS_BOARD_HAL_H
#define KAIROS_BOARD_HAL_H

#ifdef KAIROS_BOARD_WAVESHARE_128
    #include "drivers/display_gc9a01.h"
    #include "drivers/touch_cst816s.h"
    namespace DisplayHal = DisplayGc9a01;
    namespace TouchHal = TouchCst816s;
#else
    #include "drivers/display_spd2010.h"
    #include "drivers/touch_spd2010.h"
    namespace DisplayHal = DisplaySpd2010;
    namespace TouchHal = TouchSpd2010;
#endif

#endif // KAIROS_BOARD_HAL_H
