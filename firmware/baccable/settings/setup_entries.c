/*
 * setup_entries.c
 *
 * Single source of truth for setup menu entries.
 *
 * To add a simple on/off option:
 *   1. Add the runtime field in state/settings.c and state/settings.h.
 *   2. Add one SETUP_TOGGLE(...) entry below with its menu text.
 *
 * Declare the shared entry type and numeric bounds below. Rendering callbacks are read-only; draft editing and post-save
 * effects live in setup_menu.c.
 */

#include "settings/setup_menu.h"
#include "features/menu.h"
#include "features/ibs_override.h"

#if defined(BACCABLE_C1)

    #include <string.h>
    #include "app/powertrain.h"

    #define SETUP_FLASH_IMMOBILIZER 1
    #define SETUP_FLASH_START_STOP 2
    #define SETUP_FLASH_LED_CONTROLLER 3
    #define SETUP_FLASH_SHIFT_INDICATOR 4
    #define SETUP_FLASH_SHIFT_RPM 5
    #define SETUP_FLASH_MY23_IPC 6
    #define SETUP_FLASH_ROUTE_MESSAGES 7
    #define SETUP_FLASH_DYNO 8
    #define SETUP_FLASH_ACC_VIRTUAL_PAD 9
    #define SETUP_FLASH_BRAKES_OVERRIDE 10
    #define SETUP_FLASH_4WD_DISABLER 11
    #define SETUP_FLASH_REMOTE_START 12
    #define SETUP_FLASH_CLEAR_FAULTS 13
    #define SETUP_FLASH_ESC_TC_CUSTOMIZER 14
    #define SETUP_FLASH_READ_FAULTS 15
    #define SETUP_FLASH_DIESEL_PARAMS 16
    #define SETUP_FLASH_REGEN_ALERT 17
    #define SETUP_FLASH_LAUNCH_TORQUE 18
    #define SETUP_FLASH_SEATBELT_ALARM 19
    #define SETUP_FLASH_PEDAL_BOOSTER 20
    #define SETUP_FLASH_ODOMETER_BLINK 21
    #define SETUP_FLASH_SHOW_RACE_MASK 22
    #define SETUP_FLASH_PARK_MIRROR 23
    #define SETUP_FLASH_ACC_AUTOSTART 24
    #define SETUP_FLASH_CLOSE_WINDOWS 25
    #define SETUP_FLASH_OPEN_WINDOWS 26
    #define SETUP_FLASH_HAS_VIRTUAL_PAD 27
    #define SETUP_FLASH_QV_EXHAUST_FLAP 28
    #define SETUP_FLASH_PEDAL_POWER 29
    #define SETUP_FLASH_EUJOT 30
    #define SETUP_FLASH_PDC_MUTE 31
    #define SETUP_FLASH_REVERSE_AUDIO 32
    #define SETUP_FLASH_ROTATE 33

    #define SETUP_TEXT_HIDDEN 0

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

    #define SETUP_TOGGLE(flash, text, def, variable) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), text, 0, UI_ENTRY_TOGGLE, 0, 0, 0}
    #define SETUP_PROFILE(flash, text, def, variable, render_fn) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), text, render_fn, UI_ENTRY_ENUM, 0, 0, 0}
    #define SETUP_ENUM8(flash, text, max, def, variable, render_fn) \
        {flash, max, def, SETUP_VALUE_UINT8, &(variable), text, render_fn, UI_ENTRY_ENUM, 0, 0, 0}
    #define SETUP_NUMBER(flash, text, max, def, type, variable, render_fn, min, step) \
        {flash, max, def, type, &(variable), text, render_fn, UI_ENTRY_NUMBER, min, (type == SETUP_VALUE_INT8_AS_UINT8 ? 10 : max), step}
    #define SETUP_HIDDEN_TOGGLE(flash, def, variable) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), SETUP_TEXT_HIDDEN, 0, UI_ENTRY_TOGGLE, 0, 0, 0}

