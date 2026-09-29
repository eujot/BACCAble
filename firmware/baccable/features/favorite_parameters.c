#include "features/favorite_parameters.h"
#include "features/menu_model.h"
#include "app/powertrain.h"
#include "features/my23_ui.h"
#include "diagnostics/parameter_cache.h"
#include "third_party/printf/printf.h"
#include <string.h>
#include <math.h>
#if defined(BACCABLE_C1)
/* Reuse stable measurement IDs and the catalog's engine/source semantics. */
static __attribute__((noinline)) const ParameterPage *source_page(uint8_t engine, bool v6, uint8_t id, unsigned *element) {
    const ParameterPage *fallback = NULL;
    unsigned fallback_element = 0;
    for (unsigned i = 0; i < menu_page_count(engine); ++i) {
        if (!menu_page_supported(engine, i, v6)) continue;
        const ParameterPage *page = &parameter_pages[engine][i];
        for (unsigned j = 0; j < parameter_page_elements(page); ++j) {
            if (page->parameter_ids[j] != id) continue;
            if (!fallback) { fallback = page; fallback_element = j; }
            if (page->parameter_ids[0] == page->parameter_ids[1] && parameter_page_elements(page) == 2) {
                *element = 0; return page;
            }
        }
    }
    *element = fallback_element;
    return fallback;
}
const char *favorite_parameter_name(uint8_t engine, bool v6, uint8_t id) {
    if (id >= 9 && id <= 12) {
        static const char *const names[] = {"0-100 last", "100-200 last", "0-100 best", "100-200 best"};
        return names[id - 9];
    }
    if (id == 97) return "Engine RPM";
    if (id == 0) return "Oil press native";
    if (id == 3) return "Battery SOC IBS";
    if (id == 5) return "Oil temp native";
    if (id == 95) return "Battery IBS raw0";
    if (id == 96) return "Battery IBS raw1";
    unsigned element;
    const ParameterPage *page = source_page(engine, v6, id, &element);
    return page ? page->label : "Empty";
}
bool favorite_parameter_supported(uint8_t engine, bool v6, uint8_t id) {
    unsigned element;
    return id == 97 || source_page(engine, v6, id, &element) != NULL;
}
/* Keep the labels recognizable even when the first IPC line has only 14 glyphs. */
const char *favorite_parameter_label(uint8_t engine, bool v6, uint8_t id, bool compact) {
    static const struct { uint8_t id; const char *full, *compact; } names[] = {
        {0, "Oil pressure", "Oil bar"}, {29, "Oil press ECU", "Oil ECU"},
        {5, "Oil temp", "Oil temp"}, {30, "Oil temp ECU", "Oil ECU"},
        {42, "Coolant temp", "Coolant"}, {68, "Coolant ECU", "Coolant"},
        {22, "Intercooler out", "IC outlet"}, {23, "Intercooler in", "IC inlet"},
        {32, "MultiAir temp", "MultiAir"}, {33, "Gearbox temp", "Gearbox"},
        {25, "Boost pressure", "Boost"}, {6, "Gear", "Gear"},
        {7, "Speed", "Speed"}, {97, "Engine RPM", "RPM"},
        {3, "IBS SOC", "IBS SOC"}, {21, "Battery SOC", "Batt SOC"},
        {34, "BCM SOC", "BCM SOC"}, {35, "Battery V", "Batt V"},
        {4, "Battery A", "Batt A"}, {8, "DPF regen", "DPF"},
        {9, "0-100 last", "0-100"}, {10, "100-200 last", "100-200"},
        {11, "0-100 best", "Best 0-100"}, {12, "100-200 best", "Best 100-200"}
    };
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if (names[i].id == id) return compact ? names[i].compact : names[i].full;
    const char *name = favorite_parameter_name(engine, v6, id);
    return !strcmp(name, "Empty") ? favorite_parameter_name(engine, !v6, id) : name;
}
static void format_segment(uint8_t engine, uint8_t id, float value, bool tiny,
                           const char *readable_label, char *out, size_t size) {
    const char *label = NULL, *unit = "";
    unsigned decimals = 0;
    static const struct {
        uint8_t id, decimals;
        const char *short_name, *tiny_name, *unit;
    } labels[] = {
        {0, 1, "OILP", "OP", "bar"}, {29, 1, "OILP", "OP", "bar"},
        {5, 0, "OIL", "O", "\xb0"}, {30, 0, "OILE", "OE", "\xb0"},
        {42, 0, "WTR", "W", "\xb0"}, {68, 0, "WTR", "W", "\xb0"},
        {22, 0, "IC", "IC", "\xb0"}, {23, 0, "ICI", "ICI", "\xb0"},
        {32, 0, "MA", "MA", "\xb0"}, {33, 0, "GBOX", "GT", "\xb0"},
        {25, 1, "BST", "B", "bar"}, {6, 0, "GEAR", "G", ""},
        {8, 0, "DPF MODE", "DM", ""}, {13, 0, "BELT", "SB", ""},
        {15, 0, "DNA", "DNA", ""}, {17, 0, "PEDAL", "PM", ""},
        {9, 3, "0-100", "T1", "s"}, {10, 3, "100-200", "T2", "s"},
        {11, 3, "BEST 0-100", "B1", "s"}, {12, 3, "BEST 100-200", "B2", "s"},
        {7, 0, "SPD", "S", "km/h"}, {97, 0, "RPM", "R", ""},
        {3, 0, "IBS", "IBS", "%"}, {21, 0, "SOC", "SOC", "%"},
        {34, 0, "BCM", "BCM", "%"}, {35, 1, "BATT", "V", "V"},
        {4, 1, "CUR", "A", "A"}
    };
    for (unsigned i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i) {
        if (labels[i].id != id) continue;
        label = tiny ? labels[i].tiny_name : labels[i].short_name;
        unit = tiny && id == 7 ? "" : labels[i].unit;
        decimals = labels[i].decimals;
        break;
    }
    char short_label[5] = {0}, extracted_unit[8] = {0};
    if (!label) {
        unsigned element;
        const ParameterPage *page = source_page(engine, false, id, &element);
        if (!page) page = source_page(engine, true, id, &element);
        const char *name = page ? page->label : "Empty";
        unsigned n = 0;
        bool word = true;
        while (*name && n < 4) {
            char c = *name++;
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            bool alnum = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
            if (alnum && word) short_label[n++] = c;
            word = !alnum;
        }
        if (tiny && n > 2) { short_label[1] = short_label[n - 1]; short_label[2] = 0; }
        label = short_label;
        if (page) {
            const char *format = page->name;
            for (unsigned j = 0; j <= element; ++j) {
                format = strchr(format, '$');
                if (!format) break;
                ++format;
                if (j != element) continue;
                const char *dot = strchr(format, '.');
                const char *end = strchr(format, 'f');
                if (end && dot && dot < end) {
                    decimals = dot[1] >= '0' && dot[1] <= '3' ? dot[1] - '0' : 0;
                    const char *u = end + 1;
                    while (*u == ' ') ++u;
                    unsigned k = 0;
                    while (*u && *u != ' ' && *u != '$' && k < sizeof(extracted_unit) - 1) extracted_unit[k++] = *u++;
                    unit = !strcmp(extracted_unit, "C") ? "\xb0" : extracted_unit;
                }
            }
        }
    }
    char number[16];
    if (!isfinite(value) || fabsf(value) > 999999.0f) { snprintf_(number, sizeof(number), "--"); unit = ""; }
    else if (id == 6 && value >= 0 && value < 11 && value == (unsigned)value)
        snprintf_(number, sizeof(number), "%c", gear_symbols[(unsigned)value]);
    else if (id == 6) { snprintf_(number, sizeof(number), "--"); unit = ""; }
    else if (id == 8) snprintf_(number, sizeof(number), "%s", value >= 0 && value < 7 ? regeneration_labels[(unsigned)value] : "?");
    else if (id == 13) snprintf_(number, sizeof(number), "%s", value == 0 ? "ON" : value == 1 ? "OFF" : "?");
    else if (id == 15) snprintf_(number, sizeof(number), "%s", value == 0 ? "N" : value == 8 ? "D" : value == 16 ? "A" : value == 48 ? "R" : "?");
    else if (id == 17) snprintf_(number, sizeof(number), "%c", value >= 32 && value <= 126 ? (char)value : '?');
    else if ((id == 9 || id == 11) && value > 20) { snprintf_(number, sizeof(number), "MISS"); unit = ""; }
    else if ((id == 10 || id == 12) && value > 40) { snprintf_(number, sizeof(number), "MISS"); unit = ""; }
    else if ((id == 9 && statistics_state.statistics_0_100_started) || (id == 10 && statistics_state.statistics_100_200_started)) { snprintf_(number, sizeof(number), "RUN"); unit = ""; }
    else snprintf_(number, sizeof(number), "%.*f", (int)decimals, (double)value);
    /* Factory enum tables are space-padded for legacy templates. */
    size_t end = strlen(number);
    while (end && number[end - 1] == ' ') number[--end] = 0;
    if (readable_label) label = readable_label;
    snprintf_(out, size, "%s%s%s%s", label, tiny && !readable_label ? "" : " ", number, unit);
}
void favorite_parameter_segment(uint8_t engine, uint8_t id, float value, bool tiny, char *out, size_t size) {
    format_segment(engine, id, value, tiny, NULL, out, size);
}
/* One complete value per line. Hidden historical slots stay stored for migration. */
static void readable_line(uint8_t engine, uint8_t id, uint32_t now, unsigned width, char *out) {
    if (id == FAVORITE_EMPTY) { out[0] = 0; return; }
    char value[48];
    float reading = parameter_cache_get(id, now);
    const char *label = favorite_parameter_label(engine, false, id, false);
    format_segment(engine, id, reading, false, label, value, sizeof(value));
    if (strlen(value) > width) {
        label = favorite_parameter_label(engine, false, id, true);
        format_segment(engine, id, reading, false, label, value, sizeof(value));
    }
    if (strlen(value) > width) {
        char first_word[20];
        unsigned length = 0;
        while (label[length] && label[length] != ' ' && length < sizeof(first_word) - 1) {
            first_word[length] = label[length]; ++length;
        }
        first_word[length] = 0;
        format_segment(engine, id, reading, false, first_word, value, sizeof(value));
    }
    snprintf_(out, width + 1, "%s", strlen(value) <= width ? value : "Value too wide");
}
void favorite_parameters_render(uint8_t engine, const FavoriteParameters *favorite, uint32_t now,
                                char first[15], char second[23]) {
    readable_line(engine, favorite->params[0], now, MY23_L1_VISIBLE, first);
    readable_line(engine, favorite->params[1], now, MY23_L2_VISIBLE, second);
}

#endif
