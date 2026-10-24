/* Exercise the production BH selector with recorded input codes and CAN failures. */
#include "features/dna_selector.h"
#include "transport/board_uart.h"
#include "state/telemetry.h"
TelemetryState telemetry_state;
#include <assert.h>
#include <string.h>

static uint32_t now = 100, pressed_at;
static uint8_t position, pressed, changed, result, reply_token;
static uint8_t suspension_level, suspension_pressed;
static unsigned suspension_presses;
static uint8_t accept_suspension = 1, engine_reports = 1, suspension_reports = 1;
static unsigned count, replies, reject_can, reject_uart;
static uint8_t sent[80][4];
static uint32_t sent_at[80];
uint32_t HAL_GetTick(void) { return now; }

uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    assert(header->StdId == 0x256 && header->DLC == 4);
    assert(header->IDE == CAN_ID_STD && header->RTR == CAN_RTR_DATA);
    if (reject_can) { --reject_can; return HAL_ERROR; }
    assert(count < 80);
    memcpy(sent[count], data, 4);
    sent_at[count++] = now;
    uint8_t direction = data[0] & 0x60;
    if (direction != pressed) {
        pressed = direction;
        pressed_at = now;
        changed = 0;
    }
    if ((data[1] & 0x80) && !suspension_pressed) {
        ++suspension_presses;
        if (accept_suspension) suspension_level = position == 2 ? !suspension_level : 3 - suspension_level;
    }
    suspension_pressed = !!(data[1] & 0x80);
    return HAL_OK;
}

uint8_t board_uart_send(const uint8_t *data, size_t length) {
    assert(length == 4 && data[0] == C1BusID && data[1] == C1_CMD_DNA_RESULT);
    if (reject_uart) { --reject_uart; return 0; }
    reply_token = data[2]; result = data[3]; ++replies;
    return 1;
}

static void neutral(void) {
    CAN_RxHeaderTypeDef header = {.StdId = 0x256, .DLC = 4};
    /* Deliberately retain unrelated bits and the separate byte-1 button field. */
    uint8_t data[] = {0x85, 0x04, 0x12, 0x34};
    dna_selector_observe(&header, data);
}

static void report_mode(void) {
    static const uint8_t modes[] = {4, 0, 2, 12};
    dna_selector_mode(modes[position]);
    uint8_t status[] = {position == 2 ? 0x20 : position == 3 ? 0x04 : 0x40, suspension_level << 4};
    if (suspension_reports) dna_selector_suspension_status(status);
}

/* The fixture supplies delayed factory reports; it does not establish replay acceptance. */
static void tick(void) {
    now += 25;
    if (engine_reports) telemetry_state.current_rpm_updated_at = now;
    if (pressed && !changed) {
        unsigned delay = position == 2 && pressed == 0x40 ? 1200 : 110;
        if (now - pressed_at >= delay) {
            if (pressed == 0x40 && position < 3) ++position;
            if (pressed == 0x20 && position > 0) --position;
            suspension_level = position == 2 ? 1 : position == 3 ? 2 : 0;
            changed = 1;
        }
    }
    neutral();
    report_mode();
    dna_selector_process();
}

static void reset(uint8_t start) {
    dna_selector_configure(0, 2, 0, 0);
    dna_selector_process();
    count = replies = reject_can = reject_uart = 0;
    pressed = changed = 0;
    position = start;
    suspension_level = start == 2 ? 1 : start == 3 ? 2 : 0;
    suspension_presses = suspension_pressed = 0;
    accept_suspension = engine_reports = suspension_reports = 1;
    telemetry_state.current_rpm_speed = 0;
    dna_selector_process();
    telemetry_state.current_rpm_speed = 800;
    telemetry_state.current_rpm_updated_at = now;
    dna_selector_process();
    now += 2000;
    telemetry_state.current_rpm_updated_at = now;
    dna_selector_process();
}

