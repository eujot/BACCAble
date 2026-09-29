#include "features/my23_ui.h"
#include <string.h>
uint16_t my23_codepoint(uint8_t token) {
    static const uint16_t symbols[] = {0x203a, 0x2022, 0x25b2, 0x25bc, 0x2713, 0x00d7, 0x2192, 0x2026};
    return token >= 0x80 && token <= 0x87 ? symbols[token - 0x80] : token;
}
/* Both existing Latin-1/token strings and well-formed UTF-8 enter the same glyph budget. */
/* Share glyph decoding across both fields to keep the embedded image bounded. */
__attribute__((noinline)) size_t my23_text_encode(uint8_t *out, size_t limit, const char *text) {
    size_t n = 0;
    const uint8_t *p = (const uint8_t *)text;
    while (*p && n < limit) {
        uint32_t cp = *p++;
        if (cp >= 0xc2 && cp <= 0xdf && (p[0] & 0xc0) == 0x80) {
            cp = ((cp & 31) << 6) | (p[0] & 63); ++p;
        } else if (cp >= 0xe0 && cp <= 0xef && p[0] && p[1] &&
                   (p[0] & 0xc0) == 0x80 && (p[1] & 0xc0) == 0x80) {
            cp = ((cp & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63); p += 2;
        }
        uint8_t token = cp <= 255 ? cp : '?';
        for (unsigned i = 0x80; i <= 0x87; ++i)
            if (my23_codepoint(i) == cp) token = i;
        out[n++] = token;
    }
    return n;
}
/* Compose once: LIST reserves its marker before encoding the active label. */
__attribute__((noinline)) void my23_packet_mode(uint8_t out[MY23_PACKET_SIZE], UiMode mode,
                                               const char *first, const char *second) {
    memset(out, ' ', MY23_PACKET_SIZE);
    out[0] = MY23_PACKET_MARKER;
    if (mode == UI_MODE_LIST) {
        out[1] = 0x80;
        /* One look-ahead glyph detects overflow; L2 is encoded afterward. */
        size_t count = my23_text_encode(out + 3, MY23_L1_VISIBLE - 1, first);
        if (count > MY23_L1_VISIBLE - 2) out[MY23_L1_VISIBLE] = 0x87;
        out[1 + MY23_L1_VISIBLE] = ' ';
    } else {
        my23_text_encode(out + 1, MY23_L1_VISIBLE, first);
    }
    my23_text_encode(out + 1 + MY23_L1_VISIBLE, MY23_L2_VISIBLE, second);
}
void my23_packet(uint8_t out[MY23_PACKET_SIZE], const char *first, const char *second) {
    my23_packet_mode(out, UI_MODE_PARAMETER, first, second);
}
