#include "test_report.h"
#include "diagnostics/parameter_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

_Static_assert(sizeof(ParameterDefinition) == 28, "Parameter catalog must retain natural compact layout");

/* The 98 UDS sources stay intact even though the compound pages are retired. */
static void test_parameter_definition_layout(void) {
    for (unsigned id = 0; id < PARAMETER_DEFINITION_COUNT; ++id) {
        const ParameterDefinition *parameter = &parameter_definitions[id];
        assert(parameter->request_id <= 0x1fffffff);
        assert(isfinite(parameter->scale));
        assert(parameter->value_length <= 4);
        if (parameter->request_id > 0xff) {
            assert(parameter->request_length == 4);
            assert(parameter->response_id <= 0x1fffffff);
        }
    }
    assert(fabsf(parameter_definitions[79].scale - 0.001f) < 1e-8f);
    assert(fabsf(parameter_definitions[81].scale - 0.001f) < 1e-8f);
    assert(parameter_definitions[81].raw_offset == -32768);
    puts("PASS: parameter definitions retain request schema, signed scaling and 28-byte layout");
}

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(test_parameter_definition_layout)
    };
    host_tests_run("catalog", tests, sizeof(tests) / sizeof(tests[0]));
}
