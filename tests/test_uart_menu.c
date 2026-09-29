#include "test_report.h"
#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include <assert.h>
#include <stdio.h>

RuntimeState runtime_state;
PedalState pedal_state;
UART_HandleTypeDef huart1, huart2;
USART_TypeDef fake_usart1, fake_usart2;
uint32_t fake_primask;
static uint32_t now = 3000, result = HAL_OK;
static const uint8_t *active;
static uint8_t expected[UART_SCREEN_BUFFER_SIZE];
static unsigned transmissions, received;
static uint16_t active_size;
static uint8_t *rx_target;
static uint16_t rx_remaining;
static uint8_t received_frame[UART_SCREEN_BUFFER_SIZE];
extern void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart);
uint32_t HAL_GetTick(void) { return now; }
void HAL_Delay(uint32_t duration) { now += duration; }
void HAL_GPIO_Init(void *port, GPIO_InitTypeDef *config) {
    (void)port;
    (void)config;
}
void HAL_NVIC_SetPriority(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
}
void HAL_NVIC_EnableIRQ(int irq) { (void)irq; }
uint32_t HAL_HalfDuplex_Init(UART_HandleTypeDef *uart) {
    (void)uart;
    return HAL_OK;
}
uint32_t HAL_UART_Receive_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size) {
    if (uart == &huart2) {
        rx_target = data;
        rx_remaining = size;
    }
    return HAL_OK;
}
uint32_t HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size) {
    (void)uart;
    assert(size == UART_BUFFER_SIZE || size == UART_SCREEN_BUFFER_SIZE);
    if (result != HAL_OK)
        return result;
    active = data;
    active_size = size;
    memcpy(expected, data, size);
    ++transmissions;
    return HAL_OK;
}
uint32_t HAL_UART_Transmit(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size, uint32_t timeout) {
    (void)timeout;
    return HAL_UART_Transmit_IT(uart, data, size);
}
void status_led_activity(void) {}
void status_led_error(void) {}
void Error_Handler(uint16_t value) {
    (void)value;
    assert(0);
}
void board_commands_dispatch(const uint8_t *message) {
#if defined(BACCABLE_C1)
    memcpy(received_frame, message, UART_BUFFER_SIZE);
#else
    memcpy(received_frame, message, sizeof(received_frame));
#endif
    ++received;
}
void usb_modes_poll_queued(uint8_t peer, const uint8_t command[4]) { (void)peer; (void)command; }
uint8_t usb_modes_poll(uint8_t peer, uint8_t command[4]) {
    command[0] = peer ? BhBusIDgetStatus : C2BusID;
    command[1] = peer ? 0 : C2cmdGetStatus;
    return 2;
}
static void receive_bytes(const uint8_t *bytes, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        assert(rx_remaining);
        *rx_target++ = bytes[i];
        if (!--rx_remaining)
            HAL_UART_RxCpltCallback(&huart2);
    }
}
static void test_receive_gap_recovery(void) {
    uart_init();
    runtime_state.low_consume_is_active = 1; /* No background TX during RX checks. */
    uint8_t frame[UART_BUFFER_SIZE];
    memset(frame, ' ', sizeof(frame));
    frame[0] = C2_Bh_BusID;
    frame[1] = C2_BH_CMD_USB_CAPTURE;
    frame[2] = 1;
    receive_bytes(frame, 5); /* Interrupted command, followed by a fresh full command. */
    now += 30;
    receive_bytes(frame, sizeof(frame));
    board_uart_process();
    assert(received == 1);
    assert(memcmp(frame, received_frame, sizeof(frame)) == 0);
    /* Back-to-back full frames, with no idle gap, must also work. */
    receive_bytes(frame, sizeof(frame));
    receive_bytes(frame, sizeof(frame));
    board_uart_process();
    assert(received == 3);
    /* Incomplete frames across tick wrap must not poison the next command. */
    now = UINT32_MAX - 10U;
    receive_bytes(frame, 3);
    now = 20;
    receive_bytes(frame, sizeof(frame));
    board_uart_process();
    assert(received == 4);
    assert(memcmp(frame, received_frame, sizeof(frame)) == 0);
    now = 3000;
    runtime_state.low_consume_is_active = 0;
}
#if defined(BACCABLE_C1)
void parameter_cache_put(uint8_t id, float value, uint32_t when) {
    (void)id;
    (void)value;
    (void)when;
}
float native_parameter_read(uint8_t id) { return id; }
static void display(char c) {
    uint8_t frame[UART_SCREEN_BUFFER_SIZE];
    memset(frame, c, sizeof(frame));
    frame[0] = BhBusIDparamString;
    assert(board_uart_send(frame, sizeof(frame)));
}
static void finish(void) {
    assert(!memcmp(active, expected, active_size));
    HAL_UART_TxCpltCallback(&huart2);
    now += 251;
    runtime_state.all_processors_wakeup_time = now; /* Keep this test independent of background polls. */
}
static void test_uart_lost_completion(void) {
    uint8_t status[5];
    runtime_state.all_processors_wakeup_time = now;
    display('X'); board_uart_process();
    board_uart_status(status); assert(status[0]);
    now += 10; board_uart_process();
    board_uart_status(status); assert(status[0] && !status[3]);
    now += 51; board_uart_process();
    board_uart_status(status); assert(!status[0] && status[3] == 1);
    now += 251;
    display('Y'); board_uart_process();
    assert(active[1] == 'Y'); finish();
    result = HAL_BUSY;
    display('Z'); board_uart_process();
    now += 51; board_uart_process();
    board_uart_status(status); assert(status[3] == 2);
    result = HAL_OK; now += 251; board_uart_process();
    assert(active[1] == 'Z'); finish();
}
static void test_uart_menu_behavior(void) {
    uart_init();
    runtime_state.all_processors_wakeup_time = now;
    const uint8_t command[] = {C2BusID, C2cmdNormalFrontBrake};
    display('A');
    assert(board_uart_send(command, sizeof(command)));
    display('B');
    fake_primask = 1;
    board_uart_process();
    assert(fake_primask == 1);
    assert(active[0] == C2BusID && active[1] == C2cmdNormalFrontBrake);
    display('C');
    assert(board_uart_send(command, sizeof(command)));
    assert(!memcmp(active, expected, active_size));
    finish();
    board_uart_process();
    assert(active[0] == C2BusID);
    finish();
    board_uart_process();
    assert(active[0] == BhBusIDparamString && active[1] == 'C');
    display('D');
    assert(!memcmp(active, expected, active_size));
    finish();
    result = HAL_BUSY;
    unsigned before = transmissions;
    board_uart_process();
    assert(transmissions == before);
    display('E');
    result = HAL_OK;
    board_uart_process();
    assert(active[1] == 'E');
    finish();
    const uint8_t poll[] = {BhBusIDgetStatus};
    assert(board_uart_send(poll, sizeof(poll)));
    display('F');
    board_uart_process();
    assert(active[1] == 'F'); /* Screen goes ahead of poll. */
    display('G');
    finish();
    board_uart_process();
    assert(active[0] == BhBusIDgetStatus); /* Continuous display updates cannot starve polls. */
    finish();
    board_uart_process();
    assert(active[0] == BhBusIDparamString && active[1] == 'G');
    finish();
    for (unsigned i = 0; i < 10; ++i)
        assert(board_uart_send(command, sizeof(command)));
    assert(!board_uart_send(command, sizeof(command)));
    assert(!board_uart_send(NULL, 0));
    puts("PASS: real UART display coalescing, command order, active buffer, HAL_BUSY and IRQ state");
}

