#include "test_report.h"
#include "features/periodic.h"
#include "vehicle/vehicle_profile.h"
#include "state/comfort.h"
#include "state/settings.h"
#include "state/telemetry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SettingsState settings_state;
ComfortState comfort_state;
TelemetryState telemetry_state;
static uint32_t now;
static unsigned resets;
static unsigned sets;

uint32_t HAL_GetTick(void) { return now; }
void HAL_GPIO_WritePin(void *port, uint16_t pin, GPIO_PinState state) {
    (void)port;
    (void)pin;
    if (state == GPIO_PIN_RESET)
        ++resets;
    else
        ++sets;
}
void status_led_activity(void) {}
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    (void)header;
    (void)data;
    return HAL_OK;
}

static void test_exhaust_profile_and_recovery(void) {
    VehicleProfile profile; vehicle_profile_default(&profile);
    profile.confirmed = true;
    profile.equipment[VEHICLE_EQUIPMENT_EXHAUST_VALVES] = VEHICLE_EQUIPMENT_PRESENT;
    vehicle_profile_commit(&profile);
    settings_state.qv_exhaust_flap_function_enabled = 1;
    comfort_state.chinese_exhaust_valve_request = 'O';
    telemetry_state.current_rpm_speed = 1000;
    now = 100;
    exhaust_process();
    assert(sets == 1 && comfort_state.exhaust_valve_mosfet_command_time == 100);

    /* Disabling the preference must still release the already started pulse. */
    settings_state.qv_exhaust_flap_function_enabled = 0;
    now = 1101;
    exhaust_process();
    assert(resets == 2 && comfort_state.exhaust_valve_mosfet_command_time == 0);

    /* An owned open valve can still be closed after disabling the preference. */
    comfort_state.chinese_exhaust_valve_request = 'C';
    exhaust_process();
    assert(sets == 2 && comfort_state.chinese_exhaust_valve_request == 0);
    profile.equipment[VEHICLE_EQUIPMENT_EXHAUST_VALVES] = VEHICLE_EQUIPMENT_ABSENT;
    vehicle_profile_commit(&profile);
    settings_state.qv_exhaust_flap_function_enabled = 1;
    comfort_state.chinese_exhaust_valve_request = 'O';
    now += 1100; exhaust_process();
    assert(sets == 2 && comfort_state.chinese_exhaust_valve_request == 'O');

    puts("exhaust: PASS");
}
int main(void) {
    const HostTest tests[] = {HOST_TEST(test_exhaust_profile_and_recovery)};
    host_tests_run("exhaust", tests, 1);
    return 0;
}
