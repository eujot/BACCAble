#include "features/reading_catalog.h"
#include "diagnostics/parameter_catalog.h"
#include <stdio.h>
#include <string.h>

/*
 * Explicit reading metadata. Do not derive labels/units/precision from the old
 * "$..." page templates; those are intentionally retired. Source indices point
 * into the unchanged ParameterDefinition table (0..97).
 */

#define M_ALL (READING_ENGINE_PETROL20 | READING_ENGINE_PETROL29 | READING_ENGINE_DIESEL22)
#define M_GAS (READING_ENGINE_PETROL20 | READING_ENGINE_PETROL29)
#define M_P20 READING_ENGINE_PETROL20
#define M_P29 READING_ENGINE_PETROL29
#define M_D   READING_ENGINE_DIESEL22

#define R(id, grp, label, med, shrt, unit, prec, vw, vt, es, gs, ds, mask) \
    {id, grp, label, med, shrt, unit, prec, vw, vt, es, gs, ds, mask, -1}
#define RE(id, grp, label, med, shrt, unit, prec, vw, vt, es, gs, ds, mask, equip) \
    {id, grp, label, med, shrt, unit, prec, vw, vt, es, gs, ds, mask, equip}

static const ReadingMeta reading_meta[READING_COUNT] = {
    /* Oil */
    R(READING_OIL_TEMPERATURE, READING_GROUP_OIL, "Oil temp", "Oil T", "Oil", "C", 0, 4, READING_VALUE_FLOAT, 0, 5, 5, M_ALL),
    R(READING_OIL_PRESSURE, READING_GROUP_OIL, "Oil pressure", "Oil P", "OilP", "bar", 1, 5, READING_VALUE_FLOAT, 0, 0, 0, M_ALL),
    R(READING_OIL_LEVEL_MM, READING_GROUP_OIL, "Oil level", "Oil mm", "Oil", "mm", 0, 3, READING_VALUE_FLOAT, 0, 89, 64, M_ALL),
    R(READING_OIL_LEVEL_STATUS, READING_GROUP_OIL, "Oil status", "Oil st", "Oil", NULL, 0, 4, READING_VALUE_OIL_STATUS, READING_ENUM_OIL_STATUS, 89, 64, M_ALL),
    R(READING_OIL_LEVEL_BAR, READING_GROUP_OIL, "Oil bar", NULL, NULL, NULL, 0, 0, READING_VALUE_OIL_STATUS, READING_ENUM_OIL_STATUS, 89, 64, M_ALL),
    R(READING_OIL_VOLUME, READING_GROUP_OIL, "Oil volume", "Oil vol", "OilL", "L", 2, 5, READING_VALUE_FLOAT, 0, 28, 28, M_ALL),
    R(READING_OIL_QUALITY, READING_GROUP_OIL, "Oil quality", "Oil q", "Oil%", "%", 0, 4, READING_VALUE_FLOAT, 0, 31, 63, M_ALL),

    /* Temperatures */
    R(READING_COOLANT_TEMP, READING_GROUP_TEMPERATURES, "Coolant", "Cool", "Cool", "C", 0, 4, READING_VALUE_FLOAT, 0, 42, 68, M_ALL),
    R(READING_GEARBOX_TEMP, READING_GROUP_TEMPERATURES, "Gearbox", "Gear", "Gbx", "C", 0, 4, READING_VALUE_FLOAT, 0, 33, 33, M_ALL),
    R(READING_INTERCOOLER_IN, READING_GROUP_TEMPERATURES, "IC inlet", "IC in", "ICi", "C", 0, 4, READING_VALUE_FLOAT, 0, 23, READING_NO_SOURCE, M_GAS),
    R(READING_INTERCOOLER_OUT, READING_GROUP_TEMPERATURES, "IC outlet", "IC out", "ICo", "C", 0, 4, READING_VALUE_FLOAT, 0, 22, READING_NO_SOURCE, M_GAS),
    R(READING_INTAKE_AIR_TEMP, READING_GROUP_TEMPERATURES, "Intake air", "Intake", "Air", "C", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 88, M_D),
    R(READING_EXHAUST_TEMP, READING_GROUP_TEMPERATURES, "Exhaust", "Exh", "Exh", "C", 0, 5, READING_VALUE_FLOAT, 0, 40, 67, M_ALL),
    R(READING_FUEL_TEMP, READING_GROUP_TEMPERATURES, "Fuel temp", "Fuel", "Fuel", "C", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 84, M_D),
    R(READING_CATALYST_TEMP, READING_GROUP_TEMPERATURES, "Catalyst", "Cat", "Cat", "C", 0, 5, READING_VALUE_FLOAT, 0, 41, READING_NO_SOURCE, M_GAS),

    /* Engine */
    R(READING_POWER, READING_GROUP_ENGINE, "Power", "Pwr", "P", "PS", 0, 5, READING_VALUE_FLOAT, 0, 1, 1, M_ALL),
    R(READING_TORQUE, READING_GROUP_ENGINE, "Torque", "Torq", "Tq", "Nm", 0, 5, READING_VALUE_FLOAT, 0, 2, 2, M_ALL),
    R(READING_BOOST, READING_GROUP_ENGINE, "Boost", "Bst", "B", "bar", 2, 6, READING_VALUE_FLOAT, 0, 25, 79, M_ALL),
    R(READING_INTAKE_PRESSURE, READING_GROUP_ENGINE, "Intake", NULL, "Int", "bar", 2, 6, READING_VALUE_FLOAT, 0, 24, READING_NO_SOURCE, M_GAS),
    R(READING_IGNITION_CYL_1, READING_GROUP_ENGINE, "Ign cyl 1", "Ign 1", "I1", "deg", 1, 5, READING_VALUE_FLOAT, 0, 45, 45, M_ALL),
    R(READING_IGNITION_CYL_2, READING_GROUP_ENGINE, "Ign cyl 2", "Ign 2", "I2", "deg", 1, 5, READING_VALUE_FLOAT, 0, 46, 46, M_ALL),
    R(READING_IGNITION_CYL_3, READING_GROUP_ENGINE, "Ign cyl 3", "Ign 3", "I3", "deg", 1, 5, READING_VALUE_FLOAT, 0, 47, 47, M_ALL),
    R(READING_IGNITION_CYL_4, READING_GROUP_ENGINE, "Ign cyl 4", "Ign 4", "I4", "deg", 1, 5, READING_VALUE_FLOAT, 0, 48, 48, M_ALL),
    R(READING_IGNITION_CYL_5, READING_GROUP_ENGINE, "Ign cyl 5", "Ign 5", "I5", "deg", 1, 5, READING_VALUE_FLOAT, 0, 19, READING_NO_SOURCE, M_P29),
    R(READING_IGNITION_CYL_6, READING_GROUP_ENGINE, "Ign cyl 6", "Ign 6", "I6", "deg", 1, 5, READING_VALUE_FLOAT, 0, 20, READING_NO_SOURCE, M_P29),
    R(READING_MISFIRES_TOTAL, READING_GROUP_ENGINE, "Misfires", "Misfire", "Misf", NULL, 0, 5, READING_VALUE_FLOAT, 0, 90, 90, M_ALL),
    R(READING_EGR_TARGET, READING_GROUP_ENGINE, "EGR target", "EGR t", "EGRt", "%", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 73, M_D),
    R(READING_EGR_ACTUAL, READING_GROUP_ENGINE, "EGR actual", "EGR a", "EGRa", "%", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 74, M_D),
    R(READING_TURBO_TEMP, READING_GROUP_ENGINE, "Turbo temp", "Turbo", "Tbo", "C", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 78, M_D),

    /* Fuel */
    R(READING_FUEL_RATE, READING_GROUP_FUEL, "Fuel rate", "Fuel", "Fuel", "L/h", 2, 6, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 87, M_D),
    R(READING_FUEL_PRESSURE, READING_GROUP_FUEL, "Fuel rail", "Rail", "Rail", "bar", 0, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 83, M_D),

    /* Battery */
    R(READING_BATTERY_VOLTAGE, READING_GROUP_BATTERY, "Battery", "Volt", "V", "V", 2, 5, READING_VALUE_FLOAT, 0, 35, 62, M_ALL),
    R(READING_BATTERY_CURRENT, READING_GROUP_BATTERY, "Battery A", "Current", "A", "A", 1, 6, READING_VALUE_FLOAT, 0, 4, 4, M_ALL),
    R(READING_BATTERY_SOC_IBS, READING_GROUP_BATTERY, "Batt IBS", "IBS", "IBS", "%", 0, 4, READING_VALUE_FLOAT, 0, 3, 3, M_ALL),
    R(READING_BATTERY_SOC_ECU, READING_GROUP_BATTERY, "Batt ECU", "ECU", "ECU", "%", 0, 4, READING_VALUE_FLOAT, 0, 21, 21, M_ALL),
    R(READING_BATTERY_SOC_BCM, READING_GROUP_BATTERY, "Batt BCM", "BCM", "BCM", "%", 0, 4, READING_VALUE_FLOAT, 0, 34, 34, M_ALL),
    R(READING_IBS_RAW0, READING_GROUP_BATTERY, "IBS raw 0", "IBS B0", "B0", NULL, 0, 4, READING_VALUE_FLOAT, 0, 95, 95, M_ALL),
    R(READING_IBS_RAW1, READING_GROUP_BATTERY, "IBS raw 1", "IBS B1", "B1", NULL, 0, 4, READING_VALUE_FLOAT, 0, 96, 96, M_ALL),

    /* Diesel / DPF */
    R(READING_DPF_LOAD, READING_GROUP_DIESEL_DPF, "DPF load", NULL, "DPF", "%", 1, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 55, M_D),
    R(READING_DPF_TEMP, READING_GROUP_DIESEL_DPF, "DPF temp", NULL, "DPFt", "C", 0, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 56, M_D),
    R(READING_DPF_REGEN_PROGRESS, READING_GROUP_DIESEL_DPF, "Regen", "Regen", "Reg", "%", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 57, M_D),
    R(READING_DPF_REGEN_MODE, READING_GROUP_DIESEL_DPF, "Regen mode", "Regen", "Reg", NULL, 0, 8, READING_VALUE_ENUM, READING_ENUM_DPF_REGEN, READING_NO_SOURCE, 8, M_D),
    R(READING_DPF_SINCE_REGEN, READING_GROUP_DIESEL_DPF, "Regen ago", "Since", "Ago", "km", 0, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 58, M_D),
    R(READING_DPF_REGEN_COUNT, READING_GROUP_DIESEL_DPF, "Regen count", "Count", "Cnt", NULL, 0, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 59, M_D),
    R(READING_DPF_AVG_INTERVAL, READING_GROUP_DIESEL_DPF, "Regen avg km", "Avg km", "Avg", "km", 0, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 60, M_D),
    R(READING_DPF_AVG_DURATION, READING_GROUP_DIESEL_DPF, "Regen avg min", "Avg min", "Avg", "min", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 61, M_D),
    RE(READING_ADBLUE_VOLUME, READING_GROUP_DIESEL_DPF, "AdBlue", "AdBlue", "AdB", "L", 2, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 65, M_D, VEHICLE_EQUIPMENT_SCR),
    RE(READING_ADBLUE_LEVEL, READING_GROUP_DIESEL_DPF, "AdBlue %", "AdBlue", "AdB", "%", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 66, M_D, VEHICLE_EQUIPMENT_SCR),

    /* Driving */
    R(READING_SPEED, READING_GROUP_DRIVING, "Speed", NULL, NULL, "km/h", 0, 5, READING_VALUE_FLOAT, 0, 7, 7, M_ALL),
    R(READING_GEAR, READING_GROUP_DRIVING, "Gear", NULL, NULL, NULL, 0, 2, READING_VALUE_ENUM, READING_ENUM_GEAR, 6, 6, M_ALL),
    R(READING_DNA_MODE, READING_GROUP_DRIVING, "DNA mode", "DNA", "DNA", NULL, 0, 5, READING_VALUE_ENUM, READING_ENUM_DNA, 15, 15, M_ALL),
    R(READING_PEDAL_MAP, READING_GROUP_DRIVING, "Pedal map", "Pedal", "Ped", NULL, 0, 6, READING_VALUE_ENUM, READING_ENUM_PEDAL_MAP, 17, 17, M_ALL),
    R(READING_SEATBELT, READING_GROUP_DRIVING, "Seatbelt", "Belt", "Belt", NULL, 0, 8, READING_VALUE_ENUM, READING_ENUM_SEATBELT, 13, 13, M_ALL),
    R(READING_DISTANCE, READING_GROUP_DRIVING, "Distance", "Dist", "Dist", "km", 0, 6, READING_VALUE_FLOAT, 0, 27, 85, M_ALL),

    /* Performance */
    R(READING_TIME_0_100, READING_GROUP_PERFORMANCE, "0-100", "0-100", "0100", "s", 2, 5, READING_VALUE_FLOAT, 0, 9, 9, M_ALL),
    R(READING_TIME_100_200, READING_GROUP_PERFORMANCE, "100-200", "100-200", "1020", "s", 2, 5, READING_VALUE_FLOAT, 0, 10, 10, M_ALL),
    R(READING_BEST_0_100, READING_GROUP_PERFORMANCE, "Best 0-100", "Best0100", "B0100", "s", 2, 5, READING_VALUE_FLOAT, 0, 11, 11, M_ALL),
    R(READING_BEST_100_200, READING_GROUP_PERFORMANCE, "Best100-200", "Best1020", "B1020", "s", 2, 5, READING_VALUE_FLOAT, 0, 12, 12, M_ALL),

    /* Diagnostics */
    R(READING_OIL_TEMP_ECU, READING_GROUP_DIAGNOSTICS, "Oil ECU temp", "Oil ECU", "OECU", "C", 0, 4, READING_VALUE_FLOAT, 0, 30, 30, M_ALL),
    R(READING_OIL_PRESSURE_ECU, READING_GROUP_DIAGNOSTICS, "Oil ECU press", "Oil P2", "OECUP", "bar", 2, 5, READING_VALUE_FLOAT, 0, 29, 29, M_ALL),
    R(READING_MULTIAIR_TEMP, READING_GROUP_DIAGNOSTICS, "MultiAir", "MultiAir", "MA", "C", 0, 4, READING_VALUE_FLOAT, 0, 32, READING_NO_SOURCE, M_P20),
    R(READING_KNOCK_SENSOR, READING_GROUP_DIAGNOSTICS, "Knock", "Knock", "Knk", "mV", 1, 6, READING_VALUE_FLOAT, 0, 43, READING_NO_SOURCE, M_GAS),
    R(READING_KEY_ID, READING_GROUP_DIAGNOSTICS, "Key ID", "Key", "Key", NULL, 0, 4, READING_VALUE_FLOAT, 0, 44, READING_NO_SOURCE, M_GAS),
    R(READING_ENGINE_RUN_TIME, READING_GROUP_DIAGNOSTICS, "Run time", "Run", "Run", "min", 0, 5, READING_VALUE_FLOAT, 0, 37, 37, M_ALL),
    R(READING_OVER_REV_TIME, READING_GROUP_DIAGNOSTICS, "Over-rev s", "Ovr s", "Ovr", "s", 0, 4, READING_VALUE_FLOAT, 0, 38, 38, M_ALL),
    R(READING_OVER_REV_COUNT, READING_GROUP_DIAGNOSTICS, "Over-rev n", "Ovr n", "Ovr", NULL, 0, 6, READING_VALUE_FLOAT, 0, 39, 39, M_ALL),
    R(READING_AC_PRESSURE, READING_GROUP_DIAGNOSTICS, "A/C pressure", "A/C", "AC", "bar", 2, 5, READING_VALUE_FLOAT, 0, 36, 86, M_ALL),
    R(READING_TURBO_SENSOR_V, READING_GROUP_DIAGNOSTICS, "Turbo V", "TurboV", "TboV", "V", 3, 5, READING_VALUE_FLOAT, 0, 26, READING_NO_SOURCE, M_GAS),
    R(READING_INTAKE_SENSOR_V, READING_GROUP_DIAGNOSTICS, "Intake V", "IntV", "IntV", "V", 3, 5, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 82, M_D),
    R(READING_TURBO_TARGET_BAR, READING_GROUP_DIAGNOSTICS, "Turbo req bar", "Tbo req", "TboR", "bar", 2, 6, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 76, M_D),
    R(READING_TURBO_TARGET_PCT, READING_GROUP_DIAGNOSTICS, "Turbo req %", "Tbo req", "TboR", "%", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 77, M_D),
    R(READING_TURBO_ACTUAL_PCT, READING_GROUP_DIAGNOSTICS, "Turbo %", "Turbo", "Tbo", "%", 0, 4, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 80, M_D),
    R(READING_BOOST_TARGET, READING_GROUP_DIAGNOSTICS, "Boost req", "Boost r", "BstR", "bar", 2, 6, READING_VALUE_FLOAT, 0, READING_NO_SOURCE, 81, M_D),
};