#else
static void test_full_screen_receive(void) {
    uart_init();
    uint8_t screen[UART_SCREEN_BUFFER_SIZE], command[UART_BUFFER_SIZE];
    memset(screen, 'S', sizeof(screen)); screen[0] = BhBusIDparamString;
    screen[1] = 2; screen[sizeof(screen) - 1] = 'Z';
    memset(command, ' ', sizeof(command)); command[0] = C2_Bh_BusID;
    command[1] = C2_BH_CMD_USB_CAPTURE; command[2] = 1;
    unsigned before = received;
    receive_bytes(screen, UART_BUFFER_SIZE); board_uart_process();
    assert(received == before); /* Do not truncate a screen to legacy width. */
    receive_bytes(screen + UART_BUFFER_SIZE, sizeof(screen) - UART_BUFFER_SIZE);
    board_uart_process();
    assert(received == before + 1 && !memcmp(received_frame, screen, sizeof(screen)));
    receive_bytes(command, sizeof(command)); board_uart_process();
    assert(received == before + 2 && !memcmp(received_frame, command, sizeof(command)));
    for (unsigned i = sizeof(command); i < sizeof(received_frame); ++i) assert(received_frame[i] == ' ');
    receive_bytes(screen, sizeof(screen) - 1);
    now += 30; /* An interrupted large frame cannot swallow the next command. */
    receive_bytes(command, sizeof(command)); board_uart_process();
    assert(received == before + 3 && !memcmp(received_frame, command, sizeof(command)));
}
#endif

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(test_receive_gap_recovery),
#if defined(BACCABLE_C1)
        HOST_TEST(test_uart_lost_completion),
        HOST_TEST(test_uart_menu_behavior)
#else
        HOST_TEST(test_full_screen_receive)
#endif
    };
    const char *suite =
#if defined(BACCABLE_C2)
        "uart-rx-c2";
#elif defined(BACCABLE_BH)
        "uart-rx-bh";
#else
        "uart-menu-c1";
#endif
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
