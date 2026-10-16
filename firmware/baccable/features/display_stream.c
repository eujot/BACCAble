#include "features/display_stream.h"
#include "features/oil_bar_animation.h"
#include <string.h>

/* Identify fragments that still need to reach the requested screen. */
static void update_dirty(DisplayStream *stream) {
    stream->dirty = stream->forced;
    for (unsigned i = 0; i < stream->count; ++i) {
        /* Forced fragments are dirty regardless of their previous content. */
        if (stream->forced & (1U << i))
            continue;
        unsigned offset = i * DISPLAY_FRAGMENT_SIZE;
        if (!(stream->known & (1U << i)) ||
            memcmp(stream->sent + offset, (stream->transferring ? stream->active : stream->target) + offset, DISPLAY_FRAGMENT_SIZE))
            stream->dirty |= 1U << i;
    }
}

/* Prepare a full image or one oil animation step for delivery before advancing. */
static void begin_image(DisplayStream *stream) {
    memcpy(stream->active, stream->target, sizeof(stream->active));
    /* Require known fragments and a complete previous bar before animating. */
    stream->animating = stream->known == (1U << stream->count) - 1U &&
                    oil_bar_animate(stream->sent, stream->active, sizeof(stream->active));
    stream->transferring = stream->full || stream->animating;
    if (stream->transferring) stream->cursor = 0;
    if (stream->full) stream->forced = (1U << stream->count) - 1U;
    update_dirty(stream);
}
void display_stream_set_full(DisplayStream *stream, bool full) {
    if (stream->full == full) return;
    stream->full = full;
    stream->transferring = stream->animating = false;
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
        stream->transferring = stream->animating = false;
    }
    stream->count = count;
    memcpy(stream->target, text, sizeof(stream->target));
    stream->valid = true;
    update_dirty(stream);
    if (!stream->transferring && stream->dirty) begin_image(stream);
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
        memcpy(stream->offered, (stream->transferring ? stream->active : stream->target) + i * DISPLAY_FRAGMENT_SIZE, DISPLAY_FRAGMENT_SIZE);
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
    if (stream->transferring && !stream->dirty) {
        stream->transferring = stream->animating = false;
        update_dirty(stream);
        if (stream->dirty) begin_image(stream);
    }
}

/* Forget previous output when BACCAble relinquishes the display. */
void display_stream_reset(DisplayStream *stream) { memset(stream, 0, sizeof(*stream)); }

/* Restore or maintain nonblank text without repeatedly restarting a pending restoration. */
void display_stream_refresh(DisplayStream *stream) {
    if (!stream->valid || stream->forced || stream->transferring)
        return;
    for (unsigned i = 0; i < sizeof(stream->target); ++i) {
        if (stream->target[i] != ' ') {
            /* A complete IPC message begins with fragment zero. Once started,
             * subsequent refresh requests must let it reach its final fragment. */
            if (stream->full) { begin_image(stream); return; }
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
    /* A radio interruption resends the same oil step instead of skipping ahead. */
    if (stream->animating) {
        stream->cursor = 0;
        stream->forced = (1U << stream->count) - 1U;
        update_dirty(stream);
        return;
    }
    if (stream->full) { begin_image(stream); return; }
    stream->cursor = 0;
    stream->forced = (1U << stream->count) - 1U;
    update_dirty(stream);
}
