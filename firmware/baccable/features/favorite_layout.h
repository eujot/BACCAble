#ifndef BACCABLE_FAVORITE_LAYOUT_H
#define BACCABLE_FAVORITE_LAYOUT_H

#include "features/reading_catalog.h"
#include "features/reading_format.h"
#include <stdbool.h>
#include <stdint.h>

#define FAVORITE_SET_COUNT 6U
#define FAVORITE_ROWS 2U
#define FAVORITE_ROW_ITEMS 4U

typedef struct {
    ReadingId id;
    ReadingFormat format;
} FavoriteItem;

typedef struct {
    FavoriteItem rows[FAVORITE_ROWS][FAVORITE_ROW_ITEMS];
} FavoriteSet;

typedef struct {
    FavoriteSet sets[FAVORITE_SET_COUNT];
} FavoriteLayout;

typedef enum {
    FAVORITE_OK = 0,
    FAVORITE_ERR_DUPLICATE_IN_ROW,
    FAVORITE_ERR_BAR_NOT_EXCLUSIVE,
    FAVORITE_ERR_INVALID_READING,
    FAVORITE_ERR_ROW_CAPACITY,
} FavoriteError;

#define FAVORITE_EMPTY_ITEM ((FavoriteItem){.id = READING_COUNT, .format = READING_FORMAT_AUTO})

bool favorite_item_empty(FavoriteItem item);
void favorite_set_clear(FavoriteSet *set);
bool favorite_set_validate(const FavoriteSet *set, FavoriteError *error);
bool favorite_row_resolve(const FavoriteItem row[4], unsigned width, FavoriteItem resolved[4]);
bool favorite_set_fits(const FavoriteSet *set, DisplayProfile display);
/* Filter without mutating storage, then split into stable width-based continuation pages. */
unsigned favorite_row_page(const FavoriteItem row[4], const VehicleProfile *profile, unsigned width,
                           unsigned page, FavoriteItem out[4]);

unsigned favorite_row_item_count(const FavoriteItem row[FAVORITE_ROW_ITEMS]);
unsigned favorite_set_unique_sources(const FavoriteSet *set, const VehicleProfile *profile, uint8_t sources[8]);

/*
 * Deterministic defaults from the engine/equipment eligibility table. Missing or
 * unsupported slots are left empty; never filled with unrelated readings.
 */
void favorite_layout_default(FavoriteLayout *layout, const VehicleProfile *profile);

/*
 * Resolve a row into a single rendered line for the target width. Returns the
 * number of items emitted and whether the line fits.
 */
typedef enum { FAVORITE_ROW_FIT = 0, FAVORITE_ROW_OVERFLOW, FAVORITE_ROW_EMPTY } FavoriteRowFit;
FavoriteRowFit favorite_row_render(const FavoriteItem row[FAVORITE_ROW_ITEMS], const ReadingSample *samples,
                                   unsigned width, char *out, size_t size);

#endif /* BACCABLE_FAVORITE_LAYOUT_H */
