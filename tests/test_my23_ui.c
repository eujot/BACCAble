/* Production render, draft, input and migration coverage, included by test_menu.c. */
static void test_my23_glyph_budget(void) {
    const char *symbols = "›•▲▼✓×°±→…";
    const uint16_t expected[] = {0x203a, 0x2022, 0x25b2, 0x25bc, 0x2713, 0xd7, 0xb0, 0xb1, 0x2192, 0x2026};
    uint8_t encoded[16];
    memset(encoded, 'X', sizeof(encoded));
    assert(my23_text_encode(encoded, 10, symbols) == 10);
    for (unsigned i = 0; i < 10; ++i) assert(my23_codepoint(encoded[i]) == expected[i]);
    assert(encoded[10] == 'X');
    for (unsigned limit = 0; limit <= 10; ++limit) {
        memset(encoded, 'X', sizeof(encoded));
        assert(my23_text_encode(encoded, limit, symbols) == limit);
        assert(encoded[limit] == 'X');
    }
    uint8_t packet[MY23_PACKET_SIZE];
    my23_packet_mode(packet, UI_MODE_LIST, "123456789012345", "");
    assert(packet[1] == 0x80 && packet[MY23_L1_VISIBLE] == 0x87 &&
           packet[1 + MY23_L1_VISIBLE] == ' ');
    my23_packet_mode(packet, UI_MODE_LIST, "12345678901234", "next");
    assert(!memcmp(packet + 3, "12345678901234", 14) &&
           !memcmp(packet + 1 + MY23_L1_VISIBLE, "next", 4));
    my23_packet(packet, "1234567890123456EXTRA", "1234567890123456789012EXTRA");
    assert(!memcmp(packet + 1, "1234567890123456", MY23_L1_VISIBLE));
    assert(!memcmp(packet + 1 + MY23_L1_VISIBLE, "1234567890123456789012", 22));
    my23_packet(packet, "Gear", "RPM 3500");
    assert(packet[5] == ' ' && !memcmp(packet + 1 + MY23_L1_VISIBLE, "RPM 3500", 8));
    for (unsigned i = 5; i < 1 + MY23_L1_VISIBLE; ++i) assert(packet[i] == ' ');
}
/* MY23 Readings keeps each page's measurements and previews the next page. */
static void test_my23_reading_formats_fit(void) {
    const float samples[][4] = {
        {NAN, NAN, NAN, NAN}, {0, 0, 0, 0}, {3, 3, 3, 3},
        {8, 8, 8, 8}, {16, 16, 16, 16}, {48, 48, 48, 48}, {65, 65, 65, 65},
        {9.9f, 9.9f, 9.9f, 9.9f},
        {99.99f, 99.99f, 99.99f, 99.99f}, {101, 101, 101, 101},
        {999.99f, 999.99f, 999.99f, 999.99f}, {999999, 999999, 999999, 999999},
        {-9.9f, -9.9f, -9.9f, -9.9f}, {90, 85, 40, 35}, {5.5f, 80, 0, 0},
        {35, 80, 0, 0}, {8.54f, 8.54f, 8.54f, 8.54f}, {41, 41, 41, 41}
    };
    for (unsigned engine = 0; engine < 2; ++engine)
        for (unsigned i = 0; i < menu_page_count(engine); ++i) {
            const ParameterPage *page = &parameter_pages[engine][i];
            for (unsigned s = 0; s < sizeof(samples) / sizeof(samples[0]); ++s) {
                char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
                dashboard_format_my23_page(page, samples[s], text);
                size_t length = strlen(text);
                assert(length <= MY23_L1_VISIBLE);
                assert(!length || (text[0] != ' ' && text[length - 1] != ' '));
                assert(!strstr(text, "  "));
            }
            /* Fill every numeric placeholder to its declared width at once. */
            float fullest[4] = {0};
            unsigned element = 0;
            for (const char *part = page->name; *part && element < parameter_page_elements(page); ++part) {
                if (!strncmp(part, "$enum", 5)) {
                    fullest[element++] = 3; /* Long DPF state: NSC De-NOx. */
                    part += 4;
                } else if (part[0] == '$' && part[1] >= '0' && part[1] <= '9' &&
                           part[2] == '.' && part[3] >= '0' && part[3] <= '9' && part[4] == 'f') {
                    float value = 0;
                    for (unsigned digit = 0; digit < (unsigned)(part[1] - '0'); ++digit)
                        value = value * 10 + 9;
                    fullest[element++] = value + (part[3] != '0' ? 0.5f : 0);
                    part += 4;
                }
            }
            char full_text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
            dashboard_format_my23_page(page, fullest, full_text);
            if (strlen(full_text) > MY23_L1_VISIBLE)
                fprintf(stderr, "MY23 overflow: %u/%02x %s = %s (%zu)\n",
                        engine, page->id, page->label, full_text, strlen(full_text));
            assert(strlen(full_text) <= MY23_L1_VISIBLE);
        }
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    const float oil_quality[] = {35, 80, 0, 0};
    dashboard_format_my23_page(&parameter_pages[1][0x85 - 0x81], oil_quality, text);
    assert(!strcmp(text, "Oil 35.0mm Q 80%"));
    const float four_temps[] = {90, 85, 40, 35};
    dashboard_format_my23_page(&parameter_pages[0][0x39 - 1], four_temps, text);
    assert(!strcmp(text, "O 90W 85I 40X 35"));
    const float oil_coolant[] = {101, 88, 0, 0};
    dashboard_format_my23_page(&parameter_pages[0][0x04 - 1], oil_coolant, text);
    assert(!strcmp(text, "Oil101\xb0 Cool 88\xb0"));
    const float best[] = {8.54f, 8.54f, 0, 0};
    dashboard_format_my23_page(&parameter_pages[0][0x2d - 1], best, text);
    assert(!strcmp(text, "B 100-200 8.54s"));
}

