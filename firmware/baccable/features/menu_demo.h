#ifndef BACCABLE_MENU_DEMO_H
#define BACCABLE_MENU_DEMO_H

#include <stdint.h>

/* Return zero after the final display-only tour screen. */
uint16_t menu_demo_duration(uint8_t step);

/* Present one tour screen without entering an editor or changing vehicle state. */
void menu_demo_render(uint8_t step);

#endif
