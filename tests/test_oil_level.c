/* Reuse the menu fixture to check oil thresholds, persistence and both favorite lines. */
#include "features/oil_level.h"
static void test_oil_level(void) {
    const int ranges[2][2][2] = {{{72,82},{67,77}},{{61,65},{79,84}}};
    const unsigned widths[] = {16,18,22,24};
    fresh_menu();
    assert(setup_read_flash_value(40, 0) == 0);
    assert(setup_read_flash_value(40, 1) == 1);
    assert(setup_read_flash_value(40, 2) == 2);
    assert(setup_read_flash_value(40, 3) == 0);
    assert(setup_read_flash_value(40, 0xffff) == 0);
    uint16_t settings[SETUP_FLASH_PARAM_BUFFER_SIZE] = {0};
    settings_state.drivetrain = 2;
    setup_fill_flash_params(settings);
    assert(settings[39] == 2);
    for (unsigned engine = 0; engine < 2; ++engine) {
        settings_state.is_diesel_enabled = engine;
        const ParameterPage *value = &parameter_pages[engine][menu_page_index(engine, engine ? 0x9a : 0x31)];
        const ParameterPage *bar = &parameter_pages[engine][menu_page_index(engine, engine ? 0xbd : 0x67)];
        assert(value->parameter_ids[0] == bar->parameter_ids[0]);
        for (unsigned drive = 1; drive <= 2; ++drive) {
            settings_state.drivetrain = drive;
            int minimum = ranges[engine][drive-1][0], maximum = ranges[engine][drive-1][1];
            const float levels[] = {minimum - 0.1f, minimum, maximum, maximum + 0.1f};
            const char *statuses[] = {"LOW", "OK", "OK", "HIGH"};
            for (unsigned sample = 0; sample < 4; ++sample) {
                for (unsigned w = 0; w < 4; ++w) {
                    char text[25];
                    unsigned width = widths[w], left = width/5, right = width-left-1;
                    assert(oil_level_format(value, levels[sample], width, text));
                    assert(strstr(text, statuses[sample]) && strlen(text) <= width);
                    assert(oil_level_format(bar, levels[sample], width, text));
                    assert(strlen(text) == width);
                    const char *marker = strchr(text, '*');
                    assert(marker);
                    unsigned position = marker-text;
                    assert(sample == 0 ? position < left : sample == 3 ? position > right : position >= left && position <= right);
                }
            }
            char text[25];
            oil_level_format(bar, 0, 22, text); assert(text[0] == '<');
            oil_level_format(bar, 200, 22, text); assert(text[21] == '>');
            oil_level_format(bar, NAN, 22, text); assert(!strcmp(text, "Oil -- mm"));
            oil_level_format(value, INFINITY, 16, text); assert(!strcmp(text, "Oil -- mm"));
        }
        char text[25];
        settings_state.drivetrain = 0;
        oil_level_format(bar, 75, 22, text); assert(!strcmp(text, "Set drivetrain"));
        oil_level_format(value, 75, 16, text); assert(strchr(text, '?'));
        MenuPreferences prefs, restored;
        uint8_t bytes[MENU_PREFS_SIZE];
        menu_preferences_default(&prefs);
        menu_page_show(&prefs, engine, menu_page_index(engine, bar->id), false);
        menu_preferences_encode(&prefs, bytes);
        assert(menu_preferences_decode(&restored, bytes));
        assert(!menu_page_visible(&restored, engine, menu_page_index(engine, bar->id)));
        assert(menu_page_visible(&restored, engine, menu_page_index(engine, value->id)));
    }
    settings_state.is_diesel_enabled = 0;
    settings_state.gasoline_v6 = 1;
    char text[25];
    oil_level_format(&parameter_pages[0][menu_page_index(0, 0x67)], 75, 22, text);
    assert(!strcmp(text, "No oil range"));
}
static void test_oil_favorites(void) {
    for (unsigned swap = 0; swap < 2; ++swap) {
        fresh_contract();
        assert(menu_preferences_save() == 0);
        saved[MENU_PREFS_SIZE] = 2;
        saved[MENU_PREFS_SIZE + 1] = 1;
        memset(saved + MENU_PREFS_SIZE + 2, FAVORITE_EMPTY, FAVORITE_STORAGE_SIZE - 2);
        saved[MENU_PREFS_SIZE + 2] = swap ? 0x67 : 0x31;
        saved[MENU_PREFS_SIZE + 3] = swap ? 0x31 : 0x67;
        settings_state.drivetrain = 1;
        settings_state.ipc_my23_is_installed = 1;
        menu_init();
        parameter_cache_put(89, 77, now);
        menu_render();
        char first[25], second[25];
        oil_level_format(&parameter_pages[0][menu_page_index(0, saved[MENU_PREFS_SIZE+2])], 77, 16, first);
        oil_level_format(&parameter_pages[0][menu_page_index(0, saved[MENU_PREFS_SIZE+3])], 77, 22, second);
        assert(strchr(swap ? first : second, '\x81'));
        uint8_t expected[MY23_PACKET_SIZE];
        my23_packet(expected, first, second);
        assert(!memcmp(screen_packet + 1, expected, sizeof(expected)));
        now += 10000;
        menu_render();
        my23_packet(expected, "Oil -- mm", "Oil -- mm");
        assert(!memcmp(screen_packet + 1, expected, sizeof(expected)));
    }
}