/* Cover both calibrations from either starting state, plus rejected and stale feedback. */
static void test_suspension(void) {
    uint8_t request = 80;
    for (unsigned mode = 2; mode <= 3; ++mode) {
        for (unsigned softer = 0; softer <= 1; ++softer) {
            for (unsigned initially_softer = 0; initially_softer <= 1; ++initially_softer) {
                reset(mode);
                suspension_level = mode - 1 - initially_softer;
                dna_selector_configure(mode, 1, request++, softer);
                for (unsigned i = 0; i < 150 && !replies; ++i) tick();
                assert(replies == 1 && result == 1 && !suspension_pressed);
                assert(suspension_presses == (softer != initially_softer));
                assert(suspension_level == mode - 1 - softer);
                if (suspension_presses) {
                    assert(count == 2 && sent_at[1] - sent_at[0] == 150);
                    assert(sent[0][0] == 0x85 && sent[0][1] == 0x84 && sent[1][1] == 0x04);
                } else assert(count == 0);
            }
        }
    }
    reset(2); accept_suspension = 0;
    dna_selector_configure(2, 1, request++, 1);
    for (unsigned i = 0; i < 500 && !replies; ++i) tick();
    assert(result == 0 && suspension_presses == 1 && count == 2); /* Never toggle twice. */

    reset(2); suspension_reports = 0;
    dna_selector_configure(2, 1, request++, 1);
    for (unsigned i = 0; i < 500 && !replies; ++i) tick();
    assert(replies == 1 && result == 0 && count == 0);

    reset(2);
    dna_selector_configure(2, 1, request++, 1);
    tick();
    suspension_reports = 0;
    const uint8_t unknown[] = {0x20, 0x30};
    dna_selector_suspension_status(unknown);
    for (unsigned i = 0; i < 500 && !replies; ++i) tick();
    assert(result == 0 && count == 0); /* Unknown code invalidates a previously valid sample. */

    reset(2);
    dna_selector_configure(2, 1, request++, 1);
    while (!count) tick();
    reject_can = 1;
    now += 150; neutral(); report_mode(); dna_selector_process();
    assert(count == 1 && suspension_pressed);
    dna_selector_process();
    assert(count == 2 && !suspension_pressed);
    for (unsigned i = 0; i < 80 && !replies; ++i) tick();
    assert(result == 1 && suspension_presses == 1);

    reset(2);
    dna_selector_configure(2, 1, request++, 1);
    while (!count) tick();
    position = 1; report_mode(); dna_selector_process();
    assert(count == 2 && !suspension_pressed && result == 0); /* Mode changed during press. */

    for (unsigned button = 0; button < 2; ++button) {
        reset(2);
        dna_selector_configure(button ? 2 : 3, 1, request++, button);
        while (!count) tick();
        unsigned before = count;
        CAN_RxHeaderTypeDef header = {.StdId = 0x256, .DLC = 4};
        uint8_t physical[] = {button ? 0 : 0x20, button ? 0x80 : 0, 0, 0};
        dna_selector_observe(&header, physical);
        dna_selector_process();
        assert(count == before && result == 0); /* No synthetic release over the driver. */
        pressed = suspension_pressed = 0; /* The driver releases physically. */
    }
}

/* Cranking, missing RPM and a stopped engine cannot produce a late selector gesture. */
static void test_engine_gate(void) {
    reset(1);
    telemetry_state.current_rpm_speed = 400;
    dna_selector_process();
    dna_selector_configure(2, 1, 150, 0);
    for (unsigned i = 0; i < 20; ++i) tick();
    assert(count == 0 && replies == 0);
    telemetry_state.current_rpm_speed = 800;
    for (unsigned i = 0; i < 80; ++i) tick();
    assert(count == 0);
    tick();
    assert(count == 1);
    telemetry_state.current_rpm_speed = 0;
    tick();
    assert(count == 2 && pressed == 0 && replies == 1 && result == 0);

    reset(1); engine_reports = 0;
    telemetry_state.current_rpm_speed = 0;
    dna_selector_process();
    dna_selector_configure(3, 1, 151, 0);
    for (unsigned i = 0; i < 120; ++i) tick();
    assert(count == 0 && replies == 1 && result == 0);
    engine_reports = 1; telemetry_state.current_rpm_speed = 800;
    for (unsigned i = 0; i < 200; ++i) tick();
    assert(count == 0); /* Engine start after BH timeout cannot revive an old request. */
}

