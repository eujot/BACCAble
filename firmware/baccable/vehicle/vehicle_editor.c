#include "vehicle/vehicle_editor.h"
#include "app/powertrain.h"
#include "features/menu.h"
#include "vehicle/vehicle_runtime.h"
#include <stdio.h>
#include <string.h>

#if defined(BACCABLE_C1)

typedef enum {
    VEHICLE_FIELD_MODEL = 0,
    VEHICLE_FIELD_YEAR,
    VEHICLE_FIELD_ENGINE,
    VEHICLE_FIELD_DRIVETRAIN,
    VEHICLE_FIELD_DISPLAY,
    VEHICLE_FIELD_EQUIPMENT_SUSPENSION,
    VEHICLE_FIELD_EQUIPMENT_RACE,
    VEHICLE_FIELD_EQUIPMENT_EXHAUST,
    VEHICLE_FIELD_EQUIPMENT_ACC,
    VEHICLE_FIELD_EQUIPMENT_HAS,
    VEHICLE_FIELD_EQUIPMENT_LANE_ASSIST,
    VEHICLE_FIELD_EQUIPMENT_PARK_MIRROR,
    VEHICLE_FIELD_EQUIPMENT_SCR,
    VEHICLE_FIELD_COUNT,
} VehicleField;

/* Map an equipment editor field to its VehicleProfile equipment slot, or -1. */
static int equipment_slot(VehicleField f) {
    return f >= VEHICLE_FIELD_EQUIPMENT_SUSPENSION ? (int)(f - VEHICLE_FIELD_EQUIPMENT_SUSPENSION) : -1;
}

static VehicleProfile draft;
static VehicleField field;
static bool editing, confirm;
static unsigned consequence;
/* Review saved preferences that will be suppressed, without changing those preferences. */
static const char *confirmation_label(void) {
    const struct { VehicleFeature feature; bool enabled; const char *label; } options[] = {
        {VEHICLE_FEATURE_AWD_DISABLE, settings_state.awd_disabler_enabled, "OFF: AWD action"},
        {VEHICLE_FEATURE_DEFAULT_RACE, settings_state.default_drive_mode == 3, "OFF: Default Race"},
        {VEHICLE_FEATURE_SOFT_SUSPENSION, settings_state.default_suspension_softer, "OFF: Suspension"},
        {VEHICLE_FEATURE_EXHAUST_ACTION, settings_state.qv_exhaust_flap_function_enabled, "OFF: QV exhaust"},
        {VEHICLE_FEATURE_VIRTUAL_ACC, settings_state.acc_virtual_pad_enabled || settings_state.acc_autostart, "OFF: Virtual ACC"},
        {VEHICLE_FEATURE_VIRTUAL_HAS, settings_state.has_function_enabled, "OFF: Virtual HAS"},
        {VEHICLE_FEATURE_LANE_ASSIST, settings_state.lane_assist_disabled, "OFF: LKA"},
        {VEHICLE_FEATURE_PARK_MIRROR, settings_state.park_mirror, "OFF: Park mirror"},
        {VEHICLE_FEATURE_DPF_REGEN, settings_state.regeneration_alert_enabled, "OFF: DPF alert"},
    };
    VehicleProfile accepted = draft;
    accepted.confirmed = true; accepted.legacy_permissions = false;
    unsigned count = 0;
    for (unsigned i = 0; i < sizeof(options)/sizeof(options[0]); ++i)
        if (options[i].enabled && !vehicle_feature_allowed_for(&accepted, options[i].feature)) ++count;
    unsigned selected = consequence % (count + 1);
    if (!selected) return "HOLD save Vehicle";
    for (unsigned i = 0; i < sizeof(options)/sizeof(options[0]); ++i)
        if (options[i].enabled && !vehicle_feature_allowed_for(&accepted, options[i].feature) && --selected == 0)
            return options[i].label;
    return "HOLD save Vehicle";
}

void vehicle_editor_begin(void) {
    draft = *vehicle_profile_current();
    field = VEHICLE_FIELD_MODEL;
    consequence = 0;
    editing = false;
    confirm = false;
}

void vehicle_editor_cancel(void) {
    editing = false;
    confirm = false;
}

static unsigned wrap(unsigned value, unsigned count, int direction) {
    if (!count)
        return 0;
    return (value + count + (direction < 0 ? count - 1 : 1)) % count;
}

