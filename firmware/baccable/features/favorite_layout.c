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
            if (item.id > READING_COUNT || item.format > READING_FORMAT_SHORT) { if (error) *error = FAVORITE_ERR_INVALID_READING; return false; }
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
            if (vehicle_reading_support_for(profile, set->rows[r][i].id) == VEHICLE_SUPPORT_UNSUPPORTED) continue;
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
    if (vehicle_display_profile_for(profile).rows == 2) {
        /* MY23 presets intentionally use fewer fields on the 16-glyph first row. */
        layout->sets[0].rows[0][1] = FAVORITE_EMPTY_ITEM;
        layout->sets[0].rows[1][1] = (FavoriteItem){READING_COOLANT_TEMP, READING_FORMAT_AUTO};
        layout->sets[1].rows[0][1] = FAVORITE_EMPTY_ITEM;
        layout->sets[2].rows[0][0] = (FavoriteItem){READING_BOOST, READING_FORMAT_AUTO};
        layout->sets[2].rows[0][1] = FAVORITE_EMPTY_ITEM;
        layout->sets[4].rows[0][1] = FAVORITE_EMPTY_ITEM;
        layout->sets[4].rows[1][1] = (FavoriteItem){READING_BATTERY_CURRENT, READING_FORMAT_AUTO};
        if (vehicle_profile_is_diesel(profile)) {
            layout->sets[5].rows[0][1] = FAVORITE_EMPTY_ITEM;
            layout->sets[5].rows[1][1] = (FavoriteItem){READING_DPF_TEMP, READING_FORMAT_AUTO};
        }
    }
    for (unsigned s = 0; s < 6; ++s) for (unsigned r = 0; r < 2; ++r) for (unsigned i = 0; i < 4; ++i)
        if (!favorite_item_empty(layout->sets[s].rows[r][i]) &&
            vehicle_reading_support_for(profile, layout->sets[s].rows[r][i].id) == VEHICLE_SUPPORT_UNSUPPORTED)
            layout->sets[s].rows[r][i] = FAVORITE_EMPTY_ITEM;
}

/* Resolve labels from reserved field widths, independent of current samples. */
static unsigned row_width(const FavoriteItem row[4]) {
    unsigned width = 0, count = 0;
    for (unsigned i = 0; i < 4; ++i) if (!favorite_item_empty(row[i])) {
        width += (count++ ? 1 : 0) + reading_glyph_count(reading_label_for(row[i].id, row[i].format)) +
                 1 + reading_value_budget(row[i].id);
    }
    return width;
}
bool favorite_row_resolve(const FavoriteItem row[4], unsigned width, FavoriteItem out[4]) {
    memcpy(out, row, sizeof(FavoriteItem) * 4);
    if (favorite_row_item_count(row) == 1) {
        for (unsigned i = 0; i < 4; ++i) if (row[i].id == READING_OIL_LEVEL_BAR) return true;
    }
    for (unsigned i = 0; i < 4; ++i)
        if (out[i].format == READING_FORMAT_AUTO) out[i].format = READING_FORMAT_FULL;
    while (row_width(out) > width) {
        int best = -1;
        unsigned saving = 0;
        ReadingFormat chosen = READING_FORMAT_SHORT;
        for (unsigned i = 0; i < 4; ++i) {
            if (favorite_item_empty(row[i]) || row[i].format != READING_FORMAT_AUTO) continue;
            unsigned old = reading_glyph_count(reading_label_for(out[i].id, out[i].format));
            for (ReadingFormat f = out[i].format + 1; f <= READING_FORMAT_SHORT; ++f) {
                unsigned length = reading_glyph_count(reading_label_for(out[i].id, f));
                if (length < old && old - length > saving) { best = i; saving = old - length; chosen = f; }
            }
        }
        if (best < 0) return false;
        out[best].format = chosen;
    }
    return true;
}
bool favorite_set_fits(const FavoriteSet *set, DisplayProfile display) {
    FavoriteItem resolved[4];
    if (!favorite_set_validate(set, NULL)) return false;
    for (unsigned r = 0; r < 2; ++r)
        if (!favorite_row_resolve(set->rows[r], display.widths[display.rows == 2 ? r : 0], resolved)) return false;
    return true;
}
unsigned favorite_row_page(const FavoriteItem row[4], const VehicleProfile *profile, unsigned width,
                           unsigned page, FavoriteItem out[4]) {
    FavoriteItem part[4], resolved[4];
    for (unsigned i = 0; i < 4; ++i) out[i] = part[i] = FAVORITE_EMPTY_ITEM;
    unsigned pages = 0, count = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (favorite_item_empty(row[i]) || vehicle_reading_support_for(profile, row[i].id) == VEHICLE_SUPPORT_UNSUPPORTED) continue;
        part[count] = row[i];
        if (count && !favorite_row_resolve(part, width, resolved)) {
            part[count] = FAVORITE_EMPTY_ITEM;
            if (pages == page) memcpy(out, part, sizeof(part));
            ++pages;
            for (unsigned k = 0; k < 4; ++k) part[k] = FAVORITE_EMPTY_ITEM;
            count = 0; part[0] = row[i];
        }
        if (!count && !favorite_row_resolve(part, width, resolved))
            part[0].format = READING_FORMAT_AUTO; /* Temporary presentation after narrowing; storage is unchanged. */
        ++count;
    }
    if (count) { if (pages == page) memcpy(out, part, sizeof(part)); ++pages; }
    return pages;
}
FavoriteRowFit favorite_row_render(const FavoriteItem row[4], const ReadingSample *samples,
                                   unsigned width, char *out, size_t size) {
    if (!out || !size) return FAVORITE_ROW_EMPTY;
    out[0] = 0;
    if (!favorite_row_item_count(row)) return FAVORITE_ROW_EMPTY;
    FavoriteItem resolved[4];
    if (!favorite_row_resolve(row, width, resolved)) {
        snprintf(out, size, "Layout too wide"); return FAVORITE_ROW_OVERFLOW;
    }
    unsigned used = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (favorite_item_empty(resolved[i])) continue;
        ReadingSample missing = {READING_STATE_MISSING, 0};
        const ReadingSample *sample = samples ? &samples[resolved[i].id] : &missing;
        if (resolved[i].id == READING_OIL_LEVEL_BAR) {
            reading_format_value(resolved[i].id, sample, width, out, size); return FAVORITE_ROW_FIT;
        }
        char value[32];
        unsigned budget = reading_value_budget(resolved[i].id);
        reading_format_value(resolved[i].id, sample, budget, value, sizeof(value));
        const char *label = reading_label_for(resolved[i].id, resolved[i].format);
        /* Padding reserves the same position for the next item at 99 -> 100. */
        int written = snprintf(out + used, size > used ? size - used : 0, "%s%s %-*s",
                               used ? " " : "", label, (int)budget, value);
        if (written < 0 || (unsigned)written >= size - used) return FAVORITE_ROW_OVERFLOW;
        used += written;
    }
    while (used && out[used-1] == ' ') out[--used] = 0;
    return FAVORITE_ROW_FIT;
}
