#include "vehicle/vehicle_profile.h"

/*
 * Single owner of the configured car's facts and the policy derived from them.
 * This module must not depend on the menu renderer or the request scheduler.
 */

static VehicleProfile current_profile;
static uint16_t profile_generation;

void vehicle_profile_default(VehicleProfile *profile) {
    profile->model = VEHICLE_MODEL_UNKNOWN;
    profile->engine = VEHICLE_ENGINE_UNKNOWN;
    profile->model_year = 0;
    profile->drivetrain = VEHICLE_DRIVETRAIN_UNKNOWN;
    profile->display_selection = VEHICLE_DISPLAY_AUTO;
    for (unsigned i = 0; i < VEHICLE_EQUIPMENT_COUNT; ++i)
        profile->equipment[i] = VEHICLE_EQUIPMENT_UNKNOWN;
    profile->confirmed = false;
    profile->legacy_permissions = false;
}

/* Accept a wider type so untrusted decoded bytes are validated, not assumed in-range. */
static bool model_valid(int value) {
    return value >= VEHICLE_MODEL_UNKNOWN && value <= VEHICLE_MODEL_STELVIO;
}
static bool engine_valid(int value) {
    return value >= VEHICLE_ENGINE_UNKNOWN && value <= VEHICLE_ENGINE_DIESEL22;
}
static bool drivetrain_valid(int value) {
    return value >= VEHICLE_DRIVETRAIN_UNKNOWN && value <= VEHICLE_DRIVETRAIN_Q4;
}
static bool display_valid(int value) {
    return value >= VEHICLE_DISPLAY_AUTO && value <= VEHICLE_DISPLAY_MY23;
}
static bool equipment_valid(int value) {
    return value >= VEHICLE_EQUIPMENT_UNKNOWN && value <= VEHICLE_EQUIPMENT_PRESENT;
}

bool vehicle_profile_validate(const VehicleProfile *draft, VehicleProfileError *error) {
    if (error)
        *error = VEHICLE_PROFILE_OK;
    if (!draft)
        return false;
    if (!model_valid(draft->model)) {
        if (error) *error = VEHICLE_PROFILE_ERR_MODEL;
        return false;
    }
    if (!engine_valid(draft->engine)) {
        if (error) *error = VEHICLE_PROFILE_ERR_ENGINE;
        return false;
    }
    if (draft->model_year != 0 && (draft->model_year < 2000 || draft->model_year > 2099)) {
        if (error) *error = VEHICLE_PROFILE_ERR_YEAR;
        return false;
    }
    if (!drivetrain_valid(draft->drivetrain)) {
        if (error) *error = VEHICLE_PROFILE_ERR_DRIVETRAIN;
        return false;
    }
    if (!display_valid(draft->display_selection)) {
        if (error) *error = VEHICLE_PROFILE_ERR_DISPLAY;
        return false;
    }
    for (unsigned i = 0; i < VEHICLE_EQUIPMENT_COUNT; ++i) {
        if (!equipment_valid(draft->equipment[i])) {
            if (error) *error = VEHICLE_PROFILE_ERR_EQUIPMENT;
            return false;
        }
    }
    return true;
}

const VehicleProfile *vehicle_profile_current(void) { return &current_profile; }
uint16_t vehicle_profile_generation(void) { return profile_generation; }

bool vehicle_profile_commit(const VehicleProfile *profile) {
    VehicleProfileError error;
    if (!vehicle_profile_validate(profile, &error))
        return false;
    current_profile = *profile;
    ++profile_generation;
    return true;
}

bool vehicle_profile_is_gasoline(const VehicleProfile *profile) {
    return profile && (profile->engine == VEHICLE_ENGINE_PETROL20 || profile->engine == VEHICLE_ENGINE_PETROL29);
}
bool vehicle_profile_is_diesel(const VehicleProfile *profile) {
    return profile && profile->engine == VEHICLE_ENGINE_DIESEL22;
}
bool vehicle_profile_is_v6(const VehicleProfile *profile) { return profile && profile->engine == VEHICLE_ENGINE_PETROL29; }
uint8_t vehicle_profile_cylinders(const VehicleProfile *profile) {
    if (!profile)
        return 0;
    if (profile->engine == VEHICLE_ENGINE_PETROL20)
        return 4;
    if (profile->engine == VEHICLE_ENGINE_PETROL29)
        return 6;
    if (profile->engine == VEHICLE_ENGINE_DIESEL22)
        return 4;
    return 0;
}

