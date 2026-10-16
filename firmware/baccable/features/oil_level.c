#include "features/oil_level.h"
#include "state/settings.h"
#include "third_party/printf/printf.h"
#include <string.h>
#if defined(BACCABLE_C1)
/* Return the owner-supplied millimetre limits for the selected engine and drivetrain. */
static const uint8_t *oil_level_range(void) {
    static const uint8_t ranges[2][2][2] = {{{72, 82}, {67, 77}}, {{61, 65}, {79, 84}}};
    if ((!settings_state.is_diesel_enabled && settings_state.gasoline_v6) ||
        settings_state.drivetrain < 1 || settings_state.drivetrain > 2)
        return NULL;
    return ranges[settings_state.is_diesel_enabled ? 1 : 0][settings_state.drivetrain - 1];
}

/* Keep oil value/status and the full-width bar independently selectable as catalog pages. */
bool oil_level_format(const ParameterPage *page, float level, unsigned width, char *output) {
    bool bar = page->id == 0x67 || page->id == 0xbd;
    if (!bar && page->id != 0x31 && page->id != 0x9a)
        return false;
    const uint8_t *range = oil_level_range();
    int status = !range ? 0 : level < range[0] ? -1 : level > range[1] ? 1 : 0;
    /* The existing two-byte ECU heights cannot exceed 6553.5 mm. */
    if (!(level >= 0 && level <= 6553.5f)) {
        snprintf_(output, width + 1, "Oil -- mm");
    } else if (!bar) {
        const char *label = !range ? "?" : status < 0 ? "LOW" : status > 0 ? "HIGH" : "OK";
        snprintf_(output, width + 1, "%s%.1fmm %s", level >= 1000 ? "" : "Oil ", (double)level, label);
    } else if (!range) {
        snprintf_(output, width + 1, "%s", !settings_state.is_diesel_enabled && settings_state.gasoline_v6
                                              ? "No oil range" : "Set drivetrain");
    } else {
        int left = width / 5, right = width - left - 1;
        memset(output, '-', width);
        memset(output + left + 1, '=', right - left - 1);
        output[left] = '[';
        output[right] = ']';
        output[width] = 0;
        int tenths = (int)(level * 10.0f);
        int position = left + (tenths - range[0] * 10) * (right - left) / ((range[1] - range[0]) * 10);
        if (position < 0) output[0] = '<';
        else if (position >= (int)width) output[width - 1] = '>';
        else {
            int marker = position;
            /* Keep a value outside the limits visibly outside the bracket after rounding. */
            if (status < 0 && marker >= left) marker = left - 1;
            if (status > 0 && marker <= right) marker = right + 1;
            output[marker] = settings_state.ipc_my23_is_installed ? '\x81' : '*';
        }
    }
    return true;
}
#endif
