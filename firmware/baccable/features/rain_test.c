#include "features/rain_test.h"
#include "features/rain_lights.h"
#include "app/application_state.h"
#include "third_party/printf/printf.h"

/* Rain-lights debug/test surface, compiled into normal firmware.
 * C1 owns the menu and force control; C2 reports wiper counters and BH
 * reports two raw light-responsive bytes. Remove its call sites with this file. */

#define RAIN_TEST_FORCE_MS 30000U
#define RAIN_TEST_POLL_MS 1000U

#if defined(BACCABLE_C1)
static uint8_t shown;
static uint8_t force_on;
static uint8_t stop_pending, poll_known, reply_known, token;
static uint16_t cycles;
static uint8_t flags, tx;
static uint32_t force_started, poll_at, reply_at;
static uint8_t sensor_page, sensor_poll_known, sensor_reply_known, sensor_token;
static uint8_t sensor_first, sensor_second, sensor_valid;
static uint32_t sensor_poll_at, sensor_reply_at;

/* Queue one C2 command and remember which reply belongs to it. */
static bool send_test_command(void) {
    uint8_t next_token = (uint8_t)(token + 1U);
    uint8_t command[] = {C2BusID, C2_CMD_RAIN_TEST, force_on ? 1 : 0, next_token};
    if (!board_uart_send(command, sizeof(command))) return false;
    token = next_token;
    poll_at = currentTime;
    poll_known = 1;
    if (!force_on) stop_pending = 0;
    return true;
}

/* Ask BH for its latest passive 0x738 sample without changing any lamp state. */
static void request_sensor(void) {
    uint8_t next_token = (uint8_t)(sensor_token + 1U);
    uint8_t command[] = {BhBusID, BH_CMD_RAIN_SENSOR, next_token};
    if (!board_uart_send(command, sizeof(command))) return;
    sensor_token = next_token;
    sensor_poll_at = currentTime;
    sensor_poll_known = 1;
}
#elif defined(BACCABLE_BH)
static uint8_t sensor_first, sensor_second, sensor_seen;
static uint32_t sensor_seen_at;
#endif

void rain_test_enter(void) {
#if defined(BACCABLE_C1)
    shown = 1;
    force_on = 0;
    cycles = 0;
    flags = 0;
    tx = 0;
    poll_known = reply_known = 0;
    sensor_page = sensor_poll_known = sensor_reply_known = sensor_valid = 0;
    force_started = 0;
#endif
}

void rain_test_exit(void) {
#if defined(BACCABLE_C1)
    force_on = 0;
    rain_lights_force(0);
    stop_pending = 1;
    send_test_command();
    shown = 0;
#endif
}

void rain_test_toggle_force(void) {
#if defined(BACCABLE_C1)
    force_on = !force_on;
    rain_lights_force(force_on);
    stop_pending = !force_on;
    send_test_command();
    if (force_on)
        force_started = currentTime;
#endif
}

void rain_test_reply(uint8_t cycles_lo, uint8_t cycles_hi, uint8_t value_flags, uint8_t tx_count,
                     uint8_t reply_token) {
#if defined(BACCABLE_C1)
    if (!shown || !poll_known || reply_token != token || currentTime - poll_at >= RAIN_TEST_POLL_MS)
        return;
    cycles = (uint16_t)cycles_lo | ((uint16_t)cycles_hi << 8);
    flags = value_flags;
    tx = tx_count;
    reply_known = 1;
    reply_at = currentTime;
#else
    (void)cycles_lo;
    (void)cycles_hi;
    (void)value_flags;
    (void)tx_count;
    (void)reply_token;
#endif
}

/* Ignore delayed and mismatched BH replies from an earlier request or screen. */
void rain_test_sensor_reply(uint8_t first, uint8_t second, uint8_t valid, uint8_t reply_token) {
#if defined(BACCABLE_C1)
    if (!shown || !sensor_poll_known || reply_token != sensor_token ||
        currentTime - sensor_poll_at >= RAIN_TEST_POLL_MS)
        return;
    sensor_first = first;
    sensor_second = second;
    sensor_valid = valid == 1;
    sensor_reply_known = 1;
    sensor_reply_at = currentTime;
#else
    (void)first; (void)second; (void)valid; (void)reply_token;
#endif
}

/* Keep the original rain counters and the BH raw sample on separate pages. */
void rain_test_move_page(void) {
#if defined(BACCABLE_C1)
    sensor_page = !sensor_page;
#endif
}

/* Record the two independent BH channels only from the observed frame shape. */
void rain_test_sensor_observe(const CAN_RxHeaderTypeDef *header, const uint8_t data[8]) {
#if defined(BACCABLE_BH)
    if (header->IDE != CAN_ID_STD || header->RTR != CAN_RTR_DATA ||
        header->StdId != 0x738 || header->DLC != 8)
        return;
    sensor_first = data[1];
    sensor_second = data[2];
    sensor_seen_at = currentTime;
    sensor_seen = 1;
#else
    (void)header; (void)data;
#endif
}

/* Return raw values with a freshness flag; zero is a valid dark sample. */
void rain_test_sensor_debug_reply(uint8_t request_token) {
#if defined(BACCABLE_BH)
    uint8_t fresh = sensor_seen && currentTime - sensor_seen_at <= 2500U;
    uint8_t reply[] = {C1BusID, C1_CMD_RAIN_SENSOR, sensor_first, sensor_second,
                       fresh, request_token};
    board_uart_send(reply, sizeof(reply));
#else
    (void)request_token;
#endif
}

void rain_test_process(void) {
#if defined(BACCABLE_C1)
    if (force_on && currentTime - force_started >= RAIN_TEST_FORCE_MS)
        rain_test_toggle_force();
    if (stop_pending) {
        send_test_command();
        if (stop_pending) return;
    }
    if (!shown)
        return;
    if (!poll_known || currentTime - poll_at >= RAIN_TEST_POLL_MS)
        send_test_command();
    if (sensor_page && (!sensor_poll_known || currentTime - sensor_poll_at >= RAIN_TEST_POLL_MS))
        request_sensor();
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
    if (sensor_page) {
        if (!sensor_reply_known || currentTime - sensor_reply_at >= 3000U) {
            snprintf_(first, first_size, "BH738 no reply");
            snprintf_(second, second_size, "Check BH link");
        } else if (!sensor_valid) {
            snprintf_(first, first_size, "BH738 no frame");
            snprintf_(second, second_size, "Wait for 0x738");
        } else {
            snprintf_(first, first_size, "BH738 %u/%u", sensor_first, sensor_second);
            snprintf_(second, second_size, "Raw b1/b2 (no lux)");
        }
        return;
    }
    if (!reply_known || currentTime - reply_at >= 3000U) {
        snprintf_(first, first_size, "C2: no reply");
        snprintf_(second, second_size, "F=%s (C1 only)", force_on ? "ON" : "off");
        return;
    }
    snprintf_(first, first_size, "Wiper %u A=%c", cycles,
              (flags & 4) ? ((flags & 1) ? '1' : '0') : '?');
    snprintf_(second, second_size, "ARM=%c Q=%u F=%s", (flags & 2) ? '1' : '0', tx,
              force_on ? "ON" : "off");
#else
    (void)first;
    (void)first_size;
    (void)second;
    (void)second_size;
#endif
}