static void test_my23_readings_preview_and_unavailable_gear(void) {
    fresh_contract();
    settings_state.ipc_my23_is_installed = 1;
    int gear = menu_page_index(0, 0x1a);
    assert(gear >= 0);
    parameter_cache_put(6, 15, now); /* 0xF from CAN means unavailable. */
    menu_show_parameter((uint8_t)gear); menu_render();
    assert(!memcmp(screen_packet + 2, "Gear -", 6));
    MenuPreferences prefs;
    menu_preferences_default(&prefs);
    uint8_t pages[64];
    unsigned count = menu_page_list_filtered(&prefs, 0, parameter_pages[0][gear].group,
                                             false, false, false, false, pages);
    unsigned selected = 0;
    while (selected < count && pages[selected] != gear) ++selected;
    assert(selected < count && count > 1);
    const char *next = parameter_pages[0][pages[(selected + 1) % count]].label;
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, next, strlen(next) < 22 ? strlen(next) : 22));
    int coolant = menu_page_index(0, 0x20);
    assert(coolant >= 0);
    parameter_cache_put(42, 88, now);
    menu_show_parameter((uint8_t)coolant); menu_render();
    assert(!memcmp(screen_packet + 2, "Coolant 88", 10));
    int pair = menu_page_index(0, 0x04); /* Oil and coolant must both be visible. */
    assert(pair >= 0);
    parameter_cache_put(5, 101, now);
    menu_show_parameter((uint8_t)pair); menu_render();
    assert(!memcmp(screen_packet + 2, "Oil", 3));
    assert(screen_packet[2 + MY23_L1_VISIBLE] != ' '); /* The next page, never a split value. */
    int four = menu_page_index(0, 0x37);
    assert(four >= 0);
    for (unsigned i = 0; i < 4; ++i) parameter_cache_put(91 + i, i + 1, now);
    menu_show_parameter((uint8_t)four); menu_render();
    char expected[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    const float counts[] = {1, 2, 3, 4};
    dashboard_format_my23_page(&parameter_pages[0][four], counts, expected);
    assert(!memcmp(screen_packet + 2, expected, strlen(expected)));
    for (unsigned i = strlen(expected); i < MY23_L1_VISIBLE; ++i)
        assert(screen_packet[2 + i] == ' ');
    count = menu_page_list_filtered(&prefs, 0, parameter_pages[0][four].group,
                                    false, false, false, false, pages);
    selected = 0;
    while (selected < count && pages[selected] != four) ++selected;
    assert(selected < count && count > 1);
    next = parameter_pages[0][pages[(selected + 1) % count]].label;
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, next, strlen(next) < 22 ? strlen(next) : 22));
    menu_event(MENU_BACK); menu_event(MENU_BACK); /* Return to root. */
    menu_event(MENU_PREVIOUS); menu_event(MENU_SELECT); /* Information. */
    for (unsigned i = 0; i < 5; ++i) menu_event(MENU_NEXT);
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Gaps", 4));
    for (unsigned i = 0; i < 4; ++i) menu_event(MENU_NEXT);
