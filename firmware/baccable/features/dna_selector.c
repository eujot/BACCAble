#include "features/dna_selector.h"
#include "app/powertrain.h"

#if defined(BACCABLE_BH)
enum { SELECT_IDLE, SELECT_ENGINE, SELECT_READY, SELECT_PRESS, SELECT_RELEASE, SELECT_WAIT };
static uint8_t phase, target, expected, actual = 255, token, have_token;
static uint8_t data[4], have_input, physical_input, direction, steps;
static uint8_t reply_pending, reply_result, cancelled, suspension_step;
static uint8_t softer_suspension, suspension_known, suspension_mode, suspension_level;
static uint8_t engine_running;
static uint32_t started, step_at, last_input, last_mode, last_press;
static uint32_t engine_running_since, last_suspension, suspension_stable_since;
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

/* Release an accepted synthetic press before stopping on cancellation or stale data. */
static void cancel(void) {
    cancelled = 1;
    if (phase == SELECT_PRESS || phase == SELECT_RELEASE) phase = SELECT_RELEASE;
    else finish(0);
}

/* Arm one startup preference, preserve it on resync, or cancel it on OFF/save. */
void dna_selector_configure(uint8_t mode, uint8_t action, uint8_t request_token,
                            uint8_t softer_suspension_setting) {
    if (action == 0) {
        if (have_token && request_token == token && phase == SELECT_IDLE) reply_pending = 1;
        return;
    }
    if (action != 1 && action != 2) return;
    if (action == 2 || mode == 0 || mode > 3) {
        if (phase != SELECT_IDLE) cancel();
        reply_pending = 0;
        have_token = 0;
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
    softer_suspension = !!softer_suspension_setting;
    phase = SELECT_ENGINE;
    started = currentTime;
    steps = reply_pending = cancelled = suspension_step = 0;
    have_input = suspension_known = 0;
    actual = 255; /* Require new input, mode and suspension reports after arming. */
}

/* Only BH 0x256/DLC 4 carries this input; a physical press takes full control. */
void dna_selector_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *frame) {
    if (header->StdId != 0x256 || header->DLC != 4 ||
        header->IDE != CAN_ID_STD || header->RTR != CAN_RTR_DATA) return;
    memcpy(data, frame, 4);
    have_input = 1;
    physical_input = (frame[0] & 0x60) || (frame[1] & 0x80);
    last_input = currentTime;
    /* Do not inject a release over the driver's press or repeat their other button. */
    if (physical_input && phase != SELECT_IDLE) finish(0);
}

/* Unknown factory values remain unknown rather than being treated as Natural. */
void dna_selector_mode(uint8_t mode) {
    actual = mode_position(mode);
    last_mode = currentTime;
}

/* Decode the calibration field: D uses 0/1, Race uses 1/2 (Race Mid needs vehicle validation). */
void dna_selector_suspension_status(const uint8_t *frame) {
    uint8_t mode = frame[0] & 0x7C;
    uint8_t level = (frame[1] >> 4) & 3;
    mode = mode == 0x20 ? 2 : mode == 0x04 ? 3 : 255;
    if ((mode != 2 && mode != 3) || (mode == 2 && level > 1) ||
        (mode == 3 && (level < 1 || level > 2))) {
        suspension_known = 0;
        return;
    }
    if (!suspension_known || suspension_mode != mode || suspension_level != level ||
        currentTime - last_suspension > 1000U)
        suspension_stable_since = currentTime;
    suspension_mode = mode;
    suspension_level = level;
    suspension_known = 1;
    last_suspension = currentTime;
}

/* Return the requested calibration within the selected drive mode. */
static uint8_t wanted_suspension(void) {
    return target == 2 ? (softer_suspension ? 0 : 1) : (softer_suspension ? 1 : 2);
}

/* Use only fresh, settled suspension feedback for the actual target mode. */
static uint8_t suspension_ready(void) {
    return suspension_known && suspension_mode == target && actual == target &&
           currentTime - last_suspension <= 1000U &&
           currentTime - suspension_stable_since >= 300U;
}

/* Copy the observed input and change only the field owned by the current gesture. */
static uint8_t send_input(uint8_t pressed) {
    CAN_TxHeaderTypeDef header = {.StdId = 0x256, .DLC = 4,
        .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .TransmitGlobalTime = DISABLE};
    uint8_t frame[4];
    memcpy(frame, data, 4);
    if (suspension_step)
        frame[1] = (frame[1] & (uint8_t)~0x80) | (pressed ? 0x80 : 0);
    else
        frame[0] = (frame[0] & (uint8_t)~0x60) | (pressed ? direction : 0);
    return can_tx(&header, frame) == HAL_OK;
}

/* Require fresh RPM above 400 for two seconds, independently of the C1 gate. */
static uint8_t engine_ready(void) {
    if (telemetry_state.current_rpm_speed <= 400 ||
        currentTime - telemetry_state.current_rpm_updated_at > 1000U) {
        engine_running = 0;
        return 0;
    }
    if (!engine_running) {
        engine_running = 1;
        engine_running_since = currentTime;
    }
    return currentTime - engine_running_since >= 2000U;
}

/* Perform bounded selector steps and at most one suspension toggle, then report the outcome. */
void dna_selector_process(void) {
    uint32_t now = currentTime;
    uint8_t running = engine_ready();
    if (phase == SELECT_ENGINE) {
        /* C1 has already seen a running engine; missing BH reports must not arm a late change. */
        if (now - started >= 3000U) finish(0);
        else if (running) {
            started = now;
            phase = SELECT_READY;
        }
    } else if (phase != SELECT_IDLE && (!running || now - started >= 10000U)) cancel();

    if (phase == SELECT_PRESS) {
        if (now - last_input > 600U || now - last_mode > 1000U ||
            (suspension_step && actual != target)) cancel();
        else if (now - step_at >= hold_ms) phase = SELECT_RELEASE;
        else if (hold_ms > 200 && now - last_press >= 250U && send_input(1)) last_press = now;
    }
    if (phase == SELECT_RELEASE) {
        if (send_input(0)) {
            step_at = now;
            phase = SELECT_WAIT;
            if (cancelled) finish(0);
        }
    } else if (phase == SELECT_WAIT) {
        if (now - last_mode > 1000U || actual == 255 || now - step_at >= 1500U) finish(0);
        else if (suspension_step) {
            if (actual != target) finish(0);
            else if (suspension_ready() && suspension_level == wanted_suspension() &&
                     last_suspension != step_at && last_suspension - step_at <= now - step_at)
                finish(1);
        } else if (actual == expected && now - step_at >= 300U && now - last_input <= 600U &&
                   last_input != step_at && last_input - step_at <= now - step_at)
            phase = SELECT_READY;
    } else if (phase == SELECT_READY && have_input && !physical_input &&
               now - last_input <= 600U && now - last_mode <= 1000U && actual != 255) {
        if (actual == target) {
            if (target == 0) finish(1);
            else if (suspension_ready()) {
                if (suspension_level == wanted_suspension()) finish(1);
                else {
                    suspension_step = 1;
                    hold_ms = 150;
                    if (send_input(1)) {
                        step_at = now;
                        phase = SELECT_PRESS;
                    }
                }
            }
        } else if (steps >= 3) finish(0);
        else {
            suspension_step = 0;
            direction = actual < target ? 0x40 : 0x20;
            expected = actual < target ? actual + 1 : actual - 1;
            hold_ms = actual == 2 && expected == 3 ? 1800 : 200;
            if (send_input(1)) {
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
