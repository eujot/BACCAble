/*
 * setup_menu.c
 *
 * Setup menu engine. Menu entries live in setup_entries.c.
 */

#include "settings/setup_menu.h"
#include "features/menu.h"
#include "features/my23_ui.h"
#include "features/ibs_override.h"
#include "features/drive_mode_check.h"
#include "diagnostics/fault_reader.h"
#include "vehicle/vehicle_runtime.h"

#if defined(BACCABLE_C1)

    #include <string.h>
    #include "app/powertrain.h"



uint8_t setup_dashboardPageIndex = 0;
uint8_t dashboard_setup_screen[24];
uint8_t total_pages_in_setup_dashboard_menu = 0;

/* Draft values remain separate until the explicit save succeeds. */
static const SetupParam *staged;
static uint16_t stage_value, stage_original;
static uint8_t stage_extra, stage_original_extra;
static bool stage_confirm;
static void stage_cycle(int direction);
static bool park_menu;
static uint8_t park_page;
static const struct {
    UiEntryType type;
    const char *label;
} park_entries[] = {{UI_ENTRY_TOGGLE, "Enabled"},
                    {UI_ENTRY_CAPTURE, "Store position"},
                    {UI_ENTRY_ACTION, "Back"}};
/* 0: browse, 1: confirm capture, 2: queued, 3: send rejected. */
static uint8_t park_capture;

/* Identify a draft or nested capture workflow before leaving setup. */
bool setup_in_workflow(void) { return staged != NULL || park_menu; }
/* Discard unaccepted edits when setup is reset or closes through inactivity. */
void setup_cancel_edit(void) {
    staged = NULL;
    stage_confirm = false;
    park_menu = false;
    park_page = park_capture = 0;
}
/* Return one workflow level without accepting a draft or capturing a position. */
bool setup_back(void) {
    if (staged) return setup_stage_back();
    if (park_capture) {
        park_capture = 0;
        return true;
    }
    if (park_menu) {
        park_menu = false;
        return true;
    }
    return false;
}

/* Parking capture has no storage acknowledgement on the inter-board protocol. */
static void setup_park_select(void) {
    if (!vehicle_feature_allowed(VEHICLE_FEATURE_PARK_MIRROR)) { menu_notice("Configure Vehicle"); return; }
    if (park_capture == 2) {
        park_capture = 0;
    } else if (park_capture) {
        uint8_t command[2] = {BhBusID, BHcmdFunctParkMirrorStoreCurPos};
        park_capture = board_uart_send(command, sizeof(command)) ? 2 : 3;
    } else if (park_entries[park_page].type == UI_ENTRY_CAPTURE && settings_state.park_mirror) {
        park_capture = 1;
    } else if (park_entries[park_page].type == UI_ENTRY_ACTION) {
        park_menu = false;
    }
}

/* Keep enable, capture confirmation and queue feedback visually distinct. */
static void setup_park_render(char *text, size_t size) {
    if (park_capture == 1)
        ui_render_action(text, size, "Adjust;HOLD=save");
    else if (park_capture == 2)
        ui_render_value(text, size, "Store", "queued");
    else if (park_capture == 3)
        ui_render_unavailable(text, size, "Send busy; retry");
    else if (park_entries[park_page].type == UI_ENTRY_TOGGLE)
        ui_render_checkbox(text, size, park_entries[park_page].label, settings_state.park_mirror);
    else if (park_entries[park_page].type == UI_ENTRY_CAPTURE && !settings_state.park_mirror)
        ui_render_unavailable(text, size, "Enable mirror");
    else if (park_entries[park_page].type == UI_ENTRY_CAPTURE)
        ui_render_action(text, size, park_entries[park_page].label);
    else
        snprintf_(text, size, UI_SYMBOL_BACK " Back");
}

/* Read the current value of a configurable feature. */
static uint16_t setup_get_value(const SetupParam *param) {
    switch (param->value_type) {
    case SETUP_VALUE_UINT16:
        return *(uint16_t *)param->value;
    case SETUP_VALUE_INT8_AS_UINT8:
        return (uint8_t)*(int8_t *)param->value;
    case SETUP_VALUE_UINT8:
    default:
        return *(uint8_t *)param->value;
    }
}

