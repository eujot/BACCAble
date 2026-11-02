/* Integration regressions for the implementation review; included by test_menu.c. */
static void fixes_reset(void) {
    real_flash = true; fail_save = false; flash_budget = -1;
    memset(flash_pages, 0xff, sizeof(flash_pages));
    memset(&settings_state, 0, sizeof(settings_state));
    memset(&chassis_state, 0, sizeof(chassis_state));
    memset(&comfort_state, 0, sizeof(comfort_state));
    memset(&mirrors_state, 0, sizeof(mirrors_state));
    memset(&runtime_state, 0, sizeof(runtime_state));
    now = 20000; queries = 0; uart_busy = false; query_fail = false;
    setup_load_from_flash();
    VehicleProfile p = *vehicle_profile_current();
    p.model = VEHICLE_MODEL_GIULIA; p.engine = VEHICLE_ENGINE_PETROL20;
    p.display_selection = VEHICLE_DISPLAY_LEGACY18; p.confirmed = true;
    assert(settings_profile_save(&p) == 0);
    dashboard_state.baccable_dashboard_menu_visible = 1;
    menu_init();
}
static void test_profile_atomic_restart(void) {
    fixes_reset();
    settings_state.rain_lights_enabled = 1;
    assert(settings_save() == 0);
    VehicleProfile p = *vehicle_profile_current();
    p.model_year = 2024; p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_PRESENT;
    assert(settings_profile_save(&p) == 0);
    setup_load_from_flash();
    assert(vehicle_profile_current()->model_year == 2024 && settings_state.rain_lights_enabled);
    uint8_t baseline[2][2048]; memcpy(baseline, flash_pages[SETTINGS_RECORD], sizeof(baseline));
    bool succeeded = false;
    for (int cut = 0; cut < 150; ++cut) {
        memcpy(flash_pages[SETTINGS_RECORD], baseline, sizeof(baseline));
        flash_budget = -1; setup_load_from_flash();
        p = *vehicle_profile_current(); p.model_year = 2025;
        settings_state.rain_lights_enabled = 0;
        flash_budget = cut;
        unsigned result = settings_profile_save(&p);
        if (result) assert(vehicle_profile_current()->model_year == 2024);
        flash_budget = -1; setup_load_from_flash();
        assert(vehicle_profile_current()->model_year == (result ? 2024 : 2025));
        assert(settings_state.rain_lights_enabled == (result ? 1 : 0));
        if (!result) { succeeded = true; break; }
    }
    assert(succeeded);
    /* Import a genuine old payload with its enabled preferences, then restart. */
    memset(flash_pages[SETTINGS_RECORD], 0xff, sizeof(flash_pages[SETTINGS_RECORD]));
    uint16_t old[42] = {0}; old[35] = 1; old[39] = 2; old[41] = 1;
    assert(flash_record_save(SETTINGS_RECORD, 0x101, old, sizeof(old)));
    setup_load_from_flash();
    assert(vehicle_profile_current()->engine == VEHICLE_ENGINE_PETROL29);
    assert(vehicle_profile_current()->legacy_permissions && !vehicle_profile_current()->confirmed);
    assert(settings_state.rain_lights_enabled);
    assert(settings_save() == 0);
    setup_load_from_flash();
    assert(settings_state.rain_lights_enabled && vehicle_profile_current()->legacy_permissions);
}
static void test_profile_editor_failure(void) {
    fixes_reset();
    vehicle_editor_begin(); vehicle_editor_select(); vehicle_editor_navigate(1); /* Giulia -> Stelvio */
    fail_save = true;
    assert(!strcmp(vehicle_editor_save(), "Save failed"));
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_GIULIA);
    fail_save = false;
    chassis_state.awd_sequence = 4;
    assert(!strcmp(vehicle_editor_save(), "Stop AWD request"));
    chassis_state.awd_sequence = 0;
    assert(vehicle_editor_save() == NULL);
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_STELVIO && vehicle_profile_current()->confirmed);
    setup_load_from_flash();
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_STELVIO);
}
static void test_favorite_validation_restart(void) {
    fixes_reset();
    UiLayoutRecord record, loaded;
    ui_layout_default(&record, vehicle_profile_current());
    uint8_t wire[UI_LAYOUT_RECORD_SIZE];
    record.favorites.sets[0].rows[0][1] = record.favorites.sets[0].rows[0][0];
    assert(!ui_layout_encode(&record, wire));
    record.favorites.sets[0].rows[0][1] = (FavoriteItem){READING_OIL_LEVEL_BAR, READING_FORMAT_AUTO};
    assert(!ui_layout_encode(&record, wire));
    favorite_set_clear(&record.favorites.sets[0]);
    record.favorites.sets[0].rows[0][0] = (FavoriteItem){READING_SPEED, READING_FORMAT_SHORT};
    assert(ui_layout_encode(&record, wire));
    assert(flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, wire, sizeof(wire)));
    menu_init();
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, wire, sizeof(wire)));
    assert(ui_layout_decode(wire, &loaded));
    assert(loaded.favorites.sets[0].rows[0][0].id == READING_SPEED);
}
static void test_favorite_widths(void) {
    fixes_reset();
    VehicleProfile p = *vehicle_profile_current();
    FavoriteItem wide[4] = {{READING_BEST_100_200, READING_FORMAT_FULL}, FAVORITE_EMPTY_ITEM, FAVORITE_EMPTY_ITEM, FAVORITE_EMPTY_ITEM};
    FavoriteItem narrow[4], resolved[4];
    assert(favorite_row_page(wide, &p, 16, 0, narrow) == 1);
    assert(favorite_row_resolve(narrow, 16, resolved));
    assert(wide[0].format == READING_FORMAT_FULL);
    ReadingSample samples[READING_COUNT] = {0};
    for (unsigned engine = VEHICLE_ENGINE_PETROL20; engine <= VEHICLE_ENGINE_DIESEL22; ++engine) {
        p.engine = engine;
        for (unsigned mode = VEHICLE_DISPLAY_LEGACY18; mode <= VEHICLE_DISPLAY_MY23; ++mode) {
            p.display_selection = mode; vehicle_profile_commit(&p);
            FavoriteLayout layout; favorite_layout_default(&layout, &p);
            DisplayProfile d = vehicle_display_profile_for(&p);
            for (unsigned s = 0; s < 6; ++s) {
                if (d.rows == 2) assert(favorite_set_fits(&layout.sets[s], d));
                for (unsigned r = 0; r < 2; ++r) {
                    unsigned width = d.widths[d.rows == 2 ? r : 0];
                    FavoriteItem part[4];
                    unsigned pages = favorite_row_page(layout.sets[s].rows[r], &p, width, 0, part);
                    for (unsigned page = 0; page < pages; ++page) {
                        favorite_row_page(layout.sets[s].rows[r], &p, width, page, part);
                        for (unsigned scenario = 0; scenario < 3; ++scenario) {
                            for (unsigned i = 0; i < READING_COUNT; ++i)
                                samples[i] = (ReadingSample){scenario ? READING_STATE_VALID : READING_STATE_MISSING,
                                                           scenario == 1 ? 100 : -123456};
                            char line[25];
                            assert(favorite_row_render(part, samples, width, line, sizeof(line)) != FAVORITE_ROW_OVERFLOW);
                            assert(reading_glyph_count(line) <= width);
                            if (!scenario) assert(strchr(line, '#') == NULL);
                        }
                    }
                }
            }
        }
    }
}
static void test_favorite_policy_sources(void) {
    fixes_reset();
    FavoriteSet set; favorite_set_clear(&set);
    set.rows[0][0] = (FavoriteItem){READING_IGNITION_CYL_5, READING_FORMAT_AUTO};
    VehicleProfile p = *vehicle_profile_current(); uint8_t ids[8];
    p.engine = VEHICLE_ENGINE_PETROL29;
    assert(favorite_set_unique_sources(&set, &p, ids) == 1 && ids[0] == 19);
    p.engine = VEHICLE_ENGINE_PETROL20;
    assert(favorite_set_unique_sources(&set, &p, ids) == 0);
    FavoriteItem part[4]; assert(favorite_row_page(set.rows[0], &p, 18, 0, part) == 0);
    assert(set.rows[0][0].id == READING_IGNITION_CYL_5);
    set.rows[0][0].id = READING_ADBLUE_VOLUME; p.engine = VEHICLE_ENGINE_DIESEL22;
    p.equipment[VEHICLE_EQUIPMENT_SCR] = VEHICLE_EQUIPMENT_ABSENT;
    assert(favorite_set_unique_sources(&set, &p, ids) == 0);
    p.equipment[VEHICLE_EQUIPMENT_SCR] = VEHICLE_EQUIPMENT_PRESENT;
    assert(favorite_set_unique_sources(&set, &p, ids) == 1 && ids[0] == 65);
}
static void test_favorite_visible_polling(void) {
    fixes_reset();
    UiLayoutRecord layout; ui_layout_default(&layout, vehicle_profile_current());
    favorite_set_clear(&layout.favorites.sets[0]);
    layout.favorites.sets[0].rows[0][0] = (FavoriteItem){READING_COOLANT_TEMP, READING_FORMAT_AUTO};
    layout.favorites.sets[0].rows[1][0] = (FavoriteItem){READING_GEARBOX_TEMP, READING_FORMAT_AUTO};
    uint8_t wire[UI_LAYOUT_RECORD_SIZE]; assert(ui_layout_encode(&layout, wire));
    assert(flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, wire, sizeof(wire)));
    menu_init(); queries = 0;
    for (unsigned i = 0; i < 4; ++i) {
        now += 600; menu_process();
        assert(query_payload[2] == (uint8_t)(parameter_definitions[42].request_data >> 16));
        assert(query_payload[3] == (uint8_t)(parameter_definitions[42].request_data >> 24));
    }
    assert(queries == 4);
    menu_event(MENU_NEXT); now += 600; menu_process();
    assert(query_payload[2] == (uint8_t)(parameter_definitions[33].request_data >> 16));
    assert(query_payload[3] == (uint8_t)(parameter_definitions[33].request_data >> 24));
}
static void test_profile_effective_options(void) {
    fixes_reset();
    VehicleProfile p = *vehicle_profile_current();
    assert(!vehicle_feature_allowed_for(&p, VEHICLE_FEATURE_AWD_DISABLE));
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_PRESENT;
    assert(vehicle_feature_allowed_for(&p, VEHICLE_FEATURE_DEFAULT_RACE));
    assert(!vehicle_feature_allowed_for(&p, VEHICLE_FEATURE_SOFT_SUSPENSION));
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_ABSENT;
    p.equipment[VEHICLE_EQUIPMENT_SUSPENSION] = VEHICLE_EQUIPMENT_PRESENT;
    assert(vehicle_feature_allowed_for(&p, VEHICLE_FEATURE_SOFT_SUSPENSION));
    p.confirmed = false; p.legacy_permissions = true;
    assert(vehicle_feature_allowed_for(&p, VEHICLE_FEATURE_AWD_DISABLE));
    p.legacy_permissions = false;
    assert(!vehicle_feature_allowed_for(&p, VEHICLE_FEATURE_AWD_DISABLE));
    p.confirmed = true; vehicle_profile_commit(&p);
    settings_state.default_drive_mode = 3; settings_state.default_suspension_softer = 1;
    settings_state.park_mirror = 1;
    board_sync_saved(); now += 60000;
    for (unsigned i = 0; i < 10; ++i) board_sync_process();
    assert(sent_dna == BH_DNA_SOFT_SUSPENSION && last_dna_action == 2);
    assert(sent_mirror == BHcmdFunctParkMirrorDisabled);
    assert(settings_state.default_drive_mode == 3 && settings_state.park_mirror == 1);
}
static void test_runtime_display(void) {
    fixes_reset();
    VehicleProfile p = *vehicle_profile_current(); p.display_selection = VEHICLE_DISPLAY_AUTO; p.model_year = 2024;
    assert(settings_profile_save(&p) == 0);
    setup_load_from_flash(); assert(vehicle_profile_current()->display_selection == VEHICLE_DISPLAY_AUTO);
    vehicle_runtime_init();
    assert(vehicle_display_effective().rows == 1);
    vehicle_runtime_process(); vehicle_runtime_reply(display_token, 1, 0);
    assert(vehicle_display_effective().rows == 2);
    p.display_selection = VEHICLE_DISPLAY_LEGACY24;
    assert(settings_profile_save(&p) == 0);
    uint8_t previous_token = display_token;
    now += VEHICLE_DISPLAY_POLL_MS - 1; vehicle_runtime_process();
    assert(display_token == previous_token);
    now++; vehicle_runtime_process();
    assert(display_token != previous_token);
    now += 1500; /* A queued UART response can take longer than one second. */
    vehicle_runtime_reply(display_token, 1, 0);
    assert(vehicle_display_effective().widths[0] == 24);
    menu_present("123456789012345678901234");
    assert(screen_packet[1] == LEGACY24_PACKET_MARKER);
    assert(!memcmp(screen_packet + 2, "123456789012345678901234", 24));
}
static void test_restore_defaults_flow(void) {
    fixes_reset();
    UiLayoutRecord custom; ui_layout_default(&custom, vehicle_profile_current());
    favorite_set_clear(&custom.favorites.sets[0]);
    custom.favorites.sets[0].rows[0][0] = (FavoriteItem){READING_SPEED, READING_FORMAT_AUTO};
    uint8_t custom_wire[UI_LAYOUT_RECORD_SIZE]; assert(ui_layout_encode(&custom, custom_wire));
    assert(flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, custom_wire, sizeof(custom_wire)));
    menu_init();
    /* Favorites -> root -> Settings -> Favorites -> Restore defaults. */
    menu_event(MENU_BACK);
    for (unsigned i = 0; i < 3; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
    for (unsigned i = 0; i < 6; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
    uint8_t before[UI_LAYOUT_RECORD_SIZE], after[UI_LAYOUT_RECORD_SIZE];
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, before, sizeof(before)));
    fail_save = true; menu_event(MENU_HOLD); fail_save = false;
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, after, sizeof(after)));
    assert(!memcmp(before, after, sizeof(before)));
    menu_event(MENU_BACK); /* cancellation remains possible */
    menu_event(MENU_SELECT); menu_event(MENU_HOLD);
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, after, sizeof(after)));
    assert(memcmp(before, after, sizeof(before)));
    UiLayoutRecord restored; assert(ui_layout_decode(after, &restored));
    assert(restored.favorites.sets[0].rows[0][0].id == READING_OIL_TEMPERATURE);
    menu_init();
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, before, sizeof(before)));
    assert(!memcmp(before, after, sizeof(before)));
    setup_load_from_flash(); assert(vehicle_profile_current()->model == VEHICLE_MODEL_GIULIA);
}

