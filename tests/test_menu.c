#include "features/menu.h"
#include "features/display_stream.h"
#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include "diagnostics/parameter_request.h"
#include "storage/flash_records.h"
#include <assert.h>
#include <stdio.h>

SettingsState settings_state;
SecurityState security_state;
ChassisState chassis_state;
ComfortState comfort_state;
TelemetryState telemetry_state;
DiagnosticsState diagnostics_state;
RuntimeState runtime_state;
DashboardState dashboard_state;
StatisticsState statistics_state;
PedalState pedal_state;
MirrorsState mirrors_state;
DisplayState display_state;
const char *FW_VERSION = "BACCABLE test";
static uint32_t now;
static char screen[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
static uint8_t old_visibility[30], saved[MENU_PREFS_SIZE];
static bool have_old, have_saved, fail_save;
static unsigned commands, queries;
uint32_t HAL_GetTick(void) { return now; }
uint8_t board_uart_send(const uint8_t *data, size_t size) {
    if (data[0] == BhBusIDparamString) {
        assert(size == UART_BUFFER_SIZE);
        memcpy(screen, data + 1, DASHBOARD_MESSAGE_MAX_LENGTH);
        screen[DASHBOARD_MESSAGE_MAX_LENGTH] = 0;
    } else
        ++commands;
    return 1;
}
void pedal_booster_set_map(uint8_t map) { (void)map; }
void _putchar(char c) { (void)c; }
void status_led_error(void) {}
void status_led_activity(void) {}
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    (void)header;
    (void)data;
    ++queries;
    return HAL_OK;
}
float native_parameter_read(uint8_t id) { return (float)id + 0.5f; }
bool flash_record_load(unsigned slot, uint16_t type, void *data, size_t size) {
    (void)slot;
    if (type == 0x103 && have_old) {
        assert(size == sizeof(old_visibility));
        memcpy(data, old_visibility, size);
        return true;
    }
    if (type == 0x104 && have_saved) {
        assert(size == sizeof(saved));
        memcpy(data, saved, size);
        return true;
    }
    return false;
}
bool flash_record_save(unsigned slot, uint16_t type, const void *data, size_t size) {
    (void)slot;
    if (fail_save)
        return false;
    if (type == 0x104) {
        assert(size == sizeof(saved));
        memcpy(saved, data, size);
        have_saved = true;
    }
    return true;
}
static void test_input(void) {
    MenuInput input = {0};
    assert(menu_input_update(&input, 0x90, true, 0) == MENU_NONE); /* Require neutral after enable. */
    assert(menu_input_update(&input, 0x10, true, 10) == MENU_NONE);
    assert(menu_input_update(&input, 0x90, true, 20) == MENU_NONE);
    assert(menu_input_update(&input, 0x10, true, 50) == MENU_SELECT);
    assert(menu_input_update(&input, 0x10, true, 60) == MENU_NONE);
    assert(menu_input_update(&input, 0x90, true, 100) == MENU_NONE);
    for (unsigned t = 200; t < 900; t += 100)
        assert(menu_input_update(&input, 0x90, true, t) == MENU_NONE);
    assert(menu_input_update(&input, 0x90, true, 900) == MENU_BACK);
    assert(menu_input_update(&input, 0x90, true, 1000) == MENU_NONE);
    assert(menu_input_update(&input, 0x10, true, 1100) == MENU_NONE);
    assert(menu_input_update(&input, 0x18, true, 1150) == MENU_NEXT);
    assert(menu_input_update(&input, 0x18, true, 1200) == MENU_NONE);
    assert(menu_input_update(&input, 0x20, true, 1250) == MENU_NEXT_GROUP);
    menu_input_update(&input, 0x10, true, 1300);
    menu_input_update(&input, 0x90, true, 1350);
    assert(menu_input_update(&input, 0x10, true, 1800) == MENU_NONE); /* Lost input. */
    menu_input_update(&input, 0x90, true, 1850);
    menu_input_update(&input, 0x90, false, 1900);
    assert(menu_input_update(&input, 0x10, true, 1950) == MENU_NONE);
    memset(&input, 0, sizeof(input));
    uint32_t start = UINT32_MAX - 500;
    menu_input_update(&input, 0x10, true, start);
    menu_input_update(&input, 0x90, true, start + 1);
    for (unsigned t = 101; t < 801; t += 100)
        assert(menu_input_update(&input, 0x90, true, start + t) == MENU_NONE);
    assert(menu_input_update(&input, 0x90, true, start + 801) == MENU_BACK);
}
static void test_display(void) {
    DisplayStream stream = {0};
    uint8_t a[DASHBOARD_MESSAGE_MAX_LENGTH], b[sizeof(a)], c[sizeof(a)], output[sizeof(a)], part, chars[3];
    memset(a, 'A', sizeof(a));
    memset(b, 'B', sizeof(b));
    memset(c, 'C', sizeof(c));
    display_stream_submit(&stream, a);
    for (unsigned i = 0; i < sizeof(a) / 3; ++i) {
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        memcpy(output + i * 3, chars, 3);
        if (i == 1) {
            display_stream_submit(&stream, b);
            display_stream_submit(&stream, c);
        }
        display_stream_refresh(&stream); /* Factory text does not restart the view. */
        /* Failed CAN enqueue doesn't advance the stream. */
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        display_stream_accept(&stream);
    }
    assert(!memcmp(output, a, sizeof(a)));
    for (unsigned i = 0; i < sizeof(a) / 3; ++i) {
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        assert(chars[0] == 'C');
        display_stream_accept(&stream);
    }
    assert(!display_stream_peek(&stream, &part, chars));
    display_stream_refresh(&stream);
    assert(display_stream_peek(&stream, &part, chars) && part == 0 && chars[0] == 'C');
    display_stream_submit(&stream, b);
    display_stream_reset(&stream);
    assert(!display_stream_peek(&stream, &part, chars));
}
static void test_preferences(void) {
    MenuPreferences prefs, copy;
    menu_preferences_default(&prefs);
    uint8_t list[60], raw[MENU_PREFS_SIZE];
    for (unsigned e = 0; e < 2; ++e) {
        assert(menu_page_list(&prefs, e, 0, true, false, list) == 4);
        unsigned total = 0;
        for (unsigned g = 1; g < MENU_GROUPS; ++g)
            total += menu_page_list(&prefs, e, g, false, false, list);
        assert(total == menu_page_count(e));
        for (unsigned i = 0; i < menu_page_count(e); ++i) {
            const ParameterPage *page = &parameter_pages[e][i];
            assert(menu_page_index(e, page->id) == (int)i);
            assert((page->id & 0x7f) > 0 && (page->id & 0x7f) <= 64);
            assert(page->label && page->group > 0 && page->group < MENU_GROUPS);
            for (unsigned j = 0; j < i; ++j)
                assert(parameter_pages[e][j].id != page->id);
        }
    }
    uint16_t first = prefs.favorites[0][0], second = prefs.favorites[0][1];
    menu_favorite_move(&prefs, 0, first, 1);
    assert(prefs.favorites[0][0] == second && prefs.favorites[0][1] == first);
    menu_page_show(&prefs, 0, menu_page_index(0, first), false);
    assert(menu_page_list(&prefs, 0, 0, true, false, list) == 4); /* Hide != remove favorite. */
    prefs.alphabetical = 1;
    unsigned n = menu_page_list(&prefs, 0, 0, false, false, list);
    for (unsigned i = 1; i < n; ++i)
        assert(strcmp(parameter_pages[0][list[i - 1]].label, parameter_pages[0][list[i]].label) <= 0);
    menu_preferences_encode(&prefs, raw);
    assert(menu_preferences_decode(&copy, raw));
    assert(!memcmp(&prefs, &copy, sizeof(prefs)));
    assert(!menu_page_visible(&copy, 0, menu_page_index(0, first)));
    for (unsigned i = 0; i < menu_page_count(0); ++i)
        menu_page_show(&copy, 0, i, false);
    assert(menu_page_list(&copy, 0, 0, false, false, list) == 0);
    assert(menu_page_list(&copy, 0, 0, false, true, list) == menu_page_count(0));
    raw[0] = 255;
    assert(!menu_preferences_decode(&copy, raw));
}
static void fresh_menu(void) {
    settings_state.is_diesel_enabled = 0;
    dashboard_state.baccable_dashboard_menu_visible = 0;
    now = 1000;
    have_saved = false;
    have_old = false;
    fail_save = false;
    menu_init();
    menu_event(MENU_BACK);
}
static void to_settings(void) {
    menu_event(MENU_SELECT); /* Favorites -> root. */
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
}
static void test_controller(void) {
    fresh_menu();
    assert(dashboard_state.baccable_dashboard_menu_visible);
    assert(dashboard_state.dashboard_page_index == 3);
    menu_event(MENU_NEXT);
    uint8_t selected = dashboard_state.dashboard_page_index;
    menu_event(MENU_BACK);
    menu_event(MENU_BACK); /* Root, close + persist. */
    assert(!dashboard_state.baccable_dashboard_menu_visible && have_saved);
    menu_init();
    menu_event(MENU_BACK);
    assert(dashboard_state.dashboard_page_index == selected);
    fresh_menu();
    to_settings();
    menu_event(MENU_SELECT); /* Setup Save and back. */
    fail_save = true;
    menu_event(MENU_SELECT);
    assert(strstr(screen, "Save failed"));
    now += 1500;
    menu_render();
    assert(strstr(screen, "Save and back"));
    fail_save = false;
    menu_event(MENU_SELECT);
    assert(strstr(screen, "Saved"));
    now += 1500;
    menu_render();
    assert(strstr(screen, "Feature setup"));
    /* Import old visibility, retaining the existing settings record. */
    memset(old_visibility, 0xff, sizeof(old_visibility));
    old_visibility[0] &= ~2;
    have_saved = false;
    have_old = true;
    menu_init();
    assert(menu_preferences_save() == 0);
    MenuPreferences decoded;
    assert(menu_preferences_decode(&decoded, saved));
    assert(!menu_page_visible(&decoded, 0, 1) && !menu_page_visible(&decoded, 1, 1));
    fresh_menu();
    menu_show_parameter(23); /* Gasoline battery voltage UDS. */
    now = 2000;
    menu_process();
    assert(queries);
    const ParameterDefinition *p = &parameter_definitions[35];
    assert(p->value_offset + p->value_length <= 4);
    uint16_t did = ((p->request_data >> 16) & 0xff) << 8 | (p->request_data >> 24);
    uint8_t reply[8] = {7, 0x62, did >> 8, did, 0, 0, 0, 10};
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .ExtId = p->response_id, .DLC = 8};
    reply[3] ^= 1;
    parameter_request_receive(&header, reply);
    assert(isnan(parameter_cache_get(35, now)));
    reply[3] ^= 1;
    parameter_request_receive(&header, reply);
    assert(isfinite(parameter_cache_get(35, now)));
    now += 3001;
    menu_render();
    assert(strstr(screen, "--"));
    assert(isnan(parameter_cache_get(35, now)));
    parameter_cache_put(35, 14.2f, now);
    menu_render();
    assert(strstr(screen, "14.200"));
    parameter_cache_reset();
    assert(isnan(parameter_cache_get(35, now)));
    /* Data from an unrelated CAN frame cannot refresh an expired oil value. */
    parameter_cache_observe(0x4b2, 4);
    assert(isfinite(parameter_cache_get(5, now)));
    now += 3001;
    parameter_cache_observe(0x101, 3);
    assert(isnan(parameter_cache_get(5, now)));
    parameter_cache_observe(0x4b2, 3);
    assert(isnan(parameter_cache_get(5, now)));
}
static void test_navigation_regressions(void) {
    fresh_menu();
    menu_event(MENU_SELECT);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Parameter groups. */
    menu_event(MENU_SELECT); /* Engine group. */
    menu_event(MENU_NEXT);
    uint8_t selected = dashboard_state.dashboard_page_index;
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Temperatures. */
    menu_event(MENU_BACK);
    menu_event(MENU_PREVIOUS);
    menu_event(MENU_SELECT);
    assert(dashboard_state.dashboard_page_index == selected);

    fresh_menu();
    to_settings();
    menu_event(MENU_SELECT);
    menu_event(MENU_NEXT); /* A setting instead of Save and back. */
    fail_save = true;
    menu_event(MENU_BACK);
    assert(strstr(screen, "Save failed"));
    fail_save = false;
    menu_event(MENU_SELECT); /* Retry BACK, without toggling the setting. */
    now += 1500;
    menu_render();
    assert(strstr(screen, "Feature setup"));

    fresh_menu();
    settings_state.is_diesel_enabled = 1;
    menu_event(MENU_NEXT); /* A second input can arrive before menu_process. */
    assert(parameter_page_count == diesel_page_count);
    assert(dashboard_state.dashboard_page_index < diesel_page_count);
    assert(menu_parameters_active());
    settings_state.is_diesel_enabled = 0;
    menu_process();
    assert(parameter_page_count == gasoline_page_count && menu_parameters_active());

    fresh_menu();
    menu_show_parameter(23);
    now += 1000;
    menu_process();
    const ParameterDefinition *p = &parameter_definitions[35];
    uint16_t did = ((p->request_data >> 16) & 0xff) << 8 | (p->request_data >> 24);
    uint8_t reply[8] = {7, 0x62, did >> 8, did, 0, 0, 0, 10};
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .ExtId = p->response_id, .DLC = 8};
    menu_event(MENU_NEXT);
    parameter_request_receive(&header, reply);
    assert(isnan(parameter_cache_get(35, now))); /* Late answer from the previous page. */

    MenuPreferences prefs;
    menu_preferences_default(&prefs);
    memset(prefs.favorites, 0, sizeof(prefs.favorites));
    menu_page_show(&prefs, 0, 23, false);
    menu_preferences_encode(&prefs, saved);
    have_saved = true;
    dashboard_state.baccable_dashboard_menu_visible = 0;
    menu_init();
    menu_event(MENU_BACK);
    assert(strstr(screen, "No favorites"));
    menu_event(MENU_NEXT);
    assert(!menu_parameters_active());
    menu_show_parameter(23);
    assert(menu_parameters_active() && dashboard_state.dashboard_page_index == 23);
    assert(menu_preferences_save() == 0);
    assert(menu_preferences_decode(&prefs, saved));
    assert(!menu_page_visible(&prefs, 0, 23));
}
int main(void) {
    test_input();
    test_display();
    test_preferences();
    test_controller();
    test_navigation_regressions();
    puts("PASS: menu gestures, stable views, favorites, sorting, migration, save failure, UDS freshness");
}
