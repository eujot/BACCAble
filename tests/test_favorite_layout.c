#include "test_report.h"
#include "features/favorite_layout.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void _putchar(char c) { (void)c; }

static void test_favorite_item_and_clear(void) {
    assert(favorite_item_empty(FAVORITE_EMPTY_ITEM));
    FavoriteSet set;
    favorite_set_clear(&set);
    for (unsigned r = 0; r < FAVORITE_ROWS; ++r)
        assert(favorite_row_item_count(set.rows[r]) == 0);
    assert(favorite_set_validate(&set, NULL));
    puts("PASS: empty items are explicit and cleared sets are valid");
}

static void test_favorite_duplicate_and_bar(void) {
    FavoriteSet set;
    favorite_set_clear(&set);
    set.rows[0][0] = (FavoriteItem){.id = READING_POWER, .format = READING_FORMAT_AUTO};
    set.rows[0][1] = (FavoriteItem){.id = READING_POWER, .format = READING_FORMAT_AUTO};
    FavoriteError e = FAVORITE_OK;
    assert(!favorite_set_validate(&set, &e) && e == FAVORITE_ERR_DUPLICATE_IN_ROW);

    favorite_set_clear(&set);
    set.rows[0][0] = (FavoriteItem){.id = READING_OIL_LEVEL_BAR, .format = READING_FORMAT_AUTO};
    set.rows[0][1] = (FavoriteItem){.id = READING_OIL_TEMPERATURE, .format = READING_FORMAT_AUTO};
    assert(!favorite_set_validate(&set, &e) && e == FAVORITE_ERR_BAR_NOT_EXCLUSIVE);

    favorite_set_clear(&set);
    set.rows[0][0] = (FavoriteItem){.id = READING_OIL_LEVEL_BAR, .format = READING_FORMAT_AUTO};
    assert(favorite_set_validate(&set, &e));
    puts("PASS: duplicate-in-row and bar exclusivity are rejected");
}

static void test_favorite_defaults_valid(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    FavoriteLayout layout;
    favorite_layout_default(&layout, &p);
    for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s) {
        FavoriteError e = FAVORITE_OK;
        assert(favorite_set_validate(&layout.sets[s], &e));
        uint8_t sources[8];
        unsigned n = favorite_set_unique_sources(&layout.sets[s], &p, sources);
        assert(n <= 8);
    }
    puts("PASS: every default set is valid and needs at most eight unique sources");
}

static void test_favorite_defaults_engine_aware(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    FavoriteLayout layout;

    p.engine = VEHICLE_ENGINE_DIESEL22;
    favorite_layout_default(&layout, &p);
    assert(layout.sets[5].rows[0][0].id == READING_DPF_LOAD);
    assert(layout.sets[5].rows[1][0].id == READING_DPF_REGEN_PROGRESS);

    p.engine = VEHICLE_ENGINE_PETROL20;
    favorite_layout_default(&layout, &p);
    assert(layout.sets[5].rows[0][0].id == READING_TIME_0_100);
    assert(layout.sets[5].rows[1][0].id == READING_BEST_0_100);
    assert(layout.sets[3].rows[1][0].id == READING_INTERCOOLER_IN);

    p.engine = VEHICLE_ENGINE_DIESEL22;
    favorite_layout_default(&layout, &p);
    assert(layout.sets[3].rows[1][0].id == READING_INTAKE_AIR_TEMP);
    puts("PASS: defaults are selected from the eligibility table, not live RX timing");
}

static void test_favorite_unique_sources_dedup(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.engine = VEHICLE_ENGINE_PETROL20;
    FavoriteSet set;
    favorite_set_clear(&set);
    /* Same reading in two rows: polled once. */
    set.rows[0][0] = (FavoriteItem){.id = READING_POWER, .format = READING_FORMAT_AUTO};
    set.rows[1][0] = (FavoriteItem){.id = READING_POWER, .format = READING_FORMAT_AUTO};
    set.rows[0][1] = (FavoriteItem){.id = READING_TORQUE, .format = READING_FORMAT_AUTO};
    uint8_t sources[8];
    unsigned n = favorite_set_unique_sources(&set, &p, sources);
    assert(n == 2);
    puts("PASS: repeated sources are deduplicated for one request/cache slot");
}

static void test_favorite_row_render(void) {
    char out[64];
    FavoriteSet set;
    favorite_set_clear(&set);
    set.rows[0][0] = (FavoriteItem){.id = READING_POWER, .format = READING_FORMAT_AUTO};
    ReadingSample samples[READING_COUNT];
    memset(samples, 0, sizeof(samples));
    samples[READING_POWER].state = READING_STATE_VALID;
    samples[READING_POWER].value = 200.0f;
    assert(favorite_row_render(set.rows[0], samples, 24, out, sizeof(out)) == FAVORITE_ROW_FIT);
    assert(strstr(out, "200PS") != NULL);

    favorite_set_clear(&set);
    assert(favorite_row_render(set.rows[0], samples, 24, out, sizeof(out)) == FAVORITE_ROW_EMPTY);
    puts("PASS: row renderer joins label/value and reports empty/fit");
}

int main(void) {
    const char *suite = "favorite-layout";
    const HostTest tests[] = {
        HOST_TEST(test_favorite_item_and_clear),
        HOST_TEST(test_favorite_duplicate_and_bar),
        HOST_TEST(test_favorite_defaults_valid),
        HOST_TEST(test_favorite_defaults_engine_aware),
        HOST_TEST(test_favorite_unique_sources_dedup),
        HOST_TEST(test_favorite_row_render),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