static void test_cancelled_same_did(void) {
    fixes_reset(); now += 1000; parameter_request_cancel(); now += 1000;
    const ParameterDefinition *source = &parameter_definitions[42];
    parameter_request_begin_id(42, 0); unsigned sent = queries;
    now += 50; parameter_request_cancel();
    now += 50; parameter_request_begin_id(42, 0); assert(queries == sent);
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = source->response_id, .DLC = 8};
    uint8_t reply[8] = {7, 0x62, source->request_data >> 16, source->request_data >> 24, 0, 0, 0, 0};
    parameter_request_receive(&header, reply);
    assert(!isfinite(parameter_cache_get(42, now)));
    now += 401; parameter_request_begin_id(42, 0); assert(queries == sent + 1);
    parameter_request_receive(&header, reply);
    assert(isfinite(parameter_cache_get(42, now)));
}

static void test_split_profile_migration(void) {
    fixes_reset();
    memset(flash_pages[SETTINGS_RECORD], 0xff, sizeof(flash_pages[SETTINGS_RECORD]));
    RecordStorage legacy = test_storage(SETTINGS_RECORD); legacy.page_size = 0;
    uint16_t old_settings[42] = {0}; old_settings[35] = 1; old_settings[39] = 2; old_settings[41] = 1;
    VehicleProfile profile = *vehicle_profile_current();
    profile.engine = VEHICLE_ENGINE_PETROL29; profile.model_year = 2024;
    uint8_t old_profile[VEHICLE_PROFILE_WIRE_SIZE]; assert(vehicle_profile_encode(&profile, old_profile));
    old_profile[0] = 1;
    assert(record_save(&legacy, 0x101, old_settings, sizeof(old_settings)));
    assert(record_save(&legacy, 0x107, old_profile, 16));
    uint8_t baseline[2][2048]; memcpy(baseline, flash_pages[SETTINGS_RECORD], sizeof(baseline));
    bool succeeded = false;
    for (int cut = 0; cut < 150; ++cut) {
        memcpy(flash_pages[SETTINGS_RECORD], baseline, sizeof(baseline));
        setup_load_from_flash();
        assert(vehicle_profile_current()->model_year == 2024 && settings_state.rain_lights_enabled);
        flash_budget = cut;
        unsigned result = settings_save();
        flash_budget = -1;
        setup_load_from_flash();
        assert(vehicle_profile_current()->model_year == 2024);
        assert(vehicle_profile_current()->engine == VEHICLE_ENGINE_PETROL29);
        assert(settings_state.rain_lights_enabled);
        if (!result) { succeeded = true; break; }
        /* Retry after partial programming must use another erased slot. */
        assert(settings_save() == 0);
        setup_load_from_flash();
        assert(vehicle_profile_current()->model_year == 2024 && settings_state.rain_lights_enabled);
    }
    assert(succeeded);
}