/* Unknown equipment never authorizes an equipment-dependent actuation. */
static VehicleSupport equipment_support(const VehicleProfile *profile, unsigned slot) {
    if (!profile)
        return VEHICLE_SUPPORT_UNKNOWN;
    if (profile->equipment[slot] == VEHICLE_EQUIPMENT_PRESENT)
        return VEHICLE_SUPPORT_SUPPORTED;
    if (profile->equipment[slot] == VEHICLE_EQUIPMENT_ABSENT)
        return VEHICLE_SUPPORT_UNSUPPORTED;
    return VEHICLE_SUPPORT_UNKNOWN;
}

/* Confirmed-drivetrain gate: AWD-disable requires an explicitly accepted Q4 car. */
static VehicleSupport drivetrain_q4_support(const VehicleProfile *profile) {
    if (!profile || !profile->confirmed) return VEHICLE_SUPPORT_UNKNOWN;
    if (profile->drivetrain == VEHICLE_DRIVETRAIN_RWD) return VEHICLE_SUPPORT_UNSUPPORTED;
    if (profile->drivetrain != VEHICLE_DRIVETRAIN_Q4) return VEHICLE_SUPPORT_UNKNOWN;
    return VEHICLE_SUPPORT_SUPPORTED;
}

VehicleSupport vehicle_feature_support_for(const VehicleProfile *profile, VehicleFeature feature) {
    if (!profile)
        return VEHICLE_SUPPORT_UNKNOWN;
    switch (feature) {
    case VEHICLE_FEATURE_AWD_DISABLE:
        return drivetrain_q4_support(profile);
    case VEHICLE_FEATURE_SOFT_SUSPENSION:
        return equipment_support(profile, VEHICLE_EQUIPMENT_SUSPENSION);
    case VEHICLE_FEATURE_DEFAULT_RACE:
        if (profile->equipment[VEHICLE_EQUIPMENT_RACE] == VEHICLE_EQUIPMENT_PRESENT)
            return VEHICLE_SUPPORT_SUPPORTED;
        if (profile->equipment[VEHICLE_EQUIPMENT_RACE] == VEHICLE_EQUIPMENT_ABSENT)
            return VEHICLE_SUPPORT_UNSUPPORTED;
        return VEHICLE_SUPPORT_UNKNOWN;
    case VEHICLE_FEATURE_EXHAUST_ACTION:
        return equipment_support(profile, VEHICLE_EQUIPMENT_EXHAUST_VALVES);
    case VEHICLE_FEATURE_VIRTUAL_ACC:
        return equipment_support(profile, VEHICLE_EQUIPMENT_ACC);
    case VEHICLE_FEATURE_VIRTUAL_HAS:
        return equipment_support(profile, VEHICLE_EQUIPMENT_HAS);
    case VEHICLE_FEATURE_LANE_ASSIST:
        return equipment_support(profile, VEHICLE_EQUIPMENT_LANE_ASSIST);
    case VEHICLE_FEATURE_PARK_MIRROR:
        return equipment_support(profile, VEHICLE_EQUIPMENT_PARK_MIRROR);
    case VEHICLE_FEATURE_DPF_REGEN:
        if (profile->engine == VEHICLE_ENGINE_DIESEL22)
            return VEHICLE_SUPPORT_SUPPORTED;
        if (profile->engine == VEHICLE_ENGINE_UNKNOWN)
            return VEHICLE_SUPPORT_UNKNOWN;
        return VEHICLE_SUPPORT_UNSUPPORTED;
    case VEHICLE_FEATURE_ADBLUE:
        if (profile->engine == VEHICLE_ENGINE_DIESEL22) {
            if (profile->equipment[VEHICLE_EQUIPMENT_SCR] == VEHICLE_EQUIPMENT_PRESENT)
                return VEHICLE_SUPPORT_SUPPORTED;
            if (profile->equipment[VEHICLE_EQUIPMENT_SCR] == VEHICLE_EQUIPMENT_ABSENT)
                return VEHICLE_SUPPORT_UNSUPPORTED;
        }
        return VEHICLE_SUPPORT_UNKNOWN;
    default:
        return VEHICLE_SUPPORT_UNKNOWN;
    }
}