static void cycle(int direction) {
    switch (field) {
    case VEHICLE_FIELD_MODEL:
        draft.model = (VehicleModel)wrap((unsigned)draft.model, 3, direction);
        break;
    case VEHICLE_FIELD_YEAR: {
        int year = draft.model_year == 0 ? 1999 : (int)draft.model_year; /* wrap into 2000..2099 */
        year += direction < 0 ? -1 : 1;
        if (year < 2000)
            draft.model_year = 0;
        else if (year > 2099)
            draft.model_year = 2000;
        else
            draft.model_year = (uint16_t)year;
        break;
    }
    case VEHICLE_FIELD_ENGINE:
        draft.engine = (VehicleEngine)wrap((unsigned)draft.engine, 4, direction);
        break;
    case VEHICLE_FIELD_DRIVETRAIN:
        draft.drivetrain = (VehicleDrivetrain)wrap((unsigned)draft.drivetrain, 3, direction);
        break;
    case VEHICLE_FIELD_DISPLAY:
        draft.display_selection = (VehicleDisplaySelection)wrap((unsigned)draft.display_selection, 4, direction);
        break;
    default: {
        int slot = equipment_slot(field);
        if (slot >= 0)
            draft.equipment[slot] = (VehicleEquipment)wrap((unsigned)draft.equipment[slot], 3, direction);
        break;
    }
    }
}

void vehicle_editor_navigate(int direction) {
    if (confirm) { consequence = direction < 0 && consequence ? consequence - 1 : consequence + 1; return; }
    if (editing) {
        cycle(direction);
    } else if (!confirm) {
        field = (VehicleField)wrap((unsigned)field, VEHICLE_FIELD_COUNT, direction);
    }
}

void vehicle_editor_select(void) {
    if (confirm) return;
    if (!editing)
        editing = true;
    else
        cycle(1);
}

bool vehicle_editor_back(void) {
    if (editing) {
        editing = false;
        confirm = false;
        return true;
    }
    if (!confirm) {
        confirm = true;
        return true;
    }
    return false;
}

const char *vehicle_editor_save(void) {
    if (!confirm) { editing = false; confirm = true; return "Review; HOLD save"; }
    const char *reason = vehicle_profile_busy_reason();
    if (reason) return reason;
    VehicleProfile candidate = draft;
    candidate.confirmed = true;
    candidate.legacy_permissions = false;
    if (!vehicle_profile_validate(&candidate, NULL)) return "Invalid Vehicle";
    if (settings_profile_save(&candidate)) return "Save failed";
    menu_engine_changed();
    vehicle_editor_begin();
    return NULL;
}

VehicleEditorState vehicle_editor_state(void) {
    if (editing)
        return VEHICLE_EDITOR_EDIT;
    if (confirm)
        return VEHICLE_EDITOR_CONFIRM;
    return VEHICLE_EDITOR_BROWSE;
}

const char *vehicle_editor_field_label(void) {
    if (confirm) return confirmation_label();
    static const char *const labels[] = {"Model", "Model year", "Engine", "Drivetrain", "Display",
                                         "Suspension", "Race", "Exhaust valves", "ACC", "HAS",
                                         "Lane assist", "Park mirrors", "SCR"};
    return labels[field];
}

const char *vehicle_editor_value_label(char *buf, size_t size) {
    switch (field) {
    case VEHICLE_FIELD_MODEL: {
        static const char *const v[] = {"Unknown", "Giulia", "Stelvio"};
        return v[draft.model <= 2 ? draft.model : 0];
    }
    case VEHICLE_FIELD_YEAR:
        snprintf(buf, size, draft.model_year == 0 ? "Unknown" : "%u", (unsigned)draft.model_year);
        return buf;
    case VEHICLE_FIELD_ENGINE: {
        static const char *const v[] = {"Unknown", "2.0 Petrol", "2.9 Petrol", "2.2 Diesel"};
        return v[draft.engine <= 3 ? draft.engine : 0];
    }
    case VEHICLE_FIELD_DRIVETRAIN: {
        static const char *const v[] = {"Unknown", "RWD", "Q4"};
        return v[draft.drivetrain <= 2 ? draft.drivetrain : 0];
    }
    case VEHICLE_FIELD_DISPLAY: {
        static const char *const v[] = {"Auto", "Legacy 18", "Legacy 24", "MY23"};
        return v[draft.display_selection <= 3 ? draft.display_selection : 0];
    }
    default: {
        static const char *const v[] = {"Unknown", "Absent", "Present"};
        int slot = equipment_slot(field);
        if (slot >= 0)
            return v[draft.equipment[slot] <= 2 ? draft.equipment[slot] : 0];
        return "";
    }
    }
}

#endif /* BACCABLE_C1 */
