#ifndef BACCABLE_IBS_OVERRIDE_H
#define BACCABLE_IBS_OVERRIDE_H
#include <stdbool.h>
#include "app/build_config.h"
#include "stm32f0xx_hal.h"
void ibs_override_ignition(unsigned code, uint32_t now);
bool ibs_override_ignition_active(void);
void ibs_override_enable(bool enabled);
bool ibs_override_enabled(void);
void ibs_override_process(void);
#endif
