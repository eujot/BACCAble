#include "test_report.h"
#include "features/body.h"
#include "features/display_stream.h"
#include "transport/can_bus.h"
#include <assert.h>
#include <string.h>
ChassisState chassis_state;
MirrorsState mirrors_state;
static uint32_t now;
static uint8_t visible[DASHBOARD_MESSAGE_MAX_LENGTH];
uint32_t HAL_GetTick(void) { return now; }
void can_set_bitrate(enum can_bitrate bitrate) { (void)bitrate; }
void can_enable(void) {}
void parking_mirrors_process(void) {}
bool flash_record_load(unsigned r, uint16_t t, void *d, size_t n) {
    (void)r; (void)t; (void)d; (void)n; return false;
}
bool flash_record_save(unsigned r, uint16_t t, const void *d, size_t n) {
    (void)r; (void)t; (void)d; (void)n; return false;
}
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    assert(header->StdId == 0x90 && header->DLC == 8);
    unsigned i = ((data[0] & 7) << 2) | (data[1] >> 6);
    assert(i < DISPLAY_FRAGMENT_COUNT);
    visible[i * 3] = data[3];
    visible[i * 3 + 1] = data[5];
    visible[i * 3 + 2] = data[7];
    return HAL_OK;
}
static void test_refresh_during_changing_readings(void) {
    uint8_t text[DASHBOARD_MESSAGE_MAX_LENGTH];
    memset(text, 'A', sizeof(text));
    body_init();
    body_display_submit(text);
    for (now = 50; now <= 400; now += 50)
        body_process();
    assert(!memcmp(visible, text, sizeof(text)));
    /* Model lost screen content without a received factory notification.
     * Frequent changes at the end must not indefinitely suppress recovery. */
    memset(visible, '?', 3);
    for (now = 450; now <= 1500; now += 50) {
        if (now % 100 == 0) {
            text[sizeof(text) - 1] = '0' + (now / 100) % 10;
            body_display_submit(text);
        }
        body_process();
    }
    assert(!memcmp(visible, text, 3));
}
int main(void) {
    const HostTest tests[] = {HOST_TEST(test_refresh_during_changing_readings)};
    host_tests_run("body_display", tests, sizeof(tests) / sizeof(tests[0]));
}