#undef M_ALL
#undef M_GAS
#undef M_P20
#undef M_P29
#undef M_D

/* Compile-time ordering guard: the table must stay indexed by ReadingId. */
_Static_assert(READING_COUNT == sizeof(reading_meta) / sizeof(reading_meta[0]),
               "reading_meta must contain exactly READING_COUNT entries");

const char *const reading_group_names[READING_GROUP_COUNT] = {
    "Oil", "Temperatures", "Engine", "Fuel", "Battery", "Diesel / DPF", "Driving", "Performance", "Diagnostics"};

const ReadingMeta *reading_lookup(ReadingId id) {
    if (id >= READING_COUNT)
        return NULL;
    const ReadingMeta *meta = &reading_meta[id];
    return meta->id == id ? meta : NULL;
}

unsigned reading_count(void) { return READING_COUNT; }
const char *reading_group_name(ReadingGroup group) {
    return group < READING_GROUP_COUNT ? reading_group_names[group] : NULL;
}

/* Resolve the UDS source for the current engine class, or NO_SOURCE. */
uint8_t reading_source(const ReadingMeta *meta, const VehicleProfile *profile) {
    if (!meta || !profile)
        return READING_NO_SOURCE;
    if (vehicle_profile_is_diesel(profile))
        return meta->source_diesel;
    if (vehicle_profile_is_gasoline(profile))
        return meta->source_gasoline;
    /* Unknown engine: prefer the gasoline source when both exist, else none. */
    if (meta->source_gasoline != READING_NO_SOURCE)
        return meta->source_gasoline;
    return meta->source_diesel;
}

