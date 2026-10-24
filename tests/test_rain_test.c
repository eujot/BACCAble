#include "features/rain_test.h"
#include "features/rain_lights.h"
#include "app/application_state.h"
#include "transport/board_uart.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

RuntimeState runtime_state;
SettingsState settings_state;
static uint8_t uart[8];
static size_t uart_len;
static uint8_t uart_accept = 1;
static uint32_t now;
uint32_t HAL_GetTick(void) { return now; }
void _putchar(char c) { (void)c; }
uint8_t board_uart_send(const uint8_t *data, size_t length) {
    if (!uart_accept) return 0;
    assert(length <= sizeof(uart));
    memcpy(uart, data, length);
    uart_len = length;
    return 1;
}
static uint8_t sent[4];
static unsigned transmissions;
uint32_t can_forward(const CAN_RxHeaderTypeDef *h, uint8_t *data) {
    (void)h;
    memcpy(sent, data, sizeof(sent));
    ++transmissions;
    return HAL_OK;
}
static CAN_RxHeaderTypeDef light = {.StdId = 0x73e, .DLC = 4, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
#if defined(BACCABLE_C2)
static CAN_RxHeaderTypeDef wiper = {.StdId = 0x2ec, .DLC = 8, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
#endif

#if defined(BACCABLE_C2)
int main(void) {
    now = 10000;
    uint8_t lamp[4] = {0, 0, 0, 0x40}; /* AUTO set. */
    /* Forced injection must add the dipped bit regardless of the wiper trigger. */
    rain_lights_force(1);
    rain_lights_frame(&light, lamp);
    assert(transmissions == 1 && sent[0] == 0 && sent[1] == 0x40 && sent[3] == 0x40);
    now += 1000;
    rain_lights_force(1);
    now += 3000;
    rain_lights_frame(&light, lamp);
    assert(transmissions == 1); /* C2 expires after lost C1 polls. */
    rain_lights_force(1);
    rain_lights_frame(&light, lamp);
    assert(transmissions == 1); /* A stale ON cannot rearm without OFF. */
    rain_lights_force(0);
    rain_lights_force(1);
    for (unsigned i = 0; i < 29; ++i) {
        now += 1000;
        rain_lights_force(1);
    }
    now += 1000;
    rain_lights_frame(&light, lamp);
    assert(transmissions == 1); /* C2 also enforces the absolute limit. */
    rain_lights_force(0);

    /* Detection counters: enable + AUTO, then one low->high wiper edge. */
    rain_lights_options(1, 7);
    now += 50;
    rain_lights_observe(&light, lamp);
    now += 50;
    uint8_t low[8] = {0};
    uint8_t high[8] = {0, 0, 0x20, 0, 0, 0, 0, 0};
    rain_lights_observe(&wiper, low);
    now += 50;
    rain_lights_observe(&wiper, high);
    uint8_t dbg[4];
    rain_lights_debug(dbg);
    assert(dbg[0] == 1 && dbg[1] == 0);       /* one wiper cycle */
    assert(dbg[2] == 5);                       /* fresh auto_mode flag */
    assert(dbg[3] == 1);                       /* one forced injection */

    rain_test_debug_reply(5);
    assert(uart_len == 7);
    assert(uart[0] == C1BusID && uart[1] == C1_CMD_RAIN_TEST);
    assert(uart[2] == 1 && uart[3] == 0 && uart[4] == 5 && uart[5] == 1 && uart[6] == 5);
    rain_lights_options(0, 8);
    now += 50;
    rain_lights_observe(&wiper, low);
    now += 50;
    rain_lights_observe(&wiper, high);
    rain_lights_debug(dbg);
    assert(dbg[0] == 2 && !(dbg[2] & 2)); /* Raw edge with feature OFF. */
    now += 1100;
    rain_lights_debug(dbg);
    assert(!(dbg[2] & 4)); /* Old AUTO sample is unknown. */
    puts("Rain test C2 tests passed");
}
#else /* BACCABLE_C1 */
int main(void) {
    now = 10000;
    uint8_t lamp[4] = {0};
    rain_test_enter();
    char first[24], second[24];
    rain_test_render(first, sizeof(first), second, sizeof(second));
    assert(strstr(first, "no reply") != NULL);

    /* Force ON: command to C2 and local C1 position-lamp injection. */
    rain_test_toggle_force();
    assert(uart_len == 4 && uart[0] == C2BusID && uart[1] == C2_CMD_RAIN_TEST && uart[2] == 1);
    transmissions = 0;
    rain_lights_frame(&light, lamp);
    assert(transmissions == 1 && sent[0] == 0x02 && sent[2] == 0x80);

    /* Counter reply and two-line render. */
    rain_test_reply(0x34, 0x12, 7, 7, uart[3]); /* Fresh AUTO|ARM, tx=7. */
    rain_test_reply(0, 0, 0, 0, (uint8_t)(uart[3] - 1));
    rain_test_render(first, sizeof(first), second, sizeof(second));
    assert(strstr(first, "Wiper 4660") != NULL);
    assert(strstr(first, "A=1") != NULL);
    assert(strstr(second, "ARM=1") != NULL && strstr(second, "Q=7") != NULL);
    now += 3000;
    rain_test_render(first, sizeof(first), second, sizeof(second));
    assert(strstr(first, "no reply") != NULL);
    assert(strstr(second, "C1 only") != NULL);

    /* Force OFF. */
    rain_test_toggle_force();
    assert(uart[2] == 0);

    rain_test_toggle_force();
    uart_accept = 0;
    rain_test_exit();
    assert(uart[2] == 1); /* STOP was rejected. */
    uart_accept = 1;
    rain_test_process();
    assert(uart[2] == 0); /* STOP retried after leaving the screen. */
    rain_test_enter();

    /* Auto-off after 30 s even while the screen stays open. */
    rain_test_toggle_force(); /* ON */
    now += 30001;
    rain_test_process();
    assert(uart[2] == 0);

    /* Forced injection stops once force is cleared. */
    transmissions = 0;
    rain_lights_frame(&light, lamp);
    assert(transmissions == 0);

    rain_test_exit();
    puts("Rain test C1 tests passed");
}
#endif
