#include "app/powertrain.h"
#include "features/periodic.h"
#if defined(BACCABLE_C1)
static uint8_t next_option;
/* 0 = transport resync, 1 = vehicle start, 2 = saved for the next start. */
static uint8_t driver_aids_action = 1;

/* Restart auxiliary-board configuration after a save, wake-up or diagnostic session. */
void board_sync_restart(void) {
    next_option = 0;
    driver_aids_action = 0;
    runtime_state.instruct_slave_boards_trigger_enabled = 1;
}

/* A new vehicle session permits one set of start-up CAN frames. */
void board_sync_rearm(void) {
    board_sync_restart();
    driver_aids_action = 1;
    chassis_state.default_mode_override = 1;
    chassis_state.default_mode_sent = 0;
    chassis_state.default_mode_baseline = 0xFF;
}

/* Saving a preference applies it on the next vehicle start, not mid-session. */
void board_sync_saved(void) {
    board_sync_restart();
    driver_aids_action = 2;
    chassis_state.default_mode_override = 0;
}

/* Deliver every enabled-feature preference, retrying when the board link is busy. */
void board_sync_process(void) {
    if (!runtime_state.instruct_slave_boards_trigger_enabled ||
        currentTime - runtime_state.all_processors_wakeup_time <=
            TIMING__C1____DELAY_BEFORE_SERIAL_INSTRUCT_OF_C2BH_AFTER_OTHER_CHIP_WAKE_MS)
        return;
    uint8_t command[5] = {0};
    unsigned length = 2;
    switch (next_option) {
    case 0:
        command[0] = C2_Bh_BusID;
        command[1] = settings_state.esc_tc_customizator_enabled ? C2_Bh_cmdFunction_ESC_TC_Enabled
                                                                : C2_Bh_cmdFunction_ESC_TC_Disabled;
        break;
    case 1:
        command[0] = C2_Bh_BusID;
        command[1] = C2_Bh_cmdSetPedalBoostStatus;
        command[2] = settings_state.pedal_booster_enabled;
        length = 3;
        break;
    case 2:
        command[0] = BhBusID;
        command[1] =
            settings_state.disable_odometer_blink ? BHcmdOdometerBlinkDisable : BHcmdOdometerBlinkDefault;
        break;
    case 3:
        command[0] = C2BusID;
        command[1] = settings_state.show_race_mask ? C2cmdShowRaceMask : C2cmdRaceMaskDefault;
        break;
    case 4:
        command[0] = BhBusID;
        command[1] = settings_state.park_mirror ? BHcmdFunctParkMirrorEnabled : BHcmdFunctParkMirrorDisabled;
        break;
    case 5:
        command[0] = C2_Bh_BusID;
        command[1] = C2_BH_CMD_PARKING_OPTIONS;
        command[2] = settings_state.parking_sensor_mute;
        command[3] = settings_state.reverse_audio_mute;
        length = 4;
        break;
    case 6:
        command[0] = C2_Bh_BusID;
        command[1] =
            settings_state.has_function_enabled ? C2_Bh_cmdFunctHAS_Enabled : C2_Bh_cmdFunctHAS_Disabled;
        break;
    case 7:
        command[0] = C2_Bh_BusID;
        command[1] = C2_Bh_cmdSetDriverAids;
        command[2] = settings_state.default_drive_mode;
        command[3] = settings_state.lane_assist_disabled;
        command[4] = driver_aids_action;
        length = 5;
        break;
    default:
        next_option = 0;
        runtime_state.instruct_slave_boards_trigger_enabled = 0;
        return;
    }
    if (board_uart_send(command, length)) {
        if (next_option == 7) driver_aids_action = 0;
        ++next_option;
    }
}
#endif