bool reading_engine_eligible(const ReadingMeta *meta, const VehicleProfile *profile) {
    if (!meta || !profile)
        return false;
    uint8_t bit = 0;
    if (profile->engine == VEHICLE_ENGINE_PETROL20)
        bit = READING_ENGINE_PETROL20;
    else if (profile->engine == VEHICLE_ENGINE_PETROL29)
        bit = READING_ENGINE_PETROL29;
    else if (profile->engine == VEHICLE_ENGINE_DIESEL22)
        bit = READING_ENGINE_DIESEL22;
    else
        return true; /* Unknown engine: keep the item visible for later configuration. */
    if (!(meta->engine_mask & bit))
        return false;
    if (meta->equipment >= 0 && meta->equipment < VEHICLE_EQUIPMENT_COUNT &&
        profile->equipment[meta->equipment] == VEHICLE_EQUIPMENT_ABSENT)
        return false;
    return true;
}

VehicleSupport vehicle_reading_support_for(const VehicleProfile *profile, ReadingId id) {
    const ReadingMeta *meta = reading_lookup(id);
    if (!meta)
        return VEHICLE_SUPPORT_UNSUPPORTED;
    if (reading_source(meta, profile) == READING_NO_SOURCE)
        return VEHICLE_SUPPORT_UNSUPPORTED;
    if (!reading_engine_eligible(meta, profile))
        return VEHICLE_SUPPORT_UNSUPPORTED;
    if (meta->equipment >= 0 && meta->equipment < VEHICLE_EQUIPMENT_COUNT &&
        profile && profile->equipment[meta->equipment] == VEHICLE_EQUIPMENT_UNKNOWN)
        return VEHICLE_SUPPORT_UNKNOWN;
    return vehicle_parameter_support_for(profile, reading_source(meta, profile));
}

