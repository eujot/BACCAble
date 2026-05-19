/*
 * setup_menu.c
 *
 * To add a new setup feature:
 *   1. globalVariables.h - add: extern uint8_t your_var;
 *   2. globalVariables.c - add: uint8_t your_var = 0;
 *                                and a label row in dashboard_setup_menu_array
 *   3. setup_menu.c      - add one entry below
 *
 * For entries with side effects or multi-value state, use SETUP_TOGGLE_CUSTOM
 * and handle the page explicitly in processingMessage0x000002FA.c.
 */

#include "setup_menu.h"

#if defined(C1baccable)

#include "functions_C1baccable.h"  // readFromFlash, dashboard_setup_menu_array, checkbox_symbols

#define SETUP_FLASH_START_STOP             2
#define SETUP_FLASH_LED_CONTROLLER         3
#define SETUP_FLASH_SHIFT_INDICATOR        4
#define SETUP_FLASH_SHIFT_RPM              5
#define SETUP_FLASH_MY23_IPC               6
#define SETUP_FLASH_ROUTE_MESSAGES         7
#define SETUP_FLASH_DYNO                   8
#define SETUP_FLASH_ACC_VIRTUAL_PAD        9
#define SETUP_FLASH_BRAKES_OVERRIDE       10
#define SETUP_FLASH_4WD_DISABLER          11
#define SETUP_FLASH_REMOTE_START          12
#define SETUP_FLASH_CLEAR_FAULTS          13
#define SETUP_FLASH_ESC_TC_CUSTOMIZER     14
#define SETUP_FLASH_READ_FAULTS           15
#define SETUP_FLASH_DIESEL_PARAMS         16
#define SETUP_FLASH_REGEN_ALERT           17
#define SETUP_FLASH_LAUNCH_TORQUE         18
#define SETUP_FLASH_SEATBELT_ALARM        19
#define SETUP_FLASH_PEDAL_BOOSTER         20
#define SETUP_FLASH_ODOMETER_BLINK        21
#define SETUP_FLASH_SHOW_RACE_MASK        22
#define SETUP_FLASH_PARK_MIRROR           23
#define SETUP_FLASH_ACC_AUTOSTART         24
#define SETUP_FLASH_CLOSE_WINDOWS         25
#define SETUP_FLASH_OPEN_WINDOWS          26
#define SETUP_FLASH_HAS_VIRTUAL_PAD       27
#define SETUP_FLASH_QV_EXHAUST_FLAP       28
#define SETUP_FLASH_PEDAL_POWER           29
#define SETUP_FLASH_EUJOT                 30

#define SETUP_PAGE_START_STOP              1
#define SETUP_PAGE_LAUNCH_TORQUE           2
#define SETUP_PAGE_LED_CONTROLLER          3
#define SETUP_PAGE_SHIFT_INDICATOR         4
#define SETUP_PAGE_SHIFT_RPM               5
#define SETUP_PAGE_MY23_IPC                6
#define SETUP_PAGE_REGEN_ALERT             7
#define SETUP_PAGE_SEATBELT_ALARM          8
#define SETUP_PAGE_ROUTE_MESSAGES          9
#define SETUP_PAGE_ESC_TC_CUSTOMIZER      10
#define SETUP_PAGE_DYNO                   11
#define SETUP_PAGE_ACC_VIRTUAL_PAD        12
#define SETUP_PAGE_BRAKES_OVERRIDE        13
#define SETUP_PAGE_4WD_DISABLER           14
#define SETUP_PAGE_CLEAR_FAULTS           15
#define SETUP_PAGE_READ_FAULTS            16
#define SETUP_PAGE_REMOTE_START           17
#define SETUP_PAGE_DIESEL_PARAMS          18
#define SETUP_PAGE_ODOMETER_BLINK         19
#define SETUP_PAGE_PEDAL_BOOSTER          20
#define SETUP_PAGE_PEDAL_POWER            21
#define SETUP_PAGE_PARK_MIRROR            22
#define SETUP_PAGE_ACC_AUTOSTART          23
#define SETUP_PAGE_CLOSE_WINDOWS          24
#define SETUP_PAGE_OPEN_WINDOWS           25
#define SETUP_PAGE_HAS_VIRTUAL_PAD        26
#define SETUP_PAGE_QV_EXHAUST_FLAP        27
#define SETUP_PAGE_EUJOT                  28

#define SETUP_BOOL_AUTO(flash, page, variable) \
    { flash, page, 1, SETUP_VALUE_UINT8, SETUP_TOGGLE_AUTO_BOOL, SETUP_DISPLAY_CHECKBOX, &(variable) }

#define SETUP_BOOL_CUSTOM(flash, page, variable) \
    { flash, page, 1, SETUP_VALUE_UINT8, SETUP_TOGGLE_CUSTOM, SETUP_DISPLAY_CHECKBOX, &(variable) }

