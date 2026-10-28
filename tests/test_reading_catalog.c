#include "test_report.h"
#include "features/reading_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_reading_catalog_unique_and_ordered(void) {
    assert(reading_count() == READING_COUNT);
    ReadingGroup previous = 0;
    for (ReadingId id = 0; id < READING_COUNT; ++id) {
        const ReadingMeta *meta = reading_lookup(id);
        assert(meta != NULL);
        assert(meta->id == id);
        assert(meta->group >= READING_GROUP_OIL && meta->group < READING_GROUP_COUNT);
        /* Groups appear in the mandated order and never regress. */
        assert(meta->group >= previous);
        previous = meta->group;
    }
    puts("PASS: reading IDs are unique, stable and ordered by group");
}

static void test_reading_catalog_sources(void) {
    for (ReadingId id = 0; id < READING_COUNT; ++id) {
        const ReadingMeta *meta = reading_lookup(id);
        assert(meta->label && meta->label[0]);
        unsigned sources = 0;
        if (meta->source_gasoline != READING_NO_SOURCE) {
            assert(meta->source_gasoline < 98);
            ++sources;
        }
        if (meta->source_diesel != READING_NO_SOURCE) {
            assert(meta->source_diesel < 98);
            ++sources;
        }
        assert(sources >= 1);
        assert(meta->engine_mask != 0);
    }
    /* Oil level bar is a presentation of the same source as the mm reading. */
    const ReadingMeta *bar = reading_lookup(READING_OIL_LEVEL_BAR);
    const ReadingMeta *mm = reading_lookup(READING_OIL_LEVEL_MM);
    assert(bar->source_gasoline == mm->source_gasoline && bar->source_diesel == mm->source_diesel);
    assert(bar->value_type == READING_VALUE_OIL_STATUS);
    puts("PASS: every reading has one bounded source and the bar shares the mm source");
}

static void test_reading_catalog_eligibility(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_DIESEL22;
    assert(vehicle_reading_support_for(&p, READING_DPF_LOAD) == VEHICLE_SUPPORT_SUPPORTED);
    assert(vehicle_reading_support_for(&p, READING_INTERCOOLER_IN) == VEHICLE_SUPPORT_UNSUPPORTED);

    p.engine = VEHICLE_ENGINE_PETROL20;
    assert(vehicle_reading_support_for(&p, READING_MULTIAIR_TEMP) == VEHICLE_SUPPORT_SUPPORTED);
    assert(vehicle_reading_support_for(&p, READING_DPF_LOAD) == VEHICLE_SUPPORT_UNSUPPORTED);

    p.engine = VEHICLE_ENGINE_PETROL29;
    assert(vehicle_reading_support_for(&p, READING_IGNITION_CYL_5) == VEHICLE_SUPPORT_SUPPORTED);
    assert(vehicle_reading_support_for(&p, READING_MULTIAIR_TEMP) == VEHICLE_SUPPORT_UNSUPPORTED);

    /* AdBlue depends on SCR; unknown SCR stays Unknown for the reading. */
    p.engine = VEHICLE_ENGINE_DIESEL22;
    p.equipment[VEHICLE_EQUIPMENT_SCR] = VEHICLE_EQUIPMENT_UNKNOWN;
    assert(vehicle_reading_support_for(&p, READING_ADBLUE_LEVEL) == VEHICLE_SUPPORT_UNKNOWN);
    p.equipment[VEHICLE_EQUIPMENT_SCR] = VEHICLE_EQUIPMENT_PRESENT;
    assert(vehicle_reading_support_for(&p, READING_ADBLUE_LEVEL) == VEHICLE_SUPPORT_SUPPORTED);
    puts("PASS: eligibility is engine- and equipment-scoped, Unknown is a real state");
}

static void test_reading_catalog_source_resolution(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    assert(reading_source(reading_lookup(READING_COOLANT_TEMP), &p) == 42);
    p.engine = VEHICLE_ENGINE_DIESEL22;
    assert(reading_source(reading_lookup(READING_COOLANT_TEMP), &p) == 68);
    assert(reading_source(reading_lookup(READING_INTERCOOLER_IN), &p) == READING_NO_SOURCE);
    puts("PASS: source resolution distinguishes per-engine sources without substitution");
}

static void test_reading_catalog_list(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    ReadingId out[128];
    unsigned n = reading_list(&p, READING_GROUP_DIESEL_DPF, out, 128);
    assert(n == 0); /* DPF group empty on a petrol car */

    n = reading_list(&p, READING_GROUP_OIL, out, 128);
    assert(n > 0);
    for (unsigned i = 0; i < n; ++i)
        assert(reading_lookup(out[i])->group == READING_GROUP_OIL);
    puts("PASS: group enumeration filters by the profile and preserves group order");
}

static void test_reading_catalog_enum_labels(void) {
    char buf[16];
    const ReadingMeta *gear = reading_lookup(READING_GEAR);
    assert(reading_enum_label(gear, 3, buf, sizeof(buf)));
    assert(!strcmp(buf, "D"));
    const ReadingMeta *status = reading_lookup(READING_OIL_LEVEL_STATUS);
    assert(reading_enum_label(status, 1, buf, sizeof(buf)));
    assert(!strcmp(buf, "OK"));
    assert(reading_enum_label(status, -1, buf, sizeof(buf)));
    assert(!strcmp(buf, "LOW"));
    puts("PASS: enum/status labels are bounded and deterministic");
}

int main(void) {
    const char *suite = "reading-catalog";
    const HostTest tests[] = {
        HOST_TEST(test_reading_catalog_unique_and_ordered),
        HOST_TEST(test_reading_catalog_sources),
        HOST_TEST(test_reading_catalog_eligibility),
        HOST_TEST(test_reading_catalog_source_resolution),
        HOST_TEST(test_reading_catalog_list),
        HOST_TEST(test_reading_catalog_enum_labels),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
