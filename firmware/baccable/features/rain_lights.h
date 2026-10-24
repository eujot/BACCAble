#ifndef BACCABLE_RAIN_LIGHTS_H
#define BACCABLE_RAIN_LIGHTS_H
#include "stm32f0xx_hal.h"
#include <stdbool.h>

void rain_lights_process(void);
void rain_lights_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
void rain_lights_frame(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
void rain_lights_options(uint8_t enabled, uint8_t token);
void rain_lights_reply(uint8_t active, uint8_t token);
/* Debug/test surface (normal firmware, remove together with rain_test.c). */
void rain_lights_force(uint8_t on);
void rain_lights_debug(uint8_t out[4]);
#endif
