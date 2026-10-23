#ifndef BACCABLE_STATE_CHASSIS_H
#define BACCABLE_STATE_CHASSIS_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    CAN_TxHeaderTypeDef drive_train_control_module_reset_msg_header[4];
    uint8_t drive_train_control_module_reset_msg_data[4][8];
    uint32_t last_sent_drive_train_msg_time;
    uint8_t brake_intervention_acc_esc_asr;
#endif
#if defined(BACCABLE_C2)
    uint8_t dyno_mode_enabled;
    uint8_t dyno_state_machine;
    uint8_t dyno_msg_data[5][6];
    CAN_TxHeaderTypeDef dyno_msg_header;
    uint32_t last_sent_tester_presence_msg_time;
    uint32_t dyno_state_machine_last_update_time;
    uint8_t park_assist_button_press_count;
    uint32_t last_sent_rear_brake_msg_time;
    CAN_TxHeaderTypeDef rear_brake_msg_header[4];
    uint8_t rear_brake_msg_data[4][8];
#endif

    uint32_t lan_ebutton_press_begin_time;

    uint32_t lan_ebutton_first_click_time;

    uint8_t number_of_lane_button_clicks;

    uint8_t awd_sequence;

    uint8_t front_brake_forced;

    uint8_t dyno_mode_enabled_on_master;

    uint8_t launch_assist_enabled;

    uint8_t stability_inverted;

    /* 1 while the start-up mode may still be sent; 0 after a manual change. */
    uint8_t default_mode_override;

    /* Each bus frame gets at most one successful start-up injection. */
    uint8_t default_mode_sent;

    /* Raw drive-mode field seen when the override was armed; 0xFF = not sampled yet. */
    uint8_t default_mode_baseline;

    /* Bound queue retries to five seconds after this board is armed. */
    uint32_t default_mode_started_at;

    /* Start-up LKA double-click state; see LANE_ASSIST_* below. */
    uint8_t lane_assist_pending;
#if defined(BACCABLE_C2)
    uint8_t lane_assist_release_data[8];
    uint32_t lane_assist_step_at;
#endif
} ChassisState;

enum {
    LANE_ASSIST_IDLE = 0,
    LANE_ASSIST_WAIT_STATE = 1,
    LANE_ASSIST_WAIT_BUTTON = 2,
    LANE_ASSIST_RELEASE_FIRST_PENDING = 3,
    LANE_ASSIST_PRESS_SECOND_PENDING = 4,
    LANE_ASSIST_RELEASE_SECOND_PENDING = 5,
};
extern ChassisState chassis_state;
#endif
