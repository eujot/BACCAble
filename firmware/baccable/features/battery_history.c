#include "app/build_config.h"
#if defined(BACCABLE_C1)
#include "app/powertrain.h"
#include "features/battery_history.h"
#include "features/reading_catalog.h"
#include "features/ibs_override.h"
#include "diagnostics/fault_reader.h"
#include "diagnostics/parameter_cache.h"
#include "diagnostics/parameter_request.h"
static bool active, failed, running, prepared, collecting, attempted, maintenance_attempted;
static uint8_t ignition;
static uint32_t ignition_at, running_at, started, next_due, collection_at, query_at;
static unsigned samples;
void battery_history_init(void) {
    history_store_init(history_flash_storage());
    prepared = history_store_prepare();
    active = running = collecting = attempted = false; failed = !prepared;
    ignition = 0; maintenance_attempted = false;
}
void battery_history_ignition(unsigned code, uint32_t now) {
    if (code != 2 && code != 8 && code != 10) return;
    ignition_at = now;
    if (code == 10) return; /* Preserve RUN through cranking. */
    ignition = code;
    if (code == 2) { active = running = collecting = attempted = false; }
}
bool battery_history_current(void) { return active; }
bool battery_history_error(void) { return failed; }
void battery_history_process(void) {
    if (runtime_state.low_consume_is_active) {
        /* CAN receive runs before power_process clears the sleep flag. */
        if (currentTime - runtime_state.last_received_can_msg_time <=
            TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS) return;
        active = running = collecting = attempted = false; ignition = 0;
        if (!prepared && !maintenance_attempted) {
            maintenance_attempted = true;
            prepared = history_store_prepare(); failed = !prepared;
        }
        return;
    }
    maintenance_attempted = false;
    bool engine = telemetry_state.current_rpm_speed > 400 &&
        currentTime - telemetry_state.current_rpm_updated_at <= 1000U;
    if (!active) {
        if (!engine || ignition != 8 || currentTime - ignition_at > 2000U) { running = false; return; }
        if (!running) { running = true; running_at = currentTime; return; }
        if (currentTime - running_at < 2000U || attempted) return;
        attempted = true;
        if (!history_store_begin()) { failed = true; return; }
        active = true; failed = false; prepared = false; started = running_at;
        next_due = samples = 0; collecting = false;
    }
    uint32_t elapsed = currentTime - started;
    if (elapsed > 43205000U) samples = HISTORY_SAMPLES;
    if (samples >= HISTORY_SAMPLES) return;
    if (elapsed < next_due) return;
    if (!collecting) { collecting = true; collection_at = currentTime; query_at = currentTime - 1000U; }
    uint8_t voltage_id = reading_source(reading_lookup(READING_BATTERY_VOLTAGE), vehicle_profile_current());
    float soc = parameter_cache_get_live(3, currentTime);
    float volts = parameter_cache_get_live(voltage_id, currentTime);
    if (ibs_override_enabled()) soc = NAN;
    bool soc_valid = isfinite(soc) && soc >= 0 && soc <= 100;
    bool voltage_valid = isfinite(volts) && volts >= 0 && volts < 65.535f;
    bool blocked = diagnostics_state.clear_faults_request || fault_reader_busy();
    if (!voltage_valid && engine && !blocked && currentTime - query_at >= 1000U &&
        currentTime - collection_at < 5000U) {
        parameter_request_begin_background(voltage_id);
        query_at = currentTime;
    }
    /* Wait briefly for both sources; then preserve explicit missing fields. */
    if (!(soc_valid && voltage_valid) && currentTime - collection_at < 5000U) return;
    if (!samples && !soc_valid && !voltage_valid) {
        /* First record needs a real observation; bound retries while waiting. */
        if (currentTime - collection_at >= 60000U) collecting = false;
        return;
    }
    HistorySample sample = {(uint16_t)(elapsed / 60000U),
        voltage_valid ? (uint16_t)(volts * 1000.0f + 0.5f) : HISTORY_MISSING,
        soc_valid ? (uint8_t)(soc + 0.5f) : 255};
    if (!history_store_append(sample)) { failed = true; samples = HISTORY_SAMPLES; return; }
    ++samples; collecting = false;
    /* Skip missed intervals rather than manufacturing backdated samples. */
    next_due = (elapsed / 600000U + 1) * 600000U;
}
#endif
