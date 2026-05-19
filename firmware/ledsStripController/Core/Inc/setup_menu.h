/*
 * setup_menu.h
 *
 * Single source of truth for setup parameters persisted to flash.
 * See setup_menu.c for the table and how to add a new entry.
 */

#ifndef INC_SETUP_MENU_H_
#define INC_SETUP_MENU_H_

#include <stdint.h>

#if defined(C1baccable)

#define SETUP_MENU_NO_PAGE  0xFF  // entry is not shown in the setup menu
#define SETUP_FLASH_SLOTS   30    // minimum reserved flash slots (1..SETUP_FLASH_SLOTS)
#define SETUP_FLASH_PARAM_BUFFER_SIZE 40

typedef enum {
    SETUP_TOGGLE_NONE = 0,  // no setup-menu toggle
    SETUP_TOGGLE_AUTO_BOOL, // simple bool handled by setup_toggle_if_auto_bool()
    SETUP_TOGGLE_CUSTOM,    // caller must handle toggle and side effects
} SetupToggleMode;

typedef enum {
    SETUP_VALUE_UINT8 = 0,
    SETUP_VALUE_UINT16,
    SETUP_VALUE_INT8_AS_UINT8,
} SetupValueType;

typedef enum {
    SETUP_DISPLAY_NONE = 0,
    SETUP_DISPLAY_CHECKBOX,
} SetupDisplayMode;

// Describes one parameter persisted to flash. toggle_mode decides whether the
// table may toggle the entry automatically or the caller must handle side
// effects explicitly.
typedef struct {
    uint8_t         flash_index;  // 1-based slot (matches readFromFlash argument)
    uint8_t         menu_page;    // page shown in the setup menu, or SETUP_MENU_NO_PAGE
    uint16_t        max_value;    // highest valid persisted value
    SetupValueType  value_type;   // runtime variable type
    SetupToggleMode toggle_mode;  // automatic, custom, or no menu toggle
    SetupDisplayMode display_mode;// how the shared menu code may update display
    void           *value;        // pointer to the runtime variable
} SetupParam;

extern const SetupParam setup_params[];
extern const uint8_t    setup_params_count;

const SetupParam *setup_find_by_flash_index(uint8_t flash_index);

// Return the number of flash slots that must be written to persist all table
// entries, including future entries above SETUP_FLASH_SLOTS.
uint8_t setup_flash_slots_count(void);

// Load all table entries from flash into their runtime variables.
void setup_load_from_flash(void);

// Write each table entry into params[flash_index-1].
void setup_fill_flash_params(uint16_t *params);

// Return 1 if any table entry differs from its stored flash value.
uint8_t setup_is_dirty(void);

// If page_index has a table entry, write checkbox_symbols[!!*value] into
// dashboard_setup_menu_array[page_index][0]. Returns 1 if handled.
uint8_t setup_update_checkbox(uint8_t page_index);

// If page_index maps to a SETUP_TOGGLE_AUTO_BOOL entry, toggle *value and
// return 1. Returns 0 when the page needs custom toggle logic in the caller.
uint8_t setup_toggle_if_auto_bool(uint8_t page_index);

#endif /* C1baccable */

#endif /* INC_SETUP_MENU_H_ */
