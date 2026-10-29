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
    assert(vehicle_editor_save()); /* Open confirmation. */
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
    now += 1100; vehicle_runtime_process(); vehicle_runtime_reply(display_token, 1, 0);
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
