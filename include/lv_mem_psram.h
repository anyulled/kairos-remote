#ifndef KAIROS_LV_MEM_PSRAM_H
#define KAIROS_LV_MEM_PSRAM_H

#include <stddef.h>
#include <stdlib.h>

#ifdef ESP32
#include "esp_heap_caps.h"

static inline void *kairos_lv_malloc(size_t size) {
    void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ptr) {
        ptr = malloc(size);
    }
    return ptr;
}

static inline void *kairos_lv_realloc(void *ptr, size_t new_size) {
    void *new_ptr = heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!new_ptr && new_size > 0) {
        new_ptr = realloc(ptr, new_size);
    }
    return new_ptr;
}

static inline void kairos_lv_free(void *ptr) {
    free(ptr);
}
#else
#define kairos_lv_malloc  malloc
#define kairos_lv_realloc realloc
#define kairos_lv_free    free
#endif

#endif // KAIROS_LV_MEM_PSRAM_H
