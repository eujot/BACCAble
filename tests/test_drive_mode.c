/* Exercise production CAN handlers for each board role. */
#include "vehicle/drive_mode.h"
#include "vehicle/frame_handlers.h"
#include "features/drive_mode_check.h"
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
uint8_t mirror_positions_save(void) { return 0; }
uint16_t mirror_positions_read(uint8_t id) { (void)id; return 0; }
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
    chassis_state.lane_assist_pending = 0;
    telemetry_state.drive_mode = 0;
    forwards = 0;
    sends = 0;
    now = 100;
#if defined(BACCABLE_C1)
    drive_mode_check_start(1);
    drive_mode_check_engine_started();
#endif
}

#if defined(BACCABLE_C1)
/* Factory Dynamic 0x5A8, including the recorded counter and independent CRC. */
static void feed_transmission(uint8_t valid) {
    CAN_RxHeaderTypeDef header = {.StdId = 0x5A8, .DLC = 8};
    uint8_t data[] = {0x90, 0, 0x20, 0x56, 0x96, 0xc0, 3, 0xd2};
    if (!valid) data[7] ^= 1;
    vehicle_handle_transmission_mode(&header, data);
}

static void check_confirmation(void) {
    settings_state.default_drive_mode = 2;
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        arm();
        if (scenario == 6) {
            now = UINT32_MAX - 1000;
            drive_mode_check_start(1);
            drive_mode_check_engine_started();
        }
        uint32_t start = now;
        feed_mode(0);
        drive_mode_check_reply(drive_mode_check_token(), 1);
        assert(!strcmp(drive_mode_check_status(), "Waiting"));
        for (unsigned elapsed = 250; elapsed <= 5000; elapsed += 250) {
            now = start + elapsed;
            /* One transient D, or a stale matching stream, must not succeed. */
            if (scenario != 3 || elapsed <= 2000)
                feed_mode(scenario == 1 && elapsed > 250 ? 0 : 2);
            if (elapsed % 1000 == 0 && scenario != 2)
                feed_transmission(scenario != 4);
            if (scenario == 5 && elapsed == 3000) drive_mode_check_start(0);
            drive_mode_check_process();
            if (elapsed < 5000 && scenario != 5)
                assert(!strcmp(drive_mode_check_status(), "Waiting"));
        }
        const char *expected = scenario == 0 || scenario == 6 ? "Confirmed" :
                               scenario == 5 ? "Next start" : "No confirm";
        assert(!strcmp(drive_mode_check_status(), expected));
        assert(forwards == 0); /* Only BH may generate a selector gesture. */
    }

    arm();
    uint8_t old_token = drive_mode_check_token();
    drive_mode_check_start(1);
    drive_mode_check_engine_started();
    drive_mode_check_reply(old_token, 1);
    now += 5000;
    drive_mode_check_process();
    assert(!strcmp(drive_mode_check_status(), "Waiting")); /* Ignore stale BH results. */
    now += 10000;
    drive_mode_check_process();
    assert(!strcmp(drive_mode_check_status(), "No confirm")); /* Missing BH. */
    arm();
    drive_mode_check_reply(drive_mode_check_token(), 0);
    assert(!strcmp(drive_mode_check_status(), "No confirm"));
    arm();
    drive_mode_check_reply(drive_mode_check_token(), 1);
    now += 5000;
    drive_mode_check_process();
    assert(!strcmp(drive_mode_check_status(), "No confirm")); /* Silent CAN. */

}
#endif
int main(void) {
    /* Default settings never rewrite the selected-mode reports on any board. */
    for (unsigned mode = 0; mode <= 3; ++mode) {
        settings_state.default_drive_mode = mode;
        arm();
        feed_mode(0);
        feed_mode(2);
        feed_mode(4);
        assert(forwards == 0);
    }
    /* Existing ESC/TC customization still uses the actual factory selection. */
    arm();
    chassis_state.stability_inverted = 1;
    feed_mode(0);
    assert(forwards == 1);
#if defined(BACCABLE_BH)
    assert((forwarded[7] & 0x1f) == 12);
#else
    assert((forwarded[1] & 0x7c) == 0x30 && forwarded[7] == 0);
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
    assert(chassis_state.lane_assist_pending == 1); /* Initial OFF is not final readiness. */
    lane_assist_button_observe(&button_header, button);
    assert(forwards == 1); /* Never toggle an OFF system. */
    now += 5000;
    lane_assist_button_process();
    assert(chassis_state.lane_assist_pending == 0); /* Startup window expires. */
    chassis_state.lane_assist_step_at = now;
    state[6] = 2;
    chassis_state.lane_assist_pending = 1;
    vehicle_handle_lane_state(&state_header, state);
    button[button_byte] |= button_mask;
    lane_assist_button_observe(&button_header, button);
    assert(chassis_state.lane_assist_pending == 0); /* Driver pressed first. */
    button[button_byte] &= (uint8_t)~button_mask;
    lane_assist_button_observe(&button_header, button);
    assert(forwards == 1);

    /* The actual UART command entry point permits one gesture per ignition,
       survives initial OFF reports and never rearms on a transport-only resync. */
    arm();
    for (unsigned session = 0; session < 3; ++session) {
        driver_aids_configure(0, 1, 1);
        assert(chassis_state.lane_assist_pending == LANE_ASSIST_WAIT_STATE);
        state[6] = 0;
        vehicle_handle_lane_state(&state_header, state);
        lane_assist_button_observe(&button_header, button);
        assert(forwards == session);
        now += 1000; /* First OFF report was before readiness. */
        state[6] = 2;
        vehicle_handle_lane_state(&state_header, state);
        driver_aids_configure(0, 1, 0);
        lane_assist_button_observe(&button_header, button);
        assert(forwards == session + 1 && (forwarded[3] & 0x40));
        /* A save or OFF event during a pulse still completes the release sequence. */
        driver_aids_configure(0, 1, 2);
        for (unsigned step = 0; step < 3; ++step) {
            now += 100;
            lane_assist_button_process();
            assert(!!(sent[3] & 0x40) == (step == 1));
        }
        assert(sends == (session + 1) * 3 && chassis_state.lane_assist_pending == 0);
        driver_aids_configure(0, 1, 0);
        vehicle_handle_lane_state(&state_header, state);
        lane_assist_button_observe(&button_header, button);
        assert(forwards == session + 1); /* Driver may re-enable; no further injection. */
    }
    driver_aids_configure(0, 0, 1); /* LKA: ON means factory behavior. */
    vehicle_handle_lane_state(&state_header, state);
    lane_assist_button_observe(&button_header, button);
    assert(chassis_state.lane_assist_pending == 0 && forwards == 3);
    driver_aids_configure(0, 1, 1);
    now += 5000;
    vehicle_handle_lane_state(&state_header, state);
    lane_assist_button_observe(&button_header, button);
    assert(chassis_state.lane_assist_pending == 0 && forwards == 3);
    now = UINT32_MAX - 1000;
    driver_aids_configure(0, 1, 1);
    now += 5000;
    lane_assist_button_process();
    assert(chassis_state.lane_assist_pending == 0); /* Deadline survives tick wrap. */
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
#if defined(BACCABLE_C1)
    check_confirmation();
#endif
    return 0;
}
