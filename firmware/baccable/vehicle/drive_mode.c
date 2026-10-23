#include "vehicle/drive_mode.h"
#include "features/drive_mode_check.h"

/* Update the current driving mode and its related feature behavior. */
void vehicle_handle_drive_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->StdId != 0x384 || rx_header->DLC != 8 ||
        rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA)
        return;
// pressStartButton
#if defined(BACCABLE_C1)
    drive_mode_check_observe(0, (frame_data[1] & 0x7C) >> 2);
    if (telemetry_state.drive_mode !=
        (frame_data[1] & 0x7C)) { // RDNA mode was changed, reset the ESCandTCinversion
        chassis_state.stability_inverted = 0;
    }
    // current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 1 from bit 6 to bit 2
    // (0x0=Natural [shifted by 2 bits becomes 0x00], 0x2=dynamic [shifted by 2 bits becomes 0x08],
    // 0x4=AllWeather [shifted by 2 bits becomes 0x10], 0xC=race [shifted by 2 bits becomes 0x30]
    telemetry_state.drive_mode =
        frame_data[1] & 0x7C; // 7C is the mask from bit 6 to 2 (we avoid bit shift to save cpu loops)
    if (chassis_state.stability_inverted) {

        if (telemetry_state.drive_mode != 0x30) { // if not in race
            frame_data[1] =
                (frame_data[1] & ~0x7C) | 0x30; // set Race mode (0x30) to show on IPC the race screen (msg
                                                // from body to TCM, IPC, ECM, DTCM, DCTM, DASM, CDCM, BCM)
        }

        /* Captured 0x384 has no J1850 payload CRC; preserve its final byte. */
        can_forward(rx_header, frame_data);                         // transmit the modified packet
    }

#endif

#if defined(BACCABLE_C2)
    lane_assist_button_observe(rx_header, frame_data);
    // on C2 can bus, msg 0x384 contains, in byte3, bit6 contains left stalk button press status (LANE
    // indicator button)
    if ((frame_data[3] & 0x40) == 0x40) { // left stalk button was pressed (lane following indicator)
        if (chassis_state.lan_ebutton_press_begin_time ==
            0) { // if button was not pressed, and now it is pressed
            chassis_state.lan_ebutton_press_begin_time =
                currentTime; // save current time it was pressed (press begin)
            chassis_state.number_of_lane_button_clicks++;
            if (chassis_state.number_of_lane_button_clicks == 1)
                chassis_state.lan_ebutton_first_click_time = currentTime;
        }

        if ((currentTime - chassis_state.lan_ebutton_press_begin_time) > 2000) { // if pressed since 2 seconds

            if (settings_state.esc_tc_customizator_enabled) {
                chassis_state.stability_inverted = !chassis_state.stability_inverted; // toggle the status
                // if dyno is enabled or its change is in progress, avoid to switch ESP/TC.
                if (chassis_state.dyno_mode_enabled || chassis_state.dyno_state_machine != 0xff)
                    chassis_state.stability_inverted =
                        !chassis_state.stability_inverted; // revert the change. won't do both things
            }
            if (chassis_state.stability_inverted &&
                settings_state.show_race_mask) { // if enabled, notify C1 and BH
                uint8_t tmpArr1[2] = {C1_Bh_BusID, C1BHcmdShowRaceScreen};
                board_uart_send(tmpArr1, 2);
            } else {
                uint8_t tmpArr1[2] = {C1_Bh_BusID, C1BHcmdStopShowRaceScreen};
                board_uart_send(tmpArr1, 2);
            }

            status_led_activity();

            chassis_state.lan_ebutton_press_begin_time = 0; // reset the timer, like if it was not pressed
            chassis_state.number_of_lane_button_clicks = 0;
        }
    } else {
        chassis_state.lan_ebutton_press_begin_time =
            0; // use this value to remember that button is not pressed

        if (currentTime - chassis_state.lan_ebutton_first_click_time >
            1000) { // if more than 1 second is passed since first button click
            chassis_state.number_of_lane_button_clicks =
                0; // reset also the counter of the number of consecutive clics :-)
        }

        if (chassis_state.number_of_lane_button_clicks >= 2) { // if double click
            chassis_state.number_of_lane_button_clicks = 0;    // ensure we don't return here :-)
            // execute action :-)
            if (settings_state.has_function_enabled) {
                comfort_state.has_button_press_requested = 5;
                // notify to C1
                uint8_t tmpArr0[2] = {C1BusID, C1cmdLaneDoubleTap};
                board_uart_send(tmpArr0, 2);
            }
            status_led_activity();
        }
    }

    if (telemetry_state.drive_mode !=
        (frame_data[1] & 0x7C)) { // RDNA mode was changed, reset the ESCandTCinversion
        chassis_state.stability_inverted = 0;
    }
    // current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 1 from bit 6 to bit 2
    // (0x0=Natural [shifted by 2 bits becomes 0x00], 0x2=dynamic [shifted by 2 bits becomes 0x08],
    // 0x4=AllWeather [shifted by 2 bits becomes 0x10], 0xC=race [shifted by 2 bits becomes 0x30]
    telemetry_state.drive_mode =
        frame_data[1] & 0x7C; // 7C is the mask from bit 6 to 2 (we avoid bit shift to save cpu loops)
    if (chassis_state.stability_inverted) {

        uint8_t effective_mode = telemetry_state.drive_mode;
        if (effective_mode == 0x30) { // observed Race mode

            // ESC and TC
            frame_data[1] =
                (frame_data[1] & ~0x7C) | 0x08; // set Dynamic mode (0x08) to enable ESC and TC (msg from body
                                                // to HAL, ORC, EPS, BSM. this disables controls)
        } else {

            // ESC and TC
            frame_data[1] = (frame_data[1] & ~0x7C) | 0x30; // set Race mode (0x30) to disable ESC and TC
        }

        /* Preserve the observed 0x384 layout, including its final byte. */
        can_forward(rx_header, frame_data);                         // transmit the modified packet
    }
