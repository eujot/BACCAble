#include "features/rain_test.h"
#include "features/rain_lights.h"
#include "app/application_state.h"
#include "third_party/printf/printf.h"

/* Rain-lights debug/test surface, compiled into normal firmware.
 * C1 runs the menu-facing state machine and force control; C2 answers the
 * poll with its detection counters. Remove this file and its call sites to
 * drop the whole test surface. */

#define RAIN_TEST_FORCE_MS 30000U
#define RAIN_TEST_POLL_MS 1000U

#if defined(BACCABLE_C1)
static uint8_t shown;
static uint8_t force_on;
static uint16_t cycles;
static uint8_t flags, tx;
static uint32_t force_started, poll_at;
#endif

void rain_test_enter(void) {
#if defined(BACCABLE_C1)
    shown = 1;
    force_on = 0;
    cycles = 0;
    flags = 0;
    tx = 0;
    force_started = 0;
    poll_at = 0;
#endif
}

void rain_test_exit(void) {
#if defined(BACCABLE_C1)
    if (force_on) {
        force_on = 0;
        rain_lights_force(0);
        uint8_t command[] = {C2BusID, C2_CMD_RAIN_TEST, 0, 0};
        board_uart_send(command, sizeof(command));
    }
    shown = 0;
#endif
}

void rain_test_toggle_force(void) {
#if defined(BACCABLE_C1)
    force_on = !force_on;
    rain_lights_force(force_on);
    uint8_t command[] = {C2BusID, C2_CMD_RAIN_TEST, force_on ? 1 : 0, 0};
    board_uart_send(command, sizeof(command));
    if (force_on)
        force_started = currentTime;
#endif
}

void rain_test_reply(uint8_t cycles_lo, uint8_t cycles_hi, uint8_t value_flags, uint8_t tx_count) {
#if defined(BACCABLE_C1)
    cycles = (uint16_t)cycles_lo | ((uint16_t)cycles_hi << 8);
    flags = value_flags;
    tx = tx_count;
#else
    (void)cycles_lo;
    (void)cycles_hi;
    (void)value_flags;
    (void)tx_count;
#endif
}

void rain_test_process(void) {
#if defined(BACCABLE_C1)
    if (force_on && currentTime - force_started >= RAIN_TEST_FORCE_MS)
        rain_test_toggle_force();
    if (!shown)
        return;
    if (currentTime - poll_at >= RAIN_TEST_POLL_MS) {
        uint8_t command[] = {C2BusID, C2_CMD_RAIN_TEST, force_on ? 1 : 0, 0};
        if (board_uart_send(command, sizeof(command)))
            poll_at = currentTime;
    }
#endif
}

void rain_test_debug_reply(uint8_t token) {
#if defined(BACCABLE_C2)
    uint8_t dbg[4];
    rain_lights_debug(dbg);
    uint8_t reply[] = {C1BusID, C1_CMD_RAIN_TEST, dbg[0], dbg[1], dbg[2], dbg[3], token};
    board_uart_send(reply, sizeof(reply));
#else
    (void)token;
#endif
}

void rain_test_render(char *first, size_t first_size, char *second, size_t second_size) {
#if defined(BACCABLE_C1)
    snprintf_(first, first_size, "Wiper %u A=%c", cycles, (flags & 1) ? '1' : '0');
    snprintf_(second, second_size, "ARM=%c TX=%u F=%s", (flags & 2) ? '1' : '0', tx,
              force_on ? "ON" : "off");
#else
    (void)first;
    (void)first_size;
    (void)second;
    (void)second_size;
#endif
}