/* User-facing settings policy and draft regressions for the beta-35 UX correction. */
static void ux_display(bool my23) {
    VehicleProfile p = *vehicle_profile_current();
    p.display_selection = my23 ? VEHICLE_DISPLAY_MY23 : VEHICLE_DISPLAY_LEGACY18;
    assert(settings_profile_save(&p) == 0);
    vehicle_runtime_init(); menu_init();
}
static bool ux_find_setting(const char *label) {
    setup_cancel_edit();
    /* MY23 exposes the setting label even when execution is blocked. */
    for (unsigned i = 0; i < 42; ++i) {
        setup_dashboardPageIndex = i;
        now += 501;
        if (setup_present_my23() && strstr(screen, label)) return true;
    }
    return false;
}
static void test_ux_equipment_settings(void) {
    fixes_reset(); ux_display(true);
    VehicleProfile p = *vehicle_profile_current();
    p.equipment[VEHICLE_EQUIPMENT_LANE_ASSIST] = VEHICLE_EQUIPMENT_ABSENT;
    assert(settings_profile_save(&p) == 0);
    assert(!ux_find_setting("Startup LKA"));
    p.equipment[VEHICLE_EQUIPMENT_LANE_ASSIST] = VEHICLE_EQUIPMENT_UNKNOWN;
    settings_state.lane_assist_disabled = 1; /* Retained preference, blocked execution. */
    assert(settings_profile_save(&p) == 0);
    assert(ux_find_setting("Startup LKA"));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Configure Vehicle", 17));
    setup_stage_select(); setup_move_page(1); setup_stage_save();
    assert(settings_state.lane_assist_disabled == 0); /* Recovery remains available. */
    p.equipment[VEHICLE_EQUIPMENT_LANE_ASSIST] = VEHICLE_EQUIPMENT_PRESENT;
    assert(settings_profile_save(&p) == 0);
    assert(ux_find_setting("Startup LKA"));
}
static void test_ux_race_suspension(void) {
    fixes_reset(); ux_display(true);
    VehicleProfile p = *vehicle_profile_current();
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_ABSENT;
    p.equipment[VEHICLE_EQUIPMENT_SUSPENSION] = VEHICLE_EQUIPMENT_PRESENT;
    assert(settings_profile_save(&p) == 0);
    settings_state.default_drive_mode = 3;
    assert(ux_find_setting("Default mode"));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Not on vehicle", 14));
    setup_stage_select(); setup_move_page(-1); setup_stage_save();
    assert(settings_state.default_drive_mode == 2); setup_cancel_edit();
    assert(ux_find_setting("Default mode"));
    setup_stage_select(); setup_move_page(1); setup_stage_save();
    assert(settings_state.default_drive_mode == 1); /* D skips unsupported Race to A. */
    setup_cancel_edit();
    settings_state.default_drive_mode = 0; settings_state.default_suspension_softer = 1;
    assert(ux_find_setting("Suspension"));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Set D/Race first", 16));
    setup_stage_select(); setup_move_page(1); setup_stage_save();
    assert(!settings_state.default_suspension_softer); /* A stale preference can be cleared. */
    setup_cancel_edit();
    settings_state.default_drive_mode = 2; assert(ux_find_setting("Suspension"));
    setup_stage_select(); setup_move_page(1);
    settings_state.default_drive_mode = 0; setup_stage_save();
    assert(!settings_state.default_suspension_softer); /* Recheck prerequisites at save. */
    setup_cancel_edit();
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_PRESENT;
    assert(settings_profile_save(&p) == 0);
    settings_state.default_drive_mode = 2;
    assert(ux_find_setting("Default mode"));
    setup_stage_select(); setup_move_page(1); setup_stage_save();
    assert(settings_state.default_drive_mode == 3);
}
static void test_ux_vehicle_direct_save(void) {
    fixes_reset();
    vehicle_editor_begin(); assert(!vehicle_editor_back()); /* Clean exit needs no decision. */
    vehicle_editor_begin(); vehicle_editor_select(); vehicle_editor_navigate(1);
    fail_save = true; assert(!strcmp(vehicle_editor_save(), "Save failed"));
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_GIULIA);
    fail_save = false; assert(!vehicle_editor_save());
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_STELVIO);
    assert(!vehicle_editor_back());
    vehicle_editor_begin(); vehicle_editor_select(); vehicle_editor_navigate(-1);
    assert(vehicle_editor_back()); assert(vehicle_editor_back());
    vehicle_editor_navigate(1); vehicle_editor_select(); /* Confirmation locks editing. */
    assert(!vehicle_editor_back()); vehicle_editor_cancel();
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_STELVIO);
}
static void ux_open_favorite_picker(void) {
    menu_event(MENU_BACK); /* Favorites -> root. */
    for (unsigned i = 0; i < 3; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
    menu_event(MENU_SELECT); menu_event(MENU_SELECT); menu_event(MENU_SELECT);
}
static void test_ux_favorite_drafts(void) {
    fixes_reset(); ux_display(true);
    UiLayoutRecord layout; ui_layout_default(&layout, vehicle_profile_current());
    uint8_t initial[UI_LAYOUT_RECORD_SIZE]; assert(ui_layout_encode(&layout, initial));
    assert(flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, initial, sizeof(initial)));
    menu_init();
    ux_open_favorite_picker();
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Reading -> format, still unaccepted. */
    menu_event(MENU_BACK); menu_event(MENU_BACK); menu_event(MENU_BACK);
    assert(strstr(screen, "Row A")); /* No dirty confirmation after cancelling both choices. */
    menu_event(MENU_SELECT); menu_event(MENU_SELECT); menu_event(MENU_NEXT);
    for (unsigned i = 0; i <= READING_COUNT && !strstr(screen, "Empty"); ++i) menu_event(MENU_NEXT);
    assert(strstr(screen, "Empty"));
    fail_save = true; menu_event(MENU_HOLD); /* Direct picker save must attempt storage. */
    assert(strstr(screen, "Save failed"));
    fail_save = false;
    menu_event(MENU_BACK); /* Dirty decision, split prompt. */
    assert(!memcmp(screen_packet + 2, "HOLD: SAVE", 10));
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "2X: DISCARD", 11));
    menu_event(MENU_SELECT); menu_event(MENU_NEXT);
    assert(!memcmp(screen_packet + 2, "HOLD: SAVE", 10));
    menu_event(MENU_BACK); assert(strstr(screen, "Row A"));
    uint8_t retained[UI_LAYOUT_RECORD_SIZE];
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, retained, sizeof(retained)));
    assert(!memcmp(initial, retained, sizeof(initial)));
    menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    for (unsigned i = 0; i <= READING_COUNT && !strstr(screen, "Empty"); ++i) menu_event(MENU_NEXT);
    menu_event(MENU_HOLD);
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "SAVED", 5));
    now += 4000; menu_render();
    menu_event(MENU_BACK); menu_event(MENU_NEXT); /* Row B has one reading. */
    menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    for (unsigned i = 0; i <= READING_COUNT && !strstr(screen, "Oil temp"); ++i) menu_event(MENU_NEXT);
    assert(strstr(screen, "Oil temp"));
    menu_event(MENU_SELECT); menu_event(MENU_HOLD); /* Restore original reading from format picker. */
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "SAVED", 5));
}

