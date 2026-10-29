#ifndef BACCABLE_STORAGE_UI_LAYOUT_STORE_H
#define BACCABLE_STORAGE_UI_LAYOUT_STORE_H

#include "features/favorite_layout.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * New UI-layout record schema (VISIBILITY_RECORD type 0x106). This is a fresh
 * format: old 0x103/0x104/0x105 compound-page payloads are intentionally ignored,
 * not imported. The type and length differ so the record store never mistakes one
 * for the other.
 */

#define UI_LAYOUT_RECORD_TYPE 0x106U
#define UI_LAYOUT_SCHEMA_VERSION 1U
#define UI_LAYOUT_RECORD_SIZE 98U /* version + last reading + 6x2x4 favorite items */

typedef struct {
    ReadingId last_reading;
    FavoriteLayout favorites;
} UiLayoutRecord;

void ui_layout_default(UiLayoutRecord *record, const VehicleProfile *profile);
bool ui_layout_encode(const UiLayoutRecord *record, uint8_t out[UI_LAYOUT_RECORD_SIZE]);
bool ui_layout_decode(const uint8_t data[UI_LAYOUT_RECORD_SIZE], UiLayoutRecord *record);

#endif /* BACCABLE_STORAGE_UI_LAYOUT_STORE_H */
