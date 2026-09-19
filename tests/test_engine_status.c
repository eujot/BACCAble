#include "vehicle/engine_status.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

TelemetryState telemetry_state;
ChassisState chassis_state;

static uint8_t queue_accept = 1;
static uint8_t sent_frame[2];
static unsigned sent_count;

uint32_t HAL_GetTick(void) { return 0; }

uint8_t board_uart_send(const uint8_t *data, size_t length) {
    if (!queue_accept)
        return 0;
    assert(length == sizeof(sent_frame));
    memcpy(sent_frame, data, length);
    ++sent_count;
    return 1;
}

static void engine_frame(uint16_t rpm) {
    CAN_RxHeaderTypeDef header = {.DLC = 2};
    uint8_t data[2] = {(uint8_t)(rpm * 4 / 256), (uint8_t)((rpm * 4) & 0xfc)};
    vehicle_handle_engine_status(&header, data);
}

int main(void) {
    chassis_state.dyno_mode_enabled = 1;
    queue_accept = 0;
    engine_frame(0);
    assert(chassis_state.dyno_mode_enabled == 0);
    assert(sent_count == 0);

    /* A full UART queue must not lose the state change notification. */
    queue_accept = 1;
    engine_frame(0);
    assert(sent_count == 1);
    assert(sent_frame[0] == C1BusID && sent_frame[1] == C1cmdDynoNotActive);

    /* Once delivered, repeated engine frames must not spam C1. */
    engine_frame(0);
    assert(sent_count == 1);

    /* Dyno remains untouched while the engine is running. */
    chassis_state.dyno_mode_enabled = 1;
    engine_frame(1000);
    assert(chassis_state.dyno_mode_enabled == 1);
    assert(sent_count == 1);

    puts("engine status: PASS");
    return 0;
}