int main(void) {
    test_suspension();
    test_engine_gate();
    uint8_t token = 1;
    for (unsigned mode = 1; mode <= 3; ++mode) {
        for (unsigned initial = 0; initial < 4; ++initial) {
            reset(initial);
            uint8_t target = mode == 1 ? 0 : mode == 2 ? 2 : 3;
            dna_selector_configure(mode, 1, token, 0);
            for (unsigned elapsed = 0; elapsed < 11000 && !replies; elapsed += 25) tick();
            assert(replies == 1 && result == 1 && reply_token == token);
            assert(position == target && pressed == 0);
            for (unsigned i = 0; i < count; ++i) {
                assert((sent[i][0] & ~0x60) == 0x85);
                assert(sent[i][1] == 0x04 && sent[i][2] == 0x12 && sent[i][3] == 0x34);
            }
            if (initial == 1 && mode == 2) {
                assert(count == 2 && sent_at[1] - sent_at[0] == 200);
            }
            if (initial == 2 && mode == 3) {
                assert(count == 9 && sent_at[8] - sent_at[0] == 1800);
                for (unsigned i = 1; i < 8; ++i) {
                    assert((sent[i][0] & 0x60) == 0x40);
                    assert(sent_at[i] - sent_at[i - 1] == 250);
                }
            }
            unsigned before = count;
            dna_selector_configure(mode, 1, token, 0); /* Duplicate startup. */
            dna_selector_configure(mode, 0, token, 0); /* Transport-only resync. */
            for (unsigned i = 0; i < 20; ++i) tick();
            assert(count == before);
            ++token;
        }
    }

    reset(1);
    dna_selector_configure(0, 1, token++, 0); /* N leaves the car alone. */
    for (unsigned i = 0; i < 500; ++i) tick();
    assert(count == 0);

    reset(1);
    dna_selector_configure(2, 1, token++, 0);
    report_mode();
    CAN_RxHeaderTypeDef bad = {.StdId = 0x256, .DLC = 8};
    uint8_t raw[8] = {0};
    dna_selector_observe(&bad, raw);
    bad.DLC = 4; bad.IDE = CAN_ID_EXT;
    dna_selector_observe(&bad, raw);
    dna_selector_process();
    assert(count == 0); /* No fresh, valid neutral input. */
    bad.IDE = CAN_ID_STD; bad.RTR = CAN_RTR_REMOTE;
    dna_selector_observe(&bad, raw);
    dna_selector_process();
    assert(count == 0);
    neutral();
    reject_can = 1;
    dna_selector_process();
    assert(count == 0);
    dna_selector_process();
    assert(count == 1 && (sent[0][0] & 0x60) == 0x40);
    now += 199; neutral(); report_mode(); dna_selector_process();
    assert(count == 1);
    now += 1; neutral(); report_mode(); reject_can = 1; dna_selector_process();
    assert(count == 1);
    dna_selector_process();
    assert(count == 2 && (sent[1][0] & 0x60) == 0);
    assert(sent_at[1] - sent_at[0] == 200);
    for (unsigned i = 0; i < 80 && !replies; ++i) {
        now += 25; neutral(); report_mode(); dna_selector_process();
    }
    assert(replies == 1 && result == 0); /* Factory remains N: no second press. */
    assert(count == 2);

    reset(2);
    dna_selector_configure(3, 1, token++, 0);
    tick();
    for (unsigned i = 0; i < 10; ++i) tick();
    dna_selector_configure(0, 2, token, 0); /* OFF/save must release a started hold. */
    reject_can = 1;
    reject_uart = 1;
    dna_selector_process();
    assert(pressed == 0x40 && replies == 0);
    dna_selector_process();
    assert(pressed == 0 && replies == 0);
    dna_selector_process();
    assert(replies == 1 && result == 0);

    reset(1);
    dna_selector_configure(2, 1, token++, 0);
    bad.RTR = CAN_RTR_DATA;
    raw[0] = 0x20;
    dna_selector_observe(&bad, raw); /* Physical driver input wins. */
    dna_selector_process();
    assert(count == 0 && replies == 1 && result == 0);

    reset(2);
    dna_selector_configure(3, 1, token++, 0);
    tick();
    dna_selector_configure(3, 0, token, 0); /* Resync must not restart the hold clock. */
    now += 601;
    dna_selector_process();
    assert(count == 2 && pressed == 0 && replies == 1 && result == 0); /* Stale input. */

    reset(1);
    now = UINT32_MAX - 1000;
    telemetry_state.current_rpm_updated_at = now;
    dna_selector_configure(2, 1, token++, 0);
    for (unsigned i = 0; i < 500 && !replies; ++i) tick();
    assert(result == 1 && position == 2); /* Tick wrap. */

    reset(1);
    dna_selector_configure(2, 1, token++, 0);
    now += 10000;
    dna_selector_process();
    assert(count == 0 && replies == 1 && result == 0); /* Silent bus. */
    return 0;
}
