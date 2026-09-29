#include "features/menu_input.h"
#include <string.h>

#define INPUT_STREAM_TIMEOUT_MS 300U

#define REPEAT_DELAY_MS 500U
#define REPEAT_INTERVAL_MS 180U

/* Recognize one deliberate navigation gesture, with mutually exclusive clicks, pairs and holds. */
MenuEvent menu_input_update(MenuInput *input, uint8_t button, bool allowed, uint32_t now) {
    if (!allowed) {
        memset(input, 0, sizeof(*input));
        return MENU_NONE;
    }
    MenuEvent pending = (button == 0x10 || button == 0x90 || button == 0x50) ?
                        menu_input_poll(input, true, now) : MENU_NONE;
    if (input->reports_seen) {
        uint32_t gap = now - input->last_seen;
        if (gap > INPUT_STREAM_TIMEOUT_MS) {
            ++input->stream_gaps;
            if (gap > input->max_gap_ms)
                input->max_gap_ms = gap;
        }
    }
    ++input->reports_seen;
    if (input->armed && now - input->last_seen > INPUT_STREAM_TIMEOUT_MS) {
        input->armed = false; /* A lost release must never become an action. */
        input->button = 0;
        input->click_pending = false;
    }
    input->last_seen = now;
    if (button == 0x50)
        button = 0x90;
    if (button != input->button) {
        input->repeated_at = now;
        input->repeating = false;
    }
    if (!input->armed) {
        if (button == 0x10) {
            input->armed = true;
            input->button = button;
        }
        return pending;
    }
    if (button == 0x10) {
        bool select = input->button == 0x90 && !input->consumed;
        input->button = button;
        input->consumed = false;
        if (!select || now - input->started < MENU_CLICK_MIN_MS)
            return pending;
        if (input->click_pending && now - input->released_at <= MENU_DOUBLE_CLICK_MS) {
            input->click_pending = false;
            input->pair_block = true;
            input->paired_at = now;
            return MENU_BACK;
        }
        bool expired = input->click_pending;
        input->click_pending = true;
        input->released_at = now;
        return expired ? MENU_SELECT : pending;
    }
    if (button != input->button) {
        uint8_t previous = input->button;
        input->button = button;
        input->started = now;
        input->consumed = button == 0x90 && input->pair_block && now - input->paired_at <= MENU_DOUBLE_CLICK_MS;
        if (input->pair_block && now - input->paired_at > MENU_DOUBLE_CLICK_MS) input->pair_block = false;
        if (button != 0x90) { input->click_pending = false; input->pair_block = false; }
        if (button == 0x20 && (previous == 0x18 || previous == 0x10))
            return MENU_NEXT_GROUP;
        if (button == 0x00 && (previous == 0x08 || previous == 0x10))
            return MENU_PREVIOUS_GROUP;
        if (previous == 0x10 && button == 0x18)
            return MENU_NEXT;
        if (previous == 0x10 && button == 0x08)
            return MENU_PREVIOUS;
        return pending;
    }
    if (button == 0x90 && !input->consumed && now - input->started >= MENU_LONG_PRESS_MS) {
        input->consumed = true;
        input->click_pending = false;
        return MENU_HOLD;
    }
    return pending;
}

MenuEvent menu_input_repeat(MenuInput *input, bool allowed, uint32_t now) {
    if (!allowed || !input->armed || now - input->last_seen > INPUT_STREAM_TIMEOUT_MS)
        return MENU_NONE;
    if (input->button != 0x18 && input->button != 0x08)
        return MENU_NONE;
    uint32_t interval = input->repeating ? REPEAT_INTERVAL_MS : REPEAT_DELAY_MS;
    if (now - input->repeated_at < interval)
        return MENU_NONE;
    input->repeated_at = now;
    input->repeating = true;
    return input->button == 0x18 ? MENU_NEXT : MENU_PREVIOUS;
}

/* Resolve a single click only after its pair window; no delayed action after lost input. */
MenuEvent menu_input_poll(MenuInput *input, bool allowed, uint32_t now) {
    if (!allowed || !input->armed || now - input->last_seen > INPUT_STREAM_TIMEOUT_MS) {
        input->click_pending = false;
        return MENU_NONE;
    }
    if (input->button == 0x10 && input->click_pending &&
        now - input->released_at > MENU_DOUBLE_CLICK_MS) {
        input->click_pending = false;
        return MENU_SELECT;
    }
    return MENU_NONE;
}
