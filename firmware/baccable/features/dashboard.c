#include "app/powertrain.h"
#include "features/menu.h"
#if defined(BACCABLE_C1)
void dashboard_send_main(void) { menu_render(); }
void dashboard_send_setup(void) {
    setup_render_page(setup_dashboardPageIndex);
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    memcpy(text, dashboard_setup_menu_array[setup_dashboardPageIndex], DASHBOARD_MESSAGE_MAX_LENGTH);
    text[DASHBOARD_MESSAGE_MAX_LENGTH] = 0;
    menu_present(text);
}
void dashboard_send_parameter_setup(void) { menu_render(); }
void dashboard_send_values() {
    runtime_state.uart_tx_msg[0] =
        BhBusIDparamString; // first char shall be a # to talk with slave canable connected to BH can bus

    char stringToPrint[25];
    dashboard_format_values(
        parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index].name,
        displayed_parameter_values,
        parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index].parameter_ids,
        stringToPrint); // build string to print

    menu_present(stringToPrint);
}

void dashboard_clear(void) { menu_present(""); }
#endif