#ifdef MENU_DIAGNOSTICS
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "IPC diag", 8));
    menu_event(MENU_NEXT);
#endif
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "C1 firmware", 11));
}

static void test_atomic_favorite_packing(void) {
    FavoriteParameters favorite = {{6, 97, 7, 25, FAVORITE_EMPTY}};
    parameter_cache_reset(); parameter_peak_enable(false);
    parameter_cache_put(6, 3, now); parameter_cache_put(97, 3500, now);
    parameter_cache_put(7, 100, now); parameter_cache_put(25, -0.5f, now);
    char first[MY23_L1_VISIBLE + 1], second[MY23_L2_VISIBLE + 1];
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strcmp(first, "Gear 3"));
    assert(!strcmp(second, "Engine RPM 3500"));
    favorite = (FavoriteParameters){{5, 42, 22, 32, 33}};
    const uint8_t ids[] = {5, 42, 22, 32, 33};
    const float values[] = {101, 88, 42, 76, 90};
    for (unsigned i = 0; i < 5; ++i) parameter_cache_put(ids[i], values[i], now);
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strcmp(first, "Oil temp 101\xb0"));
    assert(!strcmp(second, "Coolant temp 88\xb0"));
    /* Historical slots 3–5 stay stored but never crowd the two visible values. */
    favorite.params[2] = favorite.params[3] = favorite.params[4] = FAVORITE_EMPTY;
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strcmp(second, "Coolant temp 88\xb0"));
    favorite.params[1] = FAVORITE_EMPTY;
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!second[0]);
    favorite.params[1] = 42;
    favorite_parameters_render(0, &favorite, now + 3001, first, second);
    assert(!strcmp(first, "Oil temp --") && !strcmp(second, "Coolant temp --"));
    parameter_cache_put(6, 15, now); /* Raw 0xF means unavailable, not gear 15. */
    favorite.params[0] = 6;
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strcmp(first, "Gear --"));
    assert(favorite_parameter_supported(0, false, 32));
    assert(!favorite_parameter_supported(0, true, 32));
    assert(!favorite_parameter_supported(1, false, 32));
    assert(favorite_parameter_supported(0, false, 97));
}
static void test_whole_page_favorites_and_beta19_migration(void) {
    fresh_contract();
    assert(menu_preferences_save() == 0);
    saved[MENU_PREFS_SIZE] = 2;
    saved[MENU_PREFS_SIZE + 1] = 1;
    memset(saved + MENU_PREFS_SIZE + 2, FAVORITE_EMPTY, FAVORITE_STORAGE_SIZE - 2);
    saved[MENU_PREFS_SIZE + 2] = 0x04; /* Oil/coolant is one complete reading. */
    saved[MENU_PREFS_SIZE + 3] = 0x07; /* Battery V/A is another reading. */
    settings_state.ipc_my23_is_installed = 1;
    menu_init();
    parameter_cache_put(5, 101, now); parameter_cache_put(42, 88, now);
    parameter_cache_put(35, 12.5f, now); parameter_cache_put(4, -2, now);
    menu_render();
    float oil_coolant[] = {101, 88}, battery[] = {12.5f, -2};
    char first[DASHBOARD_MESSAGE_MAX_LENGTH + 1], second[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    int oil_page = menu_page_index(0, 0x04), battery_page = menu_page_index(0, 0x07);
    assert(oil_page >= 0 && battery_page >= 0);
    dashboard_format_my23_page(&parameter_pages[0][oil_page], oil_coolant, first);
    dashboard_format_my23_page(&parameter_pages[0][battery_page], battery, second);
    uint8_t expected[MY23_PACKET_SIZE];
    my23_packet(expected, first, second);
    assert(!memcmp(screen_packet + 1, expected, sizeof(expected)));
    assert(saved[MENU_PREFS_SIZE + 2] == 0x04 && saved[MENU_PREFS_SIZE + 3] == 0x07);

    /* Beta 19 stored atomic measurement IDs; loading maps each to a page. */
    saved[MENU_PREFS_SIZE] = 1;
    saved[MENU_PREFS_SIZE + 2] = 5;
    saved[MENU_PREFS_SIZE + 3] = 42;
    saved[MENU_PREFS_SIZE + 4] = 22; /* Historical hidden slot stays untouched. */
    saved[MENU_PREFS_SIZE + 2 + FAVORITE_MAX_PARAMS] = 97; /* Legacy RPM survives. */
    dashboard_state.baccable_dashboard_menu_visible = 0;
    menu_init(); menu_event(MENU_HOLD);
    assert(menu_preferences_save() == 0);
    assert(saved[MENU_PREFS_SIZE] == 2);
    assert(saved[MENU_PREFS_SIZE + 2] != 5 && saved[MENU_PREFS_SIZE + 3] != 42);
    assert(saved[MENU_PREFS_SIZE + 4] == 22);
    assert(saved[MENU_PREFS_SIZE + 2 + FAVORITE_MAX_PARAMS] == 0xfe);
}
static void test_staged_setting_failure_and_discard(void) {
    for (unsigned profile = 0; profile < 2; ++profile) {
        fresh_contract(); to_settings(); menu_event(MENU_SELECT);
        settings_state.ipc_my23_is_installed = profile;
        setup_dashboardPageIndex = setup_page_for(5);
        settings_state.shift_threshold = 3500;
        assert(settings_save() == 0);
        unsigned before = settings_writes;
        menu_event(MENU_SELECT); menu_event(MENU_NEXT);
        assert(settings_state.shift_threshold == 3500 && setup_stage_active());
        menu_event(MENU_HOLD); /* Save is unavailable without an explicit prompt. */
        assert(settings_state.shift_threshold == 3500);
        menu_event(MENU_BACK); /* Confirm, not exit. */
        fail_save = true;
        menu_event(MENU_HOLD);
        assert(settings_state.shift_threshold == 3500 && setup_stage_active());
        fail_save = false;
        menu_event(MENU_HOLD);
        assert(settings_state.shift_threshold == 3750 && setup_stage_active());
        assert(settings_writes == before + 2);
        now += 2000; menu_render();
        menu_event(MENU_BACK); /* Return only to Features, not Settings. */
        assert(!setup_stage_active());
        if (profile) {
            assert(screen_packet[2] == 0x80);
            assert(!memcmp(screen_packet + 4, "Shift RPM", 9));
        }
        menu_event(MENU_SELECT); menu_event(MENU_PREVIOUS);
        menu_event(MENU_BACK); menu_event(MENU_BACK); /* Explicit discard. */
        assert(settings_state.shift_threshold == 3750 && !setup_stage_active());
        setup_load_from_flash();
        assert(settings_state.shift_threshold == 3750);
    }
}
static void test_usb_draft_has_no_early_effect(void) {
    fresh_contract(); to_settings(); menu_event(MENU_SELECT);
    setup_dashboardPageIndex = setup_page_for(34);
    settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
    assert(settings_save() == 0);
    unsigned applied = usb_applies, sent = commands;
    menu_event(MENU_SELECT);
    assert(!settings_state.usb_sniffer && usb_applies == applied && commands == sent);
    menu_event(MENU_BACK); menu_event(MENU_BACK);
    assert(!settings_state.usb_sniffer && usb_applies == applied);
    menu_event(MENU_SELECT);
    menu_event(MENU_BACK); menu_event(MENU_HOLD);
    assert(settings_state.usb_sniffer && usb_applies == applied + 1);
    assert(dashboard_state.baccable_dashboard_menu_visible);
}
static void test_favorite_slot_draft_flow(void) {
    fresh_contract(); to_settings();
    for (unsigned i = 0; i < 5; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Favorites settings. */
    assert(strstr(screen, "Favorite 1"));
    menu_event(MENU_SELECT); /* Slots. */
    unsigned writes = preference_writes;
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Choose slot 2. */
    menu_event(MENU_NEXT); menu_event(MENU_SELECT);
    menu_event(MENU_BACK); /* Dirty confirmation. */
    assert(preference_writes == writes);
    fail_save = true; menu_event(MENU_HOLD);
    assert(preference_writes == writes + 1);
    fail_save = false; menu_event(MENU_HOLD);
    assert(preference_writes == writes + 2);
    now += 2000; menu_render();
    assert(strstr(screen, "S2 ")); /* Successful save stays at selected slot. */
    menu_event(MENU_BACK);
    assert(strstr(screen, "Favorite 1"));
    menu_event(MENU_BACK); menu_event(MENU_BACK);
    menu_event(MENU_NEXT); menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Favorites. */
    assert(menu_parameters_active());
}
static void test_double_click_and_noise(void) {
    for (unsigned window = 279; window <= 281; ++window) {
        MenuInput input = {0};
        menu_input_update(&input, 0x10, true, 0);
        menu_input_update(&input, 0x90, true, 10);
        assert(menu_input_update(&input, 0x10, true, 50) == MENU_NONE);
        menu_input_update(&input, 0x90, true, 50 + window - 30);
        assert(menu_input_update(&input, 0x10, true, 50 + window) == (window <= 280 ? MENU_BACK : MENU_SELECT));
        if (window <= 280) {
            menu_input_update(&input, 0x90, true, 400);
            assert(menu_input_update(&input, 0x10, true, 450) == MENU_NONE);
            assert(menu_input_poll(&input, true, 730) == MENU_NONE); /* Third rapid click is consumed. */
        }
    }
    MenuInput input = {0};
    menu_input_update(&input, 0x10, true, 100);
    menu_input_update(&input, 0x90, true, 110);
    menu_input_update(&input, 0x10, true, 139); /* Debounce rejects 29 ms. */
    assert(menu_input_poll(&input, true, 420) == MENU_NONE);
    menu_input_update(&input, 0x90, true, 421);
    menu_input_update(&input, 0x10, true, 451);
    assert(menu_input_update(&input, 0x18, true, 500) == MENU_NEXT);
    assert(menu_input_poll(&input, true, 731) == MENU_NONE); /* Scroll cancels the click pair. */
    menu_input_update(&input, 0x10, true, 740);
    menu_input_update(&input, 0x90, true, 750);
    for (unsigned t = 850; t < 1650; t += 100) menu_input_update(&input, 0x90, true, t);
    assert(menu_input_update(&input, 0x90, true, 1649) == MENU_NONE);
    assert(menu_input_update(&input, 0x90, true, 1650) == MENU_HOLD);
    assert(menu_input_update(&input, 0x10, true, 1700) == MENU_NONE);
}

static void test_my23_list_and_old_favorite_migration(void) {
    fresh_contract();
    MenuPreferences old;
    menu_preferences_default(&old);
    old.favorites[0][0] = 0x39; /* Existing four-temperature page, preserved by ID. */
    menu_preferences_encode(&old, v1_preferences);
    have_v1 = true;
    settings_state.ipc_my23_is_installed = 1;
    dashboard_state.baccable_dashboard_menu_visible = 0;
    menu_init(); menu_event(MENU_HOLD);
    assert(screen_packet[1] == MY23_PACKET_MARKER);
    assert(screen_packet[2] == 'O');
    menu_event(MENU_BACK); /* Root/current + next. */
    assert(screen_packet[2] == 0x80 && !memcmp(screen_packet + 4, "Favorites", 9));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Readings", 8));
    menu_event(MENU_PREVIOUS); /* Wrap to last/current and first/next. */
    assert(!memcmp(screen_packet + 4, "Information", 11));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Favorites", 9));
    menu_event(MENU_BACK); /* Save migrated preferences on close. */
    assert(have_saved);
    MenuPreferences restored;
    assert(menu_preferences_decode(&restored, saved));
    assert(restored.favorites[0][0] == 0x39);
    assert(saved[80] == 2 && saved[82] == 0x39 && saved[83] == FAVORITE_EMPTY);
    dashboard_state.baccable_dashboard_menu_visible = 0;
    menu_init(); menu_event(MENU_HOLD);
    assert(!memcmp(screen_packet + 2, "O", 1));
    settings_state.ipc_my23_is_installed = 0;
    menu_present("Gear");
    assert(screen_packet[1] == 'G'); /* Legacy receives ordinary text, never MY23 tokens. */
}
static void test_fifth_favorite_uds_reply(void) {
    fresh_contract();
    parameter_request_begin_id(33, 4);
    const ParameterDefinition *p = &parameter_definitions[33];
    uint16_t did = ((p->request_data >> 16) & 0xff) << 8 | (p->request_data >> 24);
    uint8_t reply[8] = {5, 0x62, did >> 8, did, 100, 0, 0, 0};
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .ExtId = p->response_id, .DLC = 8};
    parameter_request_receive(&header, reply);
    assert(isfinite(parameter_cache_get(33, now)));
}

/* Every atomic browsing level presents current + next, including wrapping. */
static void expect_my23_list(const char *first, const char *next) {
    uint8_t expected[MY23_PACKET_SIZE];
    my23_packet_mode(expected, UI_MODE_LIST, first, next);
    assert(!memcmp(screen_packet + 1, expected, sizeof(expected)));
}
static void test_my23_remaining_list_previews(void) {
    fresh_contract(); to_settings(); settings_state.ipc_my23_is_installed = 1; menu_render();
    const char *entries[] = {"Features", "Favorites", "Shown pages", "Sort order", "BACCAble IPC"};
    for (unsigned i = 0; i < 5; ++i) {
        expect_my23_list(entries[i], entries[(i+1)%5]); menu_event(MENU_NEXT);
    }
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    const char *slots[] = {"S1 Oil/coolant temp", "S2 Empty"};
    for (unsigned i=0;i<2;++i) { expect_my23_list(slots[i],slots[(i+1)%2]); menu_event(MENU_NEXT); }
    menu_event(MENU_SELECT);
    expect_my23_list("Oil/coolant temp", "Oil level/qual.");
    for (unsigned i=0;i<70 && memcmp(screen_packet+4,"Empty",5);++i) menu_event(MENU_PREVIOUS);
    expect_my23_list("Empty", "Power / torque");
    menu_event(MENU_BACK); menu_event(MENU_BACK); menu_event(MENU_BACK); menu_event(MENU_BACK);
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Information. */
    for(unsigned i=0;i<9;++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* IPC test source/pattern list. */
    expect_my23_list(MY23_SAVED "USB source", "Bluetooth source");
    menu_event(MENU_PREVIOUS); expect_my23_list("Both lines", "USB source");
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT);
    expect_my23_list("UTF glyphs", "Line 1 length");
    menu_present("1234567890123456›•▲▼✓×°±→…");
    assert(!memcmp(screen_packet + 2, "1234567890123456", MY23_L1_VISIBLE));
    const uint8_t symbols[] = {0x80,0x81,0x82,0x83,0x84,0x85,0xb0,0xb1,0x86,0x87};
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, symbols, sizeof(symbols)));
}
static void test_v6_parameter_label_identity(void) {
    char fifth[40],sixth[40];
    favorite_parameter_segment(0,19,1,false,fifth,sizeof(fifth));
    favorite_parameter_segment(0,20,1,false,sixth,sizeof(sixth));
    assert(!strcmp(fifth,"IC5 1.0\xb0") && !strcmp(sixth,"IC6 1.0\xb0"));
    favorite_parameter_segment(0,19,1,true,fifth,sizeof(fifth));
    favorite_parameter_segment(0,20,1,true,sixth,sizeof(sixth));
    assert(!strcmp(fifth,"I51.0\xb0") && !strcmp(sixth,"I61.0\xb0"));
}
/* Compatibility page editors must never expose or persist an unaccepted draft. */
static void test_page_editor_transactions(void) {
    for (unsigned editor=1;editor<=3;++editor) {
        fresh_contract(); to_settings(); assert(menu_preferences_save()==0);
        uint8_t original[sizeof(saved)]; memcpy(original,saved,sizeof(saved));
        for(unsigned i=0;i<editor;++i) menu_event(MENU_NEXT);
        menu_event(MENU_SELECT); menu_event(MENU_SELECT);
        if (editor==3) menu_event(MENU_NEXT);
        assert(menu_preferences_save()==0 && !memcmp(original,saved,sizeof(saved)));
        unsigned writes=preference_writes;
        menu_event(MENU_BACK); /* Dirty prompt. */
        assert(strstr(screen,"Hold save"));
        menu_event(MENU_NEXT); /* Confirmation locks editing. */
        fail_save=true; menu_event(MENU_HOLD);
        assert(preference_writes==writes+1 && !memcmp(original,saved,sizeof(saved)));
        fail_save=false; menu_event(MENU_HOLD);
        assert(preference_writes==writes+2 && memcmp(original,saved,sizeof(saved)));
        now+=2000; menu_render(); menu_event(MENU_BACK); /* Clean editor -> Settings. */
        assert(strstr(screen, editor==1 ? "Page favorites" : editor==2 ? "Shown pages" : "Favorite order"));
        MenuPreferences committed; assert(menu_preferences_decode(&committed,saved));
        for(unsigned f=0;f<FAVORITE_SET_COUNT;++f) {
            int index=menu_page_index(0,committed.favorites[0][f]);
            if(index<0) assert(saved[82+f*5]==FAVORITE_EMPTY);
            else assert(saved[82+f*5]==parameter_pages[0][index].id);
        }
        memcpy(original,saved,sizeof(saved));
        menu_event(MENU_SELECT); menu_event(MENU_SELECT); if(editor==3) menu_event(MENU_NEXT);
        menu_event(MENU_BACK); menu_event(MENU_BACK); /* Explicit discard. */
        assert(menu_preferences_save()==0 && !memcmp(original,saved,sizeof(saved)));
        menu_event(MENU_SELECT); menu_event(MENU_SELECT); if(editor==3) menu_event(MENU_NEXT);
        now+=60001; menu_process(); /* Timeout discards too. */
        assert(menu_parameters_active() && !memcmp(original,saved,sizeof(saved)));
    }
}
static void test_my23_visibility_transaction(void) {
    fresh_contract(); to_settings(); settings_state.ipc_my23_is_installed=1;
    assert(menu_preferences_save()==0);
    MenuPreferences original; assert(menu_preferences_decode(&original,saved));
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
    assert(screen_packet[2]==0x80 && screen_packet[4]==0x84);
    uint8_t preview[22]; memcpy(preview, screen_packet + 2 + MY23_L1_VISIBLE, 22);
    menu_event(MENU_SELECT); /* Hide only in the draft. */
    assert(screen_packet[4]==0x85 && !memcmp(preview, screen_packet + 2 + MY23_L1_VISIBLE, 22));
    assert(menu_preferences_save()==0);
    MenuPreferences live; assert(menu_preferences_decode(&live,saved));
    assert(!memcmp(original.hidden,live.hidden,sizeof(live.hidden)));
    menu_event(MENU_BACK);
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "HOLD SAVE", 9));
    menu_event(MENU_BACK); /* Discard. */
    expect_my23_list("Shown pages","Sort order");
    menu_event(MENU_SELECT); assert(screen_packet[4]==0x84);
    menu_event(MENU_SELECT); menu_event(MENU_BACK); menu_event(MENU_HOLD);
    now+=2000; menu_render();
    assert(screen_packet[4]==0x85 && !memcmp(preview, screen_packet + 2 + MY23_L1_VISIBLE, 22));
    menu_event(MENU_BACK); expect_my23_list("Shown pages","Sort order");
    dashboard_state.baccable_dashboard_menu_visible=0; menu_init(); menu_event(MENU_HOLD);
    assert(menu_preferences_save()==0 && menu_preferences_decode(&live,saved));
    assert(memcmp(original.hidden,live.hidden,sizeof(live.hidden)));
}

