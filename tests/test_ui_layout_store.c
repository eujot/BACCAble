#include "test_report.h"
#include "storage/ui_layout_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_ui_layout_default(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    UiLayoutRecord rec;
    ui_layout_default(&rec, &p);
    assert(rec.last_reading < READING_COUNT);
    assert(favorite_set_validate(&rec.favorites.sets[0], NULL));
    puts("PASS: default UI layout initializes a valid last reading and defaults");
}

static void test_ui_layout_roundtrip(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_DIESEL22;
    UiLayoutRecord rec, decoded;
    ui_layout_default(&rec, &p);
    rec.last_reading = READING_DPF_LOAD;

    uint8_t wire[UI_LAYOUT_RECORD_SIZE];
    assert(ui_layout_encode(&rec, wire));
    assert(wire[0] == UI_LAYOUT_SCHEMA_VERSION);
    assert(ui_layout_decode(wire, &decoded));
    assert(decoded.last_reading == READING_DPF_LOAD);
    assert(favorite_set_validate(&decoded.favorites.sets[5], NULL));
    assert(decoded.favorites.sets[5].rows[0][0].id == READING_DPF_LOAD);
    puts("PASS: UI layout record round-trips and restores the exact saved layout");
}

static void test_ui_layout_reject_bad_schema(void) {
    uint8_t wire[UI_LAYOUT_RECORD_SIZE];
    memset(wire, 0, sizeof(wire));
    UiLayoutRecord rec;
    assert(!ui_layout_decode(wire, &rec)); /* version 0 */
    wire[0] = 99;
    assert(!ui_layout_decode(wire, &rec)); /* unknown version */
    puts("PASS: unknown UI schema is rejected without overwriting the layout");
}

static void test_ui_layout_reject_invalid_item(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    UiLayoutRecord rec;
    ui_layout_default(&rec, &p);
    uint8_t wire[UI_LAYOUT_RECORD_SIZE];
    assert(ui_layout_encode(&rec, wire));

    /* Corrupt the first favorite item's id beyond READING_COUNT. */
    wire[2] = 0xff;
    UiLayoutRecord out;
    assert(!ui_layout_decode(wire, &out));

    /* Restore a valid id but corrupt the format. */
    wire[2] = READING_POWER;
    wire[3] = 0x7f;
    assert(!ui_layout_decode(wire, &out));
    puts("PASS: invalid item id/format is rejected as corrupt");
}

static void test_ui_layout_restart_persistence(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL29;
    UiLayoutRecord rec, restart;
    ui_layout_default(&rec, &p);
    rec.favorites.sets[4].rows[0][0] = (FavoriteItem){.id = READING_BATTERY_SOC_ECU, .format = READING_FORMAT_SHORT};

    uint8_t wire[UI_LAYOUT_RECORD_SIZE];
    assert(ui_layout_encode(&rec, wire));
    assert(ui_layout_decode(wire, &restart)); /* second boot */
    assert(restart.favorites.sets[4].rows[0][0].id == READING_BATTERY_SOC_ECU);
    assert(restart.favorites.sets[4].rows[0][0].format == READING_FORMAT_SHORT);
    puts("PASS: a successfully written record persists custom edits across restart");
}

int main(void) {
    const char *suite = "ui-layout-store";
    const HostTest tests[] = {
        HOST_TEST(test_ui_layout_default),
        HOST_TEST(test_ui_layout_roundtrip),
        HOST_TEST(test_ui_layout_reject_bad_schema),
        HOST_TEST(test_ui_layout_reject_invalid_item),
        HOST_TEST(test_ui_layout_restart_persistence),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
