#include "storage/battery_history_store.h"
#include <string.h>
static HistoryStorage backend;
static uint8_t sessions[HISTORY_SESSIONS], count;
static int active = -1;
static uint16_t get16(const uint8_t *p) { return p[0] | (uint16_t)p[1] << 8; }
static uint32_t get32(const uint8_t *p) { return get16(p) | (uint32_t)get16(p + 2) << 16; }
static void put16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static const uint8_t *slot(unsigned n) { return backend.data + n * HISTORY_SLOT_SIZE; }
static uint16_t crc(const uint8_t *p, unsigned n) {
    uint16_t c = 0xffff;
    while (n--) { c ^= (uint16_t)*p++ << 8;
        for (unsigned b = 0; b < 8; ++b) c = (uint16_t)((c << 1) ^ ((c & 0x8000) ? 0x1021 : 0)); }
    return c;
}
static bool blank(unsigned n) {
    for (unsigned i = 0; i < HISTORY_SLOT_SIZE; ++i) if (slot(n)[i] != 255) return false;
    return true;
}
static bool header_valid(unsigned n) {
    const uint8_t *p = slot(n);
    return get32(p) == 0x31485342U && get16(p + 10) == 0 && get16(p + 8) == crc(p, 8);
}
static void scan(void) {
    count = 0;
    if (!backend.data) return;
    for (unsigned n = 0; n < HISTORY_SLOTS; ++n) {
        if (!header_valid(n)) continue;
        unsigned pos = 0;
        while (pos < count && (int32_t)(get32(slot(n) + 4) - get32(slot(sessions[pos]) + 4)) <= 0) ++pos;
        if (pos >= HISTORY_SESSIONS) continue;
        if (count < HISTORY_SESSIONS) ++count;
        for (unsigned j = count - 1; j > pos; --j) sessions[j] = sessions[j - 1];
        sessions[pos] = n;
    }
}
void history_store_init(HistoryStorage storage) { backend = storage; active = -1; scan(); }
unsigned history_store_count(void) { return count; }
bool history_store_prepare(void) {
    if (!backend.data) return false;
    for (unsigned n = 0; n < HISTORY_SLOTS; ++n) if (blank(n)) return true;
    for (unsigned page = 0; page < HISTORY_PAGES; ++page) {
        bool retained = false;
        for (unsigned j = 0; j < count; ++j) if (sessions[j] / 2 == page) retained = true;
        if (!retained) return backend.erase(page);
    }
    return false;
}
bool history_store_begin(void) {
    active = -1;
    if (!backend.data) return false;
    unsigned start = count ? (sessions[0] + 1) % HISTORY_SLOTS : 0;
    for (unsigned j = 0; j < HISTORY_SLOTS; ++j) {
        unsigned n = (start + j) % HISTORY_SLOTS;
        if (!blank(n)) continue;
        uint8_t h[12] = {0x42, 0x53, 0x48, 0x31};
        uint32_t seq = count ? get32(slot(sessions[0]) + 4) + 1 : 1;
        put16(h + 4, seq); put16(h + 6, seq >> 16); put16(h + 8, crc(h, 8));
        for (unsigned i = 0; i < sizeof(h); i += 2)
            if (!backend.program(n * HISTORY_SLOT_SIZE + i, get16(h + i))) return false;
        if (!header_valid(n)) return false;
        active = (int)n; scan(); return true;
    }
    return false;
}
static bool sample_valid(const uint8_t *p) {
    return get16(p + 8) == 0 && get16(p + 6) == crc(p, 6) && get16(p) <= 720 &&
           (p[4] <= 100 || p[4] == 255);
}
/* Ten-byte records include a CRC and a commit halfword written last. */
bool history_store_append(HistorySample sample) {
    if (active < 0 || sample.minute > 720) return false;
    for (unsigned i = 0; i < HISTORY_SAMPLES; ++i) {
        size_t offset = (unsigned)active * HISTORY_SLOT_SIZE + 16 + i * 10;
        bool empty = true;
        for (unsigned b = 0; b < 10; ++b) if (backend.data[offset + b] != 255) empty = false;
        if (!empty) continue;
        uint8_t p[10] = {0}; put16(p, sample.minute); put16(p + 2, sample.voltage); p[4] = sample.soc;
        put16(p + 6, crc(p, 6));
        for (unsigned b = 0; b < sizeof(p); b += 2)
            if (!backend.program(offset + b, get16(p + b))) return false;
        return sample_valid(backend.data + offset);
    }
    return false;
}
unsigned history_store_samples(unsigned session) {
    if (session >= count) return 0;
    unsigned total = 0;
    for (unsigned i = 0; i < HISTORY_SAMPLES; ++i) if (sample_valid(slot(sessions[session]) + 16 + i * 10)) ++total;
    return total;
}
bool history_store_read(unsigned session, unsigned sample, HistorySample *out) {
    if (session >= count || !out) return false;
    for (unsigned i = 0; i < HISTORY_SAMPLES; ++i) {
        const uint8_t *p = slot(sessions[session]) + 16 + i * 10;
        if (!sample_valid(p)) continue;
        if (sample--) continue;
        *out = (HistorySample){get16(p), get16(p + 2), p[4]}; return true;
    }
    return false;
}
