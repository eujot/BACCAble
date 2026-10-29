#include "test_report.h"
#include "vehicle/vehicle_runtime.h"
#include "app/powertrain.h"
#include "features/rain_lights.h"
#include "features/rain_test.h"
#include "features/usb_modes.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdio.h>

RuntimeState runtime_state;
SettingsState settings_state;
PedalState pedal_state;
UART_HandleTypeDef huart1, huart2;
USART_TypeDef fake_usart1, fake_usart2;
USBD_HandleTypeDef hUsbDeviceFS;
uint32_t fake_primask;
static uint32_t now = 10000, complete_at;
static uint8_t outgoing[UART_SCREEN_BUFFER_SIZE], *rx_target;
static uint8_t peer_mode[2], peer_token[2];
static uint32_t seen[2][2], max_gap[2][2], rain_seen, max_rain_gap;
static unsigned rain_polls, screens, display_polls, display_replies;
static unsigned test_c2_polls, test_bh_polls, queue_errors;
static uint32_t last_force_poll, max_force_gap;
void _putchar(char character) { (void)character; }
extern void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart);

uint32_t HAL_GetTick(void) { return now; }
void HAL_Delay(uint32_t delay) { now += delay; }
void HAL_GPIO_Init(void *port, GPIO_InitTypeDef *config) { (void)port; (void)config; }
void HAL_GPIO_DeInit(void *port, uint32_t pins) { (void)port; (void)pins; }
void HAL_NVIC_SetPriority(int a, int b, int c) { (void)a; (void)b; (void)c; }
void HAL_NVIC_EnableIRQ(int irq) { (void)irq; }
uint32_t HAL_HalfDuplex_Init(UART_HandleTypeDef *uart) { (void)uart; return HAL_OK; }
uint32_t HAL_UART_Receive_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size) {
    (void)size;
    if (uart == &huart2) rx_target = data;
    return HAL_OK;
}
uint32_t HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size) {
    assert(uart == &huart2 && size <= sizeof(outgoing));
    assert(!complete_at);
    memcpy(outgoing, data, size);
    complete_at = now + (size == UART_SCREEN_BUFFER_SIZE ? 11 : 6);
    return HAL_OK;
}
uint32_t HAL_UART_Transmit(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size, uint32_t timeout) {
    (void)timeout;
    return HAL_UART_Transmit_IT(uart, data, size);
}
void status_led_activity(void) {}
void status_led_error(void) { ++queue_errors; }
void Error_Handler(uint16_t value) { (void)value; assert(0); }
void parameter_cache_put(uint8_t id, float value, uint32_t when) { (void)id; (void)value; (void)when; }
float native_parameter_read(uint8_t id) { return id; }
void parameter_cache_reset(void) {}
void parameter_request_cancel(void) {}
void fault_reader_cancel(void) {}
void ibs_override_enable(bool enabled) { (void)enabled; }
void power_wake(void) {}
void board_sync_restart(void) {}
void usb_device_status(uint8_t out[8]) { memset(out, 0, 8); out[2] = hUsbDeviceFS.dev_state; }
void usb_device_start(uint8_t serial) { assert(serial); hUsbDeviceFS.dev_state = 0; }
void usb_device_stop(void) { hUsbDeviceFS.dev_state = 0; }
void MX_USB_DEVICE_Init(void) {}
void cdc_process(void) {}
uint8_t elm327_is_enabled(void) { return 0; }
void elm327_port_reset(void) {}
void elm327_set_enabled(uint8_t enabled) { (void)enabled; }
uint8_t CDC_Transmit_FS(uint8_t *data, uint16_t size) { (void)data; (void)size; return USBD_OK; }

uint32_t can_forward(const CAN_RxHeaderTypeDef *h, uint8_t *data) {
    (void)h; (void)data;
    assert(0); /* The C1 board no longer transmits rain-light CAN frames. */
    return HAL_OK;
}

/* Deliver actual UART parser output to the feature consumers exercised here. */
void board_commands_dispatch(const uint8_t *message) {
    assert(message[0] == C1BusID || message[0] == C1_Bh_BusID);
    if (message[1] == C1_CMD_DISPLAY_PROFILE) ++display_replies;
    if (message[1] == C1_CMD_RAIN_TEST)
        rain_test_reply(message[2], message[3], message[4], message[5], message[6]);
    if (message[1] == C1_CMD_RAIN_SENSOR)
        rain_test_sensor_reply(message[2], message[3], message[4], message[5], message[6]);
    if (message[1] == C1_CMD_USB_STATE)
        usb_modes_ack(message[2], message + 3);
}

