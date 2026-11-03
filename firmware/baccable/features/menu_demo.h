#ifndef BACCABLE_MENU_DEMO_H
#define BACCABLE_MENU_DEMO_H

#include <stdint.h>
#include <stdbool.h>

/* Return zero after the final read-only tour screen. */
uint16_t menu_demo_duration(uint16_t step);

/* Present live values or isolated editor previews without changing vehicle state. */
void menu_demo_render(uint16_t step);

bool menu_demo_page(uint16_t step, bool render);
#endif
