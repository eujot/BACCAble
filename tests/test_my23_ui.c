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
    my23_packet(packet, "Gear", "RPM 3500");
    assert(packet[5] == ' ' && !memcmp(packet + 15, "RPM 3500", 8));
    for (unsigned i = 5; i < 15; ++i) assert(packet[i] == ' ');
}
/* MY23 Readings keeps the old value text and previews the next eligible page. */
static void test_my23_readings_preview_and_unavailable_gear(void) {
    fresh_contract();
    settings_state.ipc_my23_is_installed = 1;
    int gear = menu_page_index(0, 0x1a);
    assert(gear >= 0);
    parameter_cache_put(6, 15, now); /* 0xF from CAN means unavailable. */
    menu_show_parameter((uint8_t)gear); menu_render();
    assert(screen_packet[2] == 0x80 && !memcmp(screen_packet + 4, "Gear --", 7));
    MenuPreferences prefs;
    menu_preferences_default(&prefs);
    uint8_t pages[64];
    unsigned count = menu_page_list_filtered(&prefs, 0, parameter_pages[0][gear].group,
                                             false, false, false, false, pages);
    unsigned selected = 0;
    while (selected < count && pages[selected] != gear) ++selected;
    assert(selected < count && count > 1);
    const char *next = parameter_pages[0][pages[(selected + 1) % count]].label;
    assert(!memcmp(screen_packet + 16, next, strlen(next) < 22 ? strlen(next) : 22));
    int coolant = menu_page_index(0, 0x20);
    assert(coolant >= 0);
    parameter_cache_put(42, 88, now);
    menu_show_parameter((uint8_t)coolant); menu_render();
    assert(screen_packet[2] == 0x80 && !memcmp(screen_packet + 4, "Coolant 88\xb0", 11));
    menu_event(MENU_BACK); menu_event(MENU_BACK); /* Return to root. */
    menu_event(MENU_PREVIOUS); menu_event(MENU_SELECT); /* Information. */
    for (unsigned i = 0; i < 5; ++i) menu_event(MENU_NEXT);
    assert(!memcmp(screen_packet + 16, "Gaps", 4));
    for (unsigned i = 0; i < 4; ++i) menu_event(MENU_NEXT);
#ifdef MENU_DIAGNOSTICS
    assert(!memcmp(screen_packet + 16, "IPC diag", 8));
    menu_event(MENU_NEXT);
#endif
    assert(!memcmp(screen_packet + 16, "C1 firmware", 11));
}

static void test_atomic_favorite_packing(void) {
    FavoriteParameters favorite = {{6, 97, 7, 25, FAVORITE_EMPTY}};
    parameter_cache_reset(); parameter_peak_enable(false);
    parameter_cache_put(6, 3, now); parameter_cache_put(97, 3500, now);
    parameter_cache_put(7, 100, now); parameter_cache_put(25, -0.5f, now);
    char first[15], second[23];
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
    assert(!memcmp(screen_packet + 2, "Oil ECU --", 7));
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
    assert(!memcmp(screen_packet + 2, "Oil ECU --", 7));
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
    const char *entries[] = {"Features", "Favorites", "Shown pages", "Sort order"};
    for (unsigned i = 0; i < 4; ++i) {
        expect_my23_list(entries[i], entries[(i+1)%4]); menu_event(MENU_NEXT);
    }
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    const char *slots[] = {"S1 Oil temp", "S2 Coolant"};
    for (unsigned i=0;i<2;++i) { expect_my23_list(slots[i],slots[(i+1)%2]); menu_event(MENU_NEXT); }
    menu_event(MENU_SELECT);
    expect_my23_list("Oil temp native", "Gear");
    for (unsigned i=0;i<101 && memcmp(screen_packet+4,"Empty",5);++i) menu_event(MENU_PREVIOUS);
    expect_my23_list("Empty", "Oil press native");
    menu_event(MENU_BACK); menu_event(MENU_BACK); menu_event(MENU_BACK); menu_event(MENU_BACK);
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Information. */
    for(unsigned i=0;i<9;++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* IPC test source/pattern list. */
    expect_my23_list(MY23_SAVED "USB source", "Bluetooth source");
    menu_event(MENU_PREVIOUS); expect_my23_list("Both lines", "USB source");
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT);
    expect_my23_list("UTF glyphs", "Line 1 length");
    menu_present("12345678901234›•▲▼✓×°±→…");
    assert(!memcmp(screen_packet+2,"12345678901234",14));
    const uint8_t symbols[] = {0x80,0x81,0x82,0x83,0x84,0x85,0xb0,0xb1,0x86,0x87};
    assert(!memcmp(screen_packet+16,symbols,sizeof(symbols)));
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
            else assert(saved[82+f*5]==parameter_pages[0][index].parameter_ids[0]);
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
    uint8_t preview[22]; memcpy(preview,screen_packet+16,22);
    menu_event(MENU_SELECT); /* Hide only in the draft. */
    assert(screen_packet[4]==0x85 && !memcmp(preview,screen_packet+16,22));
    assert(menu_preferences_save()==0);
    MenuPreferences live; assert(menu_preferences_decode(&live,saved));
    assert(!memcmp(original.hidden,live.hidden,sizeof(live.hidden)));
    menu_event(MENU_BACK);
    assert(!memcmp(screen_packet+16,"HOLD SAVE",9));
    menu_event(MENU_BACK); /* Discard. */
    expect_my23_list("Shown pages","Sort order");
    menu_event(MENU_SELECT); assert(screen_packet[4]==0x84);
    menu_event(MENU_SELECT); menu_event(MENU_BACK); menu_event(MENU_HOLD);
    now+=2000; menu_render();
    assert(screen_packet[4]==0x85 && !memcmp(preview,screen_packet+16,22));
    menu_event(MENU_BACK); expect_my23_list("Shown pages","Sort order");
    dashboard_state.baccable_dashboard_menu_visible=0; menu_init(); menu_event(MENU_HOLD);
    assert(menu_preferences_save()==0 && menu_preferences_decode(&live,saved));
    assert(memcmp(original.hidden,live.hidden,sizeof(live.hidden)));
}

