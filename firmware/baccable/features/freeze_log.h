#ifndef FREEZE_LOG_H
#define FREEZE_LOG_H
#include "app/build_config.h"
#if defined(FREEZE_DIAGNOSTICS) && !defined(ACT_AS_CANABLE)
    #include <stdint.h>
extern volatile uint32_t freeze_alloc_failures;
extern volatile uint8_t freeze_disk_busy;
uint32_t freeze_heap_boundary(void);
void freeze_log_boot(void);
void freeze_log_process(void);
#else
static inline void freeze_log_boot(void) {}
static inline void freeze_log_process(void) {}
#endif
#endif
