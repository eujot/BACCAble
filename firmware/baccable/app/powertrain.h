#ifndef BACCABLE_APP_POWERTRAIN_H
#define BACCABLE_APP_POWERTRAIN_H
#include "app/application_state.h"
#include "platform/system.h"
#include "settings/setup_menu.h"

void powertrain_init(void);
void pedal_booster_set_map(uint8_t map);
void powertrain_process(void);
uint8_t pedal_booster_needs_update(void);

void dashboard_send_setup(void);
float native_parameter_read(uint8_t paramId);
uint32_t system_free_ram(void);
void dashboard_clear(void);
uint8_t settings_save(void);
uint8_t statistics_reset();
uint8_t statistics_save_best(void);
uint16_t settings_read(uint8_t paramId);
float statistics_read_best(uint8_t paramId);
#endif /* BACCABLE_APP_POWERTRAIN_H */