static void setup_render_usb_mode(void);

static void setup_render_launch_torque(void);
static void setup_render_shift_rpm(void);
static void setup_render_diesel_params(void);
static void setup_render_pedal_booster(void);
static void setup_render_pedal_power(void);
static void setup_render_acc_autostart(void);
static void setup_render_close_windows(void);
static void setup_render_open_windows(void);

// Interaction follows the shared draft engine; no callback may mutate live values.
const SetupParam setup_params[] = {
    // Core setup
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_IMMOBILIZER, DEFAULT_IMMOBILIZER, security_state.immobilizer_enabled),
    SETUP_TOGGLE(SETUP_FLASH_START_STOP, "Block Start/Stop", DEFAULT_START_STOP,
                        settings_state.smart_disable_start_stop_enabled),
    SETUP_NUMBER(SETUP_FLASH_LAUNCH_TORQUE, "Launch Nm", 600, DEFAULT_LAUNCH_TORQUE,
                 SETUP_VALUE_UINT16, settings_state.launch_torque_threshold, setup_render_launch_torque, 25, 25),
    SETUP_TOGGLE(SETUP_FLASH_LED_CONTROLLER, "LED strip", DEFAULT_LED_CONTROLLER,
                 settings_state.led_strip_controller_enabled),
    SETUP_TOGGLE(SETUP_FLASH_SHIFT_INDICATOR, "Shift light", DEFAULT_SHIFT_INDICATOR,
                 settings_state.shift_indicator_enabled),
    SETUP_NUMBER(SETUP_FLASH_SHIFT_RPM, "Shift RPM", 6000, DEFAULT_SHIFT_RPM,
                 SETUP_VALUE_UINT16, settings_state.shift_threshold, setup_render_shift_rpm, 1500, 250),
    SETUP_TOGGLE(SETUP_FLASH_MY23_IPC, "MY23 display", DEFAULT_MY23_IPC,
                 settings_state.ipc_my23_is_installed),
    SETUP_TOGGLE(SETUP_FLASH_REGEN_ALERT, "DPF regen alert", DEFAULT_REGEN_ALERT,
                 settings_state.regeneration_alert_enabled),
    SETUP_TOGGLE(SETUP_FLASH_SEATBELT_ALARM, "Seatbelt alarm", DEFAULT_SEATBELT_ALARM,
                 settings_state.seatbelt_alarm_enabled),

    // Diagnostics and messages
    SETUP_TOGGLE(SETUP_FLASH_ROUTE_MESSAGES, "CAN routing", DEFAULT_ROUTE_MESSAGES,
                 settings_state.route_msg_enabled),
    SETUP_TOGGLE(SETUP_FLASH_ESC_TC_CUSTOMIZER, "ESC/TC control", DEFAULT_ESC_TC_CUSTOMIZER,
                        settings_state.esc_tc_customizator_enabled),
    SETUP_TOGGLE(SETUP_FLASH_DYNO, "Dyno action", DEFAULT_DYNO, settings_state.dyno_mode_master_enabled),
    SETUP_TOGGLE(SETUP_FLASH_ACC_VIRTUAL_PAD, "Virtual ACC", DEFAULT_ACC_VIRTUAL_PAD,
                 settings_state.acc_virtual_pad_enabled),
    SETUP_TOGGLE(SETUP_FLASH_BRAKES_OVERRIDE, "Brake action", DEFAULT_BRAKES_OVERRIDE,
                 settings_state.front_brake_forcer_master),
    SETUP_TOGGLE(SETUP_FLASH_4WD_DISABLER, "AWD off action", DEFAULT_4WD_DISABLER,
                 settings_state.awd_disabler_enabled),
    SETUP_TOGGLE(SETUP_FLASH_CLEAR_FAULTS, "DTC clear action", DEFAULT_CLEAR_FAULTS,
                 settings_state.clear_faults_enabled),
    SETUP_TOGGLE(SETUP_FLASH_READ_FAULTS, "BCM fault reader", DEFAULT_READ_FAULTS,
                 settings_state.read_faults_enabled),
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_REMOTE_START, DEFAULT_REMOTE_START, settings_state.remote_start_enabled),
    SETUP_PROFILE(SETUP_FLASH_DIESEL_PARAMS, "Engine profile", DEFAULT_DIESEL_PARAMS,
                               settings_state.is_diesel_enabled, setup_render_diesel_params),

    // Driver assistance and comfort
    SETUP_TOGGLE(SETUP_FLASH_ODOMETER_BLINK, "Stop odo blink", DEFAULT_ODOMETER_BLINK,
                        settings_state.disable_odometer_blink),
    SETUP_ENUM8(SETUP_FLASH_PEDAL_BOOSTER, "Pedal mode", 8, DEFAULT_PEDAL_BOOSTER,
                        settings_state.pedal_booster_enabled, setup_render_pedal_booster),
    SETUP_NUMBER(SETUP_FLASH_PEDAL_POWER, "Pedal trim", 255, DEFAULT_PEDAL_POWER,
                 SETUP_VALUE_INT8_AS_UINT8, settings_state.pedal_map_power, setup_render_pedal_power, -10, 2),
    {SETUP_FLASH_PARK_MIRROR, 1, DEFAULT_PARK_MIRROR, SETUP_VALUE_UINT8,
     &settings_state.park_mirror, "Park mirror", 0, UI_ENTRY_SUBMENU, 0, 0, 0},
    SETUP_ENUM8(SETUP_FLASH_ACC_AUTOSTART, "ACC resume", 2, DEFAULT_ACC_AUTOSTART,
                        settings_state.acc_autostart, setup_render_acc_autostart),
    SETUP_ENUM8(SETUP_FLASH_CLOSE_WINDOWS, "Close windows", 2, DEFAULT_CLOSE_WINDOWS,
                        settings_state.close_windows_with_door_lock, setup_render_close_windows),
    SETUP_ENUM8(SETUP_FLASH_OPEN_WINDOWS, "Open windows", 2, DEFAULT_OPEN_WINDOWS,
                        settings_state.open_windows_with_door_lock, setup_render_open_windows),
    SETUP_TOGGLE(SETUP_FLASH_HAS_VIRTUAL_PAD, "Virtual HAS", DEFAULT_HAS_VIRTUAL_PAD,
                        settings_state.has_function_enabled),
    SETUP_TOGGLE(SETUP_FLASH_QV_EXHAUST_FLAP, "QV exhaust", DEFAULT_QV_EXHAUST_FLAP,
                 settings_state.qv_exhaust_flap_function_enabled),
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_EUJOT, DEFAULT_EUJOT, settings_state.eujot_enabled),

    SETUP_TOGGLE(SETUP_FLASH_PDC_MUTE, "Auto PDC mute", 0, settings_state.parking_sensor_mute),
    SETUP_TOGGLE(SETUP_FLASH_REVERSE_AUDIO, "Mute audio in R", 0, settings_state.reverse_audio_mute),
    SETUP_TOGGLE(SETUP_FLASH_ROTATE, "Auto rotate", 0, settings_state.rotate_readings),

    {34, 1, 0, SETUP_VALUE_UINT8, &settings_state.usb_sniffer,
     "USB mode", setup_render_usb_mode, UI_ENTRY_EXCLUSIVE_MODE, 0, 0, 0},
    SETUP_HIDDEN_TOGGLE(35, 0, settings_state.usb_elm327),

    SETUP_TOGGLE(37, "Advanced pages", 0, settings_state.advanced_pages),
    SETUP_HIDDEN_TOGGLE(36, 0, settings_state.gasoline_v6),

    // Hidden persisted values
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_SHOW_RACE_MASK, DEFAULT_SHOW_RACE_MASK, settings_state.show_race_mask),
};
const uint8_t setup_params_count = sizeof(setup_params) / sizeof(setup_params[0]);