static void test_favorite_slot_compact_label(void) {
    fresh_contract(); to_settings(); settings_state.ipc_my23_is_installed=1;
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    /* Page 0x04 -> 0x05, without flattening Oil/coolant into two IDs. */
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
    expect_my23_list("S1 Oil level/qual.","S2 Empty");
    menu_event(MENU_BACK); menu_event(MENU_BACK); /* Discard preserves imported primary. */
    menu_event(MENU_SELECT);
    expect_my23_list("S1 Oil/coolant temp","S2 Empty");
}
static void test_my23_favorite_duplicate_rollback(void) {
    fresh_contract(); to_settings(); settings_state.ipc_my23_is_installed=1;
    assert(menu_preferences_save()==0);
    uint8_t original[sizeof(saved)]; memcpy(original,saved,sizeof(saved));
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Slot 2 picker. */
    for (unsigned i=0;i<70 && memcmp(screen_packet+4,"Oil/coolant",11);++i) menu_event(MENU_PREVIOUS);
    assert(!memcmp(screen_packet+4,"Oil/coolant",11));
    menu_event(MENU_SELECT); /* Move primary to Slot 2 only in the draft. */
    expect_my23_list("S2 Oil/coolant temp","S1 Empty");
    menu_event(MENU_PREVIOUS);
    expect_my23_list("S1 Empty","S2 Oil/coolant temp");
    menu_event(MENU_BACK); fail_save=true; menu_event(MENU_HOLD);
    assert(!memcmp(original,saved,sizeof(saved)));
    fail_save=false; menu_event(MENU_BACK); /* Discard failed draft. */
    menu_event(MENU_SELECT);
    expect_my23_list("S1 Oil/coolant temp","S2 Empty");
    assert(!memcmp(original,saved,sizeof(saved)));
}

