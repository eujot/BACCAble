#include "app/powertrain.h"
#include "features/periodic.h"
#include "features/drive_mode_check.h"
#if defined(BACCABLE_C1)
static uint8_t next_option;
/* 0 = transport resync, 1 = vehicle start, 2 = saved for the next start. */
static uint8_t driver_aids_action, dna_action;
static uint8_t engine_running;
static uint32_t engine_running_since;
/* RFHUB 0x1EF: 2 = OFF, 8 = RUN. Preserve the session through cranking (10). */
static uint8_t ignition_state;

/* Restart auxiliary-board configuration after a save, wake-up or diagnostic session. */
void board_sync_restart(void) {
    next_option = 0;
    runtime_state.instruct_slave_boards_trigger_enabled = 1;
}

/* After wake-up wait for actual RUN, rather than treating bus activity as ignition. */
void board_sync_rearm(void) {
    board_sync_restart();
    ignition_state = 0;
    driver_aids_action = dna_action = 0;
    engine_running = 0;
    drive_mode_check_start(0);
}

/* Ignition cycles do not require CAN silence or a processor sleep/wake cycle. */
void board_sync_ignition(unsigned code) {
    if (code != 2 && code != 8) return;
    if (code == ignition_state) return;
    ignition_state = code;
    board_sync_restart();
    driver_aids_action = dna_action = code == 8 ? 1 : 2;
    engine_running = 0;
    drive_mode_check_start(code == 8);
}

/* Saving a preference applies it on the next vehicle start, not mid-session. */
void board_sync_saved(void) {
    board_sync_restart();
    driver_aids_action = dna_action = 2;
    drive_mode_check_start(0);
}

/* Require a fresh RPM report and two seconds above 400 RPM before requesting DNA selection. */
static uint8_t dna_engine_ready(void) {
    if (telemetry_state.current_rpm_speed <= 400 ||
        currentTime - telemetry_state.current_rpm_updated_at > 1000U) {
        engine_running = 0;
        return 0;
    }
    if (!engine_running) {
        engine_running = 1;
        engine_running_since = currentTime;
    }
    return currentTime - engine_running_since >= 2000U;
}

/* Deliver every enabled-feature preference, retrying when the board link is busy. */
void board_sync_process(void) {
    uint8_t engine_ready = dna_engine_ready();
    drive_mode_check_process();
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
        command[1] = (settings_state.park_mirror && vehicle_feature_allowed(VEHICLE_FEATURE_PARK_MIRROR)) ? BHcmdFunctParkMirrorEnabled : BHcmdFunctParkMirrorDisabled;
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
            (settings_state.has_function_enabled && vehicle_feature_allowed(VEHICLE_FEATURE_VIRTUAL_HAS)) ? C2_Bh_cmdFunctHAS_Enabled : C2_Bh_cmdFunctHAS_Disabled;
        break;
    case 7:
        command[0] = C2_Bh_BusID;
        command[1] = C2_Bh_cmdSetDriverAids;
        command[2] = 0; /* Disable the obsolete report rewrite on older peers. */
        command[3] = settings_state.lane_assist_disabled && vehicle_feature_allowed(VEHICLE_FEATURE_LANE_ASSIST);
        command[4] = driver_aids_action;
        length = 5;
        break;
    case 8:
        if (dna_action == 1 && settings_state.default_drive_mode >= 1 &&
            settings_state.default_drive_mode <= 3) {
            if (!engine_ready) return;
        }
        command[0] = BhBusID;
        command[1] = BH_CMD_DNA_DEFAULT;
        command[2] = (settings_state.default_drive_mode == 3 && !vehicle_feature_allowed(VEHICLE_FEATURE_DEFAULT_RACE) ?
                      0 : settings_state.default_drive_mode) |
                     ((settings_state.default_suspension_softer && vehicle_feature_allowed(VEHICLE_FEATURE_SOFT_SUSPENSION)) ? BH_DNA_SOFT_SUSPENSION : 0);
        command[3] = dna_action;
        command[4] = drive_mode_check_token();
        length = 5;
        break;
    default:
        next_option = 0;
        runtime_state.instruct_slave_boards_trigger_enabled = 0;
        return;
    }
    if (board_uart_send(command, length)) {
        if (next_option == 7) driver_aids_action = 0;
        if (next_option == 8) {
            if (dna_action == 1) drive_mode_check_engine_started();
            dna_action = 0;
        }
        ++next_option;
    }
}
#endif
