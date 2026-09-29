#ifndef BACCABLE_MENU_INPUT_H
#define BACCABLE_MENU_INPUT_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    MENU_NONE,
    MENU_NEXT,
    MENU_PREVIOUS,
    MENU_NEXT_GROUP,
    MENU_PREVIOUS_GROUP,
    MENU_SELECT,
    MENU_BACK,
    MENU_HOLD
} MenuEvent;
typedef struct {
    uint32_t started, last_seen, repeated_at, released_at, paired_at;
    uint32_t reports_seen, stream_gaps, max_gap_ms;
    uint8_t button;
    bool armed, consumed, repeating, click_pending, pair_block;
} MenuInput;
#define MENU_DOUBLE_CLICK_MS 280U
#define MENU_LONG_PRESS_MS 900U
#define MENU_CLICK_MIN_MS 30U
MenuEvent menu_input_poll(MenuInput *input, bool allowed, uint32_t now);
MenuEvent menu_input_update(MenuInput *input, uint8_t button, bool allowed, uint32_t now);
MenuEvent menu_input_repeat(MenuInput *input, bool allowed, uint32_t now);
#endif
