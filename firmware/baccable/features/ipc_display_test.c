#include "features/ipc_display_test.h"

#define IPC_TEST_CAPACITY 96U /* CAN 0x090 carries at most 32 three-character fragments. */

static uint16_t characters[IPC_TEST_CAPACITY];
static uint32_t last_restart;
static uint32_t last_command;
static uint8_t length, next_fragment, source_code, pattern_index;
static bool active;

static void append(uint16_t value) {
    if (length < IPC_TEST_CAPACITY)
        characters[length++] = value;
}

static void append_ascii(const char *text) {
    while (*text)
        append((uint8_t)*text++);
}

static void append_ruler(void) {
    for (unsigned position = 1; position <= 48; ++position)
        append(position % 10 ? '0' + position % 10 : 'A' + position / 10 - 1);
}

/* Keep the exact code points in one place so the observed glyphs can be compared. */
static void prepare(uint8_t pattern) {
    length = 0;
    if (pattern == 0) {
        append_ascii("UTF A: ");
        append(0x00B0); append(' '); append(0x00B1); append(' '); append(0x00D7);
        append(' '); append(0x0104); append(' '); append(0x0141);
        append('\r');
        append_ascii("UTF B: ");
        append(0x2022); append(' '); append(0x20AC); append(' '); append(0x03A9);
        append(' '); append(0x0416); append(' '); append(0x4E2D);
    } else if (pattern == 1) {
        append_ruler();
        append('\r');
        append_ascii("L1: 48 chars");
    } else if (pattern == 2) {
        append_ascii("L2: 48 chars");
        append('\r');
        append_ruler();
    } else {
        append_ascii("TOP:12345678901234567890");
        append('\r');
        append_ascii("BOTTOM:12345678901234567");
    }
}

bool ipc_display_test_select(uint8_t source, uint8_t pattern, uint32_t now) {
    if ((source != 0x06 && source != 0x09 && source != 0x21) || pattern >= IPC_TEST_PATTERN_COUNT)
        return false;
    last_command = now;
    if (active && source_code == source && pattern_index == pattern)
        return true; /* Repeated C1 updates must not interrupt a long transfer. */
    source_code = source;
    pattern_index = pattern;
    prepare(pattern);
    next_fragment = 0;
    last_restart = now;
    active = true;
    return true;
}

void ipc_display_test_stop(void) { active = false; }
bool ipc_display_test_active(void) { return active; }
bool ipc_display_test_expired(uint32_t now) { return active && now - last_command >= IPC_TEST_LEASE_MS; }

void ipc_display_test_restart(uint32_t now) {
    if (active) {
        next_fragment = 0;
        last_restart = now;
    }
}

void ipc_display_test_refresh(uint32_t now) {
    if (active && next_fragment >= (length + 2U) / 3U && now - last_restart >= IPC_TEST_REFRESH_MS)
        ipc_display_test_restart(now);
}

bool ipc_display_test_peek(uint8_t frame[8]) {
    if (!active || next_fragment >= (length + 2U) / 3U)
        return false;
    uint8_t total = (length + 2U) / 3U - 1U;
    frame[0] = (total << 3) | (next_fragment >> 2);
    frame[1] = ((next_fragment & 3U) << 6) | source_code;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned position = next_fragment * 3U + i;
        uint16_t value = position < length ? characters[position] : 0;
        frame[2 + i * 2] = value >> 8;
        frame[3 + i * 2] = value & 0xFF;
    }
    return true;
}

void ipc_display_test_accept(void) {
    if (active && next_fragment < (length + 2U) / 3U)
        ++next_fragment;
}