VehicleSupport vehicle_feature_support(VehicleFeature feature) {
    return vehicle_feature_support_for(vehicle_profile_current(), feature);
}

/*
 * Source-applicability policy for the 98-entry parameter catalog. Engine classes
 * that own a source return Supported; explicitly wrong classes return
 * Unsupported; anything else stays Unknown so callers show a configuration reason.
 */
VehicleSupport vehicle_parameter_support_for(const VehicleProfile *profile, uint8_t parameter_id) {
    if (!profile)
        return VEHICLE_SUPPORT_UNKNOWN;
    if (parameter_id >= 98)
        return VEHICLE_SUPPORT_UNSUPPORTED;

    /* Diesel-only source indices (shared diesel coolant/voltage/quality/level are re-mapped
     * per engine by the reading catalog; these are the diesel-only UDS sources). */
    static const uint8_t diesel_only[] = {55, 56, 57, 58, 59, 60, 61, 65, 66, 73, 74,
                                          76, 77, 78, 79, 80, 81, 82, 83, 84, 87, 88};
    /* Petrol29-only: fifth and sixth ignition cylinder corrections. */
    static const uint8_t v6_only[] = {19, 20};
    /* Petrol20-only: MultiAir module oil temperature. */
    static const uint8_t petrol20_only[] = {32};

    for (unsigned i = 0; i < sizeof(diesel_only); ++i)
        if (parameter_id == diesel_only[i])
            return vehicle_profile_is_diesel(profile) ? VEHICLE_SUPPORT_SUPPORTED
                                                       : VEHICLE_SUPPORT_UNSUPPORTED;
    for (unsigned i = 0; i < sizeof(v6_only); ++i)
        if (parameter_id == v6_only[i])
            return vehicle_profile_is_v6(profile) ? VEHICLE_SUPPORT_SUPPORTED
                                                   : VEHICLE_SUPPORT_UNSUPPORTED;
    for (unsigned i = 0; i < sizeof(petrol20_only); ++i)
        if (parameter_id == petrol20_only[i])
            return profile->engine == VEHICLE_ENGINE_PETROL20 ? VEHICLE_SUPPORT_SUPPORTED
                                                               : VEHICLE_SUPPORT_UNSUPPORTED;

    /* Everything else is shared or native telemetry and remains usable. */
    return VEHICLE_SUPPORT_SUPPORTED;
}

VehicleSupport vehicle_parameter_support(uint8_t parameter_id) {
    return vehicle_parameter_support_for(vehicle_profile_current(), parameter_id);
}

DisplayProfile vehicle_display_profile_for(const VehicleProfile *profile) {
    DisplayProfile result = {.mode = VEHICLE_DISPLAY_LEGACY18, .rows = 1, .legacy_width = 18};
    result.widths[0] = 18;
    result.widths[1] = 0;

    if (!profile)
        return result;

    uint8_t selection = profile->display_selection;
    if (selection == VEHICLE_DISPLAY_AUTO) {
        /* A known year >= 2023 selects MY23; otherwise stay conservative. */
        if (profile->model_year >= 2023)
            selection = VEHICLE_DISPLAY_MY23;
        else
            selection = VEHICLE_DISPLAY_LEGACY18;
    }

    if (selection == VEHICLE_DISPLAY_MY23) {
        result.mode = VEHICLE_DISPLAY_MY23;
        result.rows = 2;
        result.widths[0] = 16;
        result.widths[1] = 22;
        result.legacy_width = 18;
    } else if (selection == VEHICLE_DISPLAY_LEGACY24) {
        result.mode = VEHICLE_DISPLAY_LEGACY24;
        result.rows = 1;
        result.widths[0] = 24;
        result.widths[1] = 0;
        result.legacy_width = 24;
    } else {
        result.mode = VEHICLE_DISPLAY_LEGACY18;
        result.rows = 1;
        result.widths[0] = 18;
        result.widths[1] = 0;
        result.legacy_width = 18;
    }
    return result;
}

