#include "features/favorite_layout.h"
#include "features/reading_format.h"
#include <stdio.h>
#include <string.h>

bool favorite_item_empty(FavoriteItem item) { return item.id >= READING_COUNT; }

void favorite_set_clear(FavoriteSet *set) {
    for (unsigned r = 0; r < FAVORITE_ROWS; ++r)
        for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i)
            set->rows[r][i] = FAVORITE_EMPTY_ITEM;
}

unsigned favorite_row_item_count(const FavoriteItem row[FAVORITE_ROW_ITEMS]) {
    unsigned count = 0;
    for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i)
        if (!favorite_item_empty(row[i]))
            ++count;
    return count;
}

bool favorite_set_validate(const FavoriteSet *set, FavoriteError *error) {
    if (error)
        *error = FAVORITE_OK;
    if (!set)
        return false;
    for (unsigned r = 0; r < FAVORITE_ROWS; ++r) {
        for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i) {
            FavoriteItem item = set->rows[r][i];
            if (favorite_item_empty(item))
                continue;
            if (reading_lookup(item.id) == NULL) {
                if (error) *error = FAVORITE_ERR_INVALID_READING;
                return false;
            }
            for (unsigned j = 0; j < i; ++j)
                if (set->rows[r][j].id == item.id) {
                    if (error) *error = FAVORITE_ERR_DUPLICATE_IN_ROW;
                    return false;
                }
        }
    }
    for (unsigned r = 0; r < FAVORITE_ROWS; ++r) {
        bool has_bar = false;
        unsigned count = favorite_row_item_count(set->rows[r]);
        for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i)
            has_bar |= set->rows[r][i].id == READING_OIL_LEVEL_BAR;
        if (has_bar && count != 1) {
            if (error) *error = FAVORITE_ERR_BAR_NOT_EXCLUSIVE;
            return false;
        }
    }
    return true;
}

/* Distinct UDS sources referenced by a set, in row-major order and deduplicated. */
unsigned favorite_set_unique_sources(const FavoriteSet *set, const VehicleProfile *profile, uint8_t sources[8]) {
    unsigned count = 0;
    if (!set)
        return 0;
    for (unsigned r = 0; r < FAVORITE_ROWS; ++r) {
        for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i) {
            if (favorite_item_empty(set->rows[r][i]))
                continue;
            const ReadingMeta *meta = reading_lookup(set->rows[r][i].id);
            uint8_t source = reading_source(meta, profile);
            if (source == READING_NO_SOURCE)
                continue;
            bool seen = false;
            for (unsigned k = 0; k < count; ++k)
                seen |= sources[k] == source;
            if (!seen && count < 8)
                sources[count++] = source;
        }
    }
    return count;
}

static void set_item(FavoriteSet *set, unsigned row, unsigned slot, ReadingId id) {
    if (row < FAVORITE_ROWS && slot < FAVORITE_ROW_ITEMS)
        set->rows[row][slot] = (FavoriteItem){.id = id, .format = READING_FORMAT_AUTO};
}

void favorite_layout_default(FavoriteLayout *layout, const VehicleProfile *profile) {
    for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s)
        favorite_set_clear(&layout->sets[s]);
    if (!profile)
        return;

    /* Warm-up */
    set_item(&layout->sets[0], 0, 0, READING_OIL_TEMPERATURE);
    set_item(&layout->sets[0], 0, 1, READING_COOLANT_TEMP);
    set_item(&layout->sets[0], 1, 0, READING_GEARBOX_TEMP);

    /* Oil */
    set_item(&layout->sets[1], 0, 0, READING_OIL_LEVEL_MM);
    set_item(&layout->sets[1], 0, 1, READING_OIL_LEVEL_STATUS);
    set_item(&layout->sets[1], 1, 0, READING_OIL_LEVEL_BAR);

    /* Engine: RPM is native telemetry (no UDS source), use Power as the documented
     * same-theme substitute for the warm-up RPM slot. */
    set_item(&layout->sets[2], 0, 0, READING_POWER);
    set_item(&layout->sets[2], 0, 1, READING_BOOST);
    set_item(&layout->sets[2], 1, 0, READING_POWER);
    set_item(&layout->sets[2], 1, 1, READING_TORQUE);

    /* Cooling */
    set_item(&layout->sets[3], 0, 0, READING_COOLANT_TEMP);
    if (vehicle_profile_is_diesel(profile)) {
        set_item(&layout->sets[3], 1, 0, READING_INTAKE_AIR_TEMP);
    } else {
        set_item(&layout->sets[3], 1, 0, READING_INTERCOOLER_IN);
        set_item(&layout->sets[3], 1, 1, READING_INTERCOOLER_OUT);
    }

    /* Battery */
    set_item(&layout->sets[4], 0, 0, READING_BATTERY_VOLTAGE);
    set_item(&layout->sets[4], 0, 1, READING_BATTERY_CURRENT);
    set_item(&layout->sets[4], 1, 0, READING_BATTERY_SOC_IBS);

    /* Diesel DPF / gasoline Performance */
    if (vehicle_profile_is_diesel(profile)) {
        set_item(&layout->sets[5], 0, 0, READING_DPF_LOAD);
        set_item(&layout->sets[5], 0, 1, READING_DPF_TEMP);
        set_item(&layout->sets[5], 1, 0, READING_DPF_REGEN_PROGRESS);
    } else {
        set_item(&layout->sets[5], 0, 0, READING_TIME_0_100);
        set_item(&layout->sets[5], 1, 0, READING_BEST_0_100);
    }
}

/* Render one row by joining item label + value with a single space. */
FavoriteRowFit favorite_row_render(const FavoriteItem row[FAVORITE_ROW_ITEMS], const ReadingSample *samples,
                                   unsigned width, char *out, size_t size) {
    if (!out || !size)
        return FAVORITE_ROW_EMPTY;
    out[0] = '\0';
    unsigned count = favorite_row_item_count(row);
    if (!count)
        return FAVORITE_ROW_EMPTY;

    unsigned used = 0;
    for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i) {
        if (favorite_item_empty(row[i]))
            continue;
        char value[32];
        const ReadingSample *sample = samples ? &samples[row[i].id] : NULL;
        ReadingSample missing = {.state = READING_STATE_MISSING, .value = 0.0f};
        if (!sample)
            sample = &missing;
        ReadingFit fit = reading_format_value(row[i].id, sample, width - used, value, sizeof(value));
        if (fit == READING_FIT_EMPTY)
            continue;
        const char *label = reading_label_for(row[i].id, row[i].format);
        char piece[64];
        snprintf(piece, sizeof(piece), "%s%s %s", used ? " " : "", label, value);
        unsigned piece_width = reading_glyph_count(piece);
        if (used + piece_width > width) {
            /* Overflow the whole row with a bounded indicator, never drop items. */
            for (unsigned k = 0; k < width && k + 1 < size; ++k)
                out[k] = '#';
            out[width < size ? width : size - 1] = '\0';
            return FAVORITE_ROW_OVERFLOW;
        }
        if (used + piece_width + 1 > size)
            return FAVORITE_ROW_OVERFLOW;
        strncat(out, piece, size - used - 1);
        used += piece_width;
    }
    return FAVORITE_ROW_FIT;
}
