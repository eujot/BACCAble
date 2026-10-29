#include "vehicle/vehicle_runtime.h"
#include "app/powertrain.h"
#include "features/menu.h"
#if defined(BACCABLE_C1)
static DisplayProfile active_display;
static uint32_t sent, replied;
static uint8_t token;
static bool pending, supported, mirror_busy, reply_valid;
void vehicle_runtime_init(void) {
    VehicleProfile fallback = *vehicle_profile_current();
    /* Old peers already understand explicit MY23; unknown Auto uses the build fallback. */
    if (fallback.display_selection == VEHICLE_DISPLAY_AUTO)
        fallback.display_selection = VEHICLE_DISPLAY_LEGACY18;
    if (fallback.display_selection != VEHICLE_DISPLAY_MY23)
        fallback.display_selection = DASHBOARD_MESSAGE_MAX_LENGTH == 24 ? VEHICLE_DISPLAY_LEGACY24 : VEHICLE_DISPLAY_LEGACY18;
    active_display = vehicle_display_profile_for(&fallback);
    pending = supported = reply_valid = false; sent = 0;
}
DisplayProfile vehicle_display_effective(void) { return active_display; }
bool vehicle_display_negotiated(void) { return supported; }
void vehicle_runtime_process(void) {
    if (currentTime < TIMING__ALL___SERIAL_IGNORE_WINDOW_MS) return;
    if (pending && currentTime - sent < 1000U) return;
    if (sent && currentTime - sent < 1000U) return;
    uint8_t next = token + 1;
    uint8_t command[] = {BhBusID, BH_CMD_DISPLAY_PROFILE, next};
    if (board_uart_send(command, sizeof(command))) { token = next; sent = currentTime; pending = true; }
}
void vehicle_runtime_reply(uint8_t response_token, uint8_t capability, uint8_t busy) {
    if (!pending || response_token != token || currentTime - sent > 1000U) return;
    pending = false; replied = currentTime; reply_valid = true; mirror_busy = busy != 0;
    if (capability != 1) return;
    supported = true;
    DisplayProfile next = vehicle_display_profile();
    if (next.mode != active_display.mode) {
        active_display = next;
        menu_engine_changed();
    }
}
const char *vehicle_profile_busy_reason(void) {
    if (chassis_state.awd_sequence) return "Stop AWD request";
    if (chassis_state.front_brake_forced) return "Release brake";
    if (chassis_state.dyno_mode_enabled_on_master) return "Stop Dyno";
    if (comfort_state.force_q_vexhaust_valve_opened || comfort_state.chinese_valve_is_opened ||
        comfort_state.chinese_exhaust_valve_request || comfort_state.exhaust_valve_mosfet_command_time)
        return "Set QV to AUTO";
    if (menu_setting_busy(8) || menu_setting_busy(10) || menu_setting_busy(27)) return "Request pending";
    if (settings_state.park_mirror) {
        if (!reply_valid || currentTime - replied > 3000U) return "Mirror status wait";
        if (mirror_busy) return "Wait for mirrors";
    }
    return NULL;
}
VehicleFeature vehicle_setting_feature(uint8_t slot) {
    switch (slot) {
    case 17: return VEHICLE_FEATURE_DPF_REGEN;
    case 9: case 24: return VEHICLE_FEATURE_VIRTUAL_ACC;
    case 11: return VEHICLE_FEATURE_AWD_DISABLE;
    case 23: return VEHICLE_FEATURE_PARK_MIRROR;
    case 27: return VEHICLE_FEATURE_VIRTUAL_HAS;
    case 28: return VEHICLE_FEATURE_EXHAUST_ACTION;
    case 39: return VEHICLE_FEATURE_LANE_ASSIST;
    case 41: return VEHICLE_FEATURE_SOFT_SUSPENSION;
    default: return VEHICLE_FEATURE_COUNT;
    }
}
const char *vehicle_setting_reason(uint8_t slot) {
    VehicleFeature feature = vehicle_setting_feature(slot);
    if (feature == VEHICLE_FEATURE_COUNT || vehicle_feature_allowed(feature)) return NULL;
    return vehicle_feature_support(feature) == VEHICLE_SUPPORT_UNSUPPORTED ? "Not on vehicle" : "Configure Vehicle";
}
#endif
