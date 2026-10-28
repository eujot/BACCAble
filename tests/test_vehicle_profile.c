#include "test_report.h"
#include "vehicle/vehicle_profile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_vehicle_profile_default(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    assert(p.model == VEHICLE_MODEL_UNKNOWN);
    assert(p.engine == VEHICLE_ENGINE_UNKNOWN);
    assert(p.model_year == 0);
    assert(p.drivetrain == VEHICLE_DRIVETRAIN_UNKNOWN);
    assert(p.display_selection == VEHICLE_DISPLAY_AUTO);
    assert(!p.confirmed);
    for (unsigned i = 0; i < VEHICLE_EQUIPMENT_COUNT; ++i)
        assert(p.equipment[i] == VEHICLE_EQUIPMENT_UNKNOWN);
    puts("PASS: profile defaults are all Unknown/unconfirmed");
}

static void test_vehicle_profile_validate(void) {
    VehicleProfile p;
    VehicleProfileError e = (VehicleProfileError)99;
    vehicle_profile_default(&p);
    assert(vehicle_profile_validate(&p, &e));
    assert(e == VEHICLE_PROFILE_OK);

    p.model = (VehicleModel)7;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_MODEL);
    p.model = VEHICLE_MODEL_GIULIA;

    p.engine = (VehicleEngine)9;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_ENGINE);
    p.engine = VEHICLE_ENGINE_PETROL20;

    p.model_year = 1999;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_YEAR);
    p.model_year = 2100;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_YEAR);
    p.model_year = 2020;
    assert(vehicle_profile_validate(&p, &e));

    p.drivetrain = (VehicleDrivetrain)5;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_DRIVETRAIN);
    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;

    p.display_selection = (VehicleDisplaySelection)8;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_DISPLAY);
    p.display_selection = VEHICLE_DISPLAY_AUTO;

    p.equipment[0] = (VehicleEquipment)7;
    assert(!vehicle_profile_validate(&p, &e) && e == VEHICLE_PROFILE_ERR_EQUIPMENT);
    p.equipment[0] = VEHICLE_EQUIPMENT_UNKNOWN;

    assert(vehicle_profile_validate(&p, &e));
    puts("PASS: profile validation bounds every enum and the year range");
}

static void test_vehicle_profile_engine_helpers(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    assert(vehicle_profile_is_gasoline(&p) && !vehicle_profile_is_diesel(&p));
    assert(!vehicle_profile_is_v6(&p));
    assert(vehicle_profile_cylinders(&p) == 4);

    p.engine = VEHICLE_ENGINE_PETROL29;
    assert(vehicle_profile_is_v6(&p));
    assert(vehicle_profile_cylinders(&p) == 6);

    p.engine = VEHICLE_ENGINE_DIESEL22;
    assert(vehicle_profile_is_diesel(&p) && !vehicle_profile_is_gasoline(&p));
    assert(vehicle_profile_cylinders(&p) == 4);
    puts("PASS: engine class helpers never infer engine from model");
}

static void test_vehicle_feature_support(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.confirmed = true;

    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_AWD_DISABLE) == VEHICLE_SUPPORT_UNKNOWN);
    p.drivetrain = VEHICLE_DRIVETRAIN_Q4;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_AWD_DISABLE) == VEHICLE_SUPPORT_SUPPORTED);

    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_PRESENT;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_DEFAULT_RACE) == VEHICLE_SUPPORT_SUPPORTED);
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_ABSENT;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_DEFAULT_RACE) == VEHICLE_SUPPORT_UNSUPPORTED);
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_UNKNOWN;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_DEFAULT_RACE) == VEHICLE_SUPPORT_UNKNOWN);

    p.engine = VEHICLE_ENGINE_DIESEL22;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_DPF_REGEN) == VEHICLE_SUPPORT_SUPPORTED);
    p.engine = VEHICLE_ENGINE_PETROL20;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_DPF_REGEN) == VEHICLE_SUPPORT_UNSUPPORTED);
    p.engine = VEHICLE_ENGINE_UNKNOWN;
    assert(vehicle_feature_support_for(&p, VEHICLE_FEATURE_DPF_REGEN) == VEHICLE_SUPPORT_UNKNOWN);
    puts("PASS: feature authorization uses confirmed drivetrain and tri-state equipment");
}

