/* Included by test_menu.c to reuse the board transport and settings fixture. */
static uint8_t setup_page_for(uint8_t slot) {
    const SetupParam *param = setup_find_by_flash_index(slot);
    assert(param && param->menu_text);
    setup_cancel_edit();
    char expected[DASHBOARD_MESSAGE_MAX_LENGTH + 1] = {0};
    if (param->render) {
        param->render();
        memcpy(expected, dashboard_setup_screen, DASHBOARD_MESSAGE_MAX_LENGTH);
    } else if (param->entry_type == UI_ENTRY_ENUM || param->entry_type == UI_ENTRY_EXCLUSIVE_MODE) {
        unsigned value = *(uint8_t *)param->value;
        const char *label = "OFF";
        static const char *const pedal[] = {"OFF", "Auto", "Bypass", "A", "N", "D", "R", "Hybrid", "Kids"};
        static const char *const acc[] = {"OFF", "RES", "+"};
        static const char *const dna[] = {"N", "A", "D", "Race"};
        static const char *const drivetrain[] = {"Not set", "RWD", "Q4"};
        if (slot == 40) label = drivetrain[value];
        if (slot == 41) label = !value ? "Stiff" : settings_state.default_drive_mode == 2 ? "Soft" :
                                settings_state.default_drive_mode == 3 ? "Mid" : "Softer";
        if (slot == 16) label = value ? "2.2 D" : settings_state.gasoline_v6 ? "2.9 V6" : "2.0 I4";
        if (slot == 20) label = pedal[value];
        if (slot == 24) label = acc[value];
        if (slot == 38) label = dna[value <= 3 ? value : 0];
        if (slot == 34) label = value ? "CAN" : settings_state.usb_elm327 ? "ELM327" : "OFF";
        ui_render_value(expected, sizeof(expected), param->menu_text, label);
        if (slot == 38 && value) {
            char result[24];
            if (value == 3 && !settings_state.ipc_my23_is_installed) label = "R";
            snprintf_(result, sizeof(result), "%s %s", label, drive_mode_check_status());
            if (!strcmp(drive_mode_check_status(), "Start engine"))
                ui_render_value(expected, sizeof(expected), dna[value], "Start engine");
            else ui_render_value(expected, sizeof(expected), "DNA", result);
        }
    } else if (slot == 39)
        ui_render_value(expected, sizeof(expected), "LKA", *(uint8_t *)param->value ? "OFF" : "ON");
    else if (param->entry_type == UI_ENTRY_TOGGLE)
        ui_render_checkbox(expected, sizeof(expected), param->menu_text, *(uint8_t *)param->value != 0);
    else if (param->entry_type == UI_ENTRY_SUBMENU)
        ui_render_action(expected, sizeof(expected), param->menu_text);
    else
        snprintf_(expected, sizeof(expected), "%s", param->menu_text);
    for (uint8_t page = 0; page < SETUP_FLASH_PARAM_BUFFER_SIZE; ++page) {
        setup_render_page(page);
        if (!memcmp(dashboard_setup_screen, expected, strlen(expected))) {
            setup_dashboardPageIndex = page;
            return page;
        }
    }
    assert(!"setup page missing");
    return 0;
}
static void setup_accept_draft(void) {
    assert(setup_stage_active());
    assert(setup_back()); /* First Back requests confirmation. */
    setup_stage_save();
    assert(setup_stage_active()); /* Saving stays on the setting. */
    assert(setup_stage_back()); /* A separate Back leaves the clean editor. */
    assert(!setup_stage_active());
}
/* The result is persistent, visible on both displays, and never edits the preference. */
static void test_default_mode_result(void) {
    fresh_menu();
    settings_state.default_drive_mode = 2;
    board_sync_rearm();
    board_sync_ignition(8);
    uint8_t page = setup_page_for(38);
    setup_render_page(page);
    assert(!memcmp(dashboard_setup_screen, "D: Start engine", 15));
    board_sync_restart();
    assert(!strcmp(drive_mode_check_status(), "Start engine"));
    drive_mode_check_engine_started(); /* This test isolates display/result formatting. */
    now += 15000;
    board_sync_process();
    setup_render_page(page);
    assert(!memcmp(dashboard_setup_screen, "DNA: D No confirm", 17));
    settings_state.default_drive_mode = 3;
    setup_render_page(page);
    assert(!memcmp(dashboard_setup_screen, "DNA: R No confirm", 17));
    settings_state.default_drive_mode = 2;
    settings_state.ipc_my23_is_installed = true;
    assert(setup_present_my23());
    assert(!memcmp(screen_packet + 2, "Default mode", 12));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "D No confirm", 12));
    assert(settings_state.default_drive_mode == 2);
    board_sync_ignition(2);
    board_sync_ignition(8);
    assert(!strcmp(drive_mode_check_status(), "Start engine"));
    board_sync_saved();
    assert(!strcmp(drive_mode_check_status(), "Next start"));
}

