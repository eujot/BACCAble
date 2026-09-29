#ifndef BACCABLE_DISPLAY_STREAM_H
#define BACCABLE_DISPLAY_STREAM_H
#include "app/build_config.h"
#include <stdbool.h>
#include <stdint.h>
#define DISPLAY_FRAGMENT_SIZE 3U
/* The IPC's second line carries a temporary BACCAble identity below the menu. */
#define DISPLAY_FOOTER "BACCAble beta"
#define DISPLAY_FOOTER_LENGTH (sizeof(DISPLAY_FOOTER) - 1U)
#define DISPLAY_OUTPUT_LENGTH 39U /* 16 glyphs, CR, 22 glyphs. */
#define DISPLAY_FRAGMENT_COUNT (DISPLAY_OUTPUT_LENGTH / DISPLAY_FRAGMENT_SIZE)
#define DISPLAY_LEGACY_FRAGMENT_COUNT ((DASHBOARD_MESSAGE_MAX_LENGTH + 1U + DISPLAY_FOOTER_LENGTH + 2U) / 3U)
_Static_assert(DASHBOARD_MESSAGE_MAX_LENGTH % DISPLAY_FRAGMENT_SIZE == 0 && DISPLAY_FRAGMENT_COUNT <= 16,
               "Display fragments must fit the tracking mask");
/* Main-loop owned. Sent text records CAN queue acceptance, not an acknowledgment from the IPC. */
typedef struct {
    uint8_t sent[DISPLAY_OUTPUT_LENGTH];
    uint8_t target[DISPLAY_OUTPUT_LENGTH];
    uint8_t offered[DISPLAY_FRAGMENT_SIZE];
    uint16_t known, forced, dirty;
    uint8_t cursor, fragment, count;
    bool valid, offering;
} DisplayStream;
void display_stream_submit(DisplayStream *stream, const uint8_t *text);
void display_stream_submit_parts(DisplayStream *stream, const uint8_t *text, uint8_t count);
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]);
void display_stream_accept(DisplayStream *stream);
void display_stream_reset(DisplayStream *stream);
void display_stream_refresh(DisplayStream *stream);
void display_stream_restart(DisplayStream *stream);
#endif