static void test_vehicle_parameter_support(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_DIESEL22;
    assert(vehicle_parameter_support_for(&p, 55) == VEHICLE_SUPPORT_SUPPORTED); /* DPF load */
    assert(vehicle_parameter_support_for(&p, 32) == VEHICLE_SUPPORT_UNSUPPORTED); /* MultiAir */

    p.engine = VEHICLE_ENGINE_PETROL20;
    assert(vehicle_parameter_support_for(&p, 32) == VEHICLE_SUPPORT_SUPPORTED);
    assert(vehicle_parameter_support_for(&p, 55) == VEHICLE_SUPPORT_UNSUPPORTED);

    p.engine = VEHICLE_ENGINE_PETROL29;
    assert(vehicle_parameter_support_for(&p, 19) == VEHICLE_SUPPORT_SUPPORTED); /* ign cyl 5 */
    assert(vehicle_parameter_support_for(&p, 19 + 100) == VEHICLE_SUPPORT_UNSUPPORTED); /* out of range */
    puts("PASS: source applicability is engine-scoped without source-table reconstruction");
}

static void test_vehicle_display_profile(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    DisplayProfile d;

    p.display_selection = VEHICLE_DISPLAY_LEGACY24;
    d = vehicle_display_profile_for(&p);
    assert(d.mode == VEHICLE_DISPLAY_LEGACY24 && d.rows == 1 && d.widths[0] == 24);

    p.display_selection = VEHICLE_DISPLAY_MY23;
    d = vehicle_display_profile_for(&p);
    assert(d.mode == VEHICLE_DISPLAY_MY23 && d.rows == 2 && d.widths[0] == 16 && d.widths[1] == 22);

    p.display_selection = VEHICLE_DISPLAY_AUTO;
    p.model_year = 2024;
    d = vehicle_display_profile_for(&p);
    assert(d.mode == VEHICLE_DISPLAY_MY23);

    p.model_year = 2019;
    d = vehicle_display_profile_for(&p);
    assert(d.mode == VEHICLE_DISPLAY_LEGACY18);
    puts("PASS: explicit display wins; Auto only maps known year >= 2023 to MY23");
}

static void test_vehicle_oil_range(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    OilRange r;

    p.engine = VEHICLE_ENGINE_PETROL20;
    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;
    assert(vehicle_oil_range_for(&p, &r) && r.known && r.minimum_mm == 72 && r.maximum_mm == 82);

    p.drivetrain = VEHICLE_DRIVETRAIN_Q4;
    assert(vehicle_oil_range_for(&p, &r) && r.known && r.minimum_mm == 67 && r.maximum_mm == 77);

    p.engine = VEHICLE_ENGINE_DIESEL22;
    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;
    assert(vehicle_oil_range_for(&p, &r) && r.known && r.minimum_mm == 61 && r.maximum_mm == 65);

    p.drivetrain = VEHICLE_DRIVETRAIN_Q4;
    assert(vehicle_oil_range_for(&p, &r) && r.known && r.minimum_mm == 79 && r.maximum_mm == 84);

    p.model = VEHICLE_MODEL_STELVIO;
    assert(!vehicle_oil_range_for(&p, &r) && !r.known);
    puts("PASS: oil ranges follow the owner table and never transfer to another model");
}

static void test_vehicle_profile_codec(void) {
    VehicleProfile p, q;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    p.engine = VEHICLE_ENGINE_PETROL29;
    p.model_year = 2021;
    p.drivetrain = VEHICLE_DRIVETRAIN_Q4;
    p.display_selection = VEHICLE_DISPLAY_MY23;
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_PRESENT;
    p.equipment[VEHICLE_EQUIPMENT_SCR] = VEHICLE_EQUIPMENT_ABSENT;
    p.confirmed = true;

    uint8_t wire[VEHICLE_PROFILE_WIRE_SIZE];
    assert(vehicle_profile_encode(&p, wire));
    assert(vehicle_profile_decode(wire, &q));
    assert(q.model == p.model && q.engine == p.engine && q.model_year == p.model_year);
    assert(q.drivetrain == p.drivetrain && q.display_selection == p.display_selection);
    assert(q.equipment[VEHICLE_EQUIPMENT_RACE] == VEHICLE_EQUIPMENT_PRESENT);
    assert(q.equipment[VEHICLE_EQUIPMENT_SCR] == VEHICLE_EQUIPMENT_ABSENT);
    assert(q.confirmed);

    uint8_t bad[VEHICLE_PROFILE_WIRE_SIZE];
    memset(bad, 0, sizeof(bad));
    assert(!vehicle_profile_decode(bad, &q)); /* schema version 0 rejected */
    puts("PASS: profile codec round-trips explicitly without struct padding");
}