DisplayProfile vehicle_display_profile(void) { return vehicle_display_profile_for(vehicle_profile_current()); }

/*
 * Owner-supplied oil-level ranges. Unknown/unverified model or engine combinations
 * return an unknown range while retaining a millimetre value. Never copy Giulia
 * limits to another model.
 */
bool vehicle_oil_range_for(const VehicleProfile *profile, OilRange *range) {
    if (!range)
        return false;
    range->known = false;
    range->minimum_mm = 0;
    range->maximum_mm = 0;

    if (!profile || profile->model != VEHICLE_MODEL_GIULIA)
        return false;

    if (profile->engine == VEHICLE_ENGINE_PETROL20) {
        if (profile->drivetrain == VEHICLE_DRIVETRAIN_RWD) {
            range->minimum_mm = 72; range->maximum_mm = 82; range->known = true;
            return true;
        }
        if (profile->drivetrain == VEHICLE_DRIVETRAIN_Q4) {
            range->minimum_mm = 67; range->maximum_mm = 77; range->known = true;
            return true;
        }
    } else if (profile->engine == VEHICLE_ENGINE_DIESEL22) {
        if (profile->drivetrain == VEHICLE_DRIVETRAIN_RWD) {
            range->minimum_mm = 61; range->maximum_mm = 65; range->known = true;
            return true;
        }
        if (profile->drivetrain == VEHICLE_DRIVETRAIN_Q4) {
            range->minimum_mm = 79; range->maximum_mm = 84; range->known = true;
            return true;
        }
    }
    return false;
}

bool vehicle_oil_range(OilRange *range) { return vehicle_oil_range_for(vehicle_profile_current(), range); }

/*
 * Profile schema v2. Fixed 17 bytes, explicit per-field encoding so the
 * layout never depends on struct padding or compiler ordering.
 */
bool vehicle_profile_encode(const VehicleProfile *profile, uint8_t out[VEHICLE_PROFILE_WIRE_SIZE]) {
    if (!profile || !out || !vehicle_profile_validate(profile, NULL))
        return false;
    for (unsigned i = 0; i < VEHICLE_PROFILE_WIRE_SIZE; ++i)
        out[i] = 0;
    out[0] = 2; /* schema version */
    out[16] = profile->legacy_permissions ? 1 : 0;
    out[1] = (uint8_t)profile->model;
    out[2] = (uint8_t)profile->engine;
    out[3] = (uint8_t)(profile->model_year & 0xff);
    out[4] = (uint8_t)(profile->model_year >> 8);
    out[5] = (uint8_t)profile->drivetrain;
    out[6] = (uint8_t)profile->display_selection;
    out[7] = profile->confirmed ? 1U : 0U;
    /* One byte each for eight equipment slots, stored in slot order. */
    for (unsigned i = 0; i < VEHICLE_EQUIPMENT_COUNT; ++i)
        out[8 + i] = (uint8_t)profile->equipment[i];
    return true;
}

bool vehicle_profile_decode(const uint8_t data[VEHICLE_PROFILE_WIRE_SIZE], VehicleProfile *profile) {
    if (!data || !profile)
        return false;
    if (data[0] != 1 && data[0] != 2)
        return false;

    VehicleProfile draft;
    vehicle_profile_default(&draft);
    draft.model = (VehicleModel)data[1];
    draft.engine = (VehicleEngine)data[2];
    draft.model_year = (uint16_t)data[3] | (uint16_t)data[4] << 8;
    draft.drivetrain = (VehicleDrivetrain)data[5];
    draft.display_selection = (VehicleDisplaySelection)data[6];
    if (data[7] > 1 || (data[0] == 2 && data[16] > 1)) return false;
    draft.confirmed = data[7] == 1U;
    draft.legacy_permissions = data[0] == 1 || data[16] == 1;
    for (unsigned i = 0; i < VEHICLE_EQUIPMENT_COUNT; ++i)
        draft.equipment[i] = (VehicleEquipment)data[8 + i];

    if (!vehicle_profile_validate(&draft, NULL))
        return false;
    *profile = draft;
    return true;
}

