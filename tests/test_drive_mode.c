/* Exercise production CAN handlers for each board role. */
#include "vehicle/drive_mode.h"
#include "vehicle/frame_handlers.h"
#include <assert.h>
#include <string.h>

TelemetryState telemetry_state;
ChassisState chassis_state;
SettingsState settings_state;
ComfortState comfort_state;
MirrorsState mirrors_state;
DisplayState display_state;
DashboardState dashboard_state;
static uint32_t now = 100;
uint32_t HAL_GetTick(void) { return now; }
void status_led_activity(void) {}
void body_display_factory_frame(const uint8_t *data, uint8_t dlc) { (void)data; (void)dlc; }
uint8_t board_uart_send(const uint8_t *data, size_t length) { (void)data; (void)length; return 1; }
static uint8_t sent[8];
static uint32_t sent_id;
static unsigned sends, send_rejected;
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    if (send_rejected) {
        --send_rejected;
        return HAL_ERROR;
    }
    assert(header->DLC == 8 && header->IDE == CAN_ID_STD && header->RTR == CAN_RTR_DATA);
    sent_id = header->StdId;
    memcpy(sent, data, 8);
    ++sends;
    return HAL_OK;
}
static uint8_t forwarded[8];
static unsigned forwards;
static unsigned rejected;
uint32_t can_forward(const CAN_RxHeaderTypeDef *received, uint8_t *data) {
    assert(received->DLC <= 8);
    if (rejected) {
        --rejected;
        return HAL_ERROR;
    }
    memcpy(forwarded, data, received->DLC);
    ++forwards;
    return HAL_OK;
}
static void feed_mode(uint8_t raw) {
    CAN_RxHeaderTypeDef header = {.StdId = 0x384, .DLC = 8};
    uint8_t data[8] = {0};
#if defined(BACCABLE_BH)
    header.StdId = 0x46C;
    data[7] = raw;
    vehicle_handle_body_drive_mode(&header, data);
#else
    data[1] = raw * 4 | 1;
    vehicle_handle_drive_mode(&header, data);
#endif
}
static void arm(void) {
    memset(&chassis_state, 0, sizeof(chassis_state));
    chassis_state.default_mode_override = 1;
    chassis_state.default_mode_baseline = 0xff;
    chassis_state.default_mode_sent = 0;
    chassis_state.lane_assist_pending = 0;
    telemetry_state.drive_mode = 0;
    forwards = 0;
    sends = 0;
    now = 100;
}
int main(void) {
    static const uint8_t codes[3][3] = {{0x10,0x04,0x10}, {0x08,0x02,0x20}, {0x30,0x0c,0x04}};
    for (unsigned mode = 1; mode <= 3; ++mode) {
        settings_state.default_drive_mode = mode;
        for (unsigned enc = 0; enc < 3; ++enc)
            assert(drive_mode_default_field(enc) == codes[mode-1][enc]);
    }
#if defined(BACCABLE_C1) || defined(BACCABLE_C2) || defined(BACCABLE_BH)
    settings_state.default_drive_mode = 2;
    arm();
    feed_mode(0);
    assert(forwards == 1);
#if defined(BACCABLE_BH)
    assert((forwarded[7] & 0x1f) == 2);
#else
    assert(forwarded[1] == 0x09);
    /* Independent J1850 fixture: 00 09 00 00 00 00 00 -> 0x46. */
    assert(forwarded[7] == 0x46);
#endif
    feed_mode(0);
    assert(forwards == 1); /* No repeated default while body still reports N. */
    feed_mode(4);
    assert(forwards == 1 && !chassis_state.default_mode_override);
    feed_mode(0);
    assert(forwards == 1);

    /* A full CAN queue postpones the only attempt until a later report. */
    arm();
    rejected = 1;
    feed_mode(0);
    assert(forwards == 0 && chassis_state.default_mode_sent == 0);
    feed_mode(0);
    feed_mode(0);
    assert(forwards == 1 && chassis_state.default_mode_sent);

    /* A baseline must be sampled even while ESC/TC owns the outgoing mode. */
    arm();
    chassis_state.stability_inverted = 1;
    feed_mode(0);
    assert(chassis_state.default_mode_baseline == 0);
    feed_mode(4);
    assert(!chassis_state.default_mode_override);
    unsigned before = forwards;
    feed_mode(0);
    assert(forwards == before);

#if defined(BACCABLE_C2)
    arm();
    settings_state.default_drive_mode = 3;
    feed_mode(0);
    chassis_state.stability_inverted = 1;
    feed_mode(0);
    assert((forwarded[1] & 0x7c) == 0x08); /* Invert effective Race to Dynamic. */
    CAN_RxHeaderTypeDef stability_header = {.DLC = 8};
    uint8_t stability[8] = {0};
    unsigned previous = forwards;
    vehicle_handle_stability_status(&stability_header, stability);
    assert(forwards == previous); /* Do not synthesize disabled-control flags. */
#endif
    arm();
    settings_state.default_drive_mode = 255;
    feed_mode(0);
    assert(forwards == 0);
#endif
#if defined(BACCABLE_BH)
    arm();
    settings_state.default_drive_mode = 2;
    CAN_RxHeaderTypeDef ipc_header = {.StdId = 0x25a, .DLC = 8};
    uint8_t ipc[8] = {0xc3,1,2,3,4,5,6,7};
    vehicle_handle_drive_style_display(&ipc_header, ipc);
    assert(forwards == 1 && forwarded[0] == 0xa3 && forwarded[7] == 7);
    vehicle_handle_drive_style_display(&ipc_header, ipc);
    assert(forwards == 1); /* One 0x25A injection in this session. */
    feed_mode(0);
    feed_mode(0);
    assert(forwards == 2); /* 0x46C is a separate one-time frame. */
    settings_state.default_drive_mode = 255;
    vehicle_handle_drive_style_display(&ipc_header, ipc);
    assert(forwards == 2);
#endif
#if defined(BACCABLE_C2)
    /* Recorded stalk gesture: press, release, press, release at 100 ms intervals. */
    arm();
    settings_state.lane_assist_disabled = 1;
    chassis_state.lane_assist_pending = 1;
    CAN_RxHeaderTypeDef state_header = {.StdId = 0x547,
                                        .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 8};
    CAN_RxHeaderTypeDef button_header = {.StdId = 0x384,
                                         .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 8};
    uint8_t state[8] = {0,0,0,0,0,8,2,0};
    uint8_t button[8] = {0};
    button[0] = 8; button[1] = 9; button[2] = 0x8a; button[3] = 0x80;
    const uint8_t button_byte = 3, button_mask = 0x40;
    uint8_t pressed_button[8];
    memcpy(pressed_button, button, sizeof(button));
    pressed_button[button_byte] |= button_mask;
    lane_assist_button_observe(&button_header, button);
    assert(forwards == 0); /* Wait for a valid, enabled status. */
    state_header.StdId = 0x4f6;
    vehicle_handle_lane_state(&state_header, state);
    assert(chassis_state.lane_assist_pending == 1); /* BH layout is not the C2 input. */
    state_header.StdId = 0x547;
    state_header.DLC = 7;
    vehicle_handle_lane_state(&state_header, state);
    assert(chassis_state.lane_assist_pending == 1);
    state_header.DLC = 8;
    vehicle_handle_lane_state(&state_header, state);
    assert(chassis_state.lane_assist_pending == 2 && forwards == 0);
    button_header.StdId = 0x5a0;
    lane_assist_button_observe(&button_header, button);
    button_header.StdId = 0x384;
    button_header.DLC = 7;
    lane_assist_button_observe(&button_header, button);
    button_header.DLC = 8;
    button_header.IDE = CAN_ID_EXT;
    lane_assist_button_observe(&button_header, button);
    button_header.IDE = CAN_ID_STD;
    button_header.RTR = CAN_RTR_REMOTE;
    lane_assist_button_observe(&button_header, button);
    button_header.RTR = CAN_RTR_DATA;
    assert(chassis_state.lane_assist_pending == 2 && forwards == 0);
    rejected = 1;
    lane_assist_button_observe(&button_header, button);
    assert(chassis_state.lane_assist_pending == 2 && forwards == 0);
    vehicle_handle_drive_mode(&button_header, button);
    assert(chassis_state.lane_assist_pending == 3 && forwards == 1);
    assert(memcmp(forwarded, pressed_button, 8) == 0);
    assert(!(button[button_byte] & button_mask));
    vehicle_handle_lane_state(&state_header, state); /* The old status cannot trigger another press. */
    lane_assist_button_observe(&button_header, button);
    assert(forwards == 1);
    now = 199;
    lane_assist_button_process();
    assert(sends == 0);
    now = 200;
    send_rejected = 1;
    lane_assist_button_process();
    assert(sends == 0 && chassis_state.lane_assist_pending == 3);
    lane_assist_button_process();
    assert(sends == 1 && sent_id == button_header.StdId);
    assert(memcmp(sent, button, 8) == 0 && chassis_state.lane_assist_pending == 4);
    state[6] = 0;
    vehicle_handle_lane_state(&state_header, state); /* The state may change mid-gesture. */
    assert(chassis_state.lane_assist_pending == 4);
    now = 299;
    lane_assist_button_process();
    assert(sends == 1);
    now = 300;
    send_rejected = 1;
    lane_assist_button_process();
    assert(sends == 1 && chassis_state.lane_assist_pending == 4);
    lane_assist_button_process();
    assert(sends == 2 && memcmp(sent, pressed_button, 8) == 0);
    assert(chassis_state.lane_assist_pending == 5);
    now = 399;
    lane_assist_button_process();
    assert(sends == 2);
    now = 400;
    send_rejected = 1;
    lane_assist_button_process();
    assert(sends == 2 && chassis_state.lane_assist_pending == 5);
    lane_assist_button_process();
    assert(sends == 3 && memcmp(sent, button, 8) == 0);
    assert(chassis_state.lane_assist_pending == 0);
    lane_assist_button_process();
    lane_assist_button_observe(&button_header, button);
    assert(sends == 3 && forwards == 1); /* No continuous override. */

    chassis_state.lane_assist_pending = 1;
    state[6] = 0;
    vehicle_handle_lane_state(&state_header, state);
    assert(chassis_state.lane_assist_pending == 0); /* Already disabled. */
    state[6] = 2;
    chassis_state.lane_assist_pending = 1;
    vehicle_handle_lane_state(&state_header, state);
    button[button_byte] |= button_mask;
    lane_assist_button_observe(&button_header, button);
    assert(chassis_state.lane_assist_pending == 0); /* Driver pressed first. */
    button[button_byte] &= (uint8_t)~button_mask;
    lane_assist_button_observe(&button_header, button);
    assert(forwards == 1);
#elif defined(BACCABLE_BH)
    /* BH sees the button and state too, but must not generate a second press. */
    settings_state.lane_assist_disabled = 1;
    chassis_state.lane_assist_pending = 1;
    CAN_RxHeaderTypeDef state_header = {.StdId = 0x4f6, .IDE = CAN_ID_STD,
                                        .RTR = CAN_RTR_DATA, .DLC = 8};
    uint8_t state[8] = {0,0,0,0,0,8,2,0};
    unsigned before_lane = forwards;
    vehicle_handle_lane_state(&state_header, state);
    assert(forwards == before_lane && sends == 0 && chassis_state.lane_assist_pending == 1);
#endif
    return 0;
}