VehicleSupport vehicle_reading_support(ReadingId id) {
    return vehicle_reading_support_for(vehicle_profile_current(), id);
}

/* Enumerate eligible readings for a group in stable catalog order. */
unsigned reading_list(const VehicleProfile *profile, ReadingGroup group, ReadingId *out, unsigned capacity) {
    unsigned count = 0;
    for (ReadingId id = 0; id < READING_COUNT; ++id) {
        const ReadingMeta *meta = reading_lookup(id);
        if (!meta || meta->group != group)
            continue;
        if (vehicle_reading_support_for(profile, id) == VEHICLE_SUPPORT_UNSUPPORTED)
            continue;
        if (count < capacity)
            out[count] = id;
        ++count;
    }
    return count;
}

/* Enum/status label lookup. Exact native decode labels are finalized at UI cutover. */
bool reading_enum_label(const ReadingMeta *meta, float value, char *out, size_t size) {
    if (!meta || !out || !size)
        return false;
    int v = (int)value;
    if (meta->enum_set == READING_ENUM_OIL_STATUS) {
        const char *label = v == 1 ? "OK" : v == 2 ? "HIGH" : v == -1 ? "LOW" : "?";
        snprintf(out, size, "%s", label);
        return true;
    }
    if (meta->enum_set == READING_ENUM_GEAR) {
        static const char *const gears[] = {"P", "R", "N", "D", "1", "2", "3", "4", "5", "6", "?"};
        snprintf(out, size, "%s", v >= 0 && v <= 10 ? gears[v] : "?");
        return true;
    }
    if (meta->enum_set == READING_ENUM_DNA) {
        static const char *const dna[] = {"N", "A", "D", "Race"};
        snprintf(out, size, "%s", v >= 0 && v < 4 ? dna[v] : "?");
        return true;
    }
    if (meta->enum_set == READING_ENUM_PEDAL_MAP) {
        static const char *const maps[] = {"Auto", "Sport", "Track", "Off"};
        snprintf(out, size, "%s", v >= 0 && v < 4 ? maps[v] : "?");
        return true;
    }
    if (meta->enum_set == READING_ENUM_SEATBELT) {
        const char *label = v == 0 ? "Fastened" : v == 1 ? "Unfastened" : "?";
        snprintf(out, size, "%s", label);
        return true;
    }
    if (meta->enum_set == READING_ENUM_DPF_REGEN) {
        static const char *const regen[] = {"Off", "Passive", "Active", "?"};
        snprintf(out, size, "%s", v >= 0 && v < 3 ? regen[v] : "?");
        return true;
    }
    snprintf(out, size, "?");
    return true;
}
