#ifndef BACCABLE_OIL_BAR_ANIMATION_H
#define BACCABLE_OIL_BAR_ANIMATION_H
#include <stdbool.h>
#include <stdint.h>
/* Move recognized oil bars one character from the previous screen toward the next. */
bool oil_bar_animate(const uint8_t *previous, uint8_t *next, unsigned length);
#endif
