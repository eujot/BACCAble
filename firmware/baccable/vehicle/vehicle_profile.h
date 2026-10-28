#ifndef BACCABLE_VEHICLE_PROFILE_H
#define BACCABLE_VEHICLE_PROFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Authoritative vehicle profile (C1): what the configured car has. Only a
 * profile commit updates the live profile; feature preferences, live state,
 * build capabilities and UI layouts are separate owners (see the action plan).
 */

typedef enum {
    VEHICLE_MODEL_UNKNOWN = 0,
    VEHICLE_MODEL_GIULIA,
    VEHICLE_MODEL_STELVIO,
} VehicleModel;

typedef enum {
    VEHICLE_ENGINE_UNKNOWN = 0,
    VEHICLE_ENGINE_PETROL20,
    VEHICLE_ENGINE_PETROL29,
    VEHICLE_ENGINE_DIESEL22,
} VehicleEngine;

typedef enum {
    VEHICLE_DRIVETRAIN_UNKNOWN = 0,
    VEHICLE_DRIVETRAIN_RWD,
    VEHICLE_DRIVETRAIN_Q4,
} VehicleDrivetrain;

typedef enum {
    VEHICLE_DISPLAY_AUTO = 0,
    VEHICLE_DISPLAY_LEGACY18,
    VEHICLE_DISPLAY_LEGACY24,
    VEHICLE_DISPLAY_MY23,
} VehicleDisplaySelection;

/* Tri-state equipment: Unknown is a real state, never numeric zero as Absent. */
typedef enum {
    VEHICLE_EQUIPMENT_UNKNOWN = 0,
    VEHICLE_EQUIPMENT_ABSENT,
    VEHICLE_EQUIPMENT_PRESENT,
} VehicleEquipment;

enum {
    VEHICLE_EQUIPMENT_SUSPENSION = 0,
    VEHICLE_EQUIPMENT_RACE,
    VEHICLE_EQUIPMENT_EXHAUST_VALVES,
    VEHICLE_EQUIPMENT_ACC,
    VEHICLE_EQUIPMENT_HAS,
    VEHICLE_EQUIPMENT_LANE_ASSIST,
    VEHICLE_EQUIPMENT_PARK_MIRROR,
    VEHICLE_EQUIPMENT_SCR,
    VEHICLE_EQUIPMENT_COUNT,
};

typedef struct {
    VehicleModel model;
    VehicleEngine engine;
    uint16_t model_year; /* 0 = unknown, otherwise 2000..2099. */
    VehicleDrivetrain drivetrain;
    VehicleDisplaySelection display_selection;
    VehicleEquipment equipment[VEHICLE_EQUIPMENT_COUNT];
    bool confirmed; /* Distinguishes imported legacy behavior from an accepted profile. */
} VehicleProfile;

typedef enum {
    VEHICLE_SUPPORT_UNKNOWN = 0,
    VEHICLE_SUPPORT_UNSUPPORTED,
    VEHICLE_SUPPORT_SUPPORTED,
} VehicleSupport;

typedef enum {
    VEHICLE_FEATURE_AWD_DISABLE = 0,
    VEHICLE_FEATURE_SOFT_SUSPENSION,
    VEHICLE_FEATURE_DEFAULT_RACE,
    VEHICLE_FEATURE_EXHAUST_ACTION,
    VEHICLE_FEATURE_VIRTUAL_ACC,
    VEHICLE_FEATURE_VIRTUAL_HAS,
    VEHICLE_FEATURE_LANE_ASSIST,
    VEHICLE_FEATURE_PARK_MIRROR,
    VEHICLE_FEATURE_DPF_REGEN,
    VEHICLE_FEATURE_ADBLUE,
    VEHICLE_FEATURE_COUNT,
} VehicleFeature;

typedef struct {
    uint8_t mode;        /* VehicleDisplaySelection, resolved for the current profile. */
    uint8_t rows;        /* Simultaneous visible rows (MY23: 2, legacy: 1). */
    uint8_t widths[2];   /* Per-row visible glyph budget. */
    uint8_t legacy_width; /* DASHBOARD_MESSAGE_MAX_LENGTH equivalent for legacy rows. */
} DisplayProfile;

typedef struct {
    uint8_t minimum_mm;
    uint8_t maximum_mm;
    bool known;
} OilRange;

typedef enum {
    VEHICLE_PROFILE_OK = 0,
    VEHICLE_PROFILE_ERR_MODEL,
    VEHICLE_PROFILE_ERR_ENGINE,
    VEHICLE_PROFILE_ERR_YEAR,
    VEHICLE_PROFILE_ERR_DRIVETRAIN,
    VEHICLE_PROFILE_ERR_DISPLAY,
    VEHICLE_PROFILE_ERR_EQUIPMENT,
} VehicleProfileError;

void vehicle_profile_default(VehicleProfile *profile);
bool vehicle_profile_validate(const VehicleProfile *draft, VehicleProfileError *error);

const VehicleProfile *vehicle_profile_current(void);
uint16_t vehicle_profile_generation(void);
bool vehicle_profile_commit(const VehicleProfile *profile);

bool vehicle_profile_is_gasoline(const VehicleProfile *profile);
bool vehicle_profile_is_diesel(const VehicleProfile *profile);
bool vehicle_profile_is_v6(const VehicleProfile *profile);
uint8_t vehicle_profile_cylinders(const VehicleProfile *profile);

VehicleSupport vehicle_feature_support_for(const VehicleProfile *profile, VehicleFeature feature);
VehicleSupport vehicle_feature_support(VehicleFeature feature);
VehicleSupport vehicle_parameter_support_for(const VehicleProfile *profile, uint8_t parameter_id);
VehicleSupport vehicle_parameter_support(uint8_t parameter_id);
DisplayProfile vehicle_display_profile_for(const VehicleProfile *profile);
DisplayProfile vehicle_display_profile(void);
bool vehicle_oil_range_for(const VehicleProfile *profile, OilRange *range);
bool vehicle_oil_range(OilRange *range);

/* Transition adapter: apply legacy engine/drivetrain/display flags to a profile,
 * preserving model/year/equipment/confirmed. legacy_width is DASHBOARD_MESSAGE_MAX_LENGTH. */
void vehicle_profile_apply_legacy_flags(VehicleProfile *profile, bool diesel, bool v6, uint8_t drivetrain,
                                        bool my23, unsigned legacy_width);

/* Serialization: new settings schema 0x107 (explicit, length-checked). */
enum { VEHICLE_PROFILE_WIRE_SIZE = 16U };
bool vehicle_profile_encode(const VehicleProfile *profile, uint8_t out[VEHICLE_PROFILE_WIRE_SIZE]);
bool vehicle_profile_decode(const uint8_t data[VEHICLE_PROFILE_WIRE_SIZE], VehicleProfile *profile);

/*
 * Legacy import: decode the old 0x101 uint16 slots without inferring equipment.
 * engine slot 16 + v6 companion 36 (diesel wins), drivetrain 40, display 6.
 */
void vehicle_profile_import_legacy(const uint16_t *slots, uint16_t slot_count, VehicleProfile *profile);

#endif /* BACCABLE_VEHICLE_PROFILE_H */
