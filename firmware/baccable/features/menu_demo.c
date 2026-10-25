#include "features/menu_demo.h"
#include "features/menu.h"
#include "features/my23_ui.h"
#include "app/application_state.h"

#if defined(BACCABLE_C1)

typedef struct {
    UiMode mode;
    const char *first;
    const char *second;
    const char *legacy;
    uint16_t duration;
} DemoScreen;

/* These example values are display-only; no menu event or vehicle command is replayed. */
static const DemoScreen screens[] = {
    {UI_MODE_LIST, "Favorites", "Readings", "> Favorites", 1500},
    {UI_MODE_PARAMETER, "Oil level", "77 mm  OK", "Oil 77 mm OK", 2300},
    {UI_MODE_PARAMETER, "Oil level bar", "72 [====*---] 82", "72 [===*--] 82", 2300},
    {UI_MODE_LIST, "Readings", "Actions", "> Readings", 1400},
    {UI_MODE_LIST, "Engine", "Temperatures", "> Engine", 1500},
    {UI_MODE_PARAMETER, "Engine RPM", "2500 rpm", "RPM 2500", 2100},
    {UI_MODE_LIST, "Temperatures", "Battery", "> Temperatures", 1500},
    {UI_MODE_PARAMETER, "Oil / coolant", "88 C  87 C", "Oil 88 W 87 C", 2200},
    {UI_MODE_LIST, "Actions", "Settings", "> Actions", 1500},
    {UI_MODE_LIST, "Read BCM faults", "Clear DTCs", "> Read BCM faults", 2000},
    {UI_MODE_LIST, "Settings", "Information", "> Settings", 1500},
    {UI_MODE_LIST, "Features", "Favorites", "> Features", 1500},
    {UI_MODE_EDIT, "Rain lights", "OFF" MY23_BULLET "HOLD SAVE", "Rain lights: OFF", 1700},
    {UI_MODE_EDIT, "Rain lights", "ON" MY23_BULLET "HOLD SAVE", "Rain lights: ON", 2300},
    {UI_MODE_EDIT, "Default mode", "N" MY23_BULLET "HOLD SAVE", "Mode: N", 1700},
    {UI_MODE_EDIT, "Default mode", "D" MY23_BULLET "HOLD SAVE", "Mode: D", 2300},
    {UI_MODE_EDIT, "Suspension", "Stiff" MY23_BULLET "HOLD SAVE", "Suspension: Stiff", 1700},
    {UI_MODE_EDIT, "Suspension", "Soft" MY23_BULLET "HOLD SAVE", "Suspension: Soft", 2300},
    {UI_MODE_LIST, "Information", "Favorites", "> Information", 1600},
};

/* Give each screen enough time for one full IPC transfer and a short read. */
uint16_t menu_demo_duration(uint8_t step) {
    return step < sizeof(screens) / sizeof(screens[0]) ? screens[step].duration : 0;
}

/* Reuse normal packet formatting so the tour resembles ordinary navigation. */
void menu_demo_render(uint8_t step) {
    if (!menu_demo_duration(step))
        return;
    const DemoScreen *screen = &screens[step];
    if (settings_state.ipc_my23_is_installed)
        menu_present_view(screen->mode, screen->first, screen->second);
    else
        menu_present(screen->legacy);
}

#endif