static void test_setup_ui(void) {
    fresh_menu();
    const uint8_t enum_slots[] = {16, 20, 24, 25, 26, 38, 40, 41};
    const uint8_t enum_sizes[] = {3, 9, 3, 3, 3, 4, 3, 2};
    for (unsigned entry = 0; entry < sizeof(enum_slots); ++entry) {
        const SetupParam *param = setup_find_by_flash_index(enum_slots[entry]);
        assert(param->entry_type == UI_ENTRY_ENUM);
        *(uint8_t *)param->value = 0;
        settings_state.gasoline_v6 = 0;
        uint8_t enum_page = setup_page_for(enum_slots[entry]);
        setup_select_page(enum_page);
        for (unsigned step = 0; step < enum_sizes[entry]; ++step) setup_select_page(enum_page);
        assert(*(uint8_t *)param->value == 0); /* Full cycle stayed in the draft. */
        setup_cancel_edit();
        /* Move one step backward from the first choice and commit the wrap. */
        setup_select_page(enum_page);
        setup_move_page(-1);
        setup_accept_draft();
        assert(*(uint8_t *)param->value == (enum_slots[entry] == 16 || enum_slots[entry] == 38 ? 1 : enum_sizes[entry] - 1));

    }
    settings_state.rotate_readings = 0;
    uint8_t page = setup_page_for(33);
    setup_select_page(page);
    assert(!settings_state.rotate_readings && setup_stage_active());
    setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.rotate_readings);
    setup_select_page(page);
    setup_move_page(1);
    setup_accept_draft();
    assert(!settings_state.rotate_readings);

    settings_state.shift_threshold = 3500;
    page = setup_page_for(5);
    setup_select_page(page);
    assert(setup_in_workflow());
    setup_move_page(1);
    assert(settings_state.shift_threshold == 3500);
    setup_accept_draft();
    assert(settings_state.shift_threshold == 3750);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.shift_threshold == 6000);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(-1);
    setup_accept_draft();
    assert(settings_state.shift_threshold == 1500);

    settings_state.launch_torque_threshold = 25;
    page = setup_page_for(18);
    setup_select_page(page);
    setup_move_page(-1);
    assert(setup_back()); /* At lower bound the unchanged draft returns directly. */
    assert(!setup_stage_active() && settings_state.launch_torque_threshold == 25);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.launch_torque_threshold == 600);

    settings_state.pedal_map_power = -10;
    page = setup_page_for(29);
    setup_select_page(page);
    setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.pedal_map_power == -8);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.pedal_map_power == 10);
    setup_select_page(page);
    setup_move_page(-1);
    setup_cancel_edit();
    assert(settings_state.pedal_map_power == 10);
    assert(setup_read_flash_value(29, (uint8_t)-10) == (uint8_t)-10);
    assert(setup_read_flash_value(5, 0) == setup_find_by_flash_index(5)->default_value);

    settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
    page = setup_page_for(34);
    setup_select_page(page);
    assert(!settings_state.usb_sniffer && !settings_state.usb_elm327);
    setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.usb_sniffer && !settings_state.usb_elm327);
    setup_select_page(page);
    setup_move_page(1);
    setup_accept_draft();
    assert(!settings_state.usb_sniffer);
#ifdef ACT_AS_ELM327
    assert(settings_state.usb_elm327);
    setup_select_page(page);
    setup_move_page(1);
    setup_accept_draft();
