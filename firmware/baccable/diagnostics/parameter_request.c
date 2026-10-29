#include "app/powertrain.h"
#include "diagnostics/parameter_request.h"
#include "diagnostics/uds_decode.h"
#include "diagnostics/parameter_cache.h"
#include "features/menu.h"
#if defined(BACCABLE_C1)
static struct {
    uint8_t active;
    uint8_t parameter;
    uint32_t started;
} request;
static bool draining;
static uint32_t drain_started;

/* Discard a pending reading when its screen is no longer relevant. */
void parameter_request_cancel(void) {
    if (request.active) { draining = true; drain_started = request.started; }
    request.active = 0;
}

/* Ask the vehicle for a single diagnostic source (UDS request_id > 0xff). */
void parameter_request_begin_id(uint8_t id, uint8_t element) {
    (void)element;
    if (draining && currentTime - drain_started < 500U) return;
    draining = false;
    if (vehicle_parameter_support(id) == VEHICLE_SUPPORT_UNSUPPORTED) return;
    if (request.active && currentTime - request.started < 500)
        return;
    request.active = 0;
    if (id >= PARAMETER_DEFINITION_COUNT)
        return;
    const ParameterDefinition *parameter = &parameter_definitions[id];
    if (parameter->request_id <= 0xff || parameter->request_length != 4)
        return;
    uint8_t data[8] = {0};
    for (unsigned i = 0; i < 4; ++i)
        data[i] = parameter->request_data >> (8 * i);
    CAN_TxHeaderTypeDef header = {
        .IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = parameter->request_id, .DLC = 4};
    if (can_tx(&header, data) != HAL_OK)
        return;
    request.parameter = id;
    request.started = currentTime;
    request.active = 1;
}

/* Accept a matching diagnostic reply and update the shared cache. */
void parameter_request_receive(const CAN_RxHeaderTypeDef *header, const uint8_t *data) {
    if (!request.active)
        return;
    if (currentTime - request.started > 500 || !dashboard_state.baccable_dashboard_menu_visible ||
        !menu_parameters_active()) {
        request.active = 0;
        return;
    }
    const ParameterDefinition *parameter = &parameter_definitions[request.parameter];
    uint16_t did = ((parameter->request_data >> 16) & 0xff) << 8 | (parameter->request_data >> 24);
    float value;
    if (header->ExtId != parameter->response_id ||
        !uds_decode_value(data, header->DLC, did, parameter->value_offset, parameter->value_length,
                          parameter->raw_offset, parameter->scale, parameter->scaled_offset, &value))
        return;
    parameter_cache_put(request.parameter, value, currentTime);
    request.active = 0;
    menu_render();
}
#endif
