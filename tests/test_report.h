#ifndef HOST_TEST_REPORT_H
#define HOST_TEST_REPORT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void (*run)(void);
    const char *name;
} HostTest;
#define HOST_TEST(fn) {fn, #fn}

/* Match a case name as one exact token in a comma-separated allowlist. */
static int host_name_selected(const char *name, const char *allow) {
    const char *p = allow;
    while (p && *p) {
        const char *comma = strchr(p, ',');
        size_t len = comma ? (size_t)(comma - p) : strlen(p);
        if (strlen(name) == len && !strncmp(name, p, len))
            return 1;
        if (!comma)
            break;
        p = comma + 1;
    }
    return 0;
}

/* Length-bounded lookup of a token against the registered suite cases. */
static int host_token_in_tests(const char *name, size_t len, const HostTest *tests, size_t count) {
    for (size_t i = 0; i < count; ++i)
        if (strlen(tests[i].name) == len && !strncmp(tests[i].name, name, len))
            return 1;
    return 0;
}

/* Announce planned cases before running them so an assertion leaves a useful report.
 * When BACCABLE_TEST_CASES is set, run only the exact named cases and fail on any
 * requested name that is not registered in this suite. */
static void host_tests_run(const char *suite, const HostTest *tests, size_t count) {
    const char *allow = getenv("BACCABLE_TEST_CASES");
    int filtered = allow && *allow;

    for (size_t i = 0; i < count; ++i)
        if (!filtered || host_name_selected(tests[i].name, allow))
            printf("CASE PLAN %s %s\n", suite, tests[i].name);
    fflush(stdout);

    for (size_t i = 0; i < count; ++i) {
        if (filtered && !host_name_selected(tests[i].name, allow))
            continue;
        printf("CASE RUN %s %s\n", suite, tests[i].name);
        fflush(stdout);
        tests[i].run();
        printf("CASE PASS %s %s\n", suite, tests[i].name);
        fflush(stdout);
    }

    if (filtered) {
        const char *p = allow;
        while (p && *p) {
            const char *comma = strchr(p, ',');
            size_t len = comma ? (size_t)(comma - p) : strlen(p);
            if (len && !host_token_in_tests(p, len, tests, count)) {
                printf("CASE MISSING %s %.*s\n", suite, (int)len, p);
                fflush(stdout);
                exit(2);
            }
            if (!comma)
                break;
            p = comma + 1;
        }
    }
}
#endif