#endif
    assert(!settings_state.usb_sniffer && !settings_state.usb_elm327);
    assert(setup_find_by_flash_index(35)->menu_text == NULL);

    settings_state.park_mirror = 0;
    page = setup_page_for(23);
    unsigned before = commands;
    setup_select_page(page); /* Open nested menu, without enabling or capturing. */
    assert(commands == before && !settings_state.park_mirror);
    setup_move_page(1);
    setup_select_page(page); /* Capture unavailable while disabled. */
    assert(commands == before);
    setup_move_page(-1);
    setup_select_page(page); /* Enabled is a draft, not a UART side effect. */
    assert(!settings_state.park_mirror && commands == before);
    setup_move_page(1);
    setup_accept_draft();
    assert(settings_state.park_mirror && commands == before);
    now = 10000; runtime_state.all_processors_wakeup_time = 0;
    for (unsigned i = 0; i < 5; ++i) board_sync_process();
    assert(last_command == BHcmdFunctParkMirrorEnabled);
    before = commands - 1;
    setup_move_page(1);
    setup_select_page(page);
    setup_select_page(page); /* Click cannot execute a HOLD-only capture. */
    assert(commands == before + 1);
    assert(setup_back()); /* Cancel capture confirmation. */
    setup_select_page(page);
    uart_busy = true;
    setup_stage_save();
    assert(commands == before + 1);
    uart_busy = false;
    setup_stage_save();
    assert(commands == before + 2);
    assert(last_command == BHcmdFunctParkMirrorStoreCurPos);
    setup_render_page(page);
    assert(!memcmp(dashboard_setup_screen, "Store: queued", 13));
    assert(setup_back());
    assert(setup_back());
    assert(!setup_in_workflow());
}

/* Reproduce the vehicle report through the shared recognizer and real menu handler. */
static MenuEvent menu_button_at(MenuInput *input, uint8_t button, uint32_t tick) {
    now = tick;
    MenuEvent event = menu_input_update(input, button, true, now);
    if (event != MENU_NONE) menu_event(event);
    return event;
}
static void test_lka_save_gestures(void) {
    for (unsigned profile = 0; profile < 2; ++profile) {
        fresh_contract(); to_settings(); menu_event(MENU_SELECT);
        settings_state.ipc_my23_is_installed = profile;
        settings_state.lane_assist_disabled = 0;
        setup_dashboardPageIndex = setup_page_for(39);
        assert(settings_save() == 0);
        unsigned writes = settings_writes;
        menu_event(MENU_SELECT);
        if (profile) assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "ON", 2));
        else assert(strstr(screen, "LKA: ON"));
        menu_event(MENU_BACK); /* Opening a setting alone never creates a dirty draft. */
        assert(!setup_stage_active() && settings_writes == writes);
        menu_event(MENU_SELECT); menu_event(MENU_NEXT);
        assert(!settings_state.lane_assist_disabled);
        if (profile) assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "OFF", 3));
        else assert(strstr(screen, "LKA: OFF"));
        MenuInput input = {0};
        uint32_t base = now;
        menu_button_at(&input, 0x10, base);
        menu_button_at(&input, 0x90, base + 10);
        menu_button_at(&input, 0x10, base + 60);
        menu_button_at(&input, 0x90, base + 310);
        assert(menu_button_at(&input, 0x10, base + 360) == MENU_BACK);
        if (profile) assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "HOLD SAVE", 9));
        else assert(strstr(screen, "Hold save"));
        /* Hold begins inside the third-click suppression window and must still save. */
        menu_button_at(&input, 0x50, base + 400);
        for (unsigned t = 500; t < 1300; t += 100)
            assert(menu_button_at(&input, 0x90, base + t) == MENU_NONE);
        assert(menu_button_at(&input, 0x90, base + 1300) == MENU_HOLD);
        assert(settings_state.lane_assist_disabled && settings_writes == writes + 1);
        assert(menu_button_at(&input, 0x10, base + 1350) == MENU_NONE);
        menu_event(MENU_BACK);
        assert(!setup_stage_active());
        settings_state.lane_assist_disabled = 0;
        setup_load_from_flash();
        assert(settings_state.lane_assist_disabled); /* Preserve slot 39's old inverse encoding. */
        menu_event(MENU_SELECT); menu_event(MENU_PREVIOUS);
        menu_event(MENU_HOLD); /* Direct hold from Off to On, no confirmation first. */
        assert(!settings_state.lane_assist_disabled && settings_writes == writes + 2);
        menu_event(MENU_HOLD);
        assert(settings_writes == writes + 2); /* No redundant Flash write. */
        menu_event(MENU_BACK);
        assert(!setup_stage_active());
        menu_event(MENU_SELECT); menu_event(MENU_NEXT);
        menu_event(MENU_BACK); menu_event(MENU_BACK);
        assert(!setup_stage_active() && !settings_state.lane_assist_disabled);
    }
}

