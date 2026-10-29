#ifndef BACCABLE_DIAGNOSTICS_PARAMETER_CATALOG_H
#define BACCABLE_DIAGNOSTICS_PARAMETER_CATALOG_H
#include "app/build_config.h"
#include "stm32f0xx_hal.h"

#define PARAMETER_DEFINITION_COUNT 98U

/* Invert byte order in a 32-bit integer. */
#define SWAP_UINT32(x)                                                                                       \
    (((uint32_t)(x) >> 24) & 0x000000FF) | (((uint32_t)(x) >> 8) & 0x0000FF00) |                             \
        (((uint32_t)(x) << 8) & 0x00FF0000) | (((uint32_t)(x) << 24) & 0xFF000000)

typedef struct {
    uint32_t request_id;
    uint32_t request_data;
    uint32_t response_id;
    int32_t raw_offset;
    float scale;
    int32_t scaled_offset;
    uint8_t request_length;
    uint8_t value_length;
    uint8_t value_offset;
} ParameterDefinition;

extern const ParameterDefinition parameter_definitions[PARAMETER_DEFINITION_COUNT];
extern uint8_t selected_parameter_element;

#endif /* BACCABLE_DIAGNOSTICS_PARAMETER_CATALOG_H */
