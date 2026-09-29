#include "test_report.h"
#include "features/body.h"
#include "features/display_stream.h"
#include "features/ipc_display_test.h"
#include "features/my23_ui.h"
#include "transport/can_bus.h"
#include <assert.h>
#include <string.h>

ChassisState chassis_state;
MirrorsState mirrors_state;
static uint32_t now;
static uint8_t transmitted[1024][8];
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
static void submit_test(uint8_t source, uint8_t pattern) {
    uint8_t message[DASHBOARD_MESSAGE_MAX_LENGTH] = {IPC_TEST_SENTINEL, source, pattern};
    body_display_submit(message);
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
    assert(transmitted_count >= DISPLAY_LEGACY_FRAGMENT_COUNT);
    uint8_t screen[DISPLAY_OUTPUT_LENGTH] = {0};
    for (unsigned i = 0; i < DISPLAY_LEGACY_FRAGMENT_COUNT; ++i) {
        assert(code(transmitted[i]) == 0x09);
        assert(fragment(transmitted[i]) == i);
        assert((transmitted[i][0] >> 3) == DISPLAY_LEGACY_FRAGMENT_COUNT - 1);
        screen[i * 3] = transmitted[i][3];
        screen[i * 3 + 1] = transmitted[i][5];
        screen[i * 3 + 2] = transmitted[i][7];
    }
    assert(!memcmp(screen, "Favorites\rBACCAble beta", sizeof("Favorites\rBACCAble beta") - 1));
    unsigned before = transmitted_count;
    advance(1200);
    assert(menu_frames_after(before, 0x09) >= DISPLAY_LEGACY_FRAGMENT_COUNT);
}

static void test_source_tracking_guard_and_missing_final(void) {
    static const uint8_t song[] = "Wolni w niewoli\rVarius Manx";
    unsigned before = transmitted_count;
    factory_text(0x06, 8, song);
    assert(transmitted_count == before); /* Do not interleave with radio text. */
    advance(50);
    assert(transmitted_count == before);
    advance(100 + DISPLAY_LEGACY_FRAGMENT_COUNT * 50);
    assert(menu_frames_after(before, 0x06) >= DISPLAY_LEGACY_FRAGMENT_COUNT);
    assert(fragment(transmitted[before]) == 0);

    static const uint8_t first[3] = {'B', 'a', 'd'};
    factory_frame(2, 0, 0x21, first); /* Incomplete transfer does not switch source. */
    advance(300 + DISPLAY_LEGACY_FRAGMENT_COUNT * 50);
    assert(code(transmitted[transmitted_count - 1]) == 0x06);

    uint8_t blank[8] = {0, 0x0F};
    body_display_factory_frame(blank, 8);
    advance(100 + DISPLAY_LEGACY_FRAGMENT_COUNT * 50);
    assert(code(transmitted[transmitted_count - 1]) == 0x06);
}

