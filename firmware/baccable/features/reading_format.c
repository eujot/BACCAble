#include "features/reading_format.h"
#include <stdio.h>
#include <string.h>

unsigned reading_glyph_count(const char *text) {
    unsigned count = 0;
    if (!text)
        return 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p)
        if ((*p & 0xc0) != 0x80)
            ++count;
    return count;
}

const char *reading_label_for(ReadingId id, ReadingFormat format) {
    const ReadingMeta *meta = reading_lookup(id);
    if (!meta)
        return "";
    if (format == READING_FORMAT_SHORT && meta->short_label)
        return meta->short_label;
    if (format == READING_FORMAT_MEDIUM && meta->medium)
        return meta->medium;
    return meta->label;
}

/* Worst-case label width (glyphs) for a concrete format. */
static unsigned label_width_for(const ReadingMeta *meta, ReadingFormat format) {
    return reading_glyph_count(reading_label_for(meta->id, format));
}

ReadingFormat reading_auto_format(ReadingId id, unsigned width) {
    const ReadingMeta *meta = reading_lookup(id);
    if (!meta)
        return READING_FORMAT_FULL;
    /* One space separator is always reserved between label and value. */
    unsigned available = width > (meta->value_width + 1U) ? width - (meta->value_width + 1U) : 0;
    if (label_width_for(meta, READING_FORMAT_FULL) <= available)
        return READING_FORMAT_FULL;
    if (label_width_for(meta, READING_FORMAT_MEDIUM) <= available)
        return READING_FORMAT_MEDIUM;
    return READING_FORMAT_SHORT;
}

/* Format the value/unit/status into a scratch buffer; never convert missing to zero. */
static bool render_value(const ReadingMeta *meta, const ReadingSample *sample, char *scratch, size_t size) {
    if (!meta || !sample || !scratch || !size)
        return false;
    if (sample->state != READING_STATE_VALID) {
        snprintf(scratch, size, "--");
        return true;
    }
    if (meta->value_type == READING_VALUE_OIL_STATUS) {
        int status = 0; /* unknown */
        OilRange range;
        if (vehicle_oil_range(&range) && range.known) {
            float mm = sample->value;
            if (mm < range.minimum_mm)
                status = -1;
            else if (mm > range.maximum_mm)
                status = 2;
            else
                status = 1;
        }
        char label[8];
        reading_enum_label(meta, (float)status, label, sizeof(label));
        snprintf(scratch, size, "%s", label);
        return true;
    }
    if (meta->value_type == READING_VALUE_ENUM) {
        char label[24];
        reading_enum_label(meta, sample->value, label, sizeof(label));
        snprintf(scratch, size, "%s", label);
        return true;
    }
    if (meta->unit)
        snprintf(scratch, size, "%.*f%s", (int)meta->precision, (double)sample->value, meta->unit);
    else
        snprintf(scratch, size, "%.*f", (int)meta->precision, (double)sample->value);
    return true;
}

/* Copy `width` glyphs of a scratch string into out; emit overflow indicator if needed. */
static ReadingFit copy_fitted(const char *scratch, unsigned width, char *out, size_t size) {
    unsigned glyphs = reading_glyph_count(scratch);
    if (glyphs <= width) {
        snprintf(out, size, "%s", scratch);
        return READING_FIT_OK;
    }
    /* Distinct bounded overflow indicator, never truncated digits. */
    if (width < 2)
        snprintf(out, size, "#");
    else {
        unsigned i = 0;
        for (; i < width && i + 1 < size; ++i)
            out[i] = '#';
        out[i] = '\0';
    }
    return READING_FIT_OVERFLOW;
}

ReadingFit reading_format_value(ReadingId id, const ReadingSample *sample, unsigned width, char *out, size_t size) {
    const ReadingMeta *meta = reading_lookup(id);
    if (!meta)
        return READING_FIT_EMPTY;
    char scratch[64];
    if (!render_value(meta, sample, scratch, sizeof(scratch)))
        return READING_FIT_EMPTY;
    return copy_fitted(scratch, width, out, size);
}

ReadingFit reading_format_line(ReadingId id, const ReadingSample *sample, ReadingFormat format, unsigned width,
                               char *out, size_t size) {
    const ReadingMeta *meta = reading_lookup(id);
    if (!meta)
        return READING_FIT_EMPTY;
    if (format == READING_FORMAT_AUTO)
        format = reading_auto_format(id, width);

    char scratch[64];
    if (!render_value(meta, sample, scratch, sizeof(scratch)))
        return READING_FIT_EMPTY;

    char line[96];
    snprintf(line, sizeof(line), "%s %s", reading_label_for(id, format), scratch);
    return copy_fitted(line, width, out, size);
}