#endif

    // Command Ignition Status is on byte0 from bit 3 to 1.
    // Command Ignition Fail Status is on byte 0 bit0 and in byte1 bit7.
    // Drive Style Status (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural, 0x2=dynamic,
    // 0x4=AllWeather, 0xC=race) External temperature is on byte 1 from bit 1 to 0 and on byte 2 from bit 7 to
    // bit 1. External temperature fail is on byte2 bit0 Low Beam Status is on byte3 bit7 Lane Indicator
    // button status (left stalk button) is on byte 3 bit6. Power Mode Status is on byte 3 from bit 5 to 4.
    // Park Brake Status is on byte 3 bit 3.
    // Int. Relay Fail Status is on byte 4 from bit 7 to 6
    // SuspensionLevel is on byte 5 bit0 and byte 6 bit7.
}

/* Apply auxiliary-board preferences and the master ignition-session event. */
void driver_aids_configure(uint8_t mode, uint8_t disable_lka, uint8_t action) {
    settings_state.default_drive_mode = mode <= 3 ? mode : 0;
    settings_state.lane_assist_disabled = disable_lka ? 1 : 0;
    /* Transport resync preserves the session; a save ends its start-up window. */
    if (action == 1) {
#if defined(BACCABLE_C2)
        if (chassis_state.lane_assist_pending < LANE_ASSIST_RELEASE_FIRST_PENDING) {
            chassis_state.lane_assist_step_at = currentTime;
            chassis_state.lane_assist_pending = settings_state.lane_assist_disabled
                                                    ? LANE_ASSIST_WAIT_STATE
                                                    : LANE_ASSIST_IDLE;
        }
#endif
    } else if (action == 2) {
#if defined(BACCABLE_C2)
        if (chassis_state.lane_assist_pending < LANE_ASSIST_RELEASE_FIRST_PENDING)
            chassis_state.lane_assist_pending = LANE_ASSIST_IDLE;
#endif
    }
}

