#include "test_report.h"
#include "features/body.h"
#include "features/display_stream.h"
#include "transport/can_bus.h"
#include <assert.h>
#include <string.h>

ChassisState chassis_state;
MirrorsState mirrors_state;
static uint32_t now;
static uint8_t transmitted[256][8];
static unsigned transmitted_count;
static bool fail_next;

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
    if (fail_next) {
        fail_next = false;
        return HAL_ERROR;
    }
    assert(transmitted_count < sizeof(transmitted) / sizeof(transmitted[0]));
    memcpy(transmitted[transmitted_count++], data, 8);
    return HAL_OK;
}

static unsigned fragment(const uint8_t frame[8]) {
    return ((frame[0] & 7U) << 2) | (frame[1] >> 6);
}
static unsigned code(const uint8_t frame[8]) { return frame[1] & 0x3FU; }
static void advance(unsigned milliseconds) {
    for (unsigned elapsed = 0; elapsed < milliseconds; elapsed += 50) {
        now += 50;
        body_process();
    }
}
static void submit(const char *value) {
    uint8_t text[DASHBOARD_MESSAGE_MAX_LENGTH];
    memset(text, ' ', sizeof(text));
    size_t length = strlen(value);
    assert(length <= sizeof(text));
    memcpy(text, value, length);
    body_display_submit(text);
}
static void factory_frame(uint8_t total, uint8_t index, uint8_t source, const uint8_t chars[3]) {
    uint8_t frame[8] = {(total << 3) | (index >> 2), ((index & 3U) << 6) | source,
                        0, chars[0], 0, chars[1], 0, chars[2]};
    body_display_factory_frame(frame, 8);
}
static void factory_text(uint8_t source, uint8_t total, const uint8_t *text) {
    for (uint8_t index = 0; index <= total; ++index) {
        factory_frame(total, index, source, text + index * 3);
        now += 20;
        body_process();
    }
}
static unsigned menu_frames_after(unsigned start, uint8_t source) {
    unsigned count = 0;
    for (unsigned i = start; i < transmitted_count; ++i) {
        if (code(transmitted[i]) == source)
            ++count;
    }
    return count;
}

static void test_menu_footer_and_periodic_reassert(void) {
    body_init();
    submit("Favorites");
    advance(700);
    assert(transmitted_count >= DISPLAY_FRAGMENT_COUNT);
    uint8_t screen[DISPLAY_OUTPUT_LENGTH] = {0};
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        assert(code(transmitted[i]) == 0x09);
        assert(fragment(transmitted[i]) == i);
        assert((transmitted[i][0] >> 3) == DISPLAY_FRAGMENT_COUNT - 1);
        screen[i * 3] = transmitted[i][3];
        screen[i * 3 + 1] = transmitted[i][5];
        screen[i * 3 + 2] = transmitted[i][7];
    }
    assert(!memcmp(screen, "Favorites\rBACCAble beta", sizeof("Favorites\rBACCAble beta") - 1));
    unsigned before = transmitted_count;
    advance(1200);
    assert(menu_frames_after(before, 0x09) >= DISPLAY_FRAGMENT_COUNT);
}

static void test_source_tracking_guard_and_missing_final(void) {
    static const uint8_t song[] = "Wolni w niewoli\rVarius Manx";
    unsigned before = transmitted_count;
    factory_text(0x06, 8, song);
    assert(transmitted_count == before); /* Do not interleave with radio text. */
    advance(50);
    assert(transmitted_count == before);
    advance(100 + DISPLAY_FRAGMENT_COUNT * 50);
    assert(menu_frames_after(before, 0x06) >= DISPLAY_FRAGMENT_COUNT);
    assert(fragment(transmitted[before]) == 0);

    static const uint8_t first[3] = {'B', 'a', 'd'};
    factory_frame(2, 0, 0x21, first); /* Incomplete transfer does not switch source. */
    advance(300 + DISPLAY_FRAGMENT_COUNT * 50);
    assert(code(transmitted[transmitted_count - 1]) == 0x06);

    uint8_t blank[8] = {0, 0x0F};
    body_display_factory_frame(blank, 8);
    advance(100 + DISPLAY_FRAGMENT_COUNT * 50);
    assert(code(transmitted[transmitted_count - 1]) == 0x06);
}

static void test_carplay_and_close_restores_radio(void) {
    uint8_t carplay[57];
    memset(carplay, ' ', sizeof(carplay));
    memcpy(carplay, "Just Gazin'\rDigitalism \" Inteligentne odtwarzanie losowe",
           sizeof("Just Gazin'\rDigitalism \" Inteligentne odtwarzanie losowe") - 1);
    factory_text(0x21, 18, carplay);
    advance(100 + DISPLAY_FRAGMENT_COUNT * 50);
    assert(code(transmitted[transmitted_count - 1]) == 0x21);
    unsigned before = transmitted_count;
    submit("");
    advance(19 * 50 + 50);
    assert(transmitted_count == before + 19);
    for (unsigned i = 0; i < 19; ++i) {
        const uint8_t *frame = transmitted[before + i];
        assert(code(frame) == 0x21 && fragment(frame) == i);
        assert(frame[3] == carplay[i * 3]);
    }
    advance(1200);
    assert(transmitted_count == before + 19); /* Closed menu stays quiet. */
}

static void test_bounded_radio_deferral_and_can_retry(void) {
    submit("Readings");
    advance(DISPLAY_FRAGMENT_COUNT * 50 + 50);
    unsigned before = transmitted_count;
    static const uint8_t chars[3] = {'R', 'a', 'd'};
    for (unsigned i = 0; i < 25; ++i) {
        factory_frame(8, 0, 0x06, chars);
        now += 50;
        body_process();
    }
    assert(transmitted_count > before); /* Continuous radio cannot hide the menu forever. */
    advance(300);
    advance(1000);
    submit("Retry");
    before = transmitted_count;
    fail_next = true;
    advance(50);
    assert(transmitted_count == before);
    advance(50);
    assert(transmitted_count == before + 1);
}

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(test_menu_footer_and_periodic_reassert),
        HOST_TEST(test_source_tracking_guard_and_missing_final),
        HOST_TEST(test_carplay_and_close_restores_radio),
        HOST_TEST(test_bounded_radio_deferral_and_can_retry),
    };
    host_tests_run("body_display", tests, sizeof(tests) / sizeof(tests[0]));
}