/* Restore a configurable feature's value using its declared type. */
static void setup_set_value(const SetupParam *param, uint16_t value) {
    switch (param->value_type) {
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

/* Check whether a saved preference has a user-facing menu entry. */
static uint8_t setup_param_is_visible(const SetupParam *param) {
    if (!param->menu_text) return 0;
    VehicleFeature feature = vehicle_setting_feature(param->flash_index);
    return feature == VEHICLE_FEATURE_COUNT || !vehicle_profile_current()->confirmed ||
           vehicle_feature_support(feature) != VEHICLE_SUPPORT_UNSUPPORTED;
}

/* Count the available feature settings, without synthetic navigation entries. */
static uint8_t setup_menu_pages_count(void) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_param_is_visible(&setup_params[i]))
            count++;
    if (count > SETUP_FLASH_PARAM_BUFFER_SIZE)
        count = SETUP_FLASH_PARAM_BUFFER_SIZE;
    total_pages_in_setup_dashboard_menu = count;
    return count;
}

/* Place a feature setting in its functional navigation group. */
static uint8_t setup_group(uint8_t id) {
    switch (id) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 17:
    case 33:
    case 37:
        return 0; /* Display */
    case 19:
    case 21:
    case 23:
    case 25:
    case 26:
    case 28:
    case 31:
    case 32:
    case 42:
        return 1; /* Comfort */
    case 9:
    case 24:
    case 27:
        return 2; /* Assistance */
    case 8:
    case 10:
    case 11:
    case 14:
    case 18:
    case 20:
    case 29:
        return 3; /* Drivetrain */
    default:
        return 4; /* Device */
    }
}

/* Find the feature setting at a visible menu position. */
static const SetupParam *setup_find_by_page(uint8_t page_index) {
    uint8_t visible_page = 0;
    for (unsigned group = 0; group < 5; ++group)
        for (unsigned i = 0; i < setup_params_count; ++i) {
            const SetupParam *param = &setup_params[i];
            if (!setup_param_is_visible(param) || setup_group(param->flash_index) != group)
                continue;
            if (visible_page++ == page_index)
                return param;
        }
    return NULL;
}

/* Jump to a different functional group of feature settings. */
void setup_move_group(int8_t delta) {
    if (setup_in_workflow()) {
        setup_move_page(delta);
        return;
    }
    const SetupParam *current = setup_find_by_page(setup_dashboardPageIndex);
    uint8_t group = current ? setup_group(current->flash_index) : 255;
    for (unsigned i = 0; i < setup_menu_pages_count(); ++i) {
        setup_move_page(delta);
        const SetupParam *next = setup_find_by_page(setup_dashboardPageIndex);
        if (!next || setup_group(next->flash_index) != group)
            return;
    }
}

/* Find a setting by the permanent identity used in saved preferences. */
const SetupParam *setup_find_by_flash_index(uint8_t flash_index) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index == flash_index)
            return &setup_params[i];
    return 0;
}

/* Determine how many saved setting positions the current feature set needs. */
uint8_t setup_flash_slots_count(void) {
    uint8_t count = SETUP_FLASH_SLOTS;
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index > count)
            count = setup_params[i].flash_index;
    return count;
}

/* Restore a supported setting value or fall back to its default. */
uint16_t setup_read_flash_value(uint8_t flash_index, uint16_t stored_value) {
    const SetupParam *param = setup_find_by_flash_index(flash_index);
    if (!param)
        return 0;
    if (stored_value == 0xFFFF || stored_value > param->max_value)
        return param->default_value;
    if (param->entry_type == UI_ENTRY_NUMBER) {
        int value = param->value_type == SETUP_VALUE_INT8_AS_UINT8 ? (int8_t)(uint8_t)stored_value : stored_value;
        if (value < param->minimum || value > param->maximum)
            return param->default_value;
    }
    return stored_value;
}

/* Restore all feature preferences before vehicle operation starts. */
void setup_load_from_flash(void) {
    settings_load_begin();
    setup_menu_pages_count();
    for (uint8_t i = 0; i < setup_params_count; i++)
        setup_set_value(&setup_params[i], settings_read(setup_params[i].flash_index));
    /* Slot 3 remains reserved, but an old LED preference must never reactivate it. */
    settings_state.retired_led_strip = 0;
    /* CAN capture already has runtime precedence over ELM327. */
    if (settings_state.usb_sniffer)
        settings_state.usb_elm327 = 0;
}

/* Collect the current feature preferences for saving. */
void setup_fill_flash_params(uint16_t *params) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        params[setup_params[i].flash_index - 1] = setup_get_value(&setup_params[i]);
}