static void test_ux_mirror_retry_hint(void) {
    fixes_reset(); ux_display(true);
    VehicleProfile p = *vehicle_profile_current();
    p.equipment[VEHICLE_EQUIPMENT_PARK_MIRROR] = VEHICLE_EQUIPMENT_PRESENT;
    assert(settings_profile_save(&p) == 0);
    settings_state.park_mirror = 1;
    assert(ux_find_setting("Park mirror"));
    setup_stage_select(); setup_move_page(1); setup_stage_select();
    uart_busy = true; setup_stage_save();
    uart_busy = false; now += 501; assert(setup_present_my23());
    assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "HOLD STORE", 10));
    setup_cancel_edit();
}

static void test_ux_reading_groups(void) {
    for (unsigned diesel = 0; diesel < 2; ++diesel) {
        fixes_reset(); ux_display(true);
        VehicleProfile p = *vehicle_profile_current();
        p.engine = diesel ? VEHICLE_ENGINE_DIESEL22 : VEHICLE_ENGINE_PETROL20;
        assert(settings_profile_save(&p) == 0); menu_init();
        menu_event(MENU_BACK); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        bool seen_dpf = false;
        for (unsigned g = 0; g < READING_GROUP_COUNT; ++g) {
            seen_dpf |= strstr(screen, "Diesel") != NULL;
            if (!diesel) assert(memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Diesel / DPF", 12));
            menu_event(MENU_NEXT);
        }
        assert(seen_dpf == !!diesel);
    }
    fixes_reset(); ux_display(true); ux_open_favorite_picker();
    for (unsigned i = 0; i <= READING_COUNT; ++i) {
        assert(!strstr(screen, "DPF") && !strstr(screen, "AdBlue") && !strstr(screen, "EGR"));
        menu_event(MENU_NEXT);
    }
}

static void test_direction_recovery(void) {
    MenuInput rapid = {0}; menu_input_update(&rapid,0x10,true,0);
    for (unsigned n=0;n<100;++n) {
        assert(menu_input_update(&rapid,0x18,true,n*40+10)==MENU_NEXT);
        assert(menu_input_update(&rapid,0x10,true,n*40+20)==MENU_NONE);
    }
    const uint8_t buttons[] = {0x08, 0x18, 0x00, 0x20};
    const MenuEvent events[] = {MENU_PREVIOUS, MENU_NEXT, MENU_PREVIOUS_GROUP, MENU_NEXT_GROUP};
    for (unsigned i=0; i<4; ++i) {
        MenuInput input = {0};
        menu_input_update(&input, 0x10, true, 0);
        assert(menu_input_update(&input, buttons[i], true, 1000) == events[i]);
        assert(input.stream_gaps == 1);
        assert(menu_input_update(&input, buttons[i], true, 1401) == MENU_NONE);
        assert(!input.armed); /* Stale hold still requires a release. */
        menu_input_update(&input, 0x10, true, 1420);
        assert(menu_input_update(&input, buttons[i], true, 1440) == events[i]);
    }
    MenuInput input = {0};
    menu_input_update(&input, 0x10, true, 0);
    assert(menu_input_update(&input, 0x08, true, 20) == MENU_PREVIOUS);
    assert(menu_input_update(&input, 0x18, true, 40) == MENU_NEXT);
    assert(menu_input_update(&input, 0x08, true, 60) == MENU_PREVIOUS);
    assert(menu_input_update(&input, 0x00, true, 80) == MENU_PREVIOUS_GROUP);
    assert(menu_input_update(&input, 0x08, true, 100) == MENU_NONE); /* Relaxing pressure is not a new click. */
    for (unsigned t=120;t<=600;t+=20) menu_input_update(&input,0x08,true,t);
    assert(menu_input_repeat(&input,true,600)==MENU_PREVIOUS);
    assert(menu_input_repeat(&input,true,601)==MENU_NONE);
    menu_input_update(&input,0x08,true,780);
    assert(menu_input_repeat(&input,true,780)==MENU_PREVIOUS);
    menu_input_update(&input,0x08,false,800);
    assert(menu_input_update(&input,0x18,true,820)==MENU_NONE);
    input=(MenuInput){0}; menu_input_update(&input,0x10,true,0);
    assert(menu_input_update(&input,0x90,true,1000)==MENU_NONE && !input.armed);
    assert(menu_input_update(&input,0x90,true,2000)==MENU_NONE);
    input=(MenuInput){0}; menu_input_update(&input,0x10,true,UINT32_MAX-100);
    assert(menu_input_update(&input,0x18,true,300)==MENU_NEXT);
}

/* Observe actual outgoing screen packets, not private scheduling flags. */
static void test_navigation_display_latency(void) {
    const uint8_t profiles[] = {VEHICLE_DISPLAY_LEGACY18, VEHICLE_DISPLAY_LEGACY24, VEHICLE_DISPLAY_MY23};
    for (unsigned profile = 0; profile < 3; ++profile) {
        fixes_reset();
        VehicleProfile p = *vehicle_profile_current();
        p.display_selection = profiles[profile];
        assert(settings_profile_save(&p) == 0);
        vehicle_runtime_init(); menu_init();
        vehicle_runtime_process(); vehicle_runtime_reply(display_token, 1, 0);
        menu_event(MENU_BACK); /* Root: every step has a different label. */
        menu_button(0x10, true);
        now += 10; menu_button(0x18, true);
        uint8_t first[UART_SCREEN_BUFFER_SIZE]; memcpy(first, screen_packet, sizeof(first));
        now += 10; menu_button(0x10, true);
        now += 10; menu_button(0x18, true); /* Logical Actions; still showing Readings. */
        assert(!memcmp(first, screen_packet, sizeof(first)));
        now += 10; menu_button(0x10, true); /* Release immediately flushes Actions. */
        assert(memcmp(first, screen_packet, sizeof(first)));
        now += 10; menu_button(0x08, true); /* Reversal immediately shows Readings. */
        assert(!memcmp(first, screen_packet, sizeof(first)));
        now += 10; menu_button(0x18, true);
        assert(memcmp(first, screen_packet, sizeof(first)));
        now += 10; menu_button(0x10, true);
        now += 10; menu_button(0x18, true); /* Settings */
        now += 10; menu_button(0x10, true);
        menu_event(MENU_SELECT);
        now += 10; menu_button(0x18, true);
        uint32_t start = now;
        now = start + 200; menu_button(0x18, true);
        now = start + 400; menu_button(0x18, true);
        now = start + 500; menu_button(0x18, true); /* First repeat unchanged. */
        memcpy(first, screen_packet, sizeof(first));
        unsigned interval = profile == 0 ? 240 : 280;
        now = start + 680; menu_button(0x18, true);
        assert(!memcmp(first, screen_packet, sizeof(first)));
        now = start + 500 + interval - 1; menu_button(0x18, true);
        assert(!memcmp(first, screen_packet, sizeof(first)));
        now++; menu_button(0x18, true);
        assert(memcmp(first, screen_packet, sizeof(first)));
    }
}

static void ergonomics_reset(uint8_t display) {
    fixes_reset();
    VehicleProfile p = *vehicle_profile_current(); p.display_selection = display;
    assert(settings_profile_save(&p) == 0);
    UiLayoutRecord layout; ui_layout_default(&layout, &p);
    favorite_set_clear(&layout.favorites.sets[0]);
    layout.favorites.sets[0].rows[0][0] = (FavoriteItem){READING_OIL_TEMPERATURE, READING_FORMAT_SHORT};
    uint8_t wire[UI_LAYOUT_RECORD_SIZE]; assert(ui_layout_encode(&layout, wire));
    assert(flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, wire, sizeof(wire)));
    vehicle_runtime_init(); menu_init();
    vehicle_runtime_process(); vehicle_runtime_reply(display_token, 1, 0);
}
static bool ergonomics_has(const char *label) {
    char text[MY23_PACKET_SIZE + 1];
    memcpy(text, screen_packet + 2, MY23_PACKET_SIZE - 1);
    text[MY23_PACKET_SIZE - 1] = 0;
    return strstr(text, label) != NULL;
}
static void ergonomics_root(unsigned entry) {
    menu_event(MENU_BACK);
    for (unsigned i = 0; i < entry; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
}
static ReadingFormat ergonomics_saved_format(void) {
    uint8_t wire[UI_LAYOUT_RECORD_SIZE]; UiLayoutRecord layout;
    assert(flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, wire, sizeof(wire)));
    assert(ui_layout_decode(wire, &layout));
    return layout.favorites.sets[0].rows[0][0].format;
}
static void test_ergonomics_favorites(void) {
    for (uint8_t display = VEHICLE_DISPLAY_LEGACY18; display <= VEHICLE_DISPLAY_MY23; ++display) {
        ergonomics_reset(display); ux_open_favorite_picker();
        menu_event(MENU_SELECT);
        assert(ergonomics_has("Short"));
        menu_event(MENU_PREVIOUS); /* Medium draft. */
        menu_event(MENU_BACK); menu_event(MENU_SELECT);
        assert(ergonomics_has("Medium"));
        now += 61000; menu_process();
        assert(!menu_parameters_active() && ergonomics_has("Medium"));
        assert(ergonomics_saved_format() == READING_FORMAT_SHORT);
        menu_event(MENU_HOLD);
        assert(ergonomics_saved_format() == READING_FORMAT_MEDIUM);
        now += 3100; menu_render();
        menu_event(MENU_SELECT); menu_event(MENU_HOLD); /* Same reading from picker keeps format. */
        assert(ergonomics_saved_format() == READING_FORMAT_MEDIUM);
        now += 3100; menu_render();
        menu_event(MENU_SELECT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        assert(ergonomics_has("Auto")); /* Only a new reading defaults to Auto. */
        menu_event(MENU_BACK); menu_event(MENU_BACK);
        assert(ergonomics_saved_format() == READING_FORMAT_MEDIUM);

        ergonomics_reset(display); ergonomics_root(3);
        menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        for (unsigned i = 0; i < 5; ++i) menu_event(MENU_NEXT);
        if (display == VEHICLE_DISPLAY_MY23)
            assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "Restore defaults", 16));
        menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        assert(ergonomics_has(display == VEHICLE_DISPLAY_MY23 ? "Reset favorites?" : "Reset fav? HOLD"));
        menu_event(MENU_BACK);
        assert(ergonomics_saved_format() == READING_FORMAT_SHORT);
    }
}
static void test_ergonomics_vehicle_idle(void) {
    for (uint8_t display = VEHICLE_DISPLAY_LEGACY18; display <= VEHICLE_DISPLAY_MY23; ++display) {
        ergonomics_reset(display); ergonomics_root(4);
        assert(ergonomics_has("Giulia"));
        menu_event(MENU_SELECT);
        if (display != VEHICLE_DISPLAY_MY23) assert(screen_packet[2] == '>');
        menu_event(MENU_NEXT);
        assert(vehicle_editor_dirty());
        now += 61000; menu_process();
        assert(!menu_parameters_active() && ergonomics_has("Stelvio"));
        assert(vehicle_profile_current()->model == VEHICLE_MODEL_GIULIA);
        fail_save = true; menu_event(MENU_HOLD);
        now += 2500; menu_render();
        assert(ergonomics_has(display == VEHICLE_DISPLAY_MY23 ? "HOLD: RETRY" : "HOLD retry"));
        fail_save = false; menu_event(MENU_HOLD);
        assert(vehicle_profile_current()->model == VEHICLE_MODEL_STELVIO);
        now += 61000; menu_process();
        assert(menu_parameters_active()); /* Clean drafts still time out. */

        ergonomics_reset(display); ergonomics_root(4);
        /* All equipment values must survive the Legacy label budget. */
        for (unsigned field = 0; field < 13; ++field) {
            if (field >= 5) {
                assert(ergonomics_has("Unknown"));
                menu_event(MENU_SELECT);
                assert(ergonomics_has("Unknown"));
                if (display != VEHICLE_DISPLAY_MY23) assert(screen_packet[2] == '>');
                menu_event(MENU_BACK);
            }
            menu_event(MENU_NEXT);
        }
    }
}
static void test_ergonomics_features_and_info(void) {
    ergonomics_reset(VEHICLE_DISPLAY_MY23); ergonomics_root(3); menu_event(MENU_SELECT);
    assert(ux_find_setting("Shift light"));
    assert(ergonomics_has(settings_state.shift_indicator_enabled ? "ON" : "OFF"));
    uint8_t original = settings_state.shift_indicator_enabled;
    menu_event(MENU_SELECT); menu_event(MENU_NEXT);
    now += 61000; menu_process();
    assert(setup_stage_dirty() && !menu_parameters_active());
    assert(settings_state.shift_indicator_enabled == original);
    menu_event(MENU_HOLD);
    assert(settings_state.shift_indicator_enabled != original);
    now += 61000; menu_process(); assert(menu_parameters_active());
    ergonomics_reset(VEHICLE_DISPLAY_MY23);
    assert(ux_find_setting("Default mode"));
    assert(screen_packet[2 + MY23_L1_VISIBLE] == 'N');
    setup_move_page(1);
    /* Suspension is policy-filtered with an unknown vehicle; verify with known equipment. */
    VehicleProfile p = *vehicle_profile_current();
    p.equipment[VEHICLE_EQUIPMENT_SUSPENSION] = VEHICLE_EQUIPMENT_PRESENT;
    assert(settings_profile_save(&p) == 0);
    assert(ux_find_setting("Default mode")); setup_move_page(1); setup_present_my23();
    assert(ergonomics_has("Suspension"));
    for (uint8_t display = VEHICLE_DISPLAY_LEGACY18; display <= VEHICLE_DISPLAY_LEGACY24; ++display) {
        ergonomics_reset(display); ergonomics_root(5);
        for (unsigned i = 0; i < 3; ++i) menu_event(MENU_NEXT);
        assert(ergonomics_has(display == VEHICLE_DISPLAY_LEGACY18 ? "Legacy 18" : "Legacy 24"));
    }
}
static void test_ergonomics_action_confirmation(void) {
    ergonomics_reset(VEHICLE_DISPLAY_MY23);
    settings_state.clear_faults_enabled = 1;
    ergonomics_root(2);
    for (unsigned i = 0; i < 13 && !ergonomics_has("Clear DTCs"); ++i) menu_event(MENU_NEXT);
    /* A preview may contain the target; advance until it is the current row. */
    if (memcmp(screen_packet + 4, "Clear DTCs", 10)) menu_event(MENU_NEXT);
    assert(!memcmp(screen_packet + 4, "Clear DTCs", 10));
    menu_event(MENU_SELECT);
    assert(ergonomics_has("Clear all ECUs?"));
    now += 5000; menu_render();
    assert(ergonomics_has("Clear all ECUs?"));
    menu_event(MENU_HOLD);
    assert(diagnostics_state.clear_faults_request == 255);
    diagnostics_state.clear_faults_request = 0;

    ergonomics_reset(VEHICLE_DISPLAY_MY23); settings_state.clear_faults_enabled = 1;
    ergonomics_root(2);
    for (unsigned i = 0; i < 13 && memcmp(screen_packet + 4, "Clear DTCs", 10); ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); now += 6001; menu_event(MENU_HOLD);
    assert(!diagnostics_state.clear_faults_request);
}