/*
 * Apply legacy feature flags to a profile without overwriting the facts the legacy
 * slots cannot express (model, year, equipment, confirmation).
 */
void vehicle_profile_apply_legacy_flags(VehicleProfile *profile, bool diesel, bool v6, uint8_t drivetrain,
                                        bool my23, unsigned legacy_width) {
    if (!profile)
        return;
    if (diesel)
        profile->engine = VEHICLE_ENGINE_DIESEL22;
    else if (v6)
        profile->engine = VEHICLE_ENGINE_PETROL29;
    else
        profile->engine = VEHICLE_ENGINE_PETROL20;
    if (drivetrain == 1)
        profile->drivetrain = VEHICLE_DRIVETRAIN_RWD;
    else if (drivetrain == 2)
        profile->drivetrain = VEHICLE_DRIVETRAIN_Q4;
    else
        profile->drivetrain = VEHICLE_DRIVETRAIN_UNKNOWN;
    profile->display_selection = my23 ? VEHICLE_DISPLAY_MY23
                                      : legacy_width >= 24 ? VEHICLE_DISPLAY_LEGACY24
                                                           : VEHICLE_DISPLAY_LEGACY18;
}

/* Derive legacy feature flags from the canonical profile. */
void vehicle_profile_legacy_flags(const VehicleProfile *profile, bool *diesel, bool *v6, uint8_t *drivetrain,
                                  bool *my23) {
    if (diesel) *diesel = vehicle_profile_is_diesel(profile);
    if (v6) *v6 = vehicle_profile_is_v6(profile);
    if (drivetrain) *drivetrain = profile && profile->drivetrain == VEHICLE_DRIVETRAIN_Q4 ? 2
                                   : profile && profile->drivetrain == VEHICLE_DRIVETRAIN_RWD ? 1 : 0;
    if (my23) *my23 = vehicle_display_profile_for(profile).mode == VEHICLE_DISPLAY_MY23;
}

/*
 * Legacy 0x101 import. Slots are 1-based (slot N lives at slots[N-1]).
 * Diesel (slot 16) takes precedence over the V6 companion (slot 36). Display
 * slot 6 only proves MY23 vs. legacy; model/year/equipment stay Unknown and the
 * profile is unconfirmed until the owner accepts it.
 */
void vehicle_profile_import_legacy(const uint16_t *slots, uint16_t slot_count, VehicleProfile *profile) {
    vehicle_profile_default(profile);
    if (!slots)
        return;

    uint16_t diesel = slot_count >= 16 ? slots[15] : 0;
    uint16_t v6 = slot_count >= 36 ? slots[35] : 0;
    if (diesel == 1)
        profile->engine = VEHICLE_ENGINE_DIESEL22;
    else if (v6 == 1)
        profile->engine = VEHICLE_ENGINE_PETROL29;
    else
        profile->engine = VEHICLE_ENGINE_PETROL20;

    if (slot_count >= 40) {
        if (slots[39] == 1)
            profile->drivetrain = VEHICLE_DRIVETRAIN_RWD;
        else if (slots[39] == 2)
            profile->drivetrain = VEHICLE_DRIVETRAIN_Q4;
    }

    if (slot_count >= 6) {
        if (slots[5] == 1)
            profile->display_selection = VEHICLE_DISPLAY_MY23;
        else
            profile->display_selection = VEHICLE_DISPLAY_LEGACY18;
    }
    profile->confirmed = false;
    profile->legacy_permissions = false;
}

bool vehicle_feature_allowed_for(const VehicleProfile *profile, VehicleFeature feature) {
    if (!profile) return false;
    if (!profile->confirmed && profile->legacy_permissions) return true;
    return vehicle_feature_support_for(profile, feature) == VEHICLE_SUPPORT_SUPPORTED;
}
bool vehicle_feature_allowed(VehicleFeature feature) {
    return vehicle_feature_allowed_for(vehicle_profile_current(), feature);
}
