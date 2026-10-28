#include "vehicle/vehicle_editor.h"
#include "app/powertrain.h"
#include "features/menu.h"
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

void vehicle_editor_begin(void) {
    draft = *vehicle_profile_current();
    field = VEHICLE_FIELD_MODEL;
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
    if (editing) {
        cycle(direction);
    } else if (!confirm) {
        field = (VehicleField)wrap((unsigned)field, VEHICLE_FIELD_COUNT, direction);
    }
}

void vehicle_editor_select(void) {
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

void vehicle_editor_save(void) {
    if (!vehicle_profile_commit(&draft))
        return;
    bool diesel = false, v6 = false, my23 = false;
    uint8_t drivetrain = 0;
    vehicle_profile_legacy_flags(&draft, &diesel, &v6, &drivetrain, &my23);
    settings_state.is_diesel_enabled = diesel;
    settings_state.gasoline_v6 = v6;
    settings_state.drivetrain = drivetrain;
    settings_state.ipc_my23_is_installed = my23;
    settings_save();
    menu_engine_changed();
    vehicle_editor_begin();
}

VehicleEditorState vehicle_editor_state(void) {
    if (editing)
        return VEHICLE_EDITOR_EDIT;
    if (confirm)
        return VEHICLE_EDITOR_CONFIRM;
    return VEHICLE_EDITOR_BROWSE;
}

const char *vehicle_editor_field_label(void) {
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
        static const char *const v[] = {"Auto", "18", "24", "MY23"};
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