/* Refuse permission changes that would hide an active feature or silently reset another one. */
static const char *setup_unavailable(const SetupParam *param) {
    const char *policy = vehicle_setting_reason(param->flash_index);
    if (policy && !setup_get_value(param)) return policy;
    if (setup_get_value(param) && menu_setting_busy(param->flash_index))
        return "Request pending";
    switch (param->flash_index) {
    case 8:
        return settings_state.dyno_mode_master_enabled && chassis_state.dyno_mode_enabled_on_master
                   ? "Stop Dyno" : NULL;
    case 10:
        return settings_state.front_brake_forcer_master && chassis_state.front_brake_forced
                   ? "Release brake" : NULL;
    case 11:
        return settings_state.awd_disabler_enabled && chassis_state.awd_sequence ? "Stop AWD request" : NULL;
    case 13:
        return settings_state.clear_faults_enabled && diagnostics_state.clear_faults_request ? "DTC clear active" : NULL;
    case 15:
        return settings_state.read_faults_enabled && fault_reader_busy() ? "BCM read active" : NULL;
    case 34:
        return ibs_override_enabled() ? "Stop IBS" : NULL;
    case 14:
        return settings_state.esc_tc_customizator_enabled && chassis_state.stability_inverted
                   ? "Reset ESC" : NULL;
    case 28:
        return settings_state.qv_exhaust_flap_function_enabled && comfort_state.force_q_vexhaust_valve_opened
                   ? "Set QV to AUTO" : NULL;
    default: return NULL;
    }
}

/* Share choice labels between the saved value and its draft. */
static const char *setup_value_label(const SetupParam *param, uint16_t value, uint8_t extra, char number[24]) {
    const char *label = number;
    if (param->entry_type == UI_ENTRY_TOGGLE) label = (value != (param->flash_index == 39)) ? "ON" : "OFF";
    else if (param->flash_index == 34) label = value ? "CAN" : extra ? "ELM327" : "OFF";
    else if (param->flash_index == 16) label = value ? "2.2 D" : extra ? "2.9 V6" : "2.0 I4";
    else if (param->flash_index == 40) {
        static const char *const choices[] = {"Not set", "RWD", "Q4"};
        label = choices[value <= 2 ? value : 0];
    }
    else if (param->flash_index == 20) {
        static const char *const modes[] = {"OFF", "Auto", "Bypass", "A", "N", "D", "R", "Hybrid", "Kids"};
        label = modes[value <= 8 ? value : 0];
    } else if (param->flash_index == 38) {
        static const char *const modes[] = {"N", "A", "D", "Race"};
        label = modes[value <= 3 ? value : 0];
    } else if (param->flash_index == 41) {
        if (!value) label = "Stiff";
        else if (settings_state.default_drive_mode == 2) label = "Soft";
        else if (settings_state.default_drive_mode == 3) label = "Mid";
        else label = "Softer";
    } else if (param->flash_index == 24) {
        static const char *const modes[] = {"OFF", "RES", "+"};
        label = modes[value <= 2 ? value : 0];
    } else if (param->flash_index == 25 || param->flash_index == 26) {
        if (!value) label = "OFF";
        else snprintf_(number, 24, "%u %s%s", value, param->flash_index == 25 ? "lock" : "unlock", value > 1 ? "s" : "");
    } else snprintf_(number, 24, param->value_type == SETUP_VALUE_INT8_AS_UINT8 ? "%+d" : "%d", param->value_type == SETUP_VALUE_INT8_AS_UINT8 ? (int8_t)value : (int)value);
    return label;
}

/* Show the saved target and the result of the current ignition's mode check. */
static void setup_default_mode_result(char result[24]) {
    static const char *const modes[] = {"N", "A", "D", "Race"};
    unsigned mode = settings_state.default_drive_mode;
    const char *label = modes[mode <= 3 ? mode : 0];
    if (mode == 3 && !(vehicle_display_effective().rows == 2)) label = "R";
    snprintf_(result, 24, "%s %s", label, drive_mode_check_status());
}

