#include "features/menu_demo.h"
#include "app/build_config.h"
#if defined(BACCABLE_C1)
/* Every selection gets six seconds; the controller never replays user actions. */
uint16_t menu_demo_duration(uint16_t step) { return menu_demo_page(step, false) ? 6000 : 0; }
void menu_demo_render(uint16_t step) { (void)menu_demo_page(step, true); }
#endif
