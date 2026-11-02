#include "test_report.h"
#include "app/powertrain.h"
#include "vehicle/frame_handlers.h"
#include "features/reading_format.h"
#include <assert.h>

SettingsState settings_state;
SecurityState security_state;
ChassisState chassis_state;
ComfortState comfort_state;
TelemetryState telemetry_state;
DiagnosticsState diagnostics_state;
RuntimeState runtime_state;
DashboardState dashboard_state;
PedalState pedal_state;
MirrorsState mirrors_state;
DisplayState display_state;
static uint32_t now;
static unsigned saves;
uint32_t HAL_GetTick(void) { return now; }
uint8_t board_uart_send(const uint8_t *data, size_t size) { (void)data; (void)size; return 1; }
void menu_show_reading(uint8_t id) { (void)id; }
void status_led_activity(void) {}
uint8_t statistics_save_best(void) { ++saves; return 0; }
uint32_t can_forward(const CAN_RxHeaderTypeDef *header, uint8_t *data) { (void)header; (void)data; return 0; }
void _putchar(char c) { (void)c; }

static void speed(float kmh, uint32_t advance) {
    now += advance;
    unsigned raw = (unsigned)(kmh * 16);
    uint8_t frame[8] = {(raw >> 11) & 31, (raw >> 3) & 255, (raw & 7) << 5};
    CAN_RxHeaderTypeDef h = {.StdId = 0x101, .DLC = 3};
    vehicle_handle_vehicle_speed(&h, frame);
}
static void reset(void) {
    memset(&statistics_state, 0, sizeof(statistics_state));
    statistics_state.chronometer_elapsed_time_0_100_km_h = 60;
    statistics_state.chronometer_elapsed_time_100_200_km_h = 60;
    saves = 0; now = 100;
}
static void test_acceleration_lifecycle(void) {
    reset(); speed(120, 0);
    assert(!statistics_state.statistics_0_100_started && !statistics_state.statistics_100_200_started && !saves);
    speed(0, 20); speed(1, 20);
    assert(statistics_state.statistics_0_100_started);
    for (unsigned i = 0; i < 5; ++i) speed(50, 1000);
    speed(100, 1000);
    assert(!statistics_state.statistics_0_100_started && saves == 1);
    assert(fabsf(statistics_state.chronometer_elapsed_time_0_100_km_h - 6.01f) < .001f);
    speed(101, 20);
    assert(statistics_state.statistics_100_200_started);
    for (unsigned i = 0; i < 9; ++i) speed(150, 1000);
    speed(200, 1000);
    assert(!statistics_state.statistics_100_200_started && saves == 2);
    assert(fabsf(statistics_state.chronometer_elapsed_time_100_200_km_h - 10.01f) < .001f);
}
static void test_acceleration_timeout_and_gap(void) {
    reset(); speed(0, 0); speed(1, 20);
    for (unsigned i = 0; i < 21; ++i) speed(50, 1000);
    assert(!statistics_state.statistics_0_100_started && !saves);
    assert(statistics_state.chronometer_elapsed_time_0_100_km_h > 20);
    speed(101, 20);
    for (unsigned i = 0; i < 41; ++i) speed(150, 1000);
    assert(!statistics_state.statistics_100_200_started && !saves);
    assert(statistics_state.chronometer_elapsed_time_100_200_km_h > 40);
    reset(); speed(0, 0); speed(1, 20); speed(120, 1001);
    assert(!saves && !statistics_state.statistics_0_100_started && !statistics_state.statistics_100_200_started);
    assert(statistics_state.chronometer_elapsed_time_0_100_km_h == 60);
    speed(100, 20); speed(101, 20); speed(200, 1001);
    assert(!saves && !statistics_state.statistics_100_200_started);
    assert(statistics_state.chronometer_elapsed_time_100_200_km_h == 60);
    reset(); now = UINT32_MAX - 10; speed(0, 0); speed(1, 20);
    assert(statistics_state.statistics_0_100_started);
    speed(100, 500);
    assert(saves == 1 && fabsf(statistics_state.chronometer_elapsed_time_0_100_km_h - .51f) < .001f);
}
int main(void) {
    const HostTest tests[] = {HOST_TEST(test_acceleration_lifecycle), HOST_TEST(test_acceleration_timeout_and_gap)};
    host_tests_run("acceleration", tests, sizeof(tests)/sizeof(tests[0]));
}
