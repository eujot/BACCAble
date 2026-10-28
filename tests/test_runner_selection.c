#include "test_report.h"
#include <stdio.h>

/* This suite proves the BACCABLE_TEST_CASES allowlist never runs unselected cases. */
static void test_runner_selection_runs(void) { puts("SELECTED CASE RAN"); }

/* Registered sentinel: must NOT run under the scoped runner. */
static void test_runner_selection_sentinel(void) { puts("SENTINEL RAN"); }

int main(void) {
    const char *suite = "runner-selection";
    const HostTest tests[] = {
        HOST_TEST(test_runner_selection_runs),
        HOST_TEST(test_runner_selection_sentinel),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
}