/* Show the selected setting and its current value. */
void setup_render_page(uint8_t page_index) {
    if (page_index >= setup_menu_pages_count())
        return;

    if (staged) {
        char number[24], hint[24];
        const char *value = setup_value_label(staged, stage_value, stage_extra, number);
        snprintf_(hint, sizeof(hint), "%s" MY23_BULLET "HOLD SAVE", value);
        if ((vehicle_display_effective().rows == 2))
            menu_present_view(stage_confirm ? UI_MODE_CONFIRM : UI_MODE_EDIT, staged->menu_text, stage_confirm ? "HOLD SAVE" MY23_BULLET "2X DISCARD" : hint);
        else {
            char text[25];
            if (stage_confirm) snprintf_(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), "Hold save/2x drop");
            else if (staged->entry_type == UI_ENTRY_NUMBER && staged->value_type == SETUP_VALUE_INT8_AS_UINT8)
                ui_render_signed_number(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), staged->menu_text, (int8_t)stage_value, true);
            else if (staged->entry_type == UI_ENTRY_NUMBER)
                ui_render_number(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), staged->menu_text, stage_value, true);
            else ui_render_value(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), staged->menu_text, value);
            menu_present(text);
        }
        return;
    }

    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    char text[25];
    const char *reason = setup_unavailable(param);
    if (reason)
        ui_render_unavailable(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), reason);
    else if (park_menu)
        setup_park_render(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U));
    else if (param->entry_type == UI_ENTRY_TOGGLE && param->flash_index != 39)
        ui_render_checkbox(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), param->menu_text, !!setup_get_value(param));
    else if (param->entry_type == UI_ENTRY_SUBMENU)
        ui_render_action(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), param->menu_text);
    else {
        if (param->render) { param->render(); return; }
        char number[24];
        uint8_t extra = param->flash_index == 34 ? settings_state.usb_elm327 : settings_state.gasoline_v6;
        const char *value = setup_value_label(param, setup_get_value(param), extra, number);
        if (param->flash_index == 38 && setup_get_value(param)) {
            char result[24];
            setup_default_mode_result(result);
            if (!strcmp(drive_mode_check_status(), "Start engine"))
                ui_render_value(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), value, "Start engine");
            else ui_render_value(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), "DNA", result);
        } else ui_render_value(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U), param->menu_text, value);
    }
    memset(dashboard_setup_screen, ' ', sizeof(dashboard_setup_screen));
    memcpy(dashboard_setup_screen, text, strlen(text));
}

/* Select the next or previous feature setting. */
void setup_move_page(int8_t delta) {
    if (staged) {
        if (stage_confirm) return;
        if (staged->entry_type == UI_ENTRY_NUMBER) {
            int value = (staged->value_type == SETUP_VALUE_INT8_AS_UINT8 ? (int8_t)stage_value : stage_value) + delta * staged->step;
            stage_value = (uint16_t)(value < staged->minimum ? staged->minimum : value > staged->maximum ? staged->maximum : value);
            if (staged->value_type == SETUP_VALUE_INT8_AS_UINT8) stage_value = (uint8_t)stage_value;
        } else stage_cycle(delta);
        return;
    }
    if (park_menu) {
        if (!park_capture)
            park_page = (uint8_t)((park_page + (delta > 0 ? 1 : 2)) % 3);
        return;
    }
    uint8_t pages_count = setup_menu_pages_count();
    if (pages_count == 0)
        return;

    int16_t page = (int16_t)setup_dashboardPageIndex + delta;
    while (page < 0)
        page += pages_count;
    while (page >= pages_count)
        page -= pages_count;

    setup_dashboardPageIndex = (uint8_t)page;
}

/* The public entry point shares the same draft path as physical menu events. */
void setup_select_page(uint8_t page_index) {
    if (page_index >= setup_menu_pages_count()) return;
    setup_dashboardPageIndex = page_index;
    setup_stage_select();
}

#endif /* BACCABLE_C1 */

#if defined(BACCABLE_C1)
/* Menu drafts are isolated from live vehicle preferences and hardware callbacks. */
static uint8_t stage_companion(const SetupParam *param) {
    return param->flash_index == 34 ? settings_state.usb_elm327 :
           param->flash_index == 16 ? settings_state.gasoline_v6 : 0;
}
static void stage_assign(uint16_t value, uint8_t extra) {
    setup_set_value(staged, value);
    if (staged->flash_index == 34) settings_state.usb_elm327 = extra;
    if (staged->flash_index == 16) settings_state.gasoline_v6 = extra;
}
void setup_stage_select(void) {
    if (stage_confirm) return;
    if (park_menu && !staged) {
        if (park_capture == 1 || park_capture == 3) return;
        if (park_page != 0 || park_capture) { setup_park_select(); return; }
        staged = setup_find_by_page(setup_dashboardPageIndex);
        if (!staged) return;
        stage_original = stage_value = settings_state.park_mirror;
        stage_original_extra = stage_extra = 0;
        return;
    }
    if (!staged) {
        const SetupParam *param = setup_find_by_page(setup_dashboardPageIndex);
        if (!param) return;
        if (param->entry_type == UI_ENTRY_SUBMENU) { park_menu = true; park_page = park_capture = 0; return; }
        const char *reason = setup_unavailable(param);
        if (reason) { menu_notice(reason); return; }
        staged = param;
        stage_original = stage_value = setup_get_value(param);
        stage_original_extra = stage_extra = stage_companion(param);
        return;
    }
    stage_cycle(1);
}
static void stage_cycle(int direction) {
    unsigned step = direction > 0 ? 1 : 0;
    if (staged->flash_index == 34) {
#ifdef ACT_AS_ELM327
        unsigned choices = 3;
#else
        unsigned choices = 2;
#endif
        unsigned mode = stage_value ? 1 : stage_extra ? 2 : 0;
        mode = (mode + choices + (step ? 1 : choices - 1)) % choices;
        stage_value = mode == 1; stage_extra = mode == 2;
    } else if (staged->flash_index == 16) {
        unsigned mode = stage_value ? 2 : stage_extra ? 1 : 0;
        mode = (mode + (step ? 1 : 2)) % 3;
        stage_value = mode == 2;
        stage_extra = mode == 1 || (mode == 2 && (stage_original ? stage_original_extra : 1));
    } else if (staged->flash_index == 38) {
        static const uint8_t order[] = {1, 0, 2, 3}; /* A, N, D, Race; keep saved encodings. */
        unsigned position = 0;
        while (position < 3 && order[position] != stage_value) ++position;
        position = (position + (step ? 1 : 3)) % 4;
        stage_value = order[position];
    } else if (staged->entry_type != UI_ENTRY_NUMBER) {
        unsigned choices = staged->max_value + 1;
        stage_value = (stage_value + choices + (step ? 1 : choices - 1)) % choices;
    }
}

