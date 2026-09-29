/* Included by test_menu.c to reuse the board transport and settings fixture. */
static uint8_t setup_page_for(uint8_t slot) {
    const SetupParam *param = setup_find_by_flash_index(slot);
    assert(param && param->menu_text);
    setup_cancel_edit();
    char expected[DASHBOARD_MESSAGE_MAX_LENGTH + 1] = {0};
    if (param->render) {
        param->render();
        memcpy(expected, dashboard_setup_screen, DASHBOARD_MESSAGE_MAX_LENGTH);
    } else if (param->entry_type == UI_ENTRY_TOGGLE)
        ui_render_checkbox(expected, sizeof(expected), param->menu_text, *(uint8_t *)param->value != 0);
    else if (param->entry_type == UI_ENTRY_SUBMENU)
        ui_render_action(expected, sizeof(expected), param->menu_text);
    else
        snprintf_(expected, sizeof(expected), "%s", param->menu_text);
    for (uint8_t page = 0; page < 40; ++page) {
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
    assert(!setup_stage_active());
}
static void test_setup_ui(void) {
    fresh_menu();
    const uint8_t enum_slots[] = {16, 20, 24, 25, 26};
    const uint8_t enum_sizes[] = {3, 9, 3, 3, 3};
    for (unsigned entry = 0; entry < sizeof(enum_slots); ++entry) {
        const SetupParam *param = setup_find_by_flash_index(enum_slots[entry]);
        assert(param->entry_type == UI_ENTRY_ENUM);
        *(uint8_t *)param->value = 0;
        settings_state.gasoline_v6 = 0;
        uint8_t enum_page = setup_page_for(enum_slots[entry]);
        setup_select_page(enum_page);
        for (unsigned step = 1; step < enum_sizes[entry]; ++step) setup_select_page(enum_page);
        assert(*(uint8_t *)param->value == 0); /* Full cycle stayed in the draft. */
        setup_cancel_edit();
        /* Move one step backward from the first choice and commit the wrap. */
        setup_select_page(enum_page);
        setup_move_page(-1); setup_move_page(-1);
        setup_accept_draft();
        assert(*(uint8_t *)param->value == (enum_slots[entry] == 16 ? 1 : enum_sizes[entry] - 1));

    }
    settings_state.rotate_readings = 0;
    uint8_t page = setup_page_for(33);
    setup_select_page(page);
    assert(!settings_state.rotate_readings && setup_stage_active());
    setup_accept_draft();
    assert(settings_state.rotate_readings);
    setup_select_page(page);
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
    setup_accept_draft();
    assert(settings_state.usb_sniffer && !settings_state.usb_elm327);
    setup_select_page(page);
    setup_accept_draft();
    assert(!settings_state.usb_sniffer);
#ifdef ACT_AS_ELM327
    assert(settings_state.usb_elm327);
    setup_select_page(page);
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