/* Simulate auxiliary replies only after the real UART queue transmitted their requests. */
static void finish_transfer(void) {
    HAL_UART_TxCpltCallback(&huart2);
    complete_at = 0;
    uint8_t reply[UART_BUFFER_SIZE] = {C1BusID};
    if (outgoing[0] == BhBusIDparamString) { ++screens; return; }
    unsigned peer = outgoing[0] == C2BusID ? 0 : 1;
    if (outgoing[1] == BH_CMD_DISPLAY_PROFILE) {
        ++display_polls;
        if (display_polls == 2) return; /* Missing reply must not starve normal status/USB polls. */
        reply[1] = C1_CMD_DISPLAY_PROFILE; reply[2] = outgoing[2]; reply[3] = 1;
    } else if (outgoing[1] == C2_CMD_RAIN_LIGHTS && peer == 0) {
        ++rain_polls;
        if (rain_seen && now - rain_seen > max_rain_gap) max_rain_gap = now - rain_seen;
        rain_seen = now;
        return; /* C2 replaces the former C1 reply with a direct BH request. */
    } else if (outgoing[1] == C2_CMD_RAIN_TEST && peer == 0) {
        ++test_c2_polls;
        if (outgoing[2] == 1) {
            if (last_force_poll && now - last_force_poll > max_force_gap)
                max_force_gap = now - last_force_poll;
            last_force_poll = now;
        }
        reply[1] = C1_CMD_RAIN_TEST;
        reply[0] = C1_Bh_BusID;
        reply[2] = 14;
        reply[4] = (settings_state.rain_lights_enabled ? 7 : 5) | (outgoing[2] ? 8 : 0);
        reply[5] = 22;
        reply[6] = outgoing[3];
    } else if (outgoing[1] == BH_CMD_RAIN_SENSOR && peer == 1) {
        ++test_bh_polls;
        reply[1] = C1_CMD_RAIN_SENSOR;
        reply[2] = 32;
        reply[3] = 4;
        reply[4] = 1;
        reply[5] = outgoing[2];
        reply[6] = 4;
    } else {
        unsigned usb = outgoing[1] == BOARD_CMD_USB_STATE;
        if (seen[peer][usb] && now - seen[peer][usb] > max_gap[peer][usb])
            max_gap[peer][usb] = now - seen[peer][usb];
        seen[peer][usb] = now;
        if (!usb) return;
        if (outgoing[2] <= 1) { peer_mode[peer] = outgoing[2]; peer_token[peer] = outgoing[3]; }
        reply[1] = C1_CMD_USB_STATE;
        reply[2] = peer;
        reply[3] = reply[4] = peer_mode[peer];
        reply[5] = peer_token[peer];
        reply[8] = peer == 0 ? USBD_STATE_CONFIGURED : 0; /* Only C2 has a USB cable. */
    }
    for (unsigned i = 0; i < sizeof(reply); ++i) {
        assert(rx_target);
        *rx_target = reply[i];
        HAL_UART_RxCpltCallback(&huart2);
    }
}

/* Keep real rain leases and USB-only-C2 capture alive while screens and both status polls compete. */
static void test_rain_with_usb_and_display(void) {
    uart_init();
    settings_state.rain_lights_enabled = 1;
    settings_state.usb_sniffer = 1;
    usb_modes_apply();
    const uint32_t start = now;
    uint32_t display_sent = now - VEHICLE_DISPLAY_POLL_MS;
    for (; now - start < 60000; ++now) {
        if (complete_at && now >= complete_at) finish_transfer();
        board_uart_process();
        usb_modes_process();
        rain_lights_process();
        if (now - display_sent >= VEHICLE_DISPLAY_POLL_MS) {
            uint8_t command[] = {BhBusID, BH_CMD_DISPLAY_PROFILE, (uint8_t)((now - start) / 1000)};
            if (board_uart_send(command, sizeof(command))) display_sent = now;
        }
        if ((now - start) % 20 == 0) {
            uint8_t screen[UART_SCREEN_BUFFER_SIZE] = {BhBusIDparamString};
            assert(board_uart_send(screen, sizeof(screen)));
        }
        if ((now - start) % 250 == 0 && now - start > 5000) {
            CAN_RxHeaderTypeDef h = {.StdId = 0x73e, .DLC = 4, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA};
            const uint8_t data[4] = {0};
            rain_lights_frame(&h, data);
            assert(usb_modes_active());
        }
    }
    for (unsigned peer = 0; peer < 2; ++peer)
        for (unsigned usb = 0; usb < 2; ++usb) {
            assert(seen[peer][usb] && now - seen[peer][usb] < 5000);
            assert(max_gap[peer][usb] < 5000);
        }
    assert(rain_polls > 50 && max_rain_gap < 2000 && screens > 10);
    assert(queue_errors == 0);
    assert(display_polls >= 11 && display_replies == display_polls - 1);
    fprintf(stdout, "Maximum reply intervals: C2 version/USB %u/%u ms, BH %u/%u ms; rain %u ms\n",
            max_gap[0][0], max_gap[0][1], max_gap[1][0], max_gap[1][1], max_rain_gap);
    settings_state.rain_lights_enabled = 0;
    rain_lights_process();
    CAN_RxHeaderTypeDef h = {.StdId = 0x73e, .DLC = 4};
    const uint8_t data[4] = {0};
    rain_lights_frame(&h, data);
}

