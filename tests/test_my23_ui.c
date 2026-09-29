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
    my23_packet_mode(packet, UI_MODE_LIST, "1234567890123", "");
    assert(packet[1] == 0x80 && packet[14] == 0x87 && packet[15] == ' ');
    my23_packet_mode(packet, UI_MODE_LIST, "123456789012", "next");
    assert(!memcmp(packet + 3, "123456789012", 12) && !memcmp(packet + 15, "next", 4));
    my23_packet(packet, "12345678901234EXTRA", "1234567890123456789012EXTRA");
    assert(!memcmp(packet + 1, "12345678901234", 14));
    assert(!memcmp(packet + 15, "1234567890123456789012", 22));
    my23_packet(packet, "GEAR", "RPM 3500");
    assert(packet[5] == ' ' && !memcmp(packet + 15, "RPM 3500", 8));
    for (unsigned i = 5; i < 15; ++i) assert(packet[i] == ' ');
}
static void test_atomic_favorite_packing(void) {
    FavoriteParameters favorite = {{6, 97, 7, 25, FAVORITE_EMPTY}};
    parameter_cache_reset(); parameter_peak_enable(false);
    parameter_cache_put(6, 3, now); parameter_cache_put(97, 3500, now);
    parameter_cache_put(7, 100, now); parameter_cache_put(25, -0.5f, now);
    char first[15], second[23];
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strcmp(first, "GEAR 3"));
    assert(!strcmp(second, "R3500" MY23_BULLET "S100" MY23_BULLET "B-0.5bar"));
    assert(strlen(second) <= 22 && (uint8_t)second[strlen(second)-1] != 0x81);
    favorite = (FavoriteParameters){{5, 42, 22, 32, 33}};
    const uint8_t ids[] = {5, 42, 22, 32, 33};
    const float values[] = {101, 88, 42, 76, 90};
    for (unsigned i = 0; i < 5; ++i) parameter_cache_put(ids[i], values[i], now);
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strcmp(first, "OIL 101\xb0"));
    assert(!strcmp(second, "W88\xb0" MY23_BULLET "IC42\xb0" MY23_BULLET "MA76\xb0" MY23_BULLET "GT90\xb0"));
    assert(strlen(second) == 22);
    /* Every supported set size has a complete primary and ordered whole segments. */
    const char *expected_lines[] = {"", "WTR 88\xb0", "WTR 88\xb0" MY23_BULLET "IC 42\xb0",
                                  "WTR 88\xb0" MY23_BULLET "IC 42\xb0" MY23_BULLET "MA 76\xb0",
                                  "W88\xb0" MY23_BULLET "IC42\xb0" MY23_BULLET "MA76\xb0" MY23_BULLET "GT90\xb0"};
    for (unsigned count = 1; count <= FAVORITE_MAX_PARAMS; ++count) {
        FavoriteParameters subset = favorite;
        for (unsigned i = count; i < FAVORITE_MAX_PARAMS; ++i) subset.params[i] = FAVORITE_EMPTY;
        favorite_parameters_render(0, &subset, now, first, second);
        assert(!strcmp(first, "OIL 101\xb0") && !strcmp(second, expected_lines[count - 1]));
    }
    parameter_cache_put(42, 8, now);
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(strlen(second) == 21);
    parameter_cache_put(42, 888, now); /* The 23-glyph candidate omits only the last segment. */
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(!strstr(second, "GT") && strstr(second, "MA76"));
    assert(strcmp(favorite_parameter_name(0, false, 9), favorite_parameter_name(0, false, 11)));
    parameter_cache_put(42, -150, now);
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(strlen(second) <= 22 && !strstr(second, "GT"));
    favorite.params[1] = FAVORITE_EMPTY;
    favorite_parameters_render(0, &favorite, now, first, second);
    assert(second[0] != (char)0x81 && second[strlen(second)-1] != (char)0x81);
    favorite_parameters_render(0, &favorite, now + 3001, first, second);
    assert(strstr(first, "--") && strstr(second, "--"));
    assert(favorite_parameter_supported(0, false, 32));
    assert(!favorite_parameter_supported(0, true, 32));
    assert(!favorite_parameter_supported(1, false, 32));
    assert(favorite_parameter_supported(0, false, 97));
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
        assert(settings_state.shift_threshold == 3750 && !setup_stage_active());
        assert(settings_writes == before + 2);
        now += 2000; menu_render();
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
    assert(strstr(screen, "Slot 2")); /* Successful save stays at selected slot. */
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
    assert(!memcmp(screen_packet + 2, "OILE --", 7));
    menu_event(MENU_BACK); /* Root/current + next. */
    assert(screen_packet[2] == 0x80 && !memcmp(screen_packet + 4, "Favorites", 9));
    assert(!memcmp(screen_packet + 16, "Readings", 8));
    menu_event(MENU_PREVIOUS); /* Wrap to last/current and first/next. */
    assert(!memcmp(screen_packet + 4, "Information", 11));
    assert(!memcmp(screen_packet + 16, "Favorites", 9));
    menu_event(MENU_BACK); /* Save migrated preferences on close. */
    assert(have_saved);
    MenuPreferences restored;
    assert(menu_preferences_decode(&restored, saved));
    assert(restored.favorites[0][0] == 0x39);
    assert(saved[82] == 30 && saved[83] == 42 && saved[84] == 23 && saved[85] == 22 && saved[86] == FAVORITE_EMPTY);
    dashboard_state.baccable_dashboard_menu_visible = 0;
    menu_init(); menu_event(MENU_HOLD);
    assert(!memcmp(screen_packet + 2, "OILE --", 7));
    settings_state.ipc_my23_is_installed = 0;
    menu_present("GEAR");
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