static void test_ignition_sessions_without_sleep(void) {
    now = 20000;
    runtime_state.all_processors_wakeup_time = 0;
    uart_busy = false;
    board_sync_rearm();
    for (unsigned i = 0; i < 10; ++i) board_sync_process();
    assert(last_driver_rearm == 0); /* Bus wake alone is not ignition. */
    for (unsigned session = 0; session < 3; ++session) {
        board_sync_ignition(2);
        for (unsigned i = 0; i < 10; ++i) board_sync_process();
        assert(last_driver_rearm == 2); /* OFF cancels an unused startup window. */
        unsigned before = commands;
        board_sync_ignition(10); /* Crank does not start a separate session. */
        board_sync_process();
        assert(commands == before);
        board_sync_ignition(8);
        board_sync_restart(); /* A USB/link resync must preserve the pending start. */
        for (unsigned i = 0; i < 10; ++i) board_sync_process();
        assert(last_driver_rearm == 1 && commands == before + 9);
        before = commands;
        for (unsigned i = 0; i < 10; ++i) {
            board_sync_ignition(8); board_sync_ignition(10); board_sync_ignition(8);
            board_sync_process();
        }
        assert(commands == before); /* No repeated action during RUN/crank/RUN. */
        board_sync_restart();
        for (unsigned i = 0; i < 10; ++i) board_sync_process();
        assert(last_driver_rearm == 0);
    }
    /* Boot while already running is one startup too. */
    board_sync_rearm(); board_sync_ignition(8);
    for (unsigned i = 0; i < 10; ++i) board_sync_process();
    assert(last_driver_rearm == 1);
}

static void test_direct_hold_other_editors(void) {
    /* Both display profiles, sort order, slot picker and legacy page editors. */
    for (unsigned profile = 0; profile < 2; ++profile) {
        fresh_contract(); to_settings();
        settings_state.ipc_my23_is_installed = profile;
        unsigned count = profile ? 3 : 4;
        for (unsigned i = 0; i < count; ++i) menu_event(MENU_NEXT);
        assert(menu_preferences_save() == 0);
        MenuPreferences original, committed;
        assert(menu_preferences_decode(&original, saved));
        menu_event(MENU_SELECT); menu_event(MENU_BACK);
        menu_event(MENU_SELECT); menu_event(MENU_NEXT); menu_event(MENU_HOLD);
        assert(menu_preferences_decode(&committed, saved));
        assert(committed.alphabetical != original.alphabetical);
        menu_event(MENU_BACK); menu_event(MENU_SELECT); menu_event(MENU_BACK);
        assert(!strstr(screen, "Hold save"));

        fresh_contract(); to_settings();
        settings_state.ipc_my23_is_installed = profile;
        menu_event(MENU_NEXT); /* Legacy: Page favorites; MY23: Favorites slots. */
        if (!profile) for (unsigned i = 0; i < 4; ++i) menu_event(MENU_NEXT);
        menu_event(MENU_SELECT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
        menu_event(MENU_NEXT);
        unsigned writes = preference_writes;
        menu_event(MENU_HOLD); /* Accept and persist directly in the picker. */
        assert(preference_writes == writes + 1);
        menu_event(MENU_BACK);
        assert(!strstr(screen, "Hold save"));
    }
    for (unsigned editor = 1; editor <= 3; ++editor) {
        fresh_contract(); to_settings(); assert(menu_preferences_save() == 0);
        for (unsigned i = 0; i < editor; ++i) menu_event(MENU_NEXT);
        menu_event(MENU_SELECT);
        /* Remove an existing page favorite, hide a page, or move the first favorite. */
        if (editor == 1) {
            for (unsigned i = 0; i < 128 && screen[0] != UI_GLYPH_CHECKED; ++i) menu_event(MENU_NEXT);
            assert(screen[0] == UI_GLYPH_CHECKED);
        }
        menu_event(MENU_SELECT);
        if (editor == 3) menu_event(MENU_NEXT);
        unsigned writes = preference_writes;
        menu_event(MENU_HOLD);
        assert(preference_writes == writes + 1);
        menu_event(MENU_BACK);
        assert(!strstr(screen, "Hold save"));
    }
}