/* Queue rejection must leave the next USB/version choice unchanged. */
static void test_poll_retry_keeps_kind(void) {
    uint8_t first[4] = {0}, retry[4] = {0};
    uint8_t length = usb_modes_poll(0, first);
    assert(usb_modes_poll(0, retry) == length);
    assert(!memcmp(first, retry, length));
}

/* Exercise both diagnostic pages with the real UART queue and immediate peers. */
static void test_rain_test_with_shared_uart(void) {
    settings_state.usb_sniffer = 0;
    settings_state.rain_lights_enabled = 0;
    rain_test_enter();
    rain_test_move_page();
    const uint32_t start = now;
    queue_errors = 0;
    memset(seen, 0, sizeof(seen));
    memset(max_gap, 0, sizeof(max_gap));
    unsigned c2_at_bh_page = 0;
    for (; now - start < 105000; ++now) {
        unsigned elapsed = now - start;
        if (complete_at && now >= complete_at) finish_transfer();
        board_uart_process();
        usb_modes_process();
        rain_lights_process();
        rain_test_process();
        if (elapsed == 25000) {
            assert(test_c2_polls == 0 && test_bh_polls >= 10);
            rain_test_move_page();
        }
        if (elapsed == 80000)
            rain_test_move_page();
        if (elapsed == 50000) {
            assert(test_c2_polls >= 10);
            c2_at_bh_page = test_c2_polls;
            settings_state.rain_lights_enabled = 1;
            rain_test_move_page(); /* Return to BH with Rain lights enabled. */
        }
        if (elapsed == 80000)
            assert(test_c2_polls == c2_at_bh_page);
        if (elapsed == 90000) {
            rain_test_move_page(); /* Force while displaying BH. */
            rain_test_toggle_force();
        }
        if (elapsed % VEHICLE_DISPLAY_POLL_MS == 0) {
            uint8_t command[] = {BhBusID, BH_CMD_DISPLAY_PROFILE, (uint8_t)(elapsed / VEHICLE_DISPLAY_POLL_MS)};
            assert(board_uart_send(command, sizeof(command)));
        }
        unsigned screen_period = elapsed >= 90000 ? 2000 : 1500;
        if (elapsed % screen_period == 0) {
            uint8_t screen[UART_SCREEN_BUFFER_SIZE] = {BhBusIDparamString};
            assert(board_uart_send(screen, sizeof(screen)));
        }
        if (elapsed % 1000 == 0 && elapsed > 3000 &&
            elapsed != 25000 && elapsed != 50000 && elapsed != 80000 && elapsed != 90000) {
            char first[32], second[32];
            rain_test_render(first, sizeof(first), second, sizeof(second));
            if (strstr(first, "no reply"))
                fprintf(stderr, "elapsed=%u first=%s second=%s c2=%u bh=%u queue=%u\n", elapsed, first, second, test_c2_polls, test_bh_polls, queue_errors);
            assert(strstr(first, "no reply") == NULL);
            assert(strstr(first, "BH738") || strstr(first, "Wiper"));
        }
    }
    assert(test_c2_polls > 20 && test_bh_polls > 15 && max_force_gap < 3000);
    assert(queue_errors == 0);
    for (unsigned peer = 0; peer < 2; ++peer)
        for (unsigned usb = 0; usb < 2; ++usb)
            assert(seen[peer][usb] && max_gap[peer][usb] < 5000);
    rain_test_exit();
}

int main(void) {
    const HostTest tests[] = {HOST_TEST(test_rain_with_usb_and_display), HOST_TEST(test_poll_retry_keeps_kind),
                              HOST_TEST(test_rain_test_with_shared_uart)};
    host_tests_run("rain-uart", tests, sizeof(tests) / sizeof(tests[0]));
}