/* Other lists and engine filtering must not replace the selected Favorite. */
static void test_atomic_favorite_selection_identity(void) {
    fresh_contract();
    assert(menu_preferences_save() == 0);
    saved[MENU_PREFS_SIZE] = 2; saved[MENU_PREFS_SIZE + 1] = 1;
    memset(saved + MENU_PREFS_SIZE + 2, FAVORITE_EMPTY, FAVORITE_STORAGE_SIZE - 2);
    saved[MENU_PREFS_SIZE + 2] = 0x14; /* MultiAir: hidden on V6. */
    saved[MENU_PREFS_SIZE + 2 + FAVORITE_MAX_PARAMS] = 0x1a; /* Gear. */
    saved[MENU_PREFS_SIZE + 2 + 2 * FAVORITE_MAX_PARAMS] = 0x28; /* Speed. */
    settings_state.ipc_my23_is_installed = 1;
    menu_init(); menu_render();
    menu_event(MENU_NEXT);
    uint8_t selected[UART_SCREEN_BUFFER_SIZE];
    memcpy(selected, screen_packet, sizeof(selected));
    assert(!memcmp(screen_packet + 2, "Gear", 4));
    menu_event(MENU_BACK); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
    menu_event(MENU_SELECT); /* First reading of the remembered group. */
    menu_event(MENU_BACK); menu_event(MENU_BACK); menu_event(MENU_PREVIOUS);
    menu_event(MENU_SELECT);
    assert(!memcmp(selected, screen_packet, sizeof(selected)));
    settings_state.gasoline_v6 = 1;
    menu_engine_changed(); menu_render();
    assert(!memcmp(selected, screen_packet, sizeof(selected)));
    menu_event(MENU_NEXT);
    assert(!memcmp(screen_packet + 2, "Speed", 3));
}

