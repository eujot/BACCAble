#include "features/rain_lights.h"
#include "app/application_state.h"
#include "transport/can_bus.h"
#include "transport/board_uart.h"

#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
static uint8_t active;
static uint8_t forced;
static uint32_t link_at;
#endif
#if defined(BACCABLE_C1)
static uint32_t poll_at;
static uint8_t token, option, poll_known;
#elif defined(BACCABLE_C2)
static uint8_t enabled, auto_mode, light_known, wiper_known, phase, count;
static uint16_t wiper_cycles, lamp_tx;
static uint32_t light_at, wiper_at, last_cycle, cycles[3];

/* Forget the rain request without transmitting a lights-off command. */
static void clear_request(void) {
    active = count = wiper_known = 0;
}

/* Require live AUTO, wiper and C1 configuration reports before adding light bits. */
static bool request_valid(void) {
    return enabled && light_known && auto_mode &&
           currentTime - link_at < 3000U && currentTime - light_at <= 1000U &&
           wiper_known && currentTime - wiper_at <= 500U;
}
#endif

/* Force the bus-specific lamp bits regardless of the wiper trigger. */
void rain_lights_force(uint8_t on) {
#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
    forced = on != 0;
#else
    (void)on;
#endif
}

/* C2: copy detection counters for the C1 test menu (cycles lo/hi, flags, tx). */
void rain_lights_debug(uint8_t out[4]) {
#if defined(BACCABLE_C2)
    out[0] = (uint8_t)(wiper_cycles & 0xFF);
    out[1] = (uint8_t)(wiper_cycles >> 8);
    out[2] = (uint8_t)((auto_mode ? 1U : 0U) | (active ? 2U : 0U));
    out[3] = (uint8_t)(lamp_tx & 0xFF);
#else
    (void)out;
#endif
}

/* Renew C2 configuration and return its current rain request in the addressed reply window. */
void rain_lights_options(uint8_t value, uint8_t request_token) {
#if defined(BACCABLE_C2)
    if (!value || !enabled || currentTime - link_at >= 3000U)
        clear_request();
    enabled = value == 1;
    link_at = currentTime;
    rain_lights_process();
    uint8_t reply[] = {C1BusID, C1_CMD_RAIN_LIGHTS, active, request_token};
    board_uart_send(reply, sizeof(reply));
#else
    (void)value;
    (void)request_token;
#endif
}

/* Accept only the latest C2 poll response while the saved setting remains enabled. */
void rain_lights_reply(uint8_t value, uint8_t request_token) {
#if defined(BACCABLE_C1)
    if (poll_known && request_token == token && currentTime - poll_at < 1000U) {
        active = value == 1 && settings_state.rain_lights_enabled;
        link_at = currentTime;
    }
#else
    (void)value;
    (void)request_token;
#endif
}

/* Count rising wiper phases from C2 0x2EC, never repeated high samples or the first sample. */
void rain_lights_observe(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
#if defined(BACCABLE_C2)
    if (h->IDE != CAN_ID_STD || h->RTR != CAN_RTR_DATA)
        return;
    if (h->StdId == 0x73e && h->DLC == 4) {
        auto_mode = (data[3] & 0x40) != 0;
        light_known = 1;
        light_at = currentTime;
        if (!auto_mode) clear_request();
    }
    if (h->StdId != 0x2ec || h->DLC != 8)
        return;
    uint8_t next_phase = (data[2] & 0x20) != 0;
    if (!request_valid()) clear_request();
    if (wiper_known && next_phase && !phase) {
        ++wiper_cycles;
        if (count < 3) cycles[count++] = currentTime;
        else {
            cycles[0] = cycles[1];
            cycles[1] = cycles[2];
            cycles[2] = currentTime;
        }
        last_cycle = currentTime;
        if (count == 3 && currentTime - cycles[0] <= 15000U)
            active = 1;
    }
    phase = next_phase;
    wiper_at = currentTime;
    wiper_known = enabled && light_known && auto_mode &&
                  currentTime - link_at < 3000U && currentTime - light_at <= 1000U;
#else
    (void)h;
    (void)data;
#endif
}

/* Maintain the 60-second hold and short inter-board leases without blocking other features. */
void rain_lights_process(void) {
#if defined(BACCABLE_C1)
    if (!settings_state.rain_lights_enabled || runtime_state.low_consume_is_active ||
        currentTime - link_at >= 3000U)
        active = 0;
    if (runtime_state.low_consume_is_active) return;
    uint8_t next_option = settings_state.rain_lights_enabled == 1;
    if (poll_known && option == next_option && (!next_option || currentTime - poll_at < 1000U))
        return;
    uint8_t command[] = {C2BusID, C2_CMD_RAIN_LIGHTS, next_option, (uint8_t)(token + 1)};
    if (board_uart_send(command, sizeof(command))) {
        token++;
        option = next_option;
        poll_at = currentTime;
        poll_known = 1;
    }
#elif defined(BACCABLE_C2)
    if (!request_valid()) clear_request();
    else if (active && currentTime - last_cycle >= 60000U)
        active = count = 0;
#endif
}

/* Add only the bus-specific lamp bits to a fresh factory template, leaving release to factory AUTO. */
void rain_lights_frame(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
    if (h->IDE != CAN_ID_STD || h->RTR != CAN_RTR_DATA || h->StdId != 0x73e || h->DLC != 4)
        return;
#if defined(BACCABLE_C1)
    if (runtime_state.low_consume_is_active)
        return;
    if (!forced && (!active || !settings_state.rain_lights_enabled ||
                    currentTime - link_at >= 3000U))
        return;
    const uint8_t bits[4] = {0x02, 0, 0x80, 0}; /* C1 left/right position lamps. */
#else
    if (!forced && (!active || !request_valid() || currentTime - last_cycle >= 60000U))
        return;
    const uint8_t bits[4] = {0, 0x40, 0, 0}; /* C2 dipped beams; preserve AUTO. */
#endif
    uint8_t output[4];
    bool changed = false;
    for (unsigned i = 0; i < 4; ++i) {
        output[i] = data[i] | bits[i];
        changed |= output[i] != data[i];
    }
    if (changed && can_forward(h, output) == HAL_OK) {
#if defined(BACCABLE_C2)
        ++lamp_tx;
#endif
    }
#else
    (void)h;
    (void)data;
#endif
}
