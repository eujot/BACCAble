#include "features/display_stream.h"
#include <string.h>

/* Identify fragments that still need to reach the requested screen. */
static void update_dirty(DisplayStream *stream) {
    stream->dirty = stream->forced;
    for (unsigned i = 0; i < stream->count; ++i) {
        unsigned offset = i * DISPLAY_FRAGMENT_SIZE;
        if (!(stream->known & (1U << i)) ||
            memcmp(stream->sent + offset, (stream->full && stream->transferring ? stream->active : stream->target) + offset, DISPLAY_FRAGMENT_SIZE))
            stream->dirty |= 1U << i;
    }
}

/* Freeze one complete image until its final fragment is accepted. */
static void begin_full(DisplayStream *stream) {
    memcpy(stream->active, stream->target, sizeof(stream->active));
    stream->transferring = true;
    stream->cursor = 0;
    stream->forced = (1U << stream->count) - 1U;
    update_dirty(stream);
}
void display_stream_set_full(DisplayStream *stream, bool full) {
    if (stream->full == full) return;
    stream->full = full;
    stream->transferring = false;
    if (stream->valid) display_stream_restart(stream);
}

/* Replace obsolete waiting content with the latest complete, space-padded screen. */
void display_stream_submit(DisplayStream *stream, const uint8_t *text) {
    display_stream_submit_parts(stream, text, DISPLAY_FRAGMENT_COUNT);
}
void display_stream_submit_parts(DisplayStream *stream, const uint8_t *text, uint8_t count) {
    if (!count || count > DISPLAY_FRAGMENT_COUNT) return;
    if (stream->count != count) {
        stream->known = stream->forced = 0;
        stream->cursor = 0;
        stream->transferring = false;
    }
    stream->count = count;
    memcpy(stream->target, text, sizeof(stream->target));
    stream->valid = true;
    update_dirty(stream);
    if (stream->full && !stream->transferring && stream->dirty) begin_full(stream);
}

/* Offer the next changed fragment fairly, including newer content after a failed send. */
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]) {
    stream->offering = false;
    if (!stream->valid)
        return false;
    for (unsigned n = 0; n < stream->count; ++n) {
        unsigned i = (stream->cursor + n) % stream->count;
        if (!(stream->dirty & (1U << i)))
            continue;
        stream->fragment = i;
        memcpy(stream->offered, (stream->full && stream->transferring ? stream->active : stream->target) + i * DISPLAY_FRAGMENT_SIZE, DISPLAY_FRAGMENT_SIZE);
        memcpy(text, stream->offered, DISPLAY_FRAGMENT_SIZE);
        *fragment = i;
        stream->offering = true;
        return true;
    }
    return false;
}

/* Remember exactly the accepted fragment; a newer target remains pending if it differs. */
void display_stream_accept(DisplayStream *stream) {
    if (!stream->offering)
        return;
    unsigned i = stream->fragment;
    memcpy(stream->sent + i * DISPLAY_FRAGMENT_SIZE, stream->offered, DISPLAY_FRAGMENT_SIZE);
    stream->known |= 1U << i;
    stream->forced &= (uint16_t)~(1U << i);
    stream->cursor = (i + 1) % stream->count;
    stream->offering = false;
    update_dirty(stream);
    if (stream->full && stream->transferring && !stream->dirty) {
        stream->transferring = false;
        update_dirty(stream);
        if (stream->dirty) begin_full(stream);
    }
}

/* Forget previous output when BACCAble relinquishes the display. */
void display_stream_reset(DisplayStream *stream) { memset(stream, 0, sizeof(*stream)); }

/* Restore or maintain nonblank text without repeatedly restarting a pending restoration. */
void display_stream_refresh(DisplayStream *stream) {
    if (!stream->valid || stream->forced)
        return;
    for (unsigned i = 0; i < sizeof(stream->target); ++i) {
        if (stream->target[i] != ' ') {
            /* A complete IPC message begins with fragment zero. Once started,
             * subsequent refresh requests must let it reach its final fragment. */
            if (stream->full) { begin_full(stream); return; }
            stream->cursor = 0;
            stream->forced = (1U << stream->count) - 1U;
            update_dirty(stream);
            return;
        }
    }
}

/* A factory message interrupted this transfer: resend every part from zero. */
void display_stream_restart(DisplayStream *stream) {
    if (!stream->valid)
        return;
    if (stream->full) { begin_full(stream); return; }
    stream->cursor = 0;
    stream->forced = (1U << stream->count) - 1U;
    update_dirty(stream);
}
