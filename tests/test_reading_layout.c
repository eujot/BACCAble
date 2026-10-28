#include "test_report.h"
#include "features/reading_format.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void _putchar(char c) { (void)c; }

static void test_reading_glyph_count(void) {
    assert(reading_glyph_count("abc") == 3);
    assert(reading_glyph_count("") == 0);
    assert(reading_glyph_count(NULL) == 0);
    /* Two-byte UTF-8 sequence counts as one glyph. */
    assert(reading_glyph_count("\xc2\xb0" "C") == 2);
    puts("PASS: glyph count is UTF-8 aware, not byte length");
}

static void test_reading_missing_never_zero(void) {
    char out[32];
    ReadingSample s = {.state = READING_STATE_MISSING, .value = 0.0f};
    assert(reading_format_value(READING_OIL_TEMPERATURE, &s, 16, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "--"));
    s.state = READING_STATE_STALE;
    assert(reading_format_value(READING_OIL_TEMPERATURE, &s, 16, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "--"));
    puts("PASS: missing/stale renders as -- and never as zero");
}

static void test_reading_numeric_format(void) {
    char out[32];
    ReadingSample s = {.state = READING_STATE_VALID, .value = 99.0f};
    assert(reading_format_value(READING_COOLANT_TEMP, &s, 16, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "99C"));

    s.value = 100.0f;
    assert(reading_format_value(READING_COOLANT_TEMP, &s, 16, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "100C")); /* 99->100 keeps the same precision, no style switch */

    s.value = -12.5f;
    assert(reading_format_value(READING_BOOST, &s, 16, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "-12.50bar"));
    puts("PASS: numeric formatting preserves precision, sign and units across 99->100");
}

static void test_reading_overflow_indicator(void) {
    char out[32];
    ReadingSample s = {.state = READING_STATE_VALID, .value = 123456.0f};
    ReadingFit fit = reading_format_value(READING_COOLANT_TEMP, &s, 3, out, sizeof(out));
    assert(fit == READING_FIT_OVERFLOW);
    assert(!strcmp(out, "###")); /* bounded indicator, no clipped digits */
    puts("PASS: overflow emits a distinct bounded indicator, never clipped digits");
}

static void test_reading_auto_format(void) {
    assert(reading_auto_format(READING_COOLANT_TEMP, 18) == READING_FORMAT_FULL);
    /* Narrow widths degrade to short labels deterministically. */
    assert(reading_auto_format(READING_COOLANT_TEMP, 6) == READING_FORMAT_SHORT);
    puts("PASS: Auto resolves to a stable concrete label for the target width");
}

static void test_reading_line_fit(void) {
    char out[32];
    ReadingSample s = {.state = READING_STATE_VALID, .value = 90.0f};
    ReadingFit fit = reading_format_line(READING_OIL_TEMPERATURE, &s, READING_FORMAT_AUTO, 18, out, sizeof(out));
    assert(fit == READING_FIT_OK);
    assert(strstr(out, "90C") != NULL);
    assert(strstr(out, "Oil") != NULL);

    fit = reading_format_line(READING_OIL_TEMPERATURE, &s, READING_FORMAT_AUTO, 4, out, sizeof(out));
    assert(fit == READING_FIT_OVERFLOW);
    puts("PASS: legacy line composes label and value and reports fit/overflow");
}

static void test_reading_oil_status(void) {
    /* Commit a Giulia 2.0 RWD profile (72..82) so the status range resolves. */
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    p.engine = VEHICLE_ENGINE_PETROL20;
    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;
    assert(vehicle_profile_commit(&p));

    char out[32];
    ReadingSample low = {.state = READING_STATE_VALID, .value = 60.0f};
    ReadingSample ok = {.state = READING_STATE_VALID, .value = 75.0f};
    ReadingSample high = {.state = READING_STATE_VALID, .value = 90.0f};
    assert(reading_format_value(READING_OIL_LEVEL_STATUS, &low, 8, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "LOW"));
    assert(reading_format_value(READING_OIL_LEVEL_STATUS, &ok, 8, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "OK"));
    assert(reading_format_value(READING_OIL_LEVEL_STATUS, &high, 8, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "HIGH"));
    puts("PASS: oil status derives LOW/OK/HIGH from the resolved owner range");
}

static void test_reading_oil_bar(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    p.engine = VEHICLE_ENGINE_PETROL20;
    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;
    assert(vehicle_profile_commit(&p));

    char out[32];
    ReadingSample ok = {.state = READING_STATE_VALID, .value = 75.0f};
    assert(reading_format_value(READING_OIL_LEVEL_BAR, &ok, 18, out, sizeof(out)) == READING_FIT_OK);
    assert(strlen(out) == 18);
    assert(out[0] == '-' && strchr(out, '[') && strchr(out, ']') && strchr(out, '=') && strchr(out, '*'));

    ReadingSample low = {.state = READING_STATE_VALID, .value = 50.0f};
    assert(reading_format_value(READING_OIL_LEVEL_BAR, &low, 18, out, sizeof(out)) == READING_FIT_OK);
    assert(out[0] == '<');

    ReadingSample high = {.state = READING_STATE_VALID, .value = 120.0f};
    assert(reading_format_value(READING_OIL_LEVEL_BAR, &high, 18, out, sizeof(out)) == READING_FIT_OK);
    assert(out[17] == '>');

    ReadingSample missing = {.state = READING_STATE_MISSING, .value = 0.0f};
    assert(reading_format_value(READING_OIL_LEVEL_BAR, &missing, 18, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "Oil -- mm"));

    /* Unknown drivetrain keeps mm valid but shows the configuration hint. */
    VehicleProfile unknown;
    vehicle_profile_default(&unknown);
    unknown.model = VEHICLE_MODEL_GIULIA;
    unknown.engine = VEHICLE_ENGINE_PETROL20;
    unknown.drivetrain = VEHICLE_DRIVETRAIN_UNKNOWN;
    assert(vehicle_profile_commit(&unknown));
    assert(reading_format_value(READING_OIL_LEVEL_BAR, &ok, 18, out, sizeof(out)) == READING_FIT_OK);
    assert(!strcmp(out, "Set drivetrain"));

    puts("PASS: oil bar renders a full-width bracket with marker and off-scale arrows");
}

int main(void) {
    const char *suite = "reading-layout";
    const HostTest tests[] = {
        HOST_TEST(test_reading_glyph_count),
        HOST_TEST(test_reading_missing_never_zero),
        HOST_TEST(test_reading_numeric_format),
        HOST_TEST(test_reading_overflow_indicator),
        HOST_TEST(test_reading_auto_format),
        HOST_TEST(test_reading_line_fit),
        HOST_TEST(test_reading_oil_status),
        HOST_TEST(test_reading_oil_bar),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
