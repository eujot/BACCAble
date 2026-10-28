#include "storage/ui_layout_store.h"
#include <string.h>

void ui_layout_default(UiLayoutRecord *record, const VehicleProfile *profile) {
    if (!record)
        return;
    record->last_reading = READING_OIL_TEMPERATURE;
    favorite_layout_default(&record->favorites, profile);
}

bool ui_layout_encode(const UiLayoutRecord *record, uint8_t out[UI_LAYOUT_RECORD_SIZE]) {
    if (!record || !out)
        return false;
    memset(out, 0, UI_LAYOUT_RECORD_SIZE);
    out[0] = UI_LAYOUT_SCHEMA_VERSION;
    out[1] = record->last_reading < READING_COUNT ? (uint8_t)record->last_reading : (uint8_t)READING_OIL_TEMPERATURE;

    size_t offset = 2;
    for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s) {
        for (unsigned r = 0; r < FAVORITE_ROWS; ++r) {
            for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i) {
                FavoriteItem item = record->favorites.sets[s].rows[r][i];
                out[offset++] = item.id < READING_COUNT ? (uint8_t)item.id : (uint8_t)READING_COUNT;
                out[offset++] = item.format <= READING_FORMAT_SHORT ? (uint8_t)item.format
                                                                    : (uint8_t)READING_FORMAT_AUTO;
            }
        }
    }
    return offset == UI_LAYOUT_RECORD_SIZE;
}

bool ui_layout_decode(const uint8_t data[UI_LAYOUT_RECORD_SIZE], UiLayoutRecord *record) {
    if (!data || !record)
        return false;
    if (data[0] != UI_LAYOUT_SCHEMA_VERSION)
        return false;

    UiLayoutRecord draft;
    draft.last_reading = data[1] < READING_COUNT ? (ReadingId)data[1] : READING_OIL_TEMPERATURE;
    for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s)
        favorite_set_clear(&draft.favorites.sets[s]);

    size_t offset = 2;
    for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s) {
        for (unsigned r = 0; r < FAVORITE_ROWS; ++r) {
            for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i) {
                uint8_t id = data[offset++];
                uint8_t format = data[offset++];
                if (id > READING_COUNT || format > READING_FORMAT_SHORT)
                    return false;
                if (id == READING_COUNT) {
                    draft.favorites.sets[s].rows[r][i] = FAVORITE_EMPTY_ITEM;
                } else {
                    draft.favorites.sets[s].rows[r][i] =
                        (FavoriteItem){.id = (ReadingId)id, .format = (ReadingFormat)format};
                }
            }
        }
    }

    FavoriteError error;
    for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s)
        if (!favorite_set_validate(&draft.favorites.sets[s], &error))
            return false;

    *record = draft;
    return true;
}
