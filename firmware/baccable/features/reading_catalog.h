#ifndef BACCABLE_READING_CATALOG_H
#define BACCABLE_READING_CATALOG_H

#include "vehicle/vehicle_profile.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Individual-reading catalog. Maps a stable ReadingId to one UDS source per
 * engine class, a group and explicit presentation metadata. It replaces the old
 * compound-page model; ParameterDefinition/source bytes are unchanged.
 */

typedef enum {
    READING_GROUP_OIL = 0,
    READING_GROUP_TEMPERATURES,
    READING_GROUP_ENGINE,
    READING_GROUP_FUEL,
    READING_GROUP_BATTERY,
    READING_GROUP_DIESEL_DPF,
    READING_GROUP_DRIVING,
    READING_GROUP_PERFORMANCE,
    READING_GROUP_DIAGNOSTICS,
    READING_GROUP_COUNT,
} ReadingGroup;

typedef enum {
    READING_OIL_TEMPERATURE = 0,
    READING_OIL_PRESSURE,
    READING_OIL_LEVEL_MM,
    READING_OIL_LEVEL_STATUS,
    READING_OIL_LEVEL_BAR,
    READING_OIL_VOLUME,
    READING_OIL_QUALITY,

    READING_COOLANT_TEMP,
    READING_GEARBOX_TEMP,
    READING_INTERCOOLER_IN,
    READING_INTERCOOLER_OUT,
    READING_INTAKE_AIR_TEMP,
    READING_EXHAUST_TEMP,
    READING_FUEL_TEMP,
    READING_CATALYST_TEMP,

    READING_POWER,
    READING_TORQUE,
    READING_BOOST,
    READING_INTAKE_PRESSURE,
    READING_IGNITION_CYL_1,
    READING_IGNITION_CYL_2,
    READING_IGNITION_CYL_3,
    READING_IGNITION_CYL_4,
    READING_IGNITION_CYL_5,
    READING_IGNITION_CYL_6,
    READING_MISFIRES_TOTAL,
    READING_EGR_TARGET,
    READING_EGR_ACTUAL,
    READING_TURBO_TEMP,

    READING_FUEL_RATE,
    READING_FUEL_PRESSURE,

    READING_BATTERY_VOLTAGE,
    READING_BATTERY_CURRENT,
    READING_BATTERY_SOC_IBS,
    READING_BATTERY_SOC_ECU,
    READING_BATTERY_SOC_BCM,
    READING_IBS_RAW0,
    READING_IBS_RAW1,

    READING_DPF_LOAD,
    READING_DPF_TEMP,
    READING_DPF_REGEN_PROGRESS,
    READING_DPF_REGEN_MODE,
    READING_DPF_SINCE_REGEN,
    READING_DPF_REGEN_COUNT,
    READING_DPF_AVG_INTERVAL,
    READING_DPF_AVG_DURATION,
    READING_ADBLUE_VOLUME,
    READING_ADBLUE_LEVEL,

    READING_SPEED,
    READING_GEAR,
    READING_DNA_MODE,
    READING_PEDAL_MAP,
    READING_SEATBELT,
    READING_DISTANCE,

    READING_TIME_0_100,
    READING_TIME_100_200,
    READING_BEST_0_100,
    READING_BEST_100_200,

    READING_OIL_TEMP_ECU,
    READING_OIL_PRESSURE_ECU,
    READING_MULTIAIR_TEMP,
    READING_KNOCK_SENSOR,
    READING_KEY_ID,
    READING_ENGINE_RUN_TIME,
    READING_OVER_REV_TIME,
    READING_OVER_REV_COUNT,
    READING_AC_PRESSURE,
    READING_TURBO_SENSOR_V,
    READING_INTAKE_SENSOR_V,
    READING_TURBO_TARGET_BAR,
    READING_TURBO_TARGET_PCT,
    READING_TURBO_ACTUAL_PCT,
    READING_BOOST_TARGET,

    READING_COUNT,
} ReadingId;

typedef enum { READING_FORMAT_AUTO = 0, READING_FORMAT_FULL, READING_FORMAT_MEDIUM, READING_FORMAT_SHORT } ReadingFormat;

typedef enum { READING_VALUE_FLOAT = 0, READING_VALUE_ENUM, READING_VALUE_OIL_STATUS } ReadingValueType;

enum {
    READING_ENUM_GEAR = 0,
    READING_ENUM_DNA,
    READING_ENUM_PEDAL_MAP,
    READING_ENUM_SEATBELT,
    READING_ENUM_DPF_REGEN,
    READING_ENUM_OIL_STATUS,
};

enum {
    READING_ENGINE_PETROL20 = 1u << 0,
    READING_ENGINE_PETROL29 = 1u << 1,
    READING_ENGINE_DIESEL22 = 1u << 2,
};

#define READING_NO_SOURCE 0xffU

typedef struct {
    ReadingId id;
    ReadingGroup group;
    const char *label;
    const char *medium;
    const char *short_label;
    const char *unit;
    uint8_t precision;
    uint8_t value_width;   /* Reserved worst-case numeric/status glyph width. */
    uint8_t value_type;
    uint8_t enum_set;
    uint8_t source_gasoline;
    uint8_t source_diesel;
    uint8_t engine_mask;
    int8_t equipment;      /* -1 = none, else VEHICLE_EQUIPMENT_* slot. */
} ReadingMeta;

const ReadingMeta *reading_lookup(ReadingId id);
unsigned reading_count(void);
const char *reading_group_name(ReadingGroup group);
uint8_t reading_source(const ReadingMeta *meta, const VehicleProfile *profile);
bool reading_engine_eligible(const ReadingMeta *meta, const VehicleProfile *profile);
VehicleSupport vehicle_reading_support_for(const VehicleProfile *profile, ReadingId id);
VehicleSupport vehicle_reading_support(ReadingId id);
unsigned reading_list(const VehicleProfile *profile, ReadingGroup group, ReadingId *out, unsigned capacity);
bool reading_enum_label(const ReadingMeta *meta, float value, char *out, size_t size);

#endif /* BACCABLE_READING_CATALOG_H */
