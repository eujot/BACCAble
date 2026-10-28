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

/* Display-only tour of the new vehicle-profile / Readings / Favorites structure.
 * No menu event or vehicle command is replayed or saved. */
static const DemoScreen screens[] = {
    {UI_MODE_LIST, "Favorites", "Readings", "> Favorites", 1500},
    {UI_MODE_PARAMETER, "Warm-up", "Oil 120 C  Cool 90 C", "Oil 120 Cool 90 C", 2300},
    {UI_MODE_LIST, "Readings", "Actions", "> Readings", 1400},
    {UI_MODE_LIST, "Oil", "Temperatures", "> Oil", 1500},
    {UI_MODE_PARAMETER, "Oil temp", "120 C", "Oil 120 C", 2100},
    {UI_MODE_PARAMETER, "Oil level bar", "72 [====*---] 82", "72 [===*--] 82", 2300},
    {UI_MODE_LIST, "Temperatures", "Engine", "> Temperatures", 1500},
    {UI_MODE_PARAMETER, "Coolant", "90 C", "Coolant 90 C", 2100},
    {UI_MODE_LIST, "Engine", "Fuel", "> Engine", 1500},
    {UI_MODE_PARAMETER, "Power / boost", "280 PS  1.4 bar", "Pwr 280 Bst 1.4", 2200},
    {UI_MODE_LIST, "Battery", "Diesel / DPF", "> Battery", 1500},
    {UI_MODE_LIST, "Driving", "Performance", "> Driving", 1500},
    {UI_MODE_LIST, "Actions", "Settings", "> Actions", 1500},
    {UI_MODE_LIST, "Read BCM faults", "Clear DTCs", "> Read BCM faults", 2000},
    {UI_MODE_LIST, "Settings", "Information", "> Settings", 1500},
    {UI_MODE_LIST, "Vehicle", "Features", "> Vehicle", 1500},
    {UI_MODE_LIST, "Model", "Model year", "> Model", 1500},
    {UI_MODE_EDIT, "Engine", "2.0 Petrol" MY23_BULLET "HOLD SAVE", "Engine: 2.0 Petrol", 1700},
    {UI_MODE_EDIT, "Drivetrain", "RWD" MY23_BULLET "HOLD SAVE", "Drivetrain: RWD", 1700},
    {UI_MODE_EDIT, "Display", "MY23" MY23_BULLET "HOLD SAVE", "Display: MY23", 1700},
    {UI_MODE_LIST, "Equipment", "Review / Save", "> Equipment", 1500},
    {UI_MODE_LIST, "Features", "Favorites", "> Features", 1500},
    {UI_MODE_LIST, "Favorites", "BACCAble IPC", "> Favorites", 1500},
    {UI_MODE_EDIT, "Favorite 1 - A", "Oil  120 C" MY23_BULLET "HOLD SAVE", "Oil 120 C", 1700},
    {UI_MODE_LIST, "Restore defaults", "Favorite 1", "> Restore defaults", 1800},
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
