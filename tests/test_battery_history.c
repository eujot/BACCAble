/* Included by test_menu.c: real persistence, scheduler, requests and menu. */
static void history_reset(void) {
    memset(history_flash, 255, sizeof(history_flash)); history_budget = -1; history_erases = 0;
    battery_history_init();
}
static void test_battery_history_store(void) {
    history_reset();
    for (unsigned session = 0; session < 40; ++session) {
        assert(history_store_prepare()); assert(history_store_begin());
        unsigned erases = history_erases;
        for (unsigned n = 0; n < HISTORY_SAMPLES; ++n)
            assert(history_store_append((HistorySample){n * 10, 12000 + session, session}));
        assert(!history_store_append((HistorySample){720, 12000, 50}));
        assert(history_erases == erases);
        assert(history_store_count() == (session < 10 ? session + 1 : 10));
        history_store_init(history_flash_storage());
        assert(history_store_samples(0) == HISTORY_SAMPLES);
        HistorySample sample;
        assert(history_store_read(0, 72, &sample) && sample.minute == 720 && sample.soc == session);
        if (session >= 9) assert(history_store_read(9, 0, &sample) && sample.soc == session - 9);
    }
    /* Every header/record halfword may be the last one before power loss. */
    for (int cut = 0; cut < 6; ++cut) {
        history_reset(); assert(history_store_begin()); assert(history_store_append((HistorySample){0, 12500, 71}));
        history_budget = cut; assert(!history_store_begin());
        history_store_init(history_flash_storage()); assert(history_store_count() == 1);
        HistorySample sample; assert(history_store_read(0, 0, &sample) && sample.soc == 71);
    }
    for (int cut = 0; cut < 5; ++cut) {
        history_reset(); assert(history_store_begin()); assert(history_store_append((HistorySample){0, 12500, 71}));
        history_budget = cut; assert(!history_store_append((HistorySample){10, 14000, 72}));
        history_store_init(history_flash_storage()); assert(history_store_samples(0) == 1);
    }
    history_reset(); assert(history_store_begin()); assert(history_store_append((HistorySample){0, 12500, 71}));
    history_flash[18] ^= 1; history_store_init(history_flash_storage()); assert(history_store_samples(0) == 0);
}
static void history_tick(uint32_t time, unsigned rpm) {
    now = time; telemetry_state.current_rpm_speed = rpm; telemetry_state.current_rpm_updated_at = now;
    battery_history_ignition(8, now); battery_history_process();
}
static void test_battery_history_runtime(void) {
    ergonomics_reset(VEHICLE_DISPLAY_MY23); history_reset();
    diagnostics_state.clear_faults_request = 0; ibs_override_enable(false);
    parameter_request_cancel(); now += 1000;
    uint32_t start = now;
    history_tick(start, 800); assert(!history_store_count());
    uint8_t source = reading_source(reading_lookup(READING_BATTERY_VOLTAGE), vehicle_profile_current());
    parameter_cache_put(3, 72, start + 2000); parameter_cache_put(source, 14.2f, start + 2000);
    parameter_peak_enable(true); parameter_cache_put(source, 12.4f, start + 2000);
    history_tick(start + 2000, 800); assert(history_store_count() == 1 && history_store_samples(0) == 1);
    HistorySample sample; assert(history_store_read(0, 0, &sample)); assert(sample.voltage == 12400 && sample.soc == 72);
    parameter_peak_enable(false);
    history_tick(start + 599999, 0); assert(history_store_samples(0) == 1); /* Start/Stop keeps session. */
    dashboard_state.baccable_dashboard_menu_visible = 0;
    unsigned sent = queries;
    history_tick(start + 600000, 800); assert(queries == sent + 1);
    const ParameterDefinition *meta = &parameter_definitions[source];
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = meta->response_id, .DLC = 8};
    uint8_t reply[8] = {7, 0x62, meta->request_data >> 16, meta->request_data >> 24, 0, 0, 0, 0};
    parameter_request_receive(&header, reply); assert(isfinite(parameter_cache_get_live(source, now)));
    parameter_cache_put(3, 73, now); battery_history_process(); assert(history_store_samples(0) == 2);
    assert(history_store_read(0, 1, &sample) && sample.minute == 10 && sample.soc == 73);
    diagnostics_state.clear_faults_request = 1; sent = queries;
    history_tick(start + 1200000, 800); history_tick(start + 1205000, 800);
    assert(queries == sent && history_store_samples(0) == 3);
    assert(history_store_read(0, 2, &sample) && sample.voltage == HISTORY_MISSING && sample.soc == 255);
    diagnostics_state.clear_faults_request = 0;
    battery_history_ignition(2, now); assert(!battery_history_current());
    history_tick(now + 100, 800); history_tick(now + 2000, 800); assert(history_store_count() == 2);
    runtime_state.low_consume_is_active = 1; battery_history_process(); assert(!battery_history_current());
    runtime_state.low_consume_is_active = 0;
}
static void test_battery_history_limits(void) {
    ergonomics_reset(VEHICLE_DISPLAY_MY23); history_reset();
    diagnostics_state.clear_faults_request = 0; parameter_cache_reset();
    uint32_t start = UINT32_MAX - 1000U;
    history_tick(start, 800); history_tick(start + 2000U, 800);
    history_tick(start + 8000U, 800); assert(history_store_samples(0) == 0);
    uint8_t source = reading_source(reading_lookup(READING_BATTERY_VOLTAGE), vehicle_profile_current());
    parameter_cache_put(3, 72, start + 9000U); parameter_cache_put(source, 14.2f, start + 9000U);
    history_tick(start + 9000U, 800); assert(history_store_samples(0) == 1);
    for (unsigned n = 1; n <= 72; ++n) {
        uint32_t time = start + n * 600000U;
        parameter_cache_put(3, 73, time); parameter_cache_put(source, 14.1f, time);
        history_tick(time, 800);
    }
    assert(history_store_samples(0) == 73);
    history_tick(start + 43800000U, 800); assert(history_store_samples(0) == 73);
    HistorySample sample; assert(history_store_read(0, 72, &sample) && sample.minute == 720);
    battery_history_ignition(2, now);
    /* Unavailable hardware remains readable as an empty store, never dereferenced. */
    history_store_init((HistoryStorage){0}); assert(history_store_count() == 0);
    assert(!history_store_begin() && !history_store_prepare() && !history_store_read(0, 0, &sample));
}
static void test_battery_history_menu(void) {
    for (unsigned display = VEHICLE_DISPLAY_LEGACY18; display <= VEHICLE_DISPLAY_MY23; ++display) {
        ergonomics_reset(display); history_reset();
        assert(history_store_begin()); assert(history_store_append((HistorySample){0, 12400, 72}));
        assert(history_store_append((HistorySample){10, 14200, 74}));
        ergonomics_root(1);
        for (unsigned n = 0; n < READING_GROUP_COUNT; ++n) {
            char title[25] = {0};
            memcpy(title, screen_packet + 2, display == VEHICLE_DISPLAY_MY23 ? MY23_L1_VISIBLE : 18);
            if (strstr(title, "Battery")) break;
            menu_event(MENU_NEXT);
        }
        assert(ergonomics_has("Battery")); menu_event(MENU_SELECT);
        for (unsigned n = 0; n < READING_COUNT && !ergonomics_has("History"); ++n) menu_event(MENU_NEXT);
        assert(ergonomics_has("History")); menu_event(MENU_SELECT); assert(ergonomics_has("Last start"));
        menu_event(MENU_SELECT); assert(ergonomics_has("74%") && ergonomics_has("14.2"));
        menu_event(MENU_NEXT); assert(ergonomics_has("72%") && ergonomics_has("12.4"));
        menu_event(MENU_BACK); assert(ergonomics_has("Last start"));
        menu_event(MENU_BACK); assert(ergonomics_has("History"));
        menu_event(MENU_BACK); assert(ergonomics_has("Battery"));
    }
}