bool setup_stage_back(void) {
    if (!staged) return false;
    if (!stage_confirm && (stage_value != stage_original || stage_extra != stage_original_extra)) {
        stage_confirm = true;
        return true;
    }
    staged = NULL;
    stage_confirm = false;
    return true;
}
void setup_stage_save(void) {
    if (!staged && park_menu && (park_capture == 1 || park_capture == 3)) { setup_park_select(); return; }
    if (!staged) return;
    /* Conditions may have changed while the draft was being edited. */
    const char *reason = setup_unavailable(staged);
    if (reason) { menu_notice(reason); return; }
    VehicleFeature feature = vehicle_setting_feature(staged->flash_index);
    if ((stage_value && feature != VEHICLE_FEATURE_COUNT && !vehicle_feature_allowed(feature)) ||
        (staged->flash_index == 38 && stage_value == 3 && !vehicle_feature_allowed(VEHICLE_FEATURE_DEFAULT_RACE))) {
        menu_notice("Configure Vehicle"); return;
    }
    stage_assign(stage_value, stage_extra);
    if (settings_save()) {
        stage_assign(stage_original, stage_original_extra);
        menu_notice(UI_SYMBOL_FAILURE " Save failed");
        return;
    }
    /* Board synchronization/USB apply ran only after successful persistence. */
    switch (staged->flash_index) {
    case 2: comfort_state.request_to_disable_start_and_stop = 0; break;
    case 14: if (!stage_value) chassis_state.stability_inverted = 0; break;
    case 20: if (!stage_value) pedal_booster_set_map(2); break;
    case 29: pedal_state.current_schizzaforte_map = '-'; break;
    case 25: comfort_state.close_windows_request = comfort_state.door_locks_requests_counter = 0; break;
    case 26: comfort_state.open_windows_request = comfort_state.door_unlocks_requests_counter = 0; break;
    default: break;
    }
    stage_original = stage_value;
    stage_original_extra = stage_extra;
    stage_confirm = false;
    menu_notice_saved(staged->menu_text);
}
#endif

#if defined(BACCABLE_C1)
bool setup_stage_active(void) { return staged != NULL; }
bool setup_present_my23(void) {
    if (!(vehicle_display_effective().rows == 2)) return false;
    if (staged) return true;
    if (park_menu) {
        char text[25];
        setup_park_render(text, (vehicle_display_effective().rows == 2 ? sizeof(text) : vehicle_display_effective().widths[0] + 1U));
        menu_present_lines(text, park_capture == 1 ? "HOLD STORE" MY23_BULLET "2X BACK" : "CLICK USE" MY23_BULLET "2X BACK");
        return true;
    }
    const SetupParam *current = setup_find_by_page(setup_dashboardPageIndex);
    const SetupParam *next = setup_find_by_page((setup_dashboardPageIndex + 1) % setup_menu_pages_count());
    if (current && current->flash_index == 38 && setup_get_value(current)) {
        char result[24];
        setup_default_mode_result(result);
        menu_present_lines(current->menu_text, result);
        return true;
    }
    menu_present_view(UI_MODE_LIST, current ? current->menu_text : "Features", next ? next->menu_text : "");
    return true;
}
#endif