static void test_carplay_and_close_restores_radio(void) {
    uint8_t carplay[57];
    memset(carplay, ' ', sizeof(carplay));
    memcpy(carplay, "Just Gazin'\rDigitalism \" Inteligentne odtwarzanie losowe",
           sizeof("Just Gazin'\rDigitalism \" Inteligentne odtwarzanie losowe") - 1);
    factory_text(0x21, 18, carplay);
    advance(100 + DISPLAY_LEGACY_FRAGMENT_COUNT * 50);
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
    advance(DISPLAY_LEGACY_FRAGMENT_COUNT * 50 + 50);
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

static void test_utf_and_line_lengths_under_each_source(void) {
    static const uint8_t sources[] = {0x06, 0x09, 0x21};
    for (unsigned source = 0; source < sizeof(sources); ++source) {
        for (uint8_t pattern = 0; pattern < IPC_TEST_PATTERN_COUNT; ++pattern) {
            unsigned start = transmitted_count;
            submit_test(sources[source], pattern);
            advance(1500);
            unsigned frames = (transmitted[start][0] >> 3) + 1U;
            assert(frames > 0 && frames <= 32);
            assert(transmitted_count == start + frames);
            uint16_t chars[96] = {0};
            for (unsigned i = 0; i < frames; ++i) {
                const uint8_t *frame = transmitted[start + i];
                assert(fragment(frame) == i && code(frame) == sources[source]);
                assert(frame[0] >> 3 == frames - 1U);
                for (unsigned j = 0; j < 3; ++j)
                    chars[i * 3 + j] = (uint16_t)(frame[2 + j * 2] << 8) | frame[3 + j * 2];
            }
            unsigned split = 0;
            while (split < frames * 3 && chars[split] != '\r') ++split;
            assert(split < frames * 3);
            unsigned second = split + 1;
            while (second < frames * 3 && chars[second]) ++second;
            if (pattern == 0) {
                bool bullet = false, polish = false;
                for (unsigned i = 0; i < frames * 3; ++i) {
                    bullet |= chars[i] == 0x2022;
                    polish |= chars[i] == 0x0104;
                }
                assert(bullet && polish);
            } else if (pattern == 1) {
                assert(split == 48 && second - split - 1 == 12);
                assert(chars[9] == 'A' && chars[19] == 'B' && chars[29] == 'C' && chars[39] == 'D');
            } else if (pattern == 2) {
                assert(split == 12 && second - split - 1 == 48);
                assert(chars[split + 10] == 'A' && chars[split + 20] == 'B');
            } else {
                assert(split == 24 && second - split - 1 == 24);
            }
        }
    }
    unsigned before = transmitted_count;
    advance(3500);
    assert(transmitted_count > before && fragment(transmitted[before]) == 0);

    submit_test(0x09, 0);
    before = transmitted_count;
    fail_next = true;
    advance(50);
    assert(transmitted_count == before);
    advance(50);
    assert(transmitted_count == before + 1 && fragment(transmitted[before]) == 0);
    submit_test(0x09, 0); /* Repeated C1 commands do not restart a transfer. */
    advance(50);
    assert(fragment(transmitted[before + 1]) == 1);
    static const uint8_t song[] = "Wolni w niewoli\rVarius Manx";
    before = transmitted_count;
    factory_text(0x06, 8, song);
    assert(transmitted_count == before);
    advance(100);
    assert(transmitted_count == before + 1);
    assert(fragment(transmitted[before]) == 0 && code(transmitted[before]) == 0x09);

    submit("Information");
    before = transmitted_count;
    advance(700);
    assert(transmitted_count > before);
    assert(code(transmitted[before]) == 0x06); /* Normal menu follows observed radio again. */

    submit_test(0x09, 0);
    advance(4000);
    submit_test(0x09, 0); /* A fresh command renews the temporary test lease. */
    advance(2500);
    assert(ipc_display_test_active());
    before = transmitted_count;
    advance(IPC_TEST_LEASE_MS - 2500);
    assert(!ipc_display_test_active());
    assert(transmitted_count > before);
    assert(code(transmitted[transmitted_count - 1]) == 0x06);
}

static void test_my23_fixed_fields_and_unicode_under_each_source(void) {
    const uint8_t sources[] = {0x06, 0x09, 0x21};
    const uint8_t radio[6] = {'R', 'a', 'd', 'i', 'o', ' '};
    for (unsigned source = 0; source < sizeof(sources); ++source) {
        factory_text(sources[source], 1, radio);
        uint8_t packet[MY23_PACKET_SIZE];
        my23_packet(packet, "› GEAR", "RPM 3500•SPD 100");
        unsigned before = transmitted_count;
        body_display_submit(packet);
        advance(100 + DISPLAY_FRAGMENT_COUNT * 50);
        uint16_t visible[DISPLAY_OUTPUT_LENGTH] = {0};
        for (unsigned i = before; i < transmitted_count; ++i) {
            const uint8_t *frame = transmitted[i];
            assert(code(frame) == sources[source]);
            unsigned part = fragment(frame);
            for (unsigned j = 0; j < 3; ++j)
                visible[part * 3 + j] = ((uint16_t)frame[2 + j*2] << 8) | frame[3 + j*2];
        }
        assert(visible[0] == 0x203a && visible[2] == 'G');
        for (unsigned i = 6; i < 14; ++i) assert(visible[i] == ' ');
        assert(visible[14] == '\r' && visible[15] == 'R' && visible[23] == 0x2022);
        /* Shortening either line must clear old glyphs without moving the field. */
        my23_packet(packet, "OIL", "✓");
        before = transmitted_count; body_display_submit(packet);
        /* A due keepalive can promote a partial dirty update to a complete transfer. */
        advance(1000 + DISPLAY_FRAGMENT_COUNT * 50);
        for (unsigned i = before; i < transmitted_count; ++i) {
            const uint8_t *frame = transmitted[i];
            unsigned part = fragment(frame);
            for (unsigned j = 0; j < 3; ++j)
                visible[part * 3 + j] = ((uint16_t)frame[2+j*2] << 8) | frame[3+j*2];
        }
        assert(visible[0] == 'O' && visible[14] == '\r' && visible[15] == 0x2713);
        for (unsigned i = 16; i < DISPLAY_OUTPUT_LENGTH; ++i) assert(visible[i] == ' ');
    }
}

/* Continuous C1 changes and retries cannot splice two full images together. */
static void test_full_snapshot_and_latest_pending(void) {
    DisplayStream stream = {0};
    uint8_t a[DISPLAY_OUTPUT_LENGTH], b[DISPLAY_OUTPUT_LENGTH], c[DISPLAY_OUTPUT_LENGTH];
    memset(a, 'A', sizeof(a)); memset(b, 'B', sizeof(b)); memset(c, 'C', sizeof(c));
    display_stream_set_full(&stream, true);
    display_stream_submit(&stream, a);
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        uint8_t part, chars[3];
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        assert(!memcmp(chars, "AAA", 3));
        if (i == 2) {
            display_stream_submit(&stream, b);
            display_stream_refresh(&stream);
            /* A failed queue offer stays on the same immutable fragment. */
            assert(display_stream_peek(&stream, &part, chars) && part == i);
            assert(!memcmp(chars, "AAA", 3));
            display_stream_submit(&stream, c);
        }
        display_stream_accept(&stream);
    }
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        uint8_t part, chars[3];
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        assert(!memcmp(chars, "CCC", 3));
        display_stream_accept(&stream);
    }
    uint8_t part, chars[3];
    assert(!display_stream_peek(&stream, &part, chars));
    display_stream_submit(&stream, b);
    assert(display_stream_peek(&stream, &part, chars) && part == 0);
    display_stream_accept(&stream);
    display_stream_submit(&stream, c);
    display_stream_restart(&stream); /* Radio interruption uses the latest image. */
    assert(display_stream_peek(&stream, &part, chars) && part == 0 && chars[0] == 'C');
}

