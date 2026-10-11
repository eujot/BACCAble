/* Host tests for the start-up default drive mode and the lane-assist override (C1 flavour). */
#include "vehicle/drive_mode.h"
#include <assert.h>
#include <string.h>

TelemetryState telemetry_state;
ChassisState chassis_state;
SettingsState settings_state;
ComfortState comfort_state;

uint32_t HAL_GetTick(void) { return 0; }
void status_led_activity(void) {}
uint8_t board_uart_send(const uint8_t *data, size_t length) {
    (void)data;
    (void)length;
    return 1;
}

static uint8_t forwarded[8];
static unsigned forwards;

uint32_t can_forward(const CAN_RxHeaderTypeDef *received, uint8_t *data) {
    assert(received->DLC <= 8);
    memcpy(forwarded, data, received->DLC);
    ++forwards;
    return 0;
}

static void feed_drive_mode(uint8_t byte1) {
    CAN_RxHeaderTypeDef header = {.DLC = 8};
    uint8_t data[8] = {0};
    data[1] = byte1;
    vehicle_handle_drive_mode(&header, data);
}

static void feed_lane_state(uint8_t byte6) {
    CAN_RxHeaderTypeDef header = {.DLC = 8};
    uint8_t data[8] = {0};
    data[5] = 0x08;
    data[6] = byte6;
    vehicle_handle_lane_state(&header, data);
}

int main(void) {
    /* Field encoding per frame layout matches the captured mode codes. */
    settings_state.default_drive_mode = 1; /* AllWeather */
    assert(drive_mode_default_field(DRIVE_MODE_ENC_CAN) == 0x10);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_BODY) == 0x04);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_IPC) == 0x10);
    settings_state.default_drive_mode = 2; /* Dynamic */
    assert(drive_mode_default_field(DRIVE_MODE_ENC_CAN) == 0x08);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_BODY) == 0x02);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_IPC) == 0x20);
    settings_state.default_drive_mode = 3; /* Race */
    assert(drive_mode_default_field(DRIVE_MODE_ENC_CAN) == 0x30);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_BODY) == 0x0C);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_IPC) == 0x04);
    settings_state.default_drive_mode = 0; /* off */
    assert(drive_mode_default_field(DRIVE_MODE_ENC_CAN) == 0);
    assert(drive_mode_default_field(DRIVE_MODE_ENC_BODY) == 0);

    /* The override stays idle while the feature is off. */
    chassis_state.default_mode_override = 1;
    chassis_state.default_mode_baseline = 0xFF;
    assert(drive_mode_default_should_apply(0x00) == 0);

    /* Dynamic default: Natural on the bus is rewritten to Dynamic and re-sent. */
    settings_state.default_drive_mode = 2;
    chassis_state.default_mode_override = 1;
    chassis_state.default_mode_baseline = 0xFF;
    telemetry_state.drive_mode = 0;
    forwards = 0;
    feed_drive_mode(0x01); /* Natural */
    assert(forwards == 1);
    assert((forwarded[1] & 0x7C) == 0x08); /* Dynamic */
    feed_drive_mode(0x01);                 /* repeated, keeps overriding */
    assert(forwards == 2);

    /* The driver turns the knob: stop overriding and leave the bus untouched. */
    forwards = 0;
    feed_drive_mode(0x11); /* AllWeather */
    assert(forwards == 0);
    assert(chassis_state.default_mode_override == 0);
    feed_drive_mode(0x01);
    assert(forwards == 0);

    /* Lane assist: a forced-off report clears the enable bit and re-sends. */
    settings_state.lane_assist_disabled = 1;
    forwards = 0;
    feed_lane_state(0x02); /* enabled */
    assert(forwards == 1 && (forwarded[6] & 0x02) == 0);
    forwards = 0;
    feed_lane_state(0x00); /* already disabled: nothing to send */
    assert(forwards == 0);
    settings_state.lane_assist_disabled = 0;
    forwards = 0;
    feed_lane_state(0x02);
    assert(forwards == 0);
    return 0;
}
