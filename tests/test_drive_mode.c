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
uint32_t HAL_GetTick(void) { return 100; }
void status_led_activity(void) {}
void body_display_factory_frame(const uint8_t *data, uint8_t dlc) { (void)data; (void)dlc; }
uint8_t board_uart_send(const uint8_t *data, size_t length) { (void)data; (void)length; return 1; }
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) { (void)header; (void)data; return 0; }
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
    /* Identical IDs on different buses do not imply identical signals. */
    settings_state.lane_assist_disabled = 1;
    chassis_state.lane_assist_pending = 1;
    for (unsigned id = 0; id < 2; ++id)
        for (unsigned dlc = 0; dlc <= 8; ++dlc) {
            CAN_RxHeaderTypeDef header = {.StdId = id ? 0x547 : 0x4f6, .DLC = dlc};
            uint8_t data[8] = {0,0,0,0,0,8,0x82,0};
            forwards = 0;
            vehicle_handle_lane_state(&header, data);
            unsigned expected = 0;
#if defined(BACCABLE_BH)
            expected = id == 0 && dlc == 8;
#elif defined(BACCABLE_C2)
            expected = id == 1 && dlc == 8;
#endif
            assert(forwards == expected);
            if (expected) {
                assert(forwarded[6] == 0x80 && forwarded[5] == 8 && forwarded[7] == 0);
                assert(data[6] == 0x82); /* Preserve the original received report. */
                vehicle_handle_lane_state(&header, data); /* Driver enabled it again. */
                assert(forwards == 1);
                chassis_state.lane_assist_pending = 1;
                data[6] = 0x82;
                settings_state.lane_assist_disabled = 0;
                vehicle_handle_lane_state(&header, data);
                settings_state.lane_assist_disabled = 1;
                header.RTR = CAN_RTR_REMOTE;
                vehicle_handle_lane_state(&header, data);
                header.RTR = CAN_RTR_DATA;
                header.IDE = CAN_ID_EXT;
                vehicle_handle_lane_state(&header, data);
                assert(forwards == 1 && data[6] == 0x82);
            }
        }
    {
        CAN_RxHeaderTypeDef header = {.StdId =
#if defined(BACCABLE_BH)
                                       0x4f6,
#else
                                       0x547,
#endif
                                       .DLC = 8};
        uint8_t data[8] = {0,0,0,0,0,8,0x82,0};
        chassis_state.lane_assist_pending = 1;
        rejected = 1;
        unsigned before = forwards;
        vehicle_handle_lane_state(&header, data);
#if defined(BACCABLE_BH) || defined(BACCABLE_C2)
        assert(forwards == before && chassis_state.lane_assist_pending);
        vehicle_handle_lane_state(&header, data);
        assert(forwards == before + 1 && !chassis_state.lane_assist_pending);
        chassis_state.lane_assist_pending = 1;
        data[6] = 0x80;
        vehicle_handle_lane_state(&header, data);
        assert(!chassis_state.lane_assist_pending); /* Already off at start. */
        data[6] = 0x82;
        vehicle_handle_lane_state(&header, data);
        assert(forwards == before + 1); /* Later stalk toggle is left alone. */
#else
        assert(forwards == before);
#endif
    }
    return 0;
}
