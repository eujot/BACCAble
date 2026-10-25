#include "features/rain_lights.h"
#include "app/application_state.h"
#include "transport/can_bus.h"
#include "transport/board_uart.h"
#include <string.h>

#if defined(BACCABLE_C2) || defined(BACCABLE_BH)
static uint8_t active, forced, force_exhausted;
static uint32_t link_at, force_at, force_started;

#define RAIN_TEST_FORCE_LEASE_MS 3000U
#define RAIN_TEST_FORCE_LIMIT_MS 30000U

/* C2 and BH drop a forced test if C1 renewals stop. */
static bool force_valid(void) {
    return forced && currentTime - force_at < RAIN_TEST_FORCE_LEASE_MS &&
           currentTime - force_started < RAIN_TEST_FORCE_LIMIT_MS;
}
#endif
#if defined(BACCABLE_C1)
static uint32_t poll_at;
static uint8_t token, option, poll_known;
#elif defined(BACCABLE_C2)
static uint8_t enabled, auto_mode, light_known, wiper_known, phase, count;
static uint8_t debug_wiper_known, debug_phase;
static uint16_t wiper_cycles, request_tx;
static uint32_t light_at, wiper_at, last_cycle, cycles[3];
#elif defined(BACCABLE_BH)
static uint8_t auto_mode, light_known, injected_frames;
static uint32_t light_at;
#endif

#if defined(BACCABLE_C2)
/* Forget the rain request without transmitting a lights-off command. */
static void clear_request(void) {
    active = count = wiper_known = 0;
}

/* Require live AUTO, wiper and C1 configuration reports before requesting darkness. */
static bool request_valid(void) {
    return enabled && light_known && auto_mode &&
           currentTime - link_at < 3000U && currentTime - light_at <= 1000U &&
           wiper_known && currentTime - wiper_at <= 500U;
}
#endif

/* Keep a forced test alive only while fresh C1 commands arrive. */
void rain_lights_force(uint8_t on) {
#if defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (!on) {
        forced = force_exhausted = 0;
        return;
    }
    if (forced && !force_valid()) {
        forced = 0;
        force_exhausted = 1;
    }
    if (force_exhausted) return;
    if (!forced)
        force_started = currentTime;
    force_at = currentTime;
    forced = on != 0;
#else
    (void)on;
#endif
}

/* C2: report wiper cycles, AUTO, request state and queued BH requests. */
void rain_lights_debug(uint8_t out[4]) {
#if defined(BACCABLE_C2)
    out[0] = (uint8_t)(wiper_cycles & 0xFF);
    out[1] = (uint8_t)(wiper_cycles >> 8);
    bool light_fresh = light_known && currentTime - light_at <= 1000U;
    out[2] = (uint8_t)((light_fresh && auto_mode ? 1U : 0U) |
                       (active && request_valid() ? 2U : 0U) |
                       (light_fresh ? 4U : 0U) | (force_valid() ? 8U : 0U));
    out[3] = (uint8_t)(request_tx & 0xFF);
#else
    (void)out;
#endif
}

/* BH reports CAN queue admissions separately from C2's request counter. */
uint8_t rain_lights_transmissions(void) {
#if defined(BACCABLE_BH)
    return injected_frames;
#else
    return 0;
#endif
}

/* Renew C2 configuration and send the request directly to BH on the shared UART. */
void rain_lights_options(uint8_t value, uint8_t request_token) {
#if defined(BACCABLE_C2)
    if (!value || !enabled || currentTime - link_at >= 3000U)
        clear_request();
    enabled = value == 1;
    link_at = currentTime;
    rain_lights_process();
    uint8_t requested = active && request_valid();
    uint8_t reply[] = {BhBusID, BH_CMD_RAIN_LIGHTS, requested, request_token};
    if (board_uart_send(reply, sizeof(reply)) && requested)
        ++request_tx;
#else
    (void)value;
    (void)request_token;
#endif
}

/* BH accepts a short-lived C2 request; an OFF message releases it immediately. */
void rain_lights_request(uint8_t value) {
#if defined(BACCABLE_BH)
    active = value == 1;
    link_at = currentTime;
#else
    (void)value;
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
    if (debug_wiper_known && currentTime - wiper_at <= 500U && next_phase && !debug_phase)
        ++wiper_cycles;
    debug_wiper_known = 1;
    debug_phase = next_phase;
    if (!request_valid()) clear_request();
    if (wiper_known && next_phase && !phase) {
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
#elif defined(BACCABLE_BH)
    if (h->IDE == CAN_ID_STD && h->RTR == CAN_RTR_DATA && h->StdId == 0x354 && h->DLC == 4) {
        auto_mode = (data[3] & 0x40U) != 0;
        light_known = 1;
        light_at = currentTime;
    }
#else
    (void)h;
    (void)data;
#endif
}

/* Maintain the 60-second hold and short inter-board leases without blocking other features. */
void rain_lights_process(void) {
#if defined(BACCABLE_C1)
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
    if (forced && !force_valid()) {
        forced = 0;
        force_exhausted = 1;
    }
    if (!request_valid()) clear_request();
    else if (active && currentTime - last_cycle >= 60000U)
        active = count = 0;
#elif defined(BACCABLE_BH)
    if (active && currentTime - link_at >= 3000U)
        active = 0;
    if (forced && !force_valid()) {
        forced = 0;
        force_exhausted = 1;
    }
#endif
}

/* Follow each fresh factory sensor frame with a dark sample while AUTO is selected. */
void rain_lights_frame(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
#if defined(BACCABLE_BH)
    if (h->IDE != CAN_ID_STD || h->RTR != CAN_RTR_DATA || h->StdId != 0x738 || h->DLC != 8 ||
        data[6] != 0 || data[7] != 0 || data[1] > 7 || data[2] > 7)
        return;
    if (runtime_state.low_consume_is_active || !light_known || !auto_mode ||
        currentTime - light_at > 1000U ||
        (!force_valid() && (!active || currentTime - link_at >= 3000U)))
        return;
    if ((data[1] == 0 && data[2] == 0) || !can_tx_idle())
        return;
    uint8_t output[8];
    memcpy(output, data, sizeof(output));
    output[1] = output[2] = 0;
    if (can_forward(h, output) == HAL_OK)
        ++injected_frames;
#else
    (void)h;
    (void)data;
#endif
}
