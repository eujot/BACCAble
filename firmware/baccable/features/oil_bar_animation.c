#include "features/oil_bar_animation.h"

/* Return the background character at a position in a full-width oil bar. */
static uint8_t bar_background(unsigned position, unsigned width) {
    unsigned left = width / 5, right = width - left - 1;
    if (position == left) return '[';
    if (position == right) return ']';
    return position > left && position < right ? '=' : '-';
}

/* Recognize the complete bar, including its single marker or off-scale arrow. */
static int bar_position(const uint8_t *line, unsigned width, uint8_t marker) {
    int position = -2;
    for (unsigned i = 0; i < width; ++i) {
        if (line[i] == bar_background(i, width)) continue;
        if (position != -2) return -2;
        if (line[i] == marker) position = i;
        else if (i == 0 && line[i] == '<') position = -1;
        else if (i == width - 1 && line[i] == '>') position = width;
        else return -2;
    }
    return position;
}

/* Replace the target point with its next adjacent position, leaving other text intact. */
static bool animate_line(const uint8_t *previous, uint8_t *next, unsigned width) {
    if (width != 16 && width != 18 && width != 22 && width != 24) return false;
    uint8_t marker = width == 16 || width == 22 ? 0x81 : '*';
    int before = bar_position(previous, width, marker);
    int target = bar_position(next, width, marker);
    if (before == -2 || target == -2 || before == target) return false;
    int position = before + (target > before ? 1 : -1);
    for (unsigned i = 0; i < width; ++i) next[i] = bar_background(i, width);
    if (position < 0) next[0] = '<';
    else if (position >= (int)width) next[width - 1] = '>';
    else next[position] = marker;
    return true;
}

/* Animate each matching line only after the preceding screen has reached the CAN queue. */
bool oil_bar_animate(const uint8_t *previous, uint8_t *next, unsigned length) {
    bool animated = false;
    unsigned start = 0;
    for (unsigned end = 0; end <= length; ++end) {
        if (end != length && next[end] != '\r') continue;
        if (end == length || previous[end] == '\r')
            animated |= animate_line(previous + start, next + start, end - start);
        start = end + 1;
    }
    return animated;
}
