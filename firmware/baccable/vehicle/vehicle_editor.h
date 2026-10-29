#ifndef BACCABLE_VEHICLE_EDITOR_H
#define BACCABLE_VEHICLE_EDITOR_H

#include "vehicle/vehicle_profile.h"
#include <stdbool.h>
#include <stddef.h>

/* Isolated complete profile draft, consequences review, then atomic save. */

typedef enum {
    VEHICLE_EDITOR_BROWSE = 0,
    VEHICLE_EDITOR_EDIT,
    VEHICLE_EDITOR_CONFIRM,
} VehicleEditorState;

void vehicle_editor_begin(void);   /* Load the current profile into an isolated draft. */
void vehicle_editor_cancel(void);  /* Discard the draft. */
void vehicle_editor_navigate(int direction);
void vehicle_editor_select(void);
bool vehicle_editor_back(void);    /* Returns false when the editor should close. */
const char *vehicle_editor_save(void);    /* NULL on success, otherwise a review/error message; retains failed draft. */

VehicleEditorState vehicle_editor_state(void);
const char *vehicle_editor_field_label(void);
const char *vehicle_editor_value_label(char *buf, size_t size);

#endif /* BACCABLE_VEHICLE_EDITOR_H */
