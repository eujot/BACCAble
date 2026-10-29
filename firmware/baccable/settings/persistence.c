#include "features/periodic.h"
#include "app/powertrain.h"
#include "storage/flash_records.h"
#include "features/menu.h"
#include "features/usb_modes.h"
#include "vehicle/vehicle_profile.h"
#if defined(BACCABLE_C1)
static uint16_t saved_settings[SETUP_FLASH_PARAM_BUFFER_SIZE];
static uint16_t best_times[2];
static uint8_t settings_loaded, times_loaded;

enum { SETTINGS_PROFILE_TYPE = 0x108, SETTINGS_PROFILE_SIZE =
    SETUP_FLASH_PARAM_BUFFER_SIZE * 2 + VEHICLE_PROFILE_WIRE_SIZE };
static uint8_t saved_wire[SETTINGS_PROFILE_SIZE];
static bool combined_loaded;

static void profile_slots(const VehicleProfile *profile, uint16_t *slots) {
    bool diesel, v6, my23;
    uint8_t drivetrain;
    vehicle_profile_legacy_flags(profile, &diesel, &v6, &drivetrain, &my23);
    slots[15] = diesel; slots[35] = v6; slots[39] = drivetrain; slots[5] = my23;
}
static void profile_runtime(const VehicleProfile *profile) {
    bool diesel, v6, my23;
    uint8_t drivetrain;
    vehicle_profile_legacy_flags(profile, &diesel, &v6, &drivetrain, &my23);
    settings_state.is_diesel_enabled = diesel;
    settings_state.gasoline_v6 = v6;
    settings_state.drivetrain = drivetrain;
    settings_state.ipc_my23_is_installed = my23;
}

/* Called before a complete settings reload, including a simulated cold boot. */
void settings_load_begin(void) { settings_loaded = 0; }

static void load_settings(void) {
    if (settings_loaded) return;
    VehicleProfile profile;
    combined_loaded = flash_record_load(SETTINGS_RECORD, SETTINGS_PROFILE_TYPE,
                                        saved_wire, sizeof(saved_wire)) &&
        vehicle_profile_decode(saved_wire + SETUP_FLASH_PARAM_BUFFER_SIZE * 2, &profile);
    if (combined_loaded) {
        for (unsigned i = 0; i < SETUP_FLASH_PARAM_BUFFER_SIZE; ++i)
            saved_settings[i] = saved_wire[2*i] | (uint16_t)saved_wire[2*i+1] << 8;
    } else {
        memset(saved_settings, 0xff, sizeof(saved_settings));
        bool imported = flash_record_load(SETTINGS_RECORD, 0x101, saved_settings, sizeof(saved_settings)) ||
            flash_record_load(SETTINGS_RECORD, 0x101, saved_settings, 41U * sizeof(uint16_t)) ||
            flash_record_load(SETTINGS_RECORD, 0x101, saved_settings, 40U * sizeof(uint16_t));
        uint8_t old_profile[VEHICLE_PROFILE_WIRE_SIZE] = {0};
        if (!(flash_record_load(SETTINGS_RECORD, 0x107, old_profile, 16) &&
              vehicle_profile_decode(old_profile, &profile))) {
            /* Erased Flash uses declared build defaults, without granting legacy permissions. */
            uint16_t normalized[SETUP_FLASH_PARAM_BUFFER_SIZE];
            for (unsigned i = 0; i < SETUP_FLASH_PARAM_BUFFER_SIZE; ++i)
                normalized[i] = setup_read_flash_value(i + 1, saved_settings[i]);
            vehicle_profile_import_legacy(normalized, SETUP_FLASH_PARAM_BUFFER_SIZE, &profile);
            if (profile.display_selection != VEHICLE_DISPLAY_MY23)
                profile.display_selection = DASHBOARD_MESSAGE_MAX_LENGTH == 24 ?
                    VEHICLE_DISPLAY_LEGACY24 : VEHICLE_DISPLAY_LEGACY18;
        }
        profile.legacy_permissions = imported;
    }
    vehicle_profile_commit(&profile);
    profile_slots(&profile, saved_settings);
    settings_loaded = 1;
}

