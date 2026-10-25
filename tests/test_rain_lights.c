#include "features/rain_lights.h"
#include "app/application_state.h"
#include "transport/board_uart.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

RuntimeState runtime_state;
SettingsState settings_state;
static uint8_t uart[4], sent[8], uart_busy, can_idle = 1;
static unsigned transmissions;
static uint32_t now;
uint32_t HAL_GetTick(void) { return now; }
uint8_t board_uart_send(const uint8_t *data, size_t length) {
    assert(length == sizeof(uart));
    if (uart_busy) return 0;
    memcpy(uart, data, length);
    return 1;
}
uint8_t can_tx_idle(void) { return can_idle; }
uint32_t can_forward(const CAN_RxHeaderTypeDef *h, uint8_t *data) {
    assert(h->StdId == 0x738 && h->DLC == 8);
    memcpy(sent, data, sizeof(sent));
    ++transmissions;
    return HAL_OK;
}

#if defined(BACCABLE_C2)
static CAN_RxHeaderTypeDef light = {.StdId = 0x73e, .DLC = 4, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
static CAN_RxHeaderTypeDef wiper = {.StdId = 0x2ec, .DLC = 8, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
static uint8_t lamp[4] = {0, 0, 0, 0x40};
static void sample(bool high) {
    uint8_t data[8] = {0, 0, high ? 0x20 : 0};
    rain_lights_observe(&light, lamp);
    rain_lights_options(1, 7);
    rain_lights_observe(&wiper, data);
    now += 50;
}
static void cycle(void) { sample(false); sample(true); sample(false); }
static void request(bool expected) {
    rain_lights_options(1, 7);
    assert(uart[0] == BhBusID && uart[1] == BH_CMD_RAIN_LIGHTS);
    assert(uart[2] == expected);
}
int main(void) {
    now = 10000;
    rain_lights_options(1, 7);
    cycle(); cycle(); request(false);
    cycle(); request(true);
    assert(transmissions == 0); /* C2 detects rain but never injects lamp frames. */
    uint8_t debug[4];
    rain_lights_debug(debug);
    assert((debug[2] & 7U) == 7U && debug[3] > 0);

    lamp[3] = 0;
    rain_lights_observe(&light, lamp);
    request(false); /* Leaving AUTO stops the BH request. */
    lamp[3] = 0x40;
    cycle(); cycle(); cycle(); request(true);
    now += 60100;
    rain_lights_observe(&light, lamp);
    rain_lights_process();
    request(false); /* Sixty seconds without wiping ends the request. */

    rain_lights_force(1);
    rain_lights_debug(debug);
    assert(debug[2] & 8U);
    now += 3000;
    rain_lights_process();
    rain_lights_debug(debug);
    assert(!(debug[2] & 8U)); /* C2 expires a lost force renewal. */
    rain_lights_force(1);
    rain_lights_debug(debug);
    assert(!(debug[2] & 8U)); /* A stale ON cannot rearm without OFF. */
    rain_lights_force(0);
    rain_lights_force(1);
    rain_lights_debug(debug);
    assert(debug[2] & 8U);
    puts("Rain lights C2 tests passed");
}
#elif defined(BACCABLE_BH)
static CAN_RxHeaderTypeDef body = {.StdId = 0x354, .DLC = 4, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
static CAN_RxHeaderTypeDef sensor = {.StdId = 0x738, .DLC = 8, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
static uint8_t report[4] = {0, 0, 0, 0x40};
static uint8_t frame[8] = {0x6d, 6, 6, 0x3b, 0xc8, 0xe8, 0, 0};
static void output(bool expected) {
    unsigned before = transmissions;
    rain_lights_frame(&sensor, frame);
    assert(transmissions == before + expected);
    if (expected) {
        assert(sent[1] == 0 && sent[2] == 0);
        assert(sent[0] == frame[0] && sent[3] == frame[3] && sent[4] == frame[4] &&
               sent[5] == frame[5] && sent[6] == frame[6] && sent[7] == frame[7]);
        assert(rain_lights_transmissions() == (uint8_t)transmissions);
    }
}
int main(void) {
    now = 10000;
    rain_lights_request(1);
    output(false); /* AUTO must be confirmed on BH. */
    rain_lights_observe(&body, report);
    output(true);
    can_idle = 0; output(false); can_idle = 1;
    frame[1] = frame[2] = 0; output(false); frame[1] = frame[2] = 6;
    sensor.DLC = 7; output(false); sensor.DLC = 8;
    report[3] = 0; rain_lights_observe(&body, report); output(false);
    report[3] = 0x40; rain_lights_observe(&body, report); output(true);
    now += 1001; output(false); /* Stale AUTO is rejected. */
    rain_lights_observe(&body, report);
    now += 2000; rain_lights_process(); output(false); /* C2 lease expired. */
    rain_lights_request(1); rain_lights_observe(&body, report); output(true);
    rain_lights_request(0); output(false);

    rain_lights_force(1); output(true); /* Forced test uses the same BH output. */
    now += 3000; rain_lights_process(); output(false);
    rain_lights_force(1); output(false); /* No rearm after an expired force. */
    rain_lights_force(0); rain_lights_force(1);
    rain_lights_observe(&body, report); output(true);
    for (unsigned i = 0; i < 29; ++i) {
        now += 1000;
        rain_lights_force(1);
        rain_lights_observe(&body, report);
    }
    now += 1000;
    rain_lights_process(); output(false); /* Absolute 30-second force limit. */
    rain_lights_force(1); output(false);
    rain_lights_force(0); rain_lights_force(1);
    rain_lights_observe(&body, report); output(true);
    runtime_state.low_consume_is_active = 1; output(false);
    puts("Rain lights BH tests passed");
}
#else /* BACCABLE_C1 */
int main(void) {
    now = 10000;
    settings_state.rain_lights_enabled = 1;
    rain_lights_process();
    assert(uart[0] == C2BusID && uart[1] == C2_CMD_RAIN_LIGHTS && uart[2] == 1);
    rain_lights_force(1);
    CAN_RxHeaderTypeDef light = {.StdId = 0x73e, .DLC = 4, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
    uint8_t lamp[4] = {0};
    rain_lights_frame(&light, lamp);
    assert(transmissions == 0); /* C1 no longer injects position lamps. */
    settings_state.rain_lights_enabled = 0;
    rain_lights_process();
    assert(uart[2] == 0);
    settings_state.rain_lights_enabled = 1;
    uart_busy = 1; rain_lights_process();
    uart_busy = 0; rain_lights_process();
    assert(uart[2] == 1);
    puts("Rain lights C1 tests passed");
}
#endif