/* Replace the current setting text and clear any leftover characters. */
static void setup_write_text(uint8_t start, const char *text) {
    uint8_t col = start;
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH && *text)
        dashboard_setup_screen[col++] = (uint8_t)*text++;
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH)
        dashboard_setup_screen[col++] = ' ';
}

static void setup_write_value(const char *label, const char *value) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    ui_render_value(text, sizeof(text), label, value);
    setup_write_text(0, text);
}
static void setup_write_number(const char *label, int value) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    ui_render_number(text, sizeof(text), label, value, false);
    setup_write_text(0, text);
}

static void setup_render_usb_mode(void) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    const char *mode = settings_state.usb_sniffer ? "CAN" : "OFF";
#ifdef ACT_AS_ELM327
    if (!settings_state.usb_sniffer && settings_state.usb_elm327)
        mode = "ELM327";
#endif
    ui_render_value(text, sizeof(text), "USB mode", mode);
    setup_write_text(0, text);
}

/* Show the selected launch-assist torque threshold. */
static void setup_render_launch_torque(void) {
    setup_write_number("Launch Nm", settings_state.launch_torque_threshold);
}

/* Show the selected shift-indicator engine speed. */
static void setup_render_shift_rpm(void) {
    setup_write_number("Shift RPM", settings_state.shift_threshold);
}