#define SETUP_UINT8_CUSTOM(flash, page, max, variable) \
    { flash, page, max, SETUP_VALUE_UINT8, SETUP_TOGGLE_CUSTOM, SETUP_DISPLAY_CHECKBOX, &(variable) }

#define SETUP_UINT16_CUSTOM(flash, page, max, variable) \
    { flash, page, max, SETUP_VALUE_UINT16, SETUP_TOGGLE_CUSTOM, SETUP_DISPLAY_NONE, &(variable) }

#define SETUP_INT8_CUSTOM(flash, page, max, variable) \
    { flash, page, max, SETUP_VALUE_INT8_AS_UINT8, SETUP_TOGGLE_CUSTOM, SETUP_DISPLAY_NONE, &(variable) }

#define SETUP_HIDDEN_BOOL(flash, variable) \
    { flash, SETUP_MENU_NO_PAGE, 1, SETUP_VALUE_UINT8, SETUP_TOGGLE_NONE, SETUP_DISPLAY_NONE, &(variable) }

// Parameter table  { flash_index | menu_page | max_value | type | toggle | display | &variable }
//
// flash_index : 1-based slot (matches readFromFlash argument)
// menu_page   : page index in dashboard_setup_menu_array, or SETUP_MENU_NO_PAGE
// max_value   : highest valid persisted value
// type        : runtime variable type
// toggle_mode : AUTO_BOOL = table may toggle it; CUSTOM = caller handles it
// display     : CHECKBOX = table may update dashboard_setup_menu_array[page][0]
const SetupParam setup_params[] = {
    // Core setup
    SETUP_BOOL_CUSTOM(SETUP_FLASH_START_STOP,      SETUP_PAGE_START_STOP,      function_smart_disable_start_stop_enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_LED_CONTROLLER,  SETUP_PAGE_LED_CONTROLLER,  function_led_strip_controller_enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_SHIFT_INDICATOR, SETUP_PAGE_SHIFT_INDICATOR, function_shift_indicator_enabled),
    SETUP_UINT16_CUSTOM(SETUP_FLASH_SHIFT_RPM,     SETUP_PAGE_SHIFT_RPM,       6000, shift_threshold),
    SETUP_BOOL_AUTO(  SETUP_FLASH_MY23_IPC,        SETUP_PAGE_MY23_IPC,        function_ipc_my23_is_installed),
    SETUP_BOOL_CUSTOM(SETUP_FLASH_DIESEL_PARAMS,   SETUP_PAGE_DIESEL_PARAMS,   function_is_diesel_enabled),

    // Diagnostics and messages
    SETUP_BOOL_AUTO(SETUP_FLASH_ROUTE_MESSAGES, SETUP_PAGE_ROUTE_MESSAGES, function_route_msg_enabled),
    SETUP_BOOL_AUTO(SETUP_FLASH_CLEAR_FAULTS,   SETUP_PAGE_CLEAR_FAULTS,   function_clear_faults_enabled),
    SETUP_BOOL_AUTO(SETUP_FLASH_READ_FAULTS,    SETUP_PAGE_READ_FAULTS,    function_read_faults_enabled),

    // Driving features
    SETUP_BOOL_AUTO(  SETUP_FLASH_DYNO,              SETUP_PAGE_DYNO,              function_dyno_mode_master_enabled),
    SETUP_BOOL_CUSTOM(SETUP_FLASH_ESC_TC_CUSTOMIZER, SETUP_PAGE_ESC_TC_CUSTOMIZER, function_esc_tc_customizator_enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_BRAKES_OVERRIDE,   SETUP_PAGE_BRAKES_OVERRIDE,   function_front_brake_forcer_master),
    SETUP_BOOL_AUTO(  SETUP_FLASH_4WD_DISABLER,      SETUP_PAGE_4WD_DISABLER,      function_4wd_disabler_enabled),
    SETUP_UINT16_CUSTOM(SETUP_FLASH_LAUNCH_TORQUE,   SETUP_PAGE_LAUNCH_TORQUE,     600, launch_torque_threshold),
    SETUP_UINT8_CUSTOM(SETUP_FLASH_PEDAL_BOOSTER,    SETUP_PAGE_PEDAL_BOOSTER,     6, function_pedal_booster_enabled),
    SETUP_INT8_CUSTOM( SETUP_FLASH_PEDAL_POWER,      SETUP_PAGE_PEDAL_POWER,       255, pedal_map_power),

    // Driver assistance and comfort
    SETUP_BOOL_AUTO(  SETUP_FLASH_ACC_VIRTUAL_PAD, SETUP_PAGE_ACC_VIRTUAL_PAD, function_acc_virtual_pad_enabled),
    SETUP_UINT8_CUSTOM(SETUP_FLASH_ACC_AUTOSTART,  SETUP_PAGE_ACC_AUTOSTART,   2, function_acc_autostart),
    SETUP_BOOL_AUTO(  SETUP_FLASH_REMOTE_START,    SETUP_PAGE_REMOTE_START,    function_remote_start_Enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_REGEN_ALERT,     SETUP_PAGE_REGEN_ALERT,     function_regeneration_alert_enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_SEATBELT_ALARM,  SETUP_PAGE_SEATBELT_ALARM,  function_seatbelt_alarm_enabled),
    SETUP_BOOL_CUSTOM(SETUP_FLASH_ODOMETER_BLINK,  SETUP_PAGE_ODOMETER_BLINK,  function_disable_odometer_blink),
    SETUP_BOOL_CUSTOM(SETUP_FLASH_PARK_MIRROR,     SETUP_PAGE_PARK_MIRROR,     function_park_mirror),
    SETUP_UINT8_CUSTOM(SETUP_FLASH_CLOSE_WINDOWS,  SETUP_PAGE_CLOSE_WINDOWS,   2, function_close_windows_with_door_lock),
    SETUP_UINT8_CUSTOM(SETUP_FLASH_OPEN_WINDOWS,   SETUP_PAGE_OPEN_WINDOWS,    2, function_open_windows_with_door_lock),
    SETUP_BOOL_CUSTOM(SETUP_FLASH_HAS_VIRTUAL_PAD, SETUP_PAGE_HAS_VIRTUAL_PAD, HAS_function_enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_QV_EXHAUST_FLAP, SETUP_PAGE_QV_EXHAUST_FLAP, QV_exhaust_flap_function_enabled),
    SETUP_BOOL_AUTO(  SETUP_FLASH_EUJOT,           SETUP_PAGE_EUJOT,           function_eujot_enabled),

    // Hidden persisted values
    SETUP_HIDDEN_BOOL(SETUP_FLASH_SHOW_RACE_MASK, function_show_race_mask),
    // ADD NEW SIMPLE CHECKBOX HERE:
    // SETUP_BOOL_AUTO(NEW_FLASH_IDX, NEW_MENU_PAGE, new_function_variable),
};
const uint8_t setup_params_count = sizeof(setup_params) / sizeof(setup_params[0]);

