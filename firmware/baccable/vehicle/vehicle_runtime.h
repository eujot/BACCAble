#ifndef BACCABLE_VEHICLE_RUNTIME_H
#define BACCABLE_VEHICLE_RUNTIME_H
#include "vehicle/vehicle_profile.h"
/* Leave UART capacity for rain leases, peer status and screen updates. */
#define VEHICLE_DISPLAY_POLL_MS 5000U
#define VEHICLE_DISPLAY_REPLY_MS 3000U
void vehicle_runtime_init(void);
void vehicle_runtime_process(void);
void vehicle_runtime_reply(uint8_t token, uint8_t capability, uint8_t mirror_busy);
DisplayProfile vehicle_display_effective(void);
bool vehicle_display_negotiated(void);
const char *vehicle_profile_busy_reason(void);
VehicleFeature vehicle_setting_feature(uint8_t slot);
const char *vehicle_setting_reason(uint8_t slot);
#endif