/* Show the engine profile used by the parameter menu. */
static void setup_render_diesel_params(void) {
    setup_write_value("Engine", settings_state.is_diesel_enabled ? "2.2 D"
                               : settings_state.gasoline_v6 ? "2.9 V6" : "2.0 I4");
}

/* Show the selected accelerator-response mode. */
static void setup_render_pedal_booster(void) {
    static const char *const labels[] = {"OFF",   "Auto",   "Bypass",
                                         "A", "N",  "D",
                                         "R", "Hybrid", "Kids"};
    uint8_t index = settings_state.pedal_booster_enabled;
    setup_write_value("Pedal mode", labels[index <= 8 ? index : 0]);
}

/* Show the signed pedal-response trim. */
static void setup_render_pedal_power(void) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    ui_render_signed_number(text, sizeof(text), "Pedal trim", settings_state.pedal_map_power, false);
    setup_write_text(0, text);
}

/* Show how adaptive cruise is configured to resume. */
static void setup_render_acc_autostart(void) {
    static const char *const labels[] = {"OFF", "RES", "+"};
    uint8_t index = settings_state.acc_autostart;
    setup_write_value("ACC resume", labels[index <= 2 ? index : 0]);
}

/* Show the lock-button gesture required to close the windows. */
static void setup_render_close_windows(void) {
#ifdef LARGE_DISPLAY
    static const char *const labels[] = {"Close windows: OFF", "Close windows: 1 lock", "Close windows: 2 locks"};
#else
    static const char *const labels[] = {"Close win: OFF", "Close win:1 lock", "Close win:2 locks"};
#endif
    uint8_t index = settings_state.close_windows_with_door_lock;
    setup_write_text(0, labels[index <= 2 ? index : 0]);
}

/* Show the unlock-button gesture required to open the windows. */
static void setup_render_open_windows(void) {
#ifdef LARGE_DISPLAY
    static const char *const labels[] = {"Open windows: OFF", "Open windows: 1 unlock", "Open windows: 2 unlocks"};
#else
    static const char *const labels[] = {"Open win: OFF", "Open win:1 unlock", "Open win:2 unlocks"};
#endif
    uint8_t index = settings_state.open_windows_with_door_lock;
    setup_write_text(0, labels[index <= 2 ? index : 0]);
}

#endif /* BACCABLE_C1 */