static void test_favorite_slot_compact_label(void) {
    fresh_contract(); to_settings(); settings_state.ipc_my23_is_installed=1;
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    /* Oil native ID 5 -> Gear, Speed, then last/best timers; diesel DPF is skipped. */
    for (unsigned i=0;i<5;++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
    expect_my23_list("S1 Best 0-100","S2 Coolant");
    menu_event(MENU_BACK); menu_event(MENU_BACK); /* Discard preserves imported primary. */
    menu_event(MENU_SELECT);
    expect_my23_list("S1 Oil temp","S2 Coolant");
}
static void test_my23_favorite_duplicate_rollback(void) {
    fresh_contract(); to_settings(); settings_state.ipc_my23_is_installed=1;
    assert(menu_preferences_save()==0);
    uint8_t original[sizeof(saved)]; memcpy(original,saved,sizeof(saved));
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Slot 2 picker. */
    for (unsigned i=0;i<101 && memcmp(screen_packet+4,"Oil temp na",11);++i) menu_event(MENU_PREVIOUS);
    assert(!memcmp(screen_packet+4,"Oil temp na",11));
    menu_event(MENU_SELECT); /* Move primary to Slot 2 only in the draft. */
    expect_my23_list("S2 Oil temp","S1 Empty");
    menu_event(MENU_PREVIOUS);
    expect_my23_list("S1 Empty","S2 Oil temp");
    menu_event(MENU_BACK); fail_save=true; menu_event(MENU_HOLD);
    assert(!memcmp(original,saved,sizeof(saved)));
    fail_save=false; menu_event(MENU_BACK); /* Discard failed draft. */
    menu_event(MENU_SELECT);
    expect_my23_list("S1 Oil temp","S2 Coolant");
    assert(!memcmp(original,saved,sizeof(saved)));
}

/* Other lists and engine filtering must not replace the selected Favorite. */
static void test_atomic_favorite_selection_identity(void) {
    fresh_contract();
    assert(menu_preferences_save() == 0);
    saved[MENU_PREFS_SIZE] = saved[MENU_PREFS_SIZE + 1] = 1;
    memset(saved + MENU_PREFS_SIZE + 2, FAVORITE_EMPTY, FAVORITE_STORAGE_SIZE - 2);
    saved[MENU_PREFS_SIZE + 2] = 32; /* MultiAir: hidden on V6. */
    saved[MENU_PREFS_SIZE + 2 + FAVORITE_MAX_PARAMS] = 6; /* Gear. */
    saved[MENU_PREFS_SIZE + 2 + 2 * FAVORITE_MAX_PARAMS] = 7; /* Speed. */
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
