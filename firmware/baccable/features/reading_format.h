#ifndef BACCABLE_READING_FORMAT_H
#define BACCABLE_READING_FORMAT_H

#include "features/reading_catalog.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    READING_FIT_OK = 0,
    READING_FIT_OVERFLOW, /* Bounded indicator emitted; digits are never clipped. */
    READING_FIT_EMPTY,
} ReadingFit;

typedef enum { READING_STATE_MISSING = 0, READING_STATE_STALE, READING_STATE_VALID } ReadingState;

typedef struct {
    ReadingState state;
    float value;
} ReadingSample;

/* Count UTF-8 glyphs (non-continuation bytes), not raw byte length. */
unsigned reading_glyph_count(const char *text);

/* Resolve a label for a reading and explicit/Auto format. */
const char *reading_label_for(ReadingId id, ReadingFormat format);

/*
 * Format only the value/unit/status portion (MY23 line 2, or the numeric field of
 * a legacy line). Fits to `width` glyphs; overflow emits a distinct bounded
 * indicator instead of clipped digits.
 */
ReadingFit reading_format_value(ReadingId id, const ReadingSample *sample, unsigned width, char *out, size_t size);

/*
 * Format a full single-line "label value" presentation (legacy Readings). When
 * `format` is Auto, choose the most readable fitting label using reserved widths.
 */
ReadingFit reading_format_line(ReadingId id, const ReadingSample *sample, ReadingFormat format, unsigned width,
                               char *out, size_t size);

/* Resolve Auto to a concrete format using reserved field widths (stable, once). */
ReadingFormat reading_auto_format(ReadingId id, unsigned width);

#endif /* BACCABLE_READING_FORMAT_H */
