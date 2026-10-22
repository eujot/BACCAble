#include "test_experiment_vectors.h"

/* Feed a complete ISO-TP response through the public receiver, including long tails. */
static void experiment_reply(uint32_t id, const uint8_t *payload, unsigned length) {
    CAN_RxHeaderTypeDef header = {.IDE = id > 0x7ff ? CAN_ID_EXT : CAN_ID_STD,
                                  .ExtId = id, .StdId = id, .DLC = 8};
    uint8_t frame[8] = {0};
    if (length <= 7) {
        frame[0] = length;
        memcpy(frame + 1, payload, length);
        experiment_receive(&header, frame);
        return;
    }
    frame[0] = 0x10 | (length >> 8); frame[1] = length;
    memcpy(frame + 2, payload, 6);
    experiment_receive(&header, frame);
    assert(query_dlc == 8 && query_payload[0] == 0x30 && query_payload[2] == 5);
    for (unsigned offset = 6, sequence = 1; offset < length; offset += 7, ++sequence) {
        unsigned count = length - offset < 7 ? length - offset : 7;
        frame[0] = 0x20 | (sequence & 15);
        memcpy(frame + 1, payload + offset, count);
        header.DLC = count + 1;
        now += 5;
        experiment_receive(&header, frame);
    }
}

/* Compare all 64 fields against independent source-derived wire vectors. */
static void test_experiment_catalog(void) {
    char value[40];
    unsigned count = 0;
    for (unsigned group = 0; group < EXPERIMENT_GROUP_COUNT; ++group)
        count += experiment_group_count(group);
    assert(count == 64);
    for (unsigned i = 0; i < sizeof(experiment_vectors) / sizeof(experiment_vectors[0]); ++i) {
        experiment_select(experiment_vectors[i].signal);
        experiment_process();
        assert(query_ecu == experiment_vectors[i].request_id && query_dlc == 8);
        assert(query_payload[0] == 3 && query_payload[1] == 0x22);
        assert(query_payload[2] == experiment_vectors[i].data[1]);
        assert(query_payload[3] == experiment_vectors[i].data[2]);
        for (unsigned j = 4; j < 8; ++j) assert(query_payload[j] == 0);
        experiment_reply(experiment_vectors[i].response_id, experiment_vectors[i].data, experiment_vectors[i].length);
        experiment_format(experiment_vectors[i].signal, value, sizeof(value));
        const char *actual = strrchr(value, ' ');
        assert(actual);
        if (strcmp(actual + 1, experiment_vectors[i].value))
            fprintf(stderr, "Vector %u: %s expected %s\n", i, value, experiment_vectors[i].value);
        assert(!strcmp(actual + 1, experiment_vectors[i].value));
        assert(strlen(value) <= 18); /* Smallest supported legacy display. */
    }
    /* Shift state has its own enum; ordinary gear codes must remain unknown. */
    for (unsigned raw = 1; raw <= 13; ++raw) {
        uint8_t reply[] = {0x62, 0x05, 0x1a, raw};
        experiment_select(63); experiment_process();
        experiment_reply(0x18DAF118, reply, sizeof(reply));
        experiment_format(63, value, sizeof(value));
        assert(strstr(value, "--"));
    }
    experiment_cancel();
}

