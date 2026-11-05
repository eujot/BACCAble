#ifndef BATTERY_HISTORY_H
#define BATTERY_HISTORY_H
#include <stdbool.h>
#include "storage/battery_history_store.h"
void battery_history_init(void);
void battery_history_ignition(unsigned code, uint32_t now);
void battery_history_process(void);
bool battery_history_current(void);
bool battery_history_error(void);
#endif