static void test_vehicle_profile_legacy_import(void) {
    uint16_t slots[42];
    memset(slots, 0, sizeof(slots));
    slots[15] = 0; /* diesel off */
    slots[35] = 1; /* V6 on */
    slots[39] = 2; /* Q4 */
    slots[5] = 1;  /* MY23 */

    VehicleProfile p;
    vehicle_profile_import_legacy(slots, 42, &p);
    assert(p.engine == VEHICLE_ENGINE_PETROL29);
    assert(p.drivetrain == VEHICLE_DRIVETRAIN_Q4);
    assert(p.display_selection == VEHICLE_DISPLAY_MY23);
    assert(!p.confirmed);
    assert(p.model == VEHICLE_MODEL_UNKNOWN);
    assert(p.equipment[VEHICLE_EQUIPMENT_RACE] == VEHICLE_EQUIPMENT_UNKNOWN);

    slots[15] = 1; /* diesel wins over V6 companion */
    vehicle_profile_import_legacy(slots, 42, &p);
    assert(p.engine == VEHICLE_ENGINE_DIESEL22);
    puts("PASS: legacy import decodes engine/display/drivetrain, never infers equipment");
}

static void test_vehicle_profile_apply_legacy_flags(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    p.model_year = 2020;
    p.equipment[VEHICLE_EQUIPMENT_RACE] = VEHICLE_EQUIPMENT_PRESENT;
    p.confirmed = true;

    vehicle_profile_apply_legacy_flags(&p, true, true, 2, true, 18);
    assert(p.engine == VEHICLE_ENGINE_DIESEL22); /* diesel wins over V6 */
    assert(p.drivetrain == VEHICLE_DRIVETRAIN_Q4);
    assert(p.display_selection == VEHICLE_DISPLAY_MY23);
    /* Facts the legacy slots cannot express are preserved. */
    assert(p.model == VEHICLE_MODEL_GIULIA && p.model_year == 2020 && p.confirmed);
    assert(p.equipment[VEHICLE_EQUIPMENT_RACE] == VEHICLE_EQUIPMENT_PRESENT);

    vehicle_profile_apply_legacy_flags(&p, false, true, 1, false, 18);
    assert(p.engine == VEHICLE_ENGINE_PETROL29);
    assert(p.drivetrain == VEHICLE_DRIVETRAIN_RWD);
    assert(p.display_selection == VEHICLE_DISPLAY_LEGACY18);

    vehicle_profile_apply_legacy_flags(&p, false, false, 0, false, 24);
    assert(p.engine == VEHICLE_ENGINE_PETROL20);
    assert(p.drivetrain == VEHICLE_DRIVETRAIN_UNKNOWN);
    assert(p.display_selection == VEHICLE_DISPLAY_LEGACY24);
    puts("PASS: legacy flags apply to a profile without overwriting model/year/equipment");
}

static void test_vehicle_profile_commit(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    assert(vehicle_profile_commit(&p));
    assert(vehicle_profile_current()->model == VEHICLE_MODEL_GIULIA);
    assert(vehicle_profile_generation() > 0);
    puts("PASS: commit publishes the canonical profile and bumps the generation");
}
int main(void) {
    const char *suite = "vehicle-profile";
    const HostTest tests[] = {
        HOST_TEST(test_vehicle_profile_default),
        HOST_TEST(test_vehicle_profile_validate),
        HOST_TEST(test_vehicle_profile_engine_helpers),
        HOST_TEST(test_vehicle_feature_support),
        HOST_TEST(test_vehicle_parameter_support),
        HOST_TEST(test_vehicle_display_profile),
        HOST_TEST(test_vehicle_oil_range),
        HOST_TEST(test_vehicle_profile_codec),
        HOST_TEST(test_vehicle_profile_legacy_import),
        HOST_TEST(test_vehicle_profile_apply_legacy_flags),
        HOST_TEST(test_vehicle_profile_commit),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
