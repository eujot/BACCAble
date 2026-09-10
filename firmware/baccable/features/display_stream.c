#include "features/display_stream.h"
#include <string.h>
void display_stream_submit(DisplayStream *stream, const uint8_t *text) {
    if (stream->sending && !memcmp(stream->active, text, sizeof(stream->active))) {
        stream->queued = false;
        return;
    }
    memcpy(stream->pending, text, sizeof(stream->pending));
    stream->queued = true;
}
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]) {
    if (!stream->sending) {
        if (!stream->queued)
            return false;
        memcpy(stream->active, stream->pending, sizeof(stream->active));
        stream->queued = false;
        stream->sending = true;
        stream->valid = true;
        stream->fragment = 0;
    }
    *fragment = stream->fragment;
    memcpy(text, stream->active + 3 * stream->fragment, 3);
    return true;
}
void display_stream_accept(DisplayStream *stream) {
    if (stream->sending && ++stream->fragment == DASHBOARD_MESSAGE_MAX_LENGTH / 3)
        stream->sending = false;
}
void display_stream_reset(DisplayStream *stream) { memset(stream, 0, sizeof(*stream)); }
void display_stream_refresh(DisplayStream *stream) {
    if (!stream->valid || stream->sending || stream->queued)
        return;
    for (unsigned i = 0; i < sizeof(stream->active); ++i) {
        if (stream->active[i] != ' ') {
            display_stream_submit(stream, stream->active);
            return;
        }
    }
}