static uint16_t setup_get_value(const SetupParam *param) {
    switch(param->value_type) {
        case SETUP_VALUE_UINT16:
            return *(uint16_t *)param->value;
        case SETUP_VALUE_INT8_AS_UINT8:
            return (uint8_t)*(int8_t *)param->value;
        case SETUP_VALUE_UINT8:
        default:
            return *(uint8_t *)param->value;
    }
}

static void setup_set_value_from_flash(const SetupParam *param, uint16_t value) {
    switch(param->value_type) {
        case SETUP_VALUE_UINT16:
            *(uint16_t *)param->value = value;
            break;
        case SETUP_VALUE_INT8_AS_UINT8:
            *(int8_t *)param->value = (int8_t)(uint8_t)value;
            break;
        case SETUP_VALUE_UINT8:
        default:
            *(uint8_t *)param->value = (uint8_t)value;
            break;
    }
}

const SetupParam *setup_find_by_flash_index(uint8_t flash_index) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index == flash_index)
            return &setup_params[i];
    return 0;
}

uint8_t setup_flash_slots_count(void) {
    uint8_t count = SETUP_FLASH_SLOTS;
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index > count)
            count = setup_params[i].flash_index;
    return count;
}

void setup_load_from_flash(void) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        setup_set_value_from_flash(&setup_params[i], readFromFlash(setup_params[i].flash_index));
}

void setup_fill_flash_params(uint16_t *params) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        params[setup_params[i].flash_index - 1] = setup_get_value(&setup_params[i]);
}

uint8_t setup_is_dirty(void) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_get_value(&setup_params[i]) != readFromFlash(setup_params[i].flash_index))
            return 1;
    return 0;
}

uint8_t setup_update_checkbox(uint8_t page_index) {
    for (uint8_t i = 0; i < setup_params_count; i++) {
        if (setup_params[i].menu_page == page_index &&
            setup_params[i].display_mode == SETUP_DISPLAY_CHECKBOX) {
            dashboard_setup_menu_array[page_index][0] = checkbox_symbols[!!setup_get_value(&setup_params[i])];
            return 1;
        }
    }
    return 0;
}

uint8_t setup_toggle_if_auto_bool(uint8_t page_index) {
    for (uint8_t i = 0; i < setup_params_count; i++) {
        if (setup_params[i].menu_page == page_index &&
            setup_params[i].toggle_mode == SETUP_TOGGLE_AUTO_BOOL &&
            setup_params[i].max_value == 1) {
            setup_set_value_from_flash(&setup_params[i], !setup_get_value(&setup_params[i]));
            return 1;
        }
    }
    return 0;
}

#endif /* C1baccable */
