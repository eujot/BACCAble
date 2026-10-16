#ifndef BACCABLE_OIL_LEVEL_H
#define BACCABLE_OIL_LEVEL_H
#include "diagnostics/parameter_catalog.h"
#include <stdbool.h>
/* Format an oil page in a width + 1 buffer (supported line widths: 16 through 24). */
bool oil_level_format(const ParameterPage *page, float level, unsigned width, char *output);
#endif