static void test_temporary_pace_and_lease(void) {
    chassis_state.stability_inverted = true; body_process();
    chassis_state.stability_inverted = false; body_init();
    submit("Pace test");
    body_display_options(2, 0);
    now += 50; body_process();
    unsigned before = transmitted_count;
    now += 9; body_process(); assert(transmitted_count == before);
    now += 1; body_process(); assert(transmitted_count == before + 1);
    fail_next = true;
    now += 10; body_process(); assert(transmitted_count == before + 1);
    now += 1; body_process(); assert(transmitted_count == before + 2);
    assert(fragment(transmitted[before + 1]) == 2);
    body_display_options(255, 255); /* Malformed commands cannot speed up the bus. */
    now += 10; body_process(); assert(transmitted_count == before + 3);
    now += 5000; body_process(); /* No renewal: default pacing and full restart. */
    before = transmitted_count;
    now += 10; body_process(); assert(transmitted_count == before);
    now += 40; body_process(); assert(transmitted_count == before + 1);
    body_display_options(1, 1);
    now += 20; body_process();
    before = transmitted_count;
    body_init(); submit("After restart");
    now += 20; body_process(); assert(transmitted_count == before);
    now += 30; body_process(); assert(transmitted_count == before + 1);
}

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(test_menu_footer_and_periodic_reassert),
        HOST_TEST(test_my23_fixed_fields_and_unicode_under_each_source),
        HOST_TEST(test_source_tracking_guard_and_missing_final),
        HOST_TEST(test_carplay_and_close_restores_radio),
        HOST_TEST(test_bounded_radio_deferral_and_can_retry),
        HOST_TEST(test_utf_and_line_lengths_under_each_source),
        HOST_TEST(test_full_snapshot_and_latest_pending),
        HOST_TEST(test_temporary_pace_and_lease),
    };
    host_tests_run(DASHBOARD_MESSAGE_MAX_LENGTH == 24 ? "body-display-24" : "body-display-18", tests, sizeof(tests) / sizeof(tests[0]));
}