/* Verify failures cannot become a complete value or prevent a later retry. */
static void test_experiment_transport(void) {
    char value[40];
    const uint8_t wheel = experiment_group_signal(5, 0);
    uint8_t payload[180] = {0x62, 0x29, 0x62};
    payload[17] = 0x01; payload[18] = 0x60;
    now = UINT32_MAX - 50;
    experiment_select(wheel); experiment_process();
    experiment_reply(0x18DAF130, payload, sizeof(payload)); /* Sequence wrap and tick wrap. */
    experiment_format(wheel, value, sizeof(value));
    assert(strstr(value, "22.0km/h"));
    experiment_select(wheel); experiment_process();
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .ExtId = 0x18DAF130, .DLC = 8};
    uint8_t first[8] = {0x10, 31, 0x62, 0x29, 0x62};
    experiment_receive(&header, first);
    uint8_t wrong[8] = {0x22};
    experiment_receive(&header, wrong);
    /* Supplying the missing frame afterwards must not revive a corrupt transfer. */
    for (unsigned seq = 1; seq <= 4; ++seq) {
        wrong[0] = 0x20 | seq;
        experiment_receive(&header, wrong);
    }
    experiment_format(wheel, value, sizeof(value));
    assert(strstr(value, "--"));
    now += 1000; experiment_process();
    experiment_reply(0x18DAF130, payload, 31);
    experiment_format(wheel, value, sizeof(value));
    assert(strstr(value, "22.0km/h"));

    experiment_select(0); query_fail = true; experiment_process();
    query_fail = false;
    now += 1000; experiment_process();
    uint8_t battery[] = {0x62, 0xd4, 0x10, 0x64, 0};
    experiment_reply(0x694, battery, sizeof(battery));
    experiment_format(0, value, sizeof(value));
    assert(strstr(value, "50.0%"));
    now += 1000; experiment_process();
    header.IDE = CAN_ID_STD; header.StdId = 0x694;
    uint8_t negative[8] = {3, 0x7f, 0x22, 0x78};
    now += 900; experiment_receive(&header, negative);
    now += 900; experiment_process();
    experiment_reply(0x694, battery, sizeof(battery));
    experiment_format(0, value, sizeof(value));
    assert(strstr(value, "50.0%"));
    now += 1000; experiment_process();
    negative[3] = 0x31; experiment_receive(&header, negative);
    experiment_format(0, value, sizeof(value));
    assert(strstr(value, "--"));
    experiment_cancel();
    unsigned before = queries;
    now += 10000; experiment_process();
    experiment_reply(0x694, battery, sizeof(battery));
    assert(queries == before);

    /* A five-second source interval must not make a valid tire value flicker. */
    uint8_t pressure[] = {0x62, 0x01, 0x3c, 0x09, 0x2e, 70};
    experiment_select(50); experiment_process();
    experiment_reply(0x18DAF1C7, pressure, sizeof(pressure));
    before = queries; now += 4999; experiment_process();
    experiment_format(50, value, sizeof(value));
    assert(strstr(value, "2.35bar") && queries == before);
    ++now; experiment_process(); assert(queries == before + 1);
    now += 1000; experiment_process();
    experiment_format(50, value, sizeof(value)); assert(strstr(value, "--"));
    experiment_cancel();
}

/* Exercise the real menu route and both display profiles, including cancellation. */
static void test_experiment_menu(void) {
    for (unsigned my23 = 0; my23 < 2; ++my23) {
        fresh_menu(); settings_state.ipc_my23_is_installed = my23;
        menu_event(MENU_BACK); /* Favorites to root. */
        for (unsigned i = 0; i < 5; ++i) menu_event(MENU_NEXT);
        menu_event(MENU_SELECT); menu_event(MENU_SELECT);
        assert(menu_experiments_active());
        unsigned before = queries;
        menu_process(); assert(queries == before + 1 && query_ecu == 0x6b4);
        uint8_t reply[] = {0x62, 0xd4, 0x10, 0x64, 0};
        experiment_reply(0x694, reply, sizeof(reply));
        now += 100; menu_process();
        bool found = false;
        for (unsigned j = 0; j + 5 <= sizeof(screen_packet); ++j)
            if (!memcmp(screen_packet + j, "50.0%", 5)) found = true;
        assert(found);
        diagnostics_state.clear_faults_request = 1;
        assert(!menu_experiments_active());
        before = queries; menu_process(); assert(queries == before);
        diagnostics_state.clear_faults_request = 0;
        menu_process(); assert(queries == before + 1);
        menu_event(MENU_BACK); before = queries;
        assert(!menu_experiments_active());
        now += 1000; menu_process(); assert(queries == before);
    }
}
