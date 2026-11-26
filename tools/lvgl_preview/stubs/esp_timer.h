#ifndef KAIROS_PREVIEW_ESP_TIMER_H
#define KAIROS_PREVIEW_ESP_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t esp_timer_get_time(void);

#ifdef __cplusplus
}
#endif

#endif // KAIROS_PREVIEW_ESP_TIMER_H
