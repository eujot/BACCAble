#ifndef BATTERY_HISTORY_STORE_H
#define BATTERY_HISTORY_STORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define HISTORY_SESSIONS 10U
#define HISTORY_SAMPLES 73U
#define HISTORY_PAGE_SIZE 2048U
#define HISTORY_PAGES 6U
#define HISTORY_SLOT_SIZE 1024U
#define HISTORY_SLOTS 12U
#define HISTORY_MISSING 65535U
typedef struct {
    const uint8_t *data;
    bool (*erase)(unsigned page);
    bool (*program)(size_t offset, uint16_t value);
} HistoryStorage;
typedef struct { uint16_t minute, voltage; uint8_t soc; } HistorySample;
/* The backend is unavailable on devices without the required physical Flash. */
HistoryStorage history_flash_storage(void);
void history_store_init(HistoryStorage storage);
bool history_store_prepare(void); /* Erase only outside active vehicle operation. */
bool history_store_begin(void);
bool history_store_append(HistorySample sample);
unsigned history_store_count(void);
unsigned history_store_samples(unsigned session);
bool history_store_read(unsigned session, unsigned sample, HistorySample *out);
#endif
