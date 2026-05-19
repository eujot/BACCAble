/*
 * setup_menu.c
 *
 * Single source of truth for setup menu entries.
 *
 * To add a simple checkbox:
 *   1. Add the runtime variable in globalVariables.c/.h.
 *   2. Add a row in dashboard_setup_menu_array.
 *   3. Add one SETUP_BOOL(...) entry below.
 *
 * Entries with side effects provide an action callback. Entries with dynamic
 * text provide a render callback. The caller does not need a page switch.
 */

#include "setup_menu.h"

#if defined(C1baccable)

#include <string.h>
#include "functions_C1baccable.h"

#define SETUP_FLASH_IMMOBILIZER            1
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

#define SETUP_PAGE_SAVE_EXIT               0
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
#define SETUP_PAGE_COUNT                  29

uint8_t setup_dashboardPageIndex = 0;
uint8_t dashboard_setup_menu_array[SETUP_PAGE_COUNT][DASHBOARD_MESSAGE_MAX_LENGTH] = {
    [SETUP_PAGE_SAVE_EXIT] = {'S','A','V','E','&','E','X','I','T',' ',' ',' ',' ',' ',' ',' ',' ',' '},
    [SETUP_PAGE_START_STOP] = {'O',' ',' ','S','t','a','r','t','&','S','t','o','p',' ',' ',' ',' ',' '},
    [SETUP_PAGE_LAUNCH_TORQUE] = {'L','a','u','n','c','h','T','o','r','q','u','e',' ','1','0','0','N','m'},
    [SETUP_PAGE_LED_CONTROLLER] = {'O',' ',' ','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',' '},
    [SETUP_PAGE_SHIFT_INDICATOR] = {'O',' ',' ','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
    [SETUP_PAGE_SHIFT_RPM] = {'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',' ',' ',' ',' '},
    [SETUP_PAGE_MY23_IPC] = {'O',' ',' ','M','y','2','3',' ','I','P','C',' ',' ',' ',' ',' ',' ',' '},
    [SETUP_PAGE_REGEN_ALERT] = {'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
    [SETUP_PAGE_SEATBELT_ALARM] = {0xD8,' ',' ','S','e','a','t','b','e','l','t',' ','A','l','a','r','m',' '},
    [SETUP_PAGE_ROUTE_MESSAGES] = {'O',' ',' ','R','o','u','t','e',' ','M','e','s','s','a','g','e','s',' '},
    [SETUP_PAGE_ESC_TC_CUSTOMIZER] = {'O',' ',' ','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',' '},
    [SETUP_PAGE_DYNO] = {'O',' ',' ','D','y','n','o',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
    [SETUP_PAGE_ACC_VIRTUAL_PAD] = {'O',' ',' ','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
    [SETUP_PAGE_BRAKES_OVERRIDE] = {'O',' ',' ','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
    [SETUP_PAGE_4WD_DISABLER] = {'O',' ',' ','4','W','D',' ','D','i','s','a','b','l','e','r',' ',' ',' '},
    [SETUP_PAGE_CLEAR_FAULTS] = {'O',' ',' ','C','l','e','a','r',' ','F','a','u','l','t','s',' ',' ',' '},
    [SETUP_PAGE_READ_FAULTS] = {'O',' ',' ','R','e','a','d',' ',' ','F','a','u','l','t','s',' ',' ',' '},
    [SETUP_PAGE_REMOTE_START] = {'O',' ',' ','R','e','m','o','t','e',' ','S','t','a','r','t',' ',' ',' '},
    [SETUP_PAGE_DIESEL_PARAMS] = {0xD8,' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
    [SETUP_PAGE_ODOMETER_BLINK] = {'O',' ',' ','O','d','o','m','e','t','e','r',' ','B','l','i','n','k',' '},
    [SETUP_PAGE_PEDAL_BOOSTER] = {'O',' ',' ','P','e','d','a','l',' ','B','o','o','s','t','e','r',' ',' '},
    [SETUP_PAGE_PEDAL_POWER] = {'P','e','d','a','l',' ','P','o','w','e','r',':',' ','0',' ',' ',' ',' '},
    [SETUP_PAGE_PARK_MIRROR] = {'O',' ',' ','P','a','r','k',' ','M','i','r','r','o','r',' ',' ',' ',' '},
    [SETUP_PAGE_ACC_AUTOSTART] = {'O',' ',' ','A','C','C',' ','A','u','t','o','s','t','a','r','t',' ',' '},
    [SETUP_PAGE_CLOSE_WINDOWS] = {'O',' ',' ','C','l','o','s','e',' ','W','i','n','d','o','w','s',' ',' '},
    [SETUP_PAGE_OPEN_WINDOWS] = {'O',' ',' ','O','p','e','n',' ',' ','W','i','n','d','o','w','s',' ',' '},
    [SETUP_PAGE_HAS_VIRTUAL_PAD] = {'O',' ',' ','H','A','S',' ','V','i','r','t','u','a','l',' ','P','a','d'},
    [SETUP_PAGE_QV_EXHAUST_FLAP] = {'O',' ',' ','Q','V',' ','E','x','h','a','u','s','t',' ','F','l','a','p'},
    [SETUP_PAGE_EUJOT] = {'O',' ',' ','e','u','j','o','t',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
};
uint8_t total_pages_in_setup_dashboard_menu = SETUP_PAGE_COUNT;

#if defined(IMMOBILIZER_ENABLED)
#define DEFAULT_IMMOBILIZER 1
#else
#define DEFAULT_IMMOBILIZER 0
#endif

#if defined(SMART_DISABLE_START_STOP)
#define DEFAULT_START_STOP 1
#else
#define DEFAULT_START_STOP 0
#endif

#if defined(LED_STRIP_CONTROLLER_ENABLED)
#define DEFAULT_LED_CONTROLLER 1
#else
#define DEFAULT_LED_CONTROLLER 0
#endif

#if defined(SHIFT_INDICATOR_ENABLED)
#define DEFAULT_SHIFT_INDICATOR 1
#else
#define DEFAULT_SHIFT_INDICATOR 0
#endif

#if defined(SHIFT_THRESHOLD)
#define DEFAULT_SHIFT_RPM SHIFT_THRESHOLD
#else
#define DEFAULT_SHIFT_RPM 3500
#endif

#if defined(IPC_MY23_IS_INSTALLED)
#define DEFAULT_MY23_IPC 1
#else
#define DEFAULT_MY23_IPC 0
#endif

#if defined(ROUTE_MSG)
#define DEFAULT_ROUTE_MESSAGES 1
#else
#define DEFAULT_ROUTE_MESSAGES 0
#endif

#if defined(DYNO_MODE_MASTER)
#define DEFAULT_DYNO 1
#else
#define DEFAULT_DYNO 0
#endif

#if defined(ACC_VIRTUAL_PAD)
#define DEFAULT_ACC_VIRTUAL_PAD 1
#else
#define DEFAULT_ACC_VIRTUAL_PAD 0
#endif

#if defined(FRONT_BRAKE_FORCER_MASTER)
#define DEFAULT_BRAKES_OVERRIDE 1
#else
#define DEFAULT_BRAKES_OVERRIDE 0
#endif

#if defined(_4WD_DISABLER)
#define DEFAULT_4WD_DISABLER 1
#else
#define DEFAULT_4WD_DISABLER 0
#endif

#if defined(REMOTE_START_ENABLED)
#define DEFAULT_REMOTE_START 1
#else
#define DEFAULT_REMOTE_START 0
#endif

#if defined(CLEAR_FAULTS_ENABLED)
#define DEFAULT_CLEAR_FAULTS 1
#else
#define DEFAULT_CLEAR_FAULTS 0
#endif

#if defined(ESC_TC_CUSTOMIZATOR_MASTER)
#define DEFAULT_ESC_TC_CUSTOMIZER 1
#else
#define DEFAULT_ESC_TC_CUSTOMIZER 0
#endif

#if defined(READ_FAULTS_ENABLED)
#define DEFAULT_READ_FAULTS 1
#else
#define DEFAULT_READ_FAULTS 0
#endif

#if defined(IS_DIESEL)
#define DEFAULT_DIESEL_PARAMS 1
#else
#define DEFAULT_DIESEL_PARAMS 0
#endif

#if defined(REGENERATION_ALERT_ENABLED)
#define DEFAULT_REGEN_ALERT 1
#else
#define DEFAULT_REGEN_ALERT 0
#endif

#if defined(LAUNCH_THRESHOLD)
#define DEFAULT_LAUNCH_TORQUE LAUNCH_THRESHOLD
#else
#define DEFAULT_LAUNCH_TORQUE 100
#endif

#if defined(SEATBELT_ALARM_DISABLED)
#define DEFAULT_SEATBELT_ALARM 0
#else
#define DEFAULT_SEATBELT_ALARM 1
#endif

#if defined(PEDAL_BOOSTER_ENABLED)
#define DEFAULT_PEDAL_BOOSTER PEDAL_BOOSTER_ENABLED
#else
#define DEFAULT_PEDAL_BOOSTER 0
#endif

#if defined(DISABLE_ODOMETER_BLINK)
#define DEFAULT_ODOMETER_BLINK 1
#else
#define DEFAULT_ODOMETER_BLINK 0
#endif

#if defined(SHOW_RACE_MASK)
#define DEFAULT_SHOW_RACE_MASK 1
#else
#define DEFAULT_SHOW_RACE_MASK 0
#endif

#if defined(PARK_MIRROR)
#define DEFAULT_PARK_MIRROR 1
#else
#define DEFAULT_PARK_MIRROR 0
#endif

#if defined(ACC_AUTOSTART)
#define DEFAULT_ACC_AUTOSTART ACC_AUTOSTART
#else
#define DEFAULT_ACC_AUTOSTART 0
#endif

#if defined(CLOSE_WINDOWS)
#define DEFAULT_CLOSE_WINDOWS CLOSE_WINDOWS
#else
#define DEFAULT_CLOSE_WINDOWS 0
#endif

#if defined(OPEN_WINDOWS)
#define DEFAULT_OPEN_WINDOWS OPEN_WINDOWS
#else
#define DEFAULT_OPEN_WINDOWS 0
#endif

#if defined(HAS_VIRTUAL_PAD)
#define DEFAULT_HAS_VIRTUAL_PAD 1
#else
#define DEFAULT_HAS_VIRTUAL_PAD 0
#endif

#if defined(QV_EXHAUST_FLAP_FUNCTION_ENABLED)
#define DEFAULT_QV_EXHAUST_FLAP 1
#else
#define DEFAULT_QV_EXHAUST_FLAP 0
#endif

#if defined(PEDAL_MAP_POWER)
#define DEFAULT_PEDAL_POWER ((uint16_t)(uint8_t)PEDAL_MAP_POWER)
#else
#define DEFAULT_PEDAL_POWER 0
#endif

#define DEFAULT_EUJOT 0

#define SETUP_ENTRY(flash, page, max, def, type, display, variable, render_fn, action_fn) \
    { flash, page, max, def, type, display, &(variable), render_fn, action_fn }

#define SETUP_BOOL(flash, page, def, variable) \
    SETUP_ENTRY(flash, page, 1, def, SETUP_VALUE_UINT8, SETUP_DISPLAY_CHECKBOX, variable, 0, 0)

#define SETUP_BOOL_ACTION(flash, page, def, variable, action_fn) \
    SETUP_ENTRY(flash, page, 1, def, SETUP_VALUE_UINT8, SETUP_DISPLAY_CHECKBOX, variable, 0, action_fn)

#define SETUP_BOOL_RENDER_ACTION(flash, page, def, variable, render_fn, action_fn) \
    SETUP_ENTRY(flash, page, 1, def, SETUP_VALUE_UINT8, SETUP_DISPLAY_CHECKBOX, variable, render_fn, action_fn)

#define SETUP_UINT8_ACTION(flash, page, max, def, variable, render_fn, action_fn) \
    SETUP_ENTRY(flash, page, max, def, SETUP_VALUE_UINT8, SETUP_DISPLAY_CHECKBOX, variable, render_fn, action_fn)

#define SETUP_UINT16_ACTION(flash, page, max, def, variable, render_fn, action_fn) \
    SETUP_ENTRY(flash, page, max, def, SETUP_VALUE_UINT16, SETUP_DISPLAY_NONE, variable, render_fn, action_fn)

#define SETUP_INT8_ACTION(flash, page, def, variable, render_fn, action_fn) \
    SETUP_ENTRY(flash, page, 255, def, SETUP_VALUE_INT8_AS_UINT8, SETUP_DISPLAY_NONE, variable, render_fn, action_fn)

#define SETUP_HIDDEN_BOOL(flash, def, variable) \
    SETUP_ENTRY(flash, SETUP_MENU_NO_PAGE, 1, def, SETUP_VALUE_UINT8, SETUP_DISPLAY_NONE, variable, 0, 0)

static void setup_action_start_stop(void);
static void setup_action_launch_torque(void);
static void setup_action_shift_rpm(void);
static void setup_action_esc_tc(void);
static void setup_action_diesel_params(void);
static void setup_action_odometer_blink(void);
static void setup_action_pedal_booster(void);
static void setup_action_pedal_power(void);
static void setup_action_park_mirror(void);
static void setup_action_acc_autostart(void);
static void setup_action_close_windows(void);
static void setup_action_open_windows(void);
static void setup_action_has_virtual_pad(void);

static void setup_render_launch_torque(uint8_t page);
static void setup_render_shift_rpm(uint8_t page);
static void setup_render_diesel_params(uint8_t page);
static void setup_render_pedal_booster(uint8_t page);
static void setup_render_pedal_power(uint8_t page);
static void setup_render_acc_autostart(uint8_t page);
static void setup_render_close_windows(uint8_t page);
static void setup_render_open_windows(uint8_t page);

const SetupParam setup_params[] = {
    // Core setup
    SETUP_HIDDEN_BOOL(SETUP_FLASH_IMMOBILIZER, DEFAULT_IMMOBILIZER, immobilizerEnabled),
    SETUP_BOOL_ACTION(SETUP_FLASH_START_STOP, SETUP_PAGE_START_STOP, DEFAULT_START_STOP, function_smart_disable_start_stop_enabled, setup_action_start_stop),
    SETUP_BOOL(SETUP_FLASH_LED_CONTROLLER, SETUP_PAGE_LED_CONTROLLER, DEFAULT_LED_CONTROLLER, function_led_strip_controller_enabled),
    SETUP_BOOL(SETUP_FLASH_SHIFT_INDICATOR, SETUP_PAGE_SHIFT_INDICATOR, DEFAULT_SHIFT_INDICATOR, function_shift_indicator_enabled),
    SETUP_UINT16_ACTION(SETUP_FLASH_SHIFT_RPM, SETUP_PAGE_SHIFT_RPM, 6000, DEFAULT_SHIFT_RPM, shift_threshold, setup_render_shift_rpm, setup_action_shift_rpm),
    SETUP_BOOL(SETUP_FLASH_MY23_IPC, SETUP_PAGE_MY23_IPC, DEFAULT_MY23_IPC, function_ipc_my23_is_installed),
    SETUP_BOOL_RENDER_ACTION(SETUP_FLASH_DIESEL_PARAMS, SETUP_PAGE_DIESEL_PARAMS, DEFAULT_DIESEL_PARAMS, function_is_diesel_enabled, setup_render_diesel_params, setup_action_diesel_params),

    // Diagnostics and messages
    SETUP_BOOL(SETUP_FLASH_ROUTE_MESSAGES, SETUP_PAGE_ROUTE_MESSAGES, DEFAULT_ROUTE_MESSAGES, function_route_msg_enabled),
    SETUP_BOOL(SETUP_FLASH_CLEAR_FAULTS, SETUP_PAGE_CLEAR_FAULTS, DEFAULT_CLEAR_FAULTS, function_clear_faults_enabled),
    SETUP_BOOL(SETUP_FLASH_READ_FAULTS, SETUP_PAGE_READ_FAULTS, DEFAULT_READ_FAULTS, function_read_faults_enabled),

    // Driving features
    SETUP_BOOL(SETUP_FLASH_DYNO, SETUP_PAGE_DYNO, DEFAULT_DYNO, function_dyno_mode_master_enabled),
    SETUP_BOOL_ACTION(SETUP_FLASH_ESC_TC_CUSTOMIZER, SETUP_PAGE_ESC_TC_CUSTOMIZER, DEFAULT_ESC_TC_CUSTOMIZER, function_esc_tc_customizator_enabled, setup_action_esc_tc),
    SETUP_BOOL(SETUP_FLASH_BRAKES_OVERRIDE, SETUP_PAGE_BRAKES_OVERRIDE, DEFAULT_BRAKES_OVERRIDE, function_front_brake_forcer_master),
    SETUP_BOOL(SETUP_FLASH_4WD_DISABLER, SETUP_PAGE_4WD_DISABLER, DEFAULT_4WD_DISABLER, function_4wd_disabler_enabled),
    SETUP_UINT16_ACTION(SETUP_FLASH_LAUNCH_TORQUE, SETUP_PAGE_LAUNCH_TORQUE, 600, DEFAULT_LAUNCH_TORQUE, launch_torque_threshold, setup_render_launch_torque, setup_action_launch_torque),
    SETUP_UINT8_ACTION(SETUP_FLASH_PEDAL_BOOSTER, SETUP_PAGE_PEDAL_BOOSTER, 6, DEFAULT_PEDAL_BOOSTER, function_pedal_booster_enabled, setup_render_pedal_booster, setup_action_pedal_booster),
    SETUP_INT8_ACTION(SETUP_FLASH_PEDAL_POWER, SETUP_PAGE_PEDAL_POWER, DEFAULT_PEDAL_POWER, pedal_map_power, setup_render_pedal_power, setup_action_pedal_power),

    // Driver assistance and comfort
    SETUP_BOOL(SETUP_FLASH_ACC_VIRTUAL_PAD, SETUP_PAGE_ACC_VIRTUAL_PAD, DEFAULT_ACC_VIRTUAL_PAD, function_acc_virtual_pad_enabled),
    SETUP_UINT8_ACTION(SETUP_FLASH_ACC_AUTOSTART, SETUP_PAGE_ACC_AUTOSTART, 2, DEFAULT_ACC_AUTOSTART, function_acc_autostart, setup_render_acc_autostart, setup_action_acc_autostart),
    SETUP_BOOL(SETUP_FLASH_REMOTE_START, SETUP_PAGE_REMOTE_START, DEFAULT_REMOTE_START, function_remote_start_Enabled),
    SETUP_BOOL(SETUP_FLASH_REGEN_ALERT, SETUP_PAGE_REGEN_ALERT, DEFAULT_REGEN_ALERT, function_regeneration_alert_enabled),
    SETUP_BOOL(SETUP_FLASH_SEATBELT_ALARM, SETUP_PAGE_SEATBELT_ALARM, DEFAULT_SEATBELT_ALARM, function_seatbelt_alarm_enabled),
    SETUP_BOOL_ACTION(SETUP_FLASH_ODOMETER_BLINK, SETUP_PAGE_ODOMETER_BLINK, DEFAULT_ODOMETER_BLINK, function_disable_odometer_blink, setup_action_odometer_blink),
    SETUP_BOOL_ACTION(SETUP_FLASH_PARK_MIRROR, SETUP_PAGE_PARK_MIRROR, DEFAULT_PARK_MIRROR, function_park_mirror, setup_action_park_mirror),
    SETUP_UINT8_ACTION(SETUP_FLASH_CLOSE_WINDOWS, SETUP_PAGE_CLOSE_WINDOWS, 2, DEFAULT_CLOSE_WINDOWS, function_close_windows_with_door_lock, setup_render_close_windows, setup_action_close_windows),
    SETUP_UINT8_ACTION(SETUP_FLASH_OPEN_WINDOWS, SETUP_PAGE_OPEN_WINDOWS, 2, DEFAULT_OPEN_WINDOWS, function_open_windows_with_door_lock, setup_render_open_windows, setup_action_open_windows),
    SETUP_BOOL_ACTION(SETUP_FLASH_HAS_VIRTUAL_PAD, SETUP_PAGE_HAS_VIRTUAL_PAD, DEFAULT_HAS_VIRTUAL_PAD, HAS_function_enabled, setup_action_has_virtual_pad),
    SETUP_BOOL(SETUP_FLASH_QV_EXHAUST_FLAP, SETUP_PAGE_QV_EXHAUST_FLAP, DEFAULT_QV_EXHAUST_FLAP, QV_exhaust_flap_function_enabled),
    SETUP_BOOL(SETUP_FLASH_EUJOT, SETUP_PAGE_EUJOT, DEFAULT_EUJOT, function_eujot_enabled),

    // Hidden persisted values
    SETUP_HIDDEN_BOOL(SETUP_FLASH_SHOW_RACE_MASK, DEFAULT_SHOW_RACE_MASK, function_show_race_mask),
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

static void setup_set_value(const SetupParam *param, uint16_t value) {
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

static const SetupParam *setup_find_by_page(uint8_t page_index) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].menu_page == page_index)
            return &setup_params[i];
    return 0;
}

static void setup_write_text(uint8_t page, uint8_t start, const char *text) {
    uint8_t col = start;
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH && *text)
        dashboard_setup_menu_array[page][col++] = (uint8_t)*text++;
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH)
        dashboard_setup_menu_array[page][col++] = ' ';
}

static void setup_write_text_field(uint8_t page, uint8_t start, uint8_t width, const char *text) {
    for (uint8_t i = 0; i < width && (start + i) < DASHBOARD_MESSAGE_MAX_LENGTH; i++)
        dashboard_setup_menu_array[page][start + i] = text[i] ? (uint8_t)text[i] : ' ';
}

static void setup_toggle_bool(const SetupParam *param) {
    setup_set_value(param, !setup_get_value(param));
}

static void setup_render_checkbox(const SetupParam *param) {
    if (param->menu_page != SETUP_MENU_NO_PAGE &&
        param->display_mode == SETUP_DISPLAY_CHECKBOX) {
        dashboard_setup_menu_array[param->menu_page][0] = checkbox_symbols[!!setup_get_value(param)];
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

uint16_t setup_read_flash_value(uint8_t flash_index, uint16_t stored_value) {
    const SetupParam *param = setup_find_by_flash_index(flash_index);
    if (!param)
        return 0;
    if (stored_value == 0xFFFF || stored_value > param->max_value)
        return param->default_value;
    return stored_value;
}

void setup_load_from_flash(void) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        setup_set_value(&setup_params[i], readFromFlash(setup_params[i].flash_index));
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

void setup_render_page(uint8_t page_index) {
    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    setup_render_checkbox(param);
    if (param->render)
        param->render(page_index);
}

void setup_move_page(int8_t delta) {
    if (total_pages_in_setup_dashboard_menu == 0)
        return;

    int16_t page = (int16_t)setup_dashboardPageIndex + delta;
    while (page < 0)
        page += total_pages_in_setup_dashboard_menu;
    while (page >= total_pages_in_setup_dashboard_menu)
        page -= total_pages_in_setup_dashboard_menu;

    setup_dashboardPageIndex = (uint8_t)page;
}

void setup_select_page(uint8_t page_index) {
    if (page_index == 0) {
        if (setup_is_dirty())
            saveOnflash();
        dashboard_menu_indent_level = 0;
        return;
    }

    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    if (param->action) {
        param->action();
    } else if (param->value_type == SETUP_VALUE_UINT8 && param->max_value == 1) {
        setup_toggle_bool(param);
    }
}

static void setup_action_start_stop(void) {
    function_smart_disable_start_stop_enabled = !function_smart_disable_start_stop_enabled;
    requestToDisableStartAndStop = 0;
}

static void setup_action_launch_torque(void) {
    launch_torque_threshold += 25;
    if (launch_torque_threshold > 600)
        launch_torque_threshold = 25;
}

static void setup_action_shift_rpm(void) {
    shift_threshold += 250;
    if (shift_threshold > 6000)
        shift_threshold = 1500;
}

static void setup_action_esc_tc(void) {
    function_esc_tc_customizator_enabled = !function_esc_tc_customizator_enabled;

    uint8_t msg[2] = {C2_Bh_BusID, C2_Bh_cmdFunction_ESC_TC_Enabled};
    if (!function_esc_tc_customizator_enabled) {
        ESCandTCinversion = 0;
        msg[1] = C2_Bh_cmdFunction_ESC_TC_Disabled;
    }
    addToUARTSendQueue(msg, 2);
}

static void setup_action_diesel_params(void) {
    function_is_diesel_enabled = !function_is_diesel_enabled;
    total_pages_in_params_setup_dashboard_menu = function_is_diesel_enabled ? total_pages_in_dashboard_menu_diesel : total_pages_in_dashboard_menu_gasoline;
}

static void setup_action_odometer_blink(void) {
    function_disable_odometer_blink = !function_disable_odometer_blink;

    uint8_t msg[2] = {BhBusID, BHcmdOdometerBlinkDefault};
    if (function_disable_odometer_blink)
        msg[1] = BHcmdOdometerBlinkDisable;
    addToUARTSendQueue(msg, 2);
}

static void setup_action_pedal_booster(void) {
    function_pedal_booster_enabled++;
    if (function_pedal_booster_enabled > 6)
        function_pedal_booster_enabled = 0;
    if (function_pedal_booster_enabled == 0)
        setSchizzaforteMap(2);

    uint8_t msg[3] = {C2_Bh_BusID, C2_Bh_cmdSetPedalBoostStatus, function_pedal_booster_enabled};
    addToUARTSendQueue(msg, 3);
}

static void setup_action_pedal_power(void) {
    pedal_map_power += 2;
    if (pedal_map_power > 10)
        pedal_map_power = -10;
    currentSchizzaforteMap = '-';
}

static void setup_action_park_mirror(void) {
    function_park_mirror = !function_park_mirror;

    uint8_t msg[2] = {BhBusID, BHcmdFunctParkMirrorDisabled};
    if (function_park_mirror)
        msg[1] = BHcmdFunctParkMirrorStoreCurPos;
    addToUARTSendQueue(msg, 2);
}

static void setup_action_acc_autostart(void) {
    function_acc_autostart++;
    if (function_acc_autostart > 2)
        function_acc_autostart = 0;
}

static void setup_action_close_windows(void) {
    function_close_windows_with_door_lock++;
    if (function_close_windows_with_door_lock > 2)
        function_close_windows_with_door_lock = 0;
    closeWindowsRequest = 0;
    doorLocksRequestsCounter = 0;
}

static void setup_action_open_windows(void) {
    function_open_windows_with_door_lock++;
    if (function_open_windows_with_door_lock > 2)
        function_open_windows_with_door_lock = 0;
    openWindowsRequest = 0;
    doorUnlocksRequestsCounter = 0;
}

static void setup_action_has_virtual_pad(void) {
    HAS_function_enabled = !HAS_function_enabled;

    uint8_t msg[2] = {C2_Bh_BusID, C2_Bh_cmdFunctHAS_Disabled};
    if (HAS_function_enabled)
        msg[1] = C2_Bh_cmdFunctHAS_Enabled;
    addToUARTSendQueue(msg, 2);
}

static void setup_render_launch_torque(uint8_t page) {
    char value[5];
    floatToStr(value, (float)launch_torque_threshold, 0, 4);

    if (strlen(value) == 2) {
        dashboard_setup_menu_array[page][13] = ' ';
        dashboard_setup_menu_array[page][14] = value[0];
        dashboard_setup_menu_array[page][15] = value[1];
    } else {
        dashboard_setup_menu_array[page][13] = value[0];
        dashboard_setup_menu_array[page][14] = value[1];
        dashboard_setup_menu_array[page][15] = value[2];
    }
}

static void setup_render_shift_rpm(uint8_t page) {
    char value[5];
    floatToStr(value, (float)shift_threshold, 0, 5);
    dashboard_setup_menu_array[page][10] = value[0];
    dashboard_setup_menu_array[page][11] = value[1];
    dashboard_setup_menu_array[page][12] = value[2];
    dashboard_setup_menu_array[page][13] = value[3];
}

static void setup_render_diesel_params(uint8_t page) {
    setup_write_text_field(page, 3, 8, function_is_diesel_enabled ? "Diesel" : "Gasoline");
}

static void setup_render_pedal_booster(uint8_t page) {
    static const char *labels[] = {
        "ooster", "  Auto", "  Bypass", "  A Map", "  N Map", "  D Map", "  R Map"
    };
    uint8_t index = function_pedal_booster_enabled;
    if (index > 6)
        index = 0;
    setup_write_text(page, 10, labels[index]);
}

static void setup_render_pedal_power(uint8_t page) {
    char value[5];
    floatToStr(value, (float)pedal_map_power, 0, 4);
    dashboard_setup_menu_array[page][12] = value[0];
    dashboard_setup_menu_array[page][13] = value[1];
    dashboard_setup_menu_array[page][14] = value[2];
    dashboard_setup_menu_array[page][15] = value[3];
}

static void setup_render_acc_autostart(uint8_t page) {
    switch (function_acc_autostart) {
        case 1:
            dashboard_setup_menu_array[page][17] = 'R';
            break;
        case 2:
            dashboard_setup_menu_array[page][17] = '+';
            break;
        default:
            dashboard_setup_menu_array[page][17] = ' ';
            break;
    }
}

static void setup_render_close_windows(uint8_t page) {
    dashboard_setup_menu_array[page][17] = function_close_windows_with_door_lock ? ('0' + function_close_windows_with_door_lock) : ' ';
}

static void setup_render_open_windows(uint8_t page) {
    dashboard_setup_menu_array[page][17] = function_open_windows_with_door_lock ? ('0' + function_open_windows_with_door_lock) : ' ';
}

#endif /* C1baccable */
