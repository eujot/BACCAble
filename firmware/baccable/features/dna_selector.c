#include "features/dna_selector.h"
#include "app/powertrain.h"

#if defined(BACCABLE_BH)
enum { SELECT_IDLE, SELECT_READY, SELECT_PRESS, SELECT_RELEASE, SELECT_WAIT,
       SELECT_SUSPENSION_PRESS, SELECT_SUSPENSION_RELEASE, SELECT_SUSPENSION_WAIT };
static uint8_t phase, target, expected, actual = 255, token, have_token;
static uint8_t data[4], have_input, physical_input, direction, steps;
static uint8_t reply_pending, reply_result, cancelled;
static uint8_t softer_suspension;
static uint8_t suspension_status_known, suspension_mode, suspension_is_softer;
static uint8_t engine_running, selection_started;
static uint32_t started, step_at, last_input, last_mode, last_press;
static uint32_t engine_running_since;
static uint32_t last_suspension_status, suspension_status_before_release;
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
    selection_started = 0;
    reply_result = result;
    reply_pending = 1;
}

/* A cancellation releases our pending press before ending the operation. */
void dna_selector_configure(uint8_t mode, uint8_t action, uint8_t request_token) {
    dna_selector_configure_default(mode, action, request_token, 0);
}

/* Start the saved drive mode and suspension preference for the next ignition. */
void dna_selector_configure_default(uint8_t mode, uint8_t action, uint8_t request_token,
                                    uint8_t softer_suspension_setting) {
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
        else if (phase == SELECT_SUSPENSION_PRESS || phase == SELECT_SUSPENSION_RELEASE)
            phase = SELECT_SUSPENSION_RELEASE;
        else {
            phase = SELECT_IDLE;
            selection_started = 0;
        }
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
    softer_suspension = softer_suspension_setting && (mode == 2 || mode == 3);
    phase = SELECT_READY;
    started = 0;
    selection_started = 0;
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
    /* The physical driver owns either selector or suspension input. */
    if ((physical_input || (frame[1] & 0x80)) && phase != SELECT_IDLE) {
        if (phase == SELECT_PRESS || phase == SELECT_RELEASE) phase = SELECT_RELEASE;
        else if (phase == SELECT_SUSPENSION_PRESS || phase == SELECT_SUSPENSION_RELEASE)
            phase = SELECT_SUSPENSION_RELEASE;
        else finish(0);
        cancelled = 1;
    }
}

/* Unknown factory values remain unknown rather than being treated as Natural. */
void dna_selector_mode(uint8_t mode) {
    actual = mode_position(mode);
    last_mode = currentTime;
}

/* Track whether the current mode reports its softer suspension calibration. */
void dna_selector_suspension_status(const uint8_t *frame) {
    switch (frame[0] & 0x7C) {
    case 0x40: suspension_mode = 1; break;
    case 0x20: suspension_mode = 2; break;
    case 0x04: suspension_mode = 3; break;
    default: return;
    }
    suspension_is_softer = (frame[1] & 0x10) != 0;
    suspension_status_known = 1;
    last_suspension_status = currentTime;
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

/* Press or release the suspension button without changing the DNA direction. */
static uint8_t send_suspension_button(uint8_t pressed) {
    CAN_TxHeaderTypeDef header = {.StdId = 0x256, .DLC = 4,
        .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .TransmitGlobalTime = DISABLE};
    uint8_t frame[4];
    memcpy(frame, data, 4);
    frame[1] = (frame[1] & (uint8_t)~0x80) | (pressed ? 0x80 : 0);
    return can_tx(&header, frame) == HAL_OK;
}

/* Match recorded short deflections and the longer Dynamic-to-Race hold. */
void dna_selector_process(void) {
    uint32_t now = currentTime;
    uint8_t engine_ready = telemetry_state.current_rpm_speed > 400 &&
                           now - telemetry_state.current_rpm_updated_at <= 1000U;
    if (engine_ready) {
        if (!engine_running) {
            engine_running = 1;
            engine_running_since = now;
        }
        engine_ready = now - engine_running_since >= 2000U;
    } else {
        engine_running = 0;
    }
    if (phase != SELECT_IDLE && !selection_started && engine_ready) {
        selection_started = 1;
        started = now;
    } else if (phase != SELECT_IDLE && selection_started && !engine_ready) {
        cancelled = 1;
        if (phase == SELECT_PRESS || phase == SELECT_RELEASE) phase = SELECT_RELEASE;
        else if (phase == SELECT_SUSPENSION_PRESS || phase == SELECT_SUSPENSION_RELEASE)
            phase = SELECT_SUSPENSION_RELEASE;
        else finish(0);
    }
    if (selection_started && phase != SELECT_IDLE && now - started >= 10000U) {
        cancelled = 1;
        if (phase == SELECT_PRESS || phase == SELECT_RELEASE) phase = SELECT_RELEASE;
        else if (phase == SELECT_SUSPENSION_PRESS || phase == SELECT_SUSPENSION_RELEASE)
            phase = SELECT_SUSPENSION_RELEASE;
        else finish(0);
    }
    if (selection_started && phase == SELECT_PRESS) {
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
    } else if (selection_started && phase == SELECT_WAIT) {
        if (now - last_mode > 1000U || now - step_at >= 1500U) finish(0);
        else if (actual == expected && now - step_at >= 300U && now - last_input <= 600U &&
                 last_input != step_at && last_input - step_at <= now - step_at)
            phase = SELECT_READY;
    } else if (selection_started && phase == SELECT_READY && have_input && !physical_input &&
               now - last_input <= 600U && now - last_mode <= 1000U && actual != 255) {
        if (actual == target) {
            if (target == 0 || target == 1 || !softer_suspension) finish(1);
            else if (suspension_status_known && suspension_mode == target &&
                     now - last_suspension_status <= 1000U) {
                if (suspension_is_softer == softer_suspension) finish(1);
                else if (send_suspension_button(1)) {
                    step_at = now;
                    phase = SELECT_SUSPENSION_PRESS;
                }
            }
        } else if (steps >= 3) finish(0);
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
    if (selection_started && phase == SELECT_SUSPENSION_PRESS) {
        if (cancelled || now - last_input > 600U) {
            cancelled = 1;
            phase = SELECT_SUSPENSION_RELEASE;
        } else if (now - step_at >= 150U) phase = SELECT_SUSPENSION_RELEASE;
    }
    if (phase == SELECT_SUSPENSION_RELEASE) {
        if (send_suspension_button(0)) {
            step_at = now;
            suspension_status_before_release = last_suspension_status;
            phase = SELECT_SUSPENSION_WAIT;
            if (cancelled) finish(0);
        }
    } else if (selection_started && phase == SELECT_SUSPENSION_WAIT) {
        if (now - step_at >= 1500U) finish(0);
        else if (last_suspension_status != suspension_status_before_release &&
                 now - last_suspension_status <= 1000U && suspension_mode == target &&
                 suspension_is_softer == softer_suspension) finish(1);
    }
    if (reply_pending) {
        uint8_t reply[] = {C1BusID, C1_CMD_DNA_RESULT, token, reply_result};
        if (board_uart_send(reply, sizeof(reply))) reply_pending = 0;
    }
}
#endif