/* Load saved performance records once for subsequent lookups. */
static void load_times(void) {
    if (times_loaded)
        return;
    memset(best_times, 0xff, sizeof(best_times));
    flash_record_load(STATISTICS_RECORD, 0x102, best_times, sizeof(best_times));
    times_loaded = 1;
}

/* Return a saved feature preference or its unavailable marker. */
uint16_t settings_read(uint8_t id) {
    if (!id || id > SETUP_FLASH_PARAM_BUFFER_SIZE)
        return 0;
    load_settings();
    return setup_read_flash_value(id, saved_settings[id - 1]);
}

/* Persist the complete transaction before publishing the candidate profile. */
uint8_t settings_profile_save(const VehicleProfile *profile) {
    uint16_t values[SETUP_FLASH_PARAM_BUFFER_SIZE] = {0};
    uint8_t wire[SETTINGS_PROFILE_SIZE];
    if (setup_flash_slots_count() > SETUP_FLASH_PARAM_BUFFER_SIZE ||
        !vehicle_profile_encode(profile, wire + sizeof(values))) return 254;
    setup_fill_flash_params(values);
    profile_slots(profile, values);
    load_settings();
    for (unsigned i = 0; i < SETUP_FLASH_PARAM_BUFFER_SIZE; ++i) {
        wire[2*i] = values[i]; wire[2*i+1] = values[i] >> 8;
    }
    if (combined_loaded && !memcmp(saved_wire, wire, sizeof(wire))) {
        if (usb_modes_needs_apply()) { board_sync_restart(); usb_modes_apply(); }
        return 0;
    }
    if (!flash_record_save(SETTINGS_RECORD, SETTINGS_PROFILE_TYPE, wire, sizeof(wire))) return 255;
    memcpy(saved_wire, wire, sizeof(wire));
    memcpy(saved_settings, values, sizeof(values));
    combined_loaded = true;
    vehicle_profile_commit(profile);
    profile_runtime(profile);
    board_sync_saved();
    usb_modes_apply();
    return 0;
}
uint8_t settings_save(void) {
    load_settings();
    VehicleProfile profile = *vehicle_profile_current();
    return settings_profile_save(&profile);
}

/* Return the best saved acceleration time for the requested interval. */
float statistics_read_best(uint8_t id) {
    if (id < 1 || id > 2)
        return 65.535f;
    load_times();
    return best_times[id - 1] / 1000.0f;
}

/* Clear saved acceleration records and report whether the save succeeded. */
uint8_t statistics_reset(void) {
    uint16_t empty[2] = {UINT16_MAX, UINT16_MAX};
    if (!flash_record_save(STATISTICS_RECORD, 0x102, empty, sizeof(empty)))
        return 255;
    memcpy(best_times, empty, sizeof(empty));
    times_loaded = 1;
    return 0;
}

/* Save improved acceleration records without replacing a better result. */
uint8_t statistics_save_best(void) {
    load_times();
    uint16_t next[2] = {best_times[0], best_times[1]};
    const float seconds[2] = {statistics_state.chronometer_elapsed_time_0_100_km_h,
                              statistics_state.chronometer_elapsed_time_100_200_km_h};
    const uint8_t running[2] = {statistics_state.statistics_0_100_started,
                                statistics_state.statistics_100_200_started};
    const float limits[2] = {20.0f, 40.0f};
    for (unsigned i = 0; i < 2; ++i) {
        if (!running[i] && isfinite(seconds[i]) && seconds[i] > 0 && seconds[i] <= limits[i]) {
            uint16_t milliseconds = (uint16_t)(seconds[i] * 1000.0f + 0.5f);
            if (milliseconds < next[i])
                next[i] = milliseconds;
        }
    }
    if (!memcmp(next, best_times, sizeof(next)))
        return 253;
    if (!flash_record_save(STATISTICS_RECORD, 0x102, next, sizeof(next)))
        return 255;
    memcpy(best_times, next, sizeof(next));
    return 0;
}
#endif
