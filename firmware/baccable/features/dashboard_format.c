#include "app/powertrain.h"
#include "features/ui_glyphs.h"
#include <stdbool.h>
#if defined(BACCABLE_C1)

/* Append display text while respecting the available screen width. */
static void append_text(char *output, size_t *length, const char *text) {
    while (*text && *length < DASHBOARD_MESSAGE_MAX_LENGTH)
        output[(*length)++] = *text++;
}

/* Choose a known status label or the unavailable fallback. */
static unsigned enum_index(float value, unsigned fallback) {
    return isfinite(value) && value >= 0 && value < fallback ? (unsigned)value : fallback;
}

/* Describe a reported gear, regeneration, drive mode or pedal-map status. */
static const char *enum_text(uint32_t id, float value, char symbol[2]) {
    switch (id) {
    case 0x17:
        symbol[0] = value >= 0 && value < 11 ? gear_symbols[(unsigned)value] : '-';
        return symbol;
    case 0x19:
        return regeneration_labels[enum_index(value, 7)];
    case 0x1e:
        return seatbelt_labels[enum_index(value, 2)];
    case 0x20:
        if (!isfinite(value) || value < 0 || value > 0x30 || value != (unsigned)value)
            return "?";
        switch ((unsigned)value) {
        case 0x00:
            return "N";
        case 0x08:
            return "D";
        case 0x10:
            return "A";
        case 0x30:
            return "R";
        default:
            return "?";
        }
    case 0x22:
        symbol[0] = isfinite(value) && value >= 32 && value <= 126 && value == (unsigned)value ? (char)value : '?';
        return symbol;
    default:
        return "";
    }
}

/* Format a reading or performance-run status without showing truncated numbers. */
static const char *number_text(uint32_t id, float value, unsigned decimals, unsigned width, char buffer[20]) {
    bool short_run = id == 0x1a || id == 0x1c;
    bool long_run = id == 0x1b || id == 0x1d;
    if ((short_run && value > 20) || (long_run && value > 40))
        return statistics_labels[0];
    if ((id == 0x1a && statistics_state.statistics_0_100_started) ||
        (id == 0x1b && statistics_state.statistics_100_200_started))
        return statistics_labels[1];
    if (width > 19)
        width = 19;
    char number[32];
    float half_step = 0.5f;
    for (unsigned i = 0; i < decimals; ++i)
        half_step *= 0.1f;
    if (value < 0 && value > -half_step)
        value = 0;
    int length =
        isfinite(value) ? snprintf_(number, sizeof(number), "%.*f", (int)decimals, (double)value) : -1;
    memset(buffer, ' ', width);
    buffer[width] = 0;
    if (length < 0 || (unsigned)length > width) {
        if (width)
            buffer[width - 1] = '-';
        if (width > 1)
            buffer[width - 2] = '-';
    } else
        memcpy(buffer + width - length, number, length);
    return buffer;
}

/* Recognize a numeric field in a display template. */
static bool number_placeholder(const char *text) {
    /* Each successful check also proves that byte is not the string terminator. */
    return text[0] == '$' && text[1] >= '0' && text[1] <= '9' && text[2] == '.' && text[3] >= '0' &&
           text[3] <= '9' && text[4] == 'f';
}

/* The caller provides DASHBOARD_MESSAGE_MAX_LENGTH + 1 output bytes. */