/* Wait for an enabled LKA state before offering the start-up double-click. */
void vehicle_handle_lane_state(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
#if defined(BACCABLE_C2)
    if (rx_header->StdId != 0x547)
        return;
    if (rx_header->DLC != 8 || rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA)
        return;

    lane_assist_button_process();
    if (!settings_state.lane_assist_disabled || chassis_state.lane_assist_pending == LANE_ASSIST_IDLE ||
        chassis_state.lane_assist_pending >= LANE_ASSIST_RELEASE_FIRST_PENDING)
        return;
    /* Initial OFF reports can precede system readiness. Only act on a current ON
       report, within the bounded start-up window; never toggle an OFF system. */
    chassis_state.lane_assist_pending = frame_data[6] & 0x02
                                          ? LANE_ASSIST_WAIT_BUTTON : LANE_ASSIST_WAIT_STATE;
#else
    (void)rx_header;
    (void)frame_data;
#endif
}

/* Copy a released factory button report and start the observed double-click. */
void lane_assist_button_observe(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
#if defined(BACCABLE_C2)
    const uint32_t id = 0x384;
    const uint8_t byte = 3, mask = 0x40;
    if (rx_header->StdId != id || rx_header->DLC != 8 || rx_header->IDE != CAN_ID_STD ||
        rx_header->RTR != CAN_RTR_DATA || !settings_state.lane_assist_disabled)
        return;
    lane_assist_button_process();
    if (frame_data[byte] & mask) {
        /* A driver press wins over the pending start-up action. */
        if (chassis_state.lane_assist_pending == LANE_ASSIST_WAIT_STATE ||
            chassis_state.lane_assist_pending == LANE_ASSIST_WAIT_BUTTON)
            chassis_state.lane_assist_pending = LANE_ASSIST_IDLE;
        return;
    }
    if (chassis_state.lane_assist_pending != LANE_ASSIST_WAIT_BUTTON)
        return;
    memcpy(chassis_state.lane_assist_release_data, frame_data, 8);
    frame_data[byte] |= mask;
    uint32_t result = can_forward(rx_header, frame_data);
    frame_data[byte] &= (uint8_t)~mask;
    if (result == HAL_OK) {
        chassis_state.lane_assist_step_at = currentTime;
        chassis_state.lane_assist_pending = LANE_ASSIST_RELEASE_FIRST_PENDING;
    }
#else
    (void)rx_header;
    (void)frame_data;
#endif
}

/* The sniff shows press, release, press, release at roughly 100 ms intervals. */
void lane_assist_button_process(void) {
#if defined(BACCABLE_C2)
    uint8_t stage = chassis_state.lane_assist_pending;
    if (stage == LANE_ASSIST_WAIT_STATE || stage == LANE_ASSIST_WAIT_BUTTON) {
        if (currentTime - chassis_state.lane_assist_step_at >= 5000U)
            chassis_state.lane_assist_pending = LANE_ASSIST_IDLE;
        return;
    }
    if (stage < LANE_ASSIST_RELEASE_FIRST_PENDING ||
        currentTime - chassis_state.lane_assist_step_at < 100U)
        return;
    CAN_TxHeaderTypeDef header = {.StdId = 0x384,
                                  .IDE = CAN_ID_STD,
                                  .RTR = CAN_RTR_DATA,
                                  .DLC = 8,
                                  .TransmitGlobalTime = DISABLE};
    uint8_t data[8];
    memcpy(data, chassis_state.lane_assist_release_data, sizeof(data));
    if (stage == LANE_ASSIST_PRESS_SECOND_PENDING)
        data[3] |= 0x40;
    if (can_tx(&header, data) != HAL_OK)
        return;
    chassis_state.lane_assist_step_at = currentTime;
    if (stage == LANE_ASSIST_RELEASE_FIRST_PENDING)
        chassis_state.lane_assist_pending = LANE_ASSIST_PRESS_SECOND_PENDING;
    else if (stage == LANE_ASSIST_PRESS_SECOND_PENDING)
        chassis_state.lane_assist_pending = LANE_ASSIST_RELEASE_SECOND_PENDING;
    else
        chassis_state.lane_assist_pending = LANE_ASSIST_IDLE;
#endif
}