static void test_ipc_options_are_volatile(void) {
    for (unsigned profile = 0; profile < 2; ++profile) {
        fresh_contract(); to_settings();
        settings_state.ipc_my23_is_installed = profile;
        menu_event(MENU_PREVIOUS); menu_event(MENU_SELECT);
        assert(strstr(screen, "Safe 50ms"));
        unsigned writes = settings_writes + preference_writes;
        uart_busy = true; menu_event(MENU_SELECT); uart_busy = false;
        now += 2000; menu_render(); assert(strstr(screen, "Safe 50ms"));
        menu_event(MENU_SELECT);
        assert(last_command == BH_CMD_IPC_OPTIONS && last_ipc_pace == 1 && last_ipc_method == 0);
        menu_event(MENU_SELECT); assert(last_ipc_pace == 2);
        menu_event(MENU_NEXT); menu_event(MENU_SELECT); assert(last_ipc_method == 1);
        now += 1000; menu_process(); assert(last_ipc_pace == 2 && last_ipc_method == 1);
        assert(settings_writes + preference_writes == writes);
        menu_init(); menu_event(MENU_BACK);
        /* menu_init keeps visible state; BACK above returns to root. */
        menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        menu_event(MENU_PREVIOUS); menu_event(MENU_SELECT);
        assert(strstr(screen, "Safe 50ms"));
        menu_event(MENU_SELECT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        assert(last_ipc_pace == 0 && last_ipc_method == 0);
        assert(settings_writes + preference_writes == writes);
    }
}