/* Build a readable screen from its labels, values and units. */
void dashboard_format_values(const char *template, const float *values, const uint8_t *paramId,
                             char *result) {
    size_t length = 0;
    unsigned element = 0, degrees = 0;
    size_t degree_positions[4];
    const char *celsius = NULL;
    for (const char *text = template; *text && length < DASHBOARD_MESSAGE_MAX_LENGTH;) {
        bool numeric = number_placeholder(text);
        bool enumeration = !strncmp(text, "$enum", 5);
        if (element >= 4 || paramId[element] >= 100 || (!numeric && !enumeration)) {
            if (text == celsius && length && text[-1] == ' ')
                result[length - 1] = UI_GLYPH_DEGREE; /* Reuse the optional unit gap. */
            else if (text == celsius && degrees < 4)
                degree_positions[degrees++] = length;
            result[length++] = *text++;
            continue;
        }
        uint32_t id = parameter_definitions[paramId[element]].request_id;
        char buffer[20];
        char symbol[2] = {0};
        const char *formatted;
        if (numeric) {
            unsigned decimals = text[3] - '0';
            unsigned width = text[1] - '0' + decimals + (decimals > 0);
            formatted = number_text(id, values[element], decimals, width, buffer);
        } else {
            formatted = isfinite(values[element]) ? enum_text(id, values[element], symbol) : "--";
        }
        append_text(result, &length, formatted);
        ++element;
        text += 5;
        const char *unit = text;
        while (*unit == ' ') ++unit;
        celsius = numeric && unit[0] == 'C' &&
                  (unit[1] == 0 || unit[1] == ' ' || unit[1] == '/' || unit[1] == '$') ? unit : NULL;
        /* A run status is text, so it must not acquire the numeric seconds suffix. */
        if (formatted == statistics_labels[0] || formatted == statistics_labels[1]) {
            if (*text == ' ')
                ++text;
            if (*text == 's')
                ++text;
        }
    }
    result[length] = '\0';
    /* Degrees are optional: dense pages retain all original numbers and units. */
    if (length + degrees <= DASHBOARD_MESSAGE_MAX_LENGTH)
        while (degrees) {
            size_t position = degree_positions[--degrees];
            memmove(result + position + 1, result + position, length - position + 1);
            result[position] = UI_GLYPH_DEGREE;
            ++length;
        }
}

/* Keep legacy catalog text intact; only MY23's 16-column view uses these forms. */
static const char *my23_template(const ParameterPage *page) {
    switch (page->id) {
    case 0x01: case 0x81: return "Pwr$3.0fPS Tq$3.0fNm";
    case 0x02: case 0x82: return "Oil$1.1fbar W$3.0f\xb0" "C";
    case 0x04: case 0x84: return "Oil$3.0f\xb0 Cool$3.0f\xb0";
    case 0x05: return "Oil $1.1fL Q$3.0f%";
    case 0x06: case 0x86: return "SOC$3.0f% $4.1fA";
    case 0x07: case 0x87: return "Bat$2.1fV $4.1fA";
    case 0x0c: return "Intake $2.2fbar";
    case 0x0f: case 0xac: return "Dist $6.0fkm";
    case 0x10: return "Oil vol $2.2fL";
    case 0x11: case 0x99: return "Oil P $2.2fbar";
    case 0x12: return "Oil ECU $3.0f\xb0" "C";
    case 0x16: return "BCM SOC $3.0f%";
    case 0x19: case 0xad: return "A/C $2.2fbar";
    case 0x1b: return "Run $6.0fmin";
    case 0x20: case 0xa0: return "Coolant $3.0f\xb0" "C";
    case 0x29: case 0xb1: return "Belt $enum";
    case 0x2c: case 0xb4: return "Best0-100 $2.2fs";
    case 0x2d: case 0xb5: return "B 100-200 $2.2fs";
    case 0x31: case 0x9a: return "Oil lev $3.1fmm";
    case 0x39: return "O$3.0fW$3.0fI$3.0fX$3.0f";
    case 0x3a: case 0xbc: return "Oil$3.0f W$3.0f G$3.0f";
    case 0x3c: case 0xb9: return "IBS B0$3.0f B1$3.0f";
    case 0x85: return "Oil $2.1fmm Q$3.0f%";
    case 0x89: return "Regen$3.0f% T$4.0f\xb0";
    case 0x8f: return "DPF $enum";
    case 0x90: return "DPF ago $5.0fkm";
    case 0x91: return "Regen #$5.0f";
    case 0x92: return "Avg DPF $5.0fkm";
    case 0x9c: return "AdBlue $3.0f%";
    case 0xa3: return "Turbo tgt$3.2f";
    case 0xa8: return "Boost tgt$3.2f";
    case 0xaa: return "Fuel $4.0fbar";
    case 0xae: return "Fuel $2.2fL/h";
    default: return page->name;
    }
}

/* Padded numeric fields aid legacy alignment but waste MY23 columns. */
void dashboard_format_my23_page(const ParameterPage *page, const float *values, char *result) {
    dashboard_format_values(my23_template(page), values, page->parameter_ids, result);
    size_t read = 0, write = 0;
    while (result[read]) {
        char glyph = result[read++];
        if (glyph == ' ' && (!write || result[write - 1] == ' '))
            continue;
        result[write++] = glyph;
    }
    while (write && result[write - 1] == ' ')
        --write;
    result[write] = 0;
}

#endif
