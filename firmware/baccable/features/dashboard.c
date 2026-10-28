#include "app/powertrain.h"
#include "features/menu.h"
#include "features/ui_entry.h"
#if defined(BACCABLE_C1)

/* Present the currently selected feature setting. */
void dashboard_send_setup(void) {
    setup_render_page(setup_dashboardPageIndex);
    if (setup_stage_active() || setup_present_my23()) return;
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    memcpy(text, dashboard_setup_screen, DASHBOARD_MESSAGE_MAX_LENGTH);
    text[DASHBOARD_MESSAGE_MAX_LENGTH] = 0;
    menu_present(text);
}

/* Remove BACCAble text from the dashboard. */
void dashboard_clear(void) { menu_present(""); }
#endif