static void test_ergonomics_validation_labels(void) {
    for (uint8_t display = VEHICLE_DISPLAY_LEGACY18; display <= VEHICLE_DISPLAY_MY23; ++display) {
        ergonomics_reset(display);
        UiLayoutRecord layout; ui_layout_default(&layout, vehicle_profile_current());
        favorite_set_clear(&layout.favorites.sets[0]);
        layout.favorites.sets[0].rows[1][0] = (FavoriteItem){READING_OIL_TEMPERATURE, READING_FORMAT_SHORT};
        uint8_t wire[UI_LAYOUT_RECORD_SIZE]; assert(ui_layout_encode(&layout, wire));
        assert(flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, wire, sizeof(wire)));
        menu_init(); vehicle_runtime_process(); vehicle_runtime_reply(display_token, 1, 0);
        ergonomics_root(3); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        menu_event(MENU_SELECT); menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Row B. */
        menu_event(MENU_NEXT);
        assert(ergonomics_has("Slot 2: Empty"));
        menu_event(MENU_SELECT);
        menu_event(MENU_NEXT); /* Empty wraps to Oil temp; do not match the next-row preview. */
        menu_event(MENU_HOLD);
        assert(ergonomics_has("Row B"));
        assert(ergonomics_has(display == VEHICLE_DISPLAY_MY23 ? "Duplicate reading" : "duplicate"));
        now += 2500; menu_render(); assert(ergonomics_has("Row B"));
    }
    /* Exercise production packet encoding with every action label and full prompts.
     * List ellipses are intentional; confirmation and error text must fit in full. */
    const char *labels[] = {"QV exhaust req", "Press HAS button", "Toggle ESC/TC", "Toggle Dyno mode",
        "Brake override", "Disable launch", "AWD off request", "Rain test", "Read BCM faults",
        "Clear DTCs", "Reset best times", "Peak hold", "IBS SOC override", "Edit favorites"};
    ergonomics_reset(VEHICLE_DISPLAY_MY23);
    for (unsigned i = 0; i < sizeof(labels)/sizeof(labels[0]); ++i) {
        now += 501; menu_present_view(UI_MODE_LIST, labels[i], "next");
        unsigned n = strlen(labels[i]);
        assert(!memcmp(screen_packet + 4, labels[i], n > 14 ? 13 : n));
        if (n > 14) assert(screen_packet[1 + MY23_L1_VISIBLE] == 0x87);
        assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, "next", 4));
    }
    const char *titles[] = {"Reset favorites?", "Brake + launch?", "Dyno + ESC?", "Clear all ECUs?", "Save failed"};
    const char *hints[] = {"HOLD RESET / 2X BACK", "HOLD APPLY / 2X BACK", "HOLD: RETRY", "RES RETRY / 2X BACK"};
    for (unsigned i = 0; i < sizeof(titles)/sizeof(titles[0]); ++i)
        for (unsigned j = 0; j < sizeof(hints)/sizeof(hints[0]); ++j) {
            assert(strlen(titles[i]) <= MY23_L1_VISIBLE && strlen(hints[j]) <= MY23_L2_VISIBLE);
            menu_present_view(UI_MODE_CONFIRM, titles[i], hints[j]);
            assert(!memcmp(screen_packet + 2, titles[i], strlen(titles[i])));
            assert(!memcmp(screen_packet + 2 + MY23_L1_VISIBLE, hints[j], strlen(hints[j])));
        }
}
