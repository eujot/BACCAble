#include "features/rain_lights.h"
#include "app/application_state.h"
#include "transport/board_uart.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

RuntimeState runtime_state;
SettingsState settings_state;
static uint8_t uart[4], sent[4];
static unsigned transmissions;
static uint8_t uart_busy;
static uint32_t now;
uint32_t HAL_GetTick(void) { return now; }
uint8_t board_uart_send(const uint8_t *data, size_t length) {
    assert(length == sizeof(uart));
    if (uart_busy) return 0;
    memcpy(uart, data, length);
    return 1;
}
uint32_t can_forward(const CAN_RxHeaderTypeDef *h, uint8_t *data) {
    assert(h->StdId == 0x73e && h->DLC == 4);
    memcpy(sent, data, sizeof(sent));
    ++transmissions;
    return HAL_OK;
}
static CAN_RxHeaderTypeDef light = {.StdId = 0x73e, .DLC = 4, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
static uint8_t lamp[4] = {0x10, 0x80, 0x08, 0x40};
static void output(bool expected) {
    unsigned before = transmissions;
    rain_lights_frame(&light, lamp);
    assert(transmissions == before + expected);
    if (expected) {
#if defined(BACCABLE_C2)
        assert(sent[0] == lamp[0] && sent[1] == (lamp[1] | 0x40));
        assert(sent[2] == lamp[2] && sent[3] == lamp[3]);
#else
        assert(sent[0] == (lamp[0] | 2) && sent[1] == lamp[1]);
        assert(sent[2] == (lamp[2] | 0x80) && sent[3] == lamp[3]);
#endif
    }
}
#if defined(BACCABLE_C2)
static CAN_RxHeaderTypeDef wiper = {.StdId = 0x2ec, .DLC = 8, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
/* Feed 50 ms reports independently of the feature's internal implementation. */
static void sample(bool high) {
    uint8_t data[8] = {0, 0, high ? 0x20 : 0};
    rain_lights_observe(&light, lamp);
    rain_lights_options(1, 7);
    rain_lights_observe(&wiper, data);
    rain_lights_process();
    now += 50;
}
static void wait_ms(unsigned milliseconds) {
    for (unsigned t = 0; t < milliseconds; t += 50) sample(false);
}
static void cycle(void) { sample(false); sample(true); sample(true); sample(false); }
static void reset(void) {
    rain_lights_options(0, 7);
    rain_lights_options(1, 7);
    sample(false);
}
int main(void) {
    now = 10000;
    reset();
    /* First high and repeated highs cannot invent cycles; malformed frames cannot count. */
    rain_lights_options(0, 7); rain_lights_options(1, 7);
    sample(true); sample(true); output(false);
    reset();
    wiper.DLC = 7; cycle(); cycle(); cycle(); output(false); wiper.DLC = 8;
    wiper.IDE = CAN_ID_EXT; cycle(); cycle(); cycle(); output(false); wiper.IDE = CAN_ID_STD;
    wiper.RTR = CAN_RTR_REMOTE; cycle(); cycle(); cycle(); output(false); wiper.RTR = CAN_RTR_DATA;
    reset(); cycle(); cycle(); output(false); cycle(); output(true);
    /* Stop counting a held high as activity and expire at 60 s after the last rising edge. */
    wait_ms(59000); output(true); wait_ms(1000); output(false);
    reset(); cycle(); wait_ms(7300); cycle(); wait_ms(7300); cycle(); output(true);
    reset(); cycle(); wait_ms(7300); cycle(); wait_ms(7350); cycle(); output(false);
    reset(); cycle(); wait_ms(8000); cycle(); wait_ms(8000); cycle(); output(false);
    cycle(); cycle(); output(true); /* Sliding window recovers from an old first cycle. */
    wait_ms(59000); cycle(); wait_ms(2000); output(true);
    /* Leaving AUTO cancels immediately, with no lights-off transmission. */
    lamp[3] = 0; rain_lights_observe(&light, lamp); output(false);
    lamp[3] = 0x40; reset(); cycle(); cycle(); cycle(); output(true);
    rain_lights_options(0, 7); output(false);
    reset(); cycle(); cycle(); cycle(); output(true);
    now += 501; output(false); rain_lights_process(); sample(false); output(false);
    /* Missing C1 configuration expires even if lamp and wiper data continue. */
    reset(); cycle(); cycle(); cycle(); output(true);
    now += 3000; rain_lights_observe(&light, lamp); output(false);
    /* Unsigned elapsed-time arithmetic must work across tick wrap. */
    now = UINT32_MAX - 1000; reset(); cycle(); cycle(); cycle(); output(true);
    wait_ms(2000); output(true);
    light.DLC = 8; output(false);
    puts("Rain lights C2 tests passed");
}
#else
int main(void) {
    now = 10000;
    settings_state.rain_lights_enabled = 1;
    rain_lights_process();
    assert(uart[0] == C2BusID && uart[1] == C2_CMD_RAIN_LIGHTS && uart[2] == 1);
    uint8_t token = uart[3];
    rain_lights_reply(1, token - 1); output(false);
    rain_lights_reply(1, token); output(true);
    now += 3000; output(false);
    rain_lights_process(); rain_lights_reply(1, uart[3]); output(true);
    settings_state.rain_lights_enabled = 0; output(false); rain_lights_process();
    assert(uart[2] == 0);
    rain_lights_reply(1, uart[3]); output(false);
    settings_state.rain_lights_enabled = 1;
    uart_busy = 1; rain_lights_process(); output(false);
    uart_busy = 0; rain_lights_process(); rain_lights_reply(1, uart[3]); output(true);
    runtime_state.low_consume_is_active = 1; output(false); rain_lights_process();
    runtime_state.low_consume_is_active = 0; output(false);
    now = UINT32_MAX - 100; rain_lights_process(); rain_lights_reply(1, uart[3]); output(true);
    now += 500; output(true); now += 2500; output(false);
    puts("Rain lights C1 tests passed");
}
#endif
