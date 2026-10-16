/* Included by test_body_display.c to exercise the real fragment queue and BH pacing. */
#include "features/oil_bar_animation.h"

static void oil_bar(uint8_t *line, unsigned width, int position) {
    unsigned left = width / 5, right = width - left - 1;
    memset(line, '-', width);
    memset(line + left + 1, '=', right - left - 1);
    line[left] = '['; line[right] = ']';
    if (position < 0) line[0] = '<';
    else if (position >= (int)width) line[width - 1] = '>';
    else line[position] = width == 16 || width == 22 ? 0x81 : '*';
}

/* Deliver one immutable step, retrying a rejected offer without advancing it. */
static void send_oil_step(DisplayStream *stream, const uint8_t *expected, bool restart) {
    assert(!memcmp(stream->active, expected, DISPLAY_OUTPUT_LENGTH));
    for (unsigned attempts = 0; attempts < 2 * DISPLAY_FRAGMENT_COUNT; ++attempts) {
        uint8_t part, text[3], retry_part, retry_text[3];
        assert(display_stream_peek(stream, &part, text));
        assert(!memcmp(text, expected + part * 3, 3));
        /* No accept means a failed CAN queue offer: the next offer is identical. */
        assert(display_stream_peek(stream, &retry_part, retry_text));
        assert(retry_part == part && !memcmp(text, retry_text, 3));
        bool final = stream->dirty == (1U << part);
        display_stream_accept(stream);
        if (final) {
            assert(!memcmp(stream->sent, expected, DISPLAY_OUTPUT_LENGTH));
            return;
        }
        if (restart && attempts == 0) display_stream_restart(stream);
        display_stream_refresh(stream);
        assert(!memcmp(stream->active, expected, DISPLAY_OUTPUT_LENGTH));
    }
    assert(!"oil step did not finish");
}

static void test_oil_animation_steps(void) {
    const unsigned widths[] = {16, 18, 22, 24};
    for (unsigned mode = 0; mode < 2; ++mode) {
        for (unsigned w = 0; w < 4; ++w) {
            unsigned width = widths[w], offset = width == 22 ? 17 : 0;
            DisplayStream stream = {0};
            uint8_t first[DISPLAY_OUTPUT_LENGTH], target[DISPLAY_OUTPUT_LENGTH], expected[DISPLAY_OUTPUT_LENGTH];
            memset(first, ' ', sizeof(first));
            if (offset) { memcpy(first, "Oil 77.0mm OK", 12); first[16] = '\r'; }
            else first[width] = '\r';
            oil_bar(first + offset, width, 10);
            display_stream_set_full(&stream, mode == 0);
            display_stream_submit(&stream, first);
            while (stream.dirty) {
                uint8_t part, text[3];
                assert(display_stream_peek(&stream, &part, text));
                display_stream_accept(&stream);
            }
            assert(!memcmp(stream.sent, first, sizeof(first))); /* No sweep on first appearance. */
            const int goals[] = {7, -1, 10, (int)width, 4};
            int position = 10;
            for (unsigned goal = 0; goal < sizeof(goals)/sizeof(goals[0]); ++goal) {
                memcpy(target, first, sizeof(target));
                oil_bar(target + offset, width, goals[goal]);
                display_stream_submit(&stream, target);
                while (position != goals[goal]) {
                    position += goals[goal] > position ? 1 : -1;
                    memcpy(expected, target, sizeof(expected));
                    oil_bar(expected + offset, width, position);
                    /* Repeated C1 updates must not replace the in-flight step. */
                    display_stream_submit(&stream, target);
                    send_oil_step(&stream, expected, position == 6);
                }
                assert(!stream.dirty && !stream.transferring);
            }
            /* A new target reverses direction after the current step completes. */
            oil_bar(target + offset, width, 10); display_stream_submit(&stream, target);
            memcpy(expected, target, sizeof(expected)); oil_bar(expected + offset, width, 5);
            oil_bar(target + offset, width, 2); display_stream_submit(&stream, target);
            send_oil_step(&stream, expected, false);
            for (int position = 4; position >= 2; --position) {
                oil_bar(expected + offset, width, position);
                send_oil_step(&stream, expected, false);
            }
            /* Missing samples remove the bar, with no invented intermediate value. */
            memset(target + offset, ' ', width); memcpy(target + offset, "Oil -- mm", 9);
            display_stream_submit(&stream, target);
            assert(!stream.animating);
            while (stream.dirty) {
                uint8_t part, text[3];
                assert(display_stream_peek(&stream, &part, text)); display_stream_accept(&stream);
            }
            assert(!memcmp(stream.sent, target, sizeof(target)));
        }
    }
}

static void test_oil_animation_both_lines(void) {
    uint8_t previous[DISPLAY_OUTPUT_LENGTH], next[DISPLAY_OUTPUT_LENGTH];
    oil_bar(previous, 16, 10); previous[16] = '\r'; oil_bar(previous + 17, 22, 0);
    oil_bar(next, 16, 7); next[16] = '\r'; oil_bar(next + 17, 22, 10);
    assert(oil_bar_animate(previous, next, sizeof(next)));
    uint8_t expected[DISPLAY_OUTPUT_LENGTH];
    oil_bar(expected, 16, 9); expected[16] = '\r'; oil_bar(expected + 17, 22, 1);
    assert(!memcmp(next, expected, sizeof(next)));
    /* Text that merely contains a bullet or asterisk must never animate. */
    memset(next, ' ', sizeof(next)); memcpy(next, "[Oil * reading]", 15);
    memcpy(expected, next, sizeof(next));
    assert(!oil_bar_animate(previous, next, sizeof(next)));
    assert(!memcmp(next, expected, sizeof(next)));
}

static void test_oil_animation_can_pacing(void) {
    chassis_state.stability_inverted = false;
    body_init(); transmitted_count = 0;
    uint8_t packet[MY23_PACKET_SIZE];
    memset(packet, ' ', sizeof(packet)); packet[0] = MY23_PACKET_MARKER;
    memcpy(packet + 1, "Oil 77.0mm OK", 12); oil_bar(packet + 17, 22, 10);
    body_display_submit(packet);
    for (unsigned i = 0; i < 13; ++i) { now += 20; body_process(); }
    assert(transmitted_count == 13);
    memcpy(packet + 1, "Oil 74.0mm OK", 12); oil_bar(packet + 17, 22, 6);
    body_display_submit(packet);
    for (int position = 9; position >= 6; --position) {
        unsigned start = transmitted_count;
        for (unsigned i = 0; i < 13; ++i) {
            now += 19; body_process(); assert(transmitted_count == start + i);
            now += 1; body_process(); assert(transmitted_count == start + i + 1);
        }
        uint8_t visible[DISPLAY_OUTPUT_LENGTH];
        for (unsigned i = 0; i < 13; ++i) {
            const uint8_t *frame = transmitted[start + i];
            assert(fragment(frame) == i);
            for (unsigned j = 0; j < 3; ++j)
                visible[i*3+j] = frame[2+j*2] == 0x20 && frame[3+j*2] == 0x22 ? 0x81 : frame[3+j*2];
        }
        uint8_t expected[22]; oil_bar(expected, 22, position);
        assert(!memcmp(visible + 17, expected, 22));
        assert(!memcmp(visible, "Oil 74.0mm OK", 12));
    }
}
