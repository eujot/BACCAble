#include "test_report.h"
#include "features/body.h"
#include "features/display_stream.h"
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
        for (unsigned i = 6; i < MY23_L1_VISIBLE; ++i) assert(visible[i] == ' ');
        assert(visible[MY23_L1_VISIBLE] == '\r' && visible[MY23_L1_VISIBLE + 1] == 'R' &&
               visible[MY23_L1_VISIBLE + 9] == 0x2022);
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
        assert(visible[0] == 'O' && visible[MY23_L1_VISIBLE] == '\r' &&
               visible[MY23_L1_VISIBLE + 1] == 0x2713);
        for (unsigned i = MY23_L1_VISIBLE + 2; i < DISPLAY_OUTPUT_LENGTH; ++i) assert(visible[i] == ' ');
        /* The measured 16-column upper field and the 22-column lower field
         * must share one complete 13-fragment CAN transfer. */
        my23_packet(packet, "1234567890123456EXTRA", "ABCDEFGHIJKLMNOPQRSTUVEXTRA");
        before = transmitted_count; body_display_submit(packet);
        advance(1000 + DISPLAY_FRAGMENT_COUNT * 50);
        unsigned seen = 0;
        for (unsigned i = before; i < transmitted_count; ++i) {
            const uint8_t *frame = transmitted[i];
            unsigned part = fragment(frame);
            seen |= 1U << part;
            for (unsigned j = 0; j < 3; ++j)
                visible[part * 3 + j] = ((uint16_t)frame[2+j*2] << 8) | frame[3+j*2];
        }
        assert(seen == (1U << DISPLAY_FRAGMENT_COUNT) - 1);
        for (unsigned i = 0; i < MY23_L1_VISIBLE; ++i)
            assert(visible[i] == (uint16_t)"1234567890123456"[i]);
        assert(visible[MY23_L1_VISIBLE] == '\r');
        for (unsigned i = 0; i < MY23_L2_VISIBLE; ++i)
            assert(visible[MY23_L1_VISIBLE + 1 + i] == 'A' + i);
        /* A blank first line must not release a nonblank second line. */
        my23_packet(packet, "", " Next page");
        before = transmitted_count; body_display_submit(packet);
        advance(1000 + DISPLAY_FRAGMENT_COUNT * 50);
        for (unsigned i = before; i < transmitted_count; ++i) {
            const uint8_t *frame = transmitted[i];
            unsigned part = fragment(frame);
            for (unsigned j = 0; j < 3; ++j)
                visible[part * 3 + j] = ((uint16_t)frame[2+j*2] << 8) | frame[3+j*2];
        }
        assert(visible[MY23_L1_VISIBLE] == '\r' && visible[MY23_L1_VISIBLE + 2] == 'N');
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
    /* A keepalive must resend even equal fragments; repeated requests must
     * neither skip these fragments nor restart an in-progress pass. */
    display_stream_refresh(&stream);
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        display_stream_refresh(&stream);
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        assert(!memcmp(chars, "CCC", 3));
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        display_stream_accept(&stream);
    }
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
    now += 10; body_process(); assert(transmitted_count == before + 1);
    body_display_options(0, 0); /* Slower than default must also expire. */
    now += 49; body_process(); assert(transmitted_count == before + 1);
    now += 1; body_process(); assert(transmitted_count == before + 2);
    now += 5000; body_process();
    before = transmitted_count;
    now += 19; body_process(); assert(transmitted_count == before);
    now += 1; body_process(); assert(transmitted_count == before + 1);
    body_display_options(1, 1);
    now += 20; body_process();
    before = transmitted_count;
    body_init(); submit("After restart");
    now += 19; body_process(); assert(transmitted_count == before);
    now += 1; body_process(); assert(transmitted_count == before + 1);
}

#include "test_oil_animation.c"

static void test_runtime_legacy_packet_widths(void) {
    now = 10000; transmitted_count = 0; body_init();
    const unsigned widths[] = {24, 18, 24};
    for (unsigned pass = 0; pass < 3; ++pass) {
        uint8_t packet[39]; memset(packet, ' ', sizeof(packet));
        unsigned width = widths[pass];
        packet[0] = width == 24 ? LEGACY24_PACKET_MARKER : LEGACY18_PACKET_MARKER;
        for (unsigned i = 0; i < width; ++i) packet[i+1] = 'A' + i;
        body_display_submit(packet);
        unsigned first = transmitted_count;
        advance(750);
        unsigned fragments = (width + 1 + DISPLAY_FOOTER_LENGTH + 2) / 3;
        assert(transmitted_count >= first + fragments);
        uint8_t screen[39] = {0};
        for (unsigned i = 0; i < fragments; ++i) {
            assert(fragment(transmitted[first+i]) == i);
            assert(transmitted[first+i][0] >> 3 == fragments - 1);
            screen[3*i] = transmitted[first+i][3];
            screen[3*i+1] = transmitted[first+i][5];
            screen[3*i+2] = transmitted[first+i][7];
        }
        for (unsigned i = 0; i < width; ++i) assert(screen[i] == 'A' + i);
        assert(screen[width] == '\r');
        assert(!memcmp(screen + width + 1, DISPLAY_FOOTER, DISPLAY_FOOTER_LENGTH));
    }
    const uint8_t song[] = "Radio!";
    factory_text(0x06, 1, song);
    uint8_t packet[39]; memset(packet, ' ', sizeof(packet)); packet[0] = LEGACY18_PACKET_MARKER;
    body_display_submit(packet); /* Width switch + release keeps the original source. */
    unsigned first = transmitted_count; advance(400);
    assert(transmitted_count >= first + 2);
    assert(code(transmitted[first]) == 0x06 && fragment(transmitted[first]) == 0);
    assert(transmitted[first][3] == 'R' && transmitted[first+1][7] == '!');
}

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(test_runtime_legacy_packet_widths),
        HOST_TEST(test_oil_animation_steps),
        HOST_TEST(test_oil_animation_both_lines),
        HOST_TEST(test_menu_footer_and_periodic_reassert),
        HOST_TEST(test_my23_fixed_fields_and_unicode_under_each_source),
        HOST_TEST(test_source_tracking_guard_and_missing_final),
        HOST_TEST(test_carplay_and_close_restores_radio),
        HOST_TEST(test_bounded_radio_deferral_and_can_retry),
        HOST_TEST(test_full_snapshot_and_latest_pending),
        HOST_TEST(test_temporary_pace_and_lease),
        HOST_TEST(test_oil_animation_can_pacing),
    };
    host_tests_run(DASHBOARD_MESSAGE_MAX_LENGTH == 24 ? "body-display-24" : "body-display-18", tests, sizeof(tests) / sizeof(tests[0]));
}
