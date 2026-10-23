#include "features/dna_selector.h"
#include "app/powertrain.h"

#if defined(BACCABLE_BH)
enum { SELECT_IDLE, SELECT_READY, SELECT_PRESS, SELECT_RELEASE, SELECT_WAIT };
static uint8_t phase, target, expected, actual = 255, token, have_token;
static uint8_t data[4], have_input, physical_input, direction, steps;
static uint8_t reply_pending, reply_result, cancelled;
static uint32_t started, step_at, last_input, last_mode, last_press;
static uint16_t hold_ms;

/* Map the factory codes into the physical A/N/D/Race selector order. */
static uint8_t mode_position(uint8_t mode) {
    switch (mode) {
    case 4: return 0;
    case 0: return 1;
    case 2: return 2;
    case 12: return 3;
    default: return 255;
    }
}

/* Keep an outcome until the inter-board queue accepts it. */
static void finish(uint8_t result) {
    phase = SELECT_IDLE;
    reply_result = result;
    reply_pending = 1;
}

/* A cancellation releases our pending press before ending the operation. */
void dna_selector_configure(uint8_t mode, uint8_t action, uint8_t request_token) {
    if (action == 0) {
        if (have_token && request_token == token && phase == SELECT_IDLE) reply_pending = 1;
        return;
    }
    if (action != 1 && action != 2) return;
    if (action == 2 || mode == 0 || mode > 3) {
        cancelled = 1;
        reply_pending = 0;
        have_token = 0;
        if (phase == SELECT_PRESS || phase == SELECT_RELEASE) phase = SELECT_RELEASE;
        else phase = SELECT_IDLE;
        return;
    }
    if (have_token && request_token == token) {
        if (phase == SELECT_IDLE) reply_pending = 1;
        return;
    }
    /* A new request cannot replace a press that still needs releasing. */
    if (phase != SELECT_IDLE) return;
    token = request_token;
    have_token = 1;
    target = mode == 1 ? 0 : mode == 2 ? 2 : 3;
    phase = SELECT_READY;
    started = currentTime;
    steps = reply_pending = cancelled = 0;
    have_input = 0; /* Require a new neutral report after arming. */
    actual = 255;
}

/* Only BH 0x256/DLC 4 carries this input; preserve every unrelated bit. */
void dna_selector_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *frame) {
    if (header->StdId != 0x256 || header->DLC != 4 ||
        header->IDE != CAN_ID_STD || header->RTR != CAN_RTR_DATA) return;
    memcpy(data, frame, 4);
    have_input = 1;
    physical_input = frame[0] & 0x60;
    last_input = currentTime;
    /* The physical driver owns a reported deflection, including during our hold. */
    if (physical_input && phase != SELECT_IDLE) finish(0);
}

/* Unknown factory values remain unknown rather than being treated as Natural. */
void dna_selector_mode(uint8_t mode) {
    actual = mode_position(mode);
    last_mode = currentTime;
}

/* Queue a fresh copy of the observed input with only its direction changed. */
static uint8_t send_direction(uint8_t value) {
    CAN_TxHeaderTypeDef header = {.StdId = 0x256, .DLC = 4,
        .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .TransmitGlobalTime = DISABLE};
    uint8_t frame[4];
    memcpy(frame, data, 4);
    frame[0] = (frame[0] & (uint8_t)~0x60) | value;
    return can_tx(&header, frame) == HAL_OK;
}

/* Match recorded short deflections and the longer Dynamic-to-Race hold. */
void dna_selector_process(void) {
    uint32_t now = currentTime;
    if (phase != SELECT_IDLE && now - started >= 10000U) {
        cancelled = 1;
        if (phase == SELECT_PRESS || phase == SELECT_RELEASE) phase = SELECT_RELEASE;
        else finish(0);
    }
    if (phase == SELECT_PRESS) {
        if (now - step_at >= hold_ms) phase = SELECT_RELEASE;
        else if (now - last_input > 600U || now - last_mode > 1000U) {
            cancelled = 1;
            phase = SELECT_RELEASE;
        } else if (hold_ms > 200 && now - last_press >= 250U && send_direction(direction))
            last_press = now;
    }
    if (phase == SELECT_RELEASE) {
        if (send_direction(0)) {
            step_at = now;
            phase = SELECT_WAIT;
            if (cancelled) finish(0);
        }
    } else if (phase == SELECT_WAIT) {
        if (now - last_mode > 1000U || now - step_at >= 1500U) finish(0);
        else if (actual == expected && now - step_at >= 300U && now - last_input <= 600U &&
                 last_input != step_at && last_input - step_at <= now - step_at)
            phase = SELECT_READY;
    } else if (phase == SELECT_READY && have_input && !physical_input &&
               now - last_input <= 600U && now - last_mode <= 1000U && actual != 255) {
        if (actual == target) finish(1);
        else if (steps >= 3) finish(0);
        else {
            direction = actual < target ? 0x40 : 0x20;
            expected = actual < target ? actual + 1 : actual - 1;
            hold_ms = actual == 2 && expected == 3 ? 1800 : 200;
            if (send_direction(direction)) {
                ++steps;
                step_at = last_press = now;
                phase = SELECT_PRESS;
            }
        }
    }
    if (reply_pending) {
        uint8_t reply[] = {C1BusID, C1_CMD_DNA_RESULT, token, reply_result};
        if (board_uart_send(reply, sizeof(reply))) reply_pending = 0;
    }
}
#endif
