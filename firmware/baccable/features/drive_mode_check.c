#include "features/drive_mode_check.h"
#include "app/powertrain.h"

#if defined(BACCABLE_C1)
enum { CHECK_IDLE, CHECK_QUEUE, CHECK_REPORTS, CHECK_CONFIRMED, CHECK_FAILED };
static uint8_t state;
static uint8_t expected[2], samples[2];
static uint32_t started, first_match[2], last_match[2];

/* A new ignition starts a bounded attempt; saves and OFF clear the old result. */
void drive_mode_check_start(uint8_t enabled) {
    static const uint8_t dna[] = {0, 4, 2, 12};
    static const uint8_t transmission[] = {0, 1, 2, 6};
    unsigned mode = settings_state.default_drive_mode;
    state = enabled && mode >= 1 && mode <= 3 ? CHECK_QUEUE : CHECK_IDLE;
    started = currentTime;
    samples[0] = samples[1] = 0;
    if (state == CHECK_QUEUE) {
        expected[0] = dna[mode];
        expected[1] = transmission[mode];
    }
}

/* Queue acceptance starts observation, but never counts as vehicle acceptance. */
void drive_mode_check_queued(void) {
    if (state != CHECK_QUEUE) return;
    state = CHECK_REPORTS;
    started = currentTime;
    samples[0] = samples[1] = 0;
}

/* Disagreement or a report gap restarts the required run of matching samples. */
void drive_mode_check_observe(uint8_t source, uint8_t value) {
    if (state != CHECK_REPORTS || source > 1) return;
    uint32_t now = currentTime;
    unsigned gap = source ? 1500U : 600U;
    if (value != expected[source]) {
        samples[source] = 0;
        return;
    }
    if (!samples[source] || now - last_match[source] > gap) {
        samples[source] = 0;
        first_match[source] = now;
    }
    if (samples[source] < 255) ++samples[source];
    last_match[source] = now;
}

/* Confirm only fresh, sustained reports at the end of the five-second window. */
void drive_mode_check_process(void) {
    uint32_t now = currentTime;
    if ((state != CHECK_QUEUE && state != CHECK_REPORTS) || now - started < 5000U)
        return;
    uint8_t confirmed = state == CHECK_REPORTS &&
        samples[0] >= 4 && last_match[0] - first_match[0] >= 2000U &&
        now - last_match[0] <= 600U &&
        samples[1] >= 2 && last_match[1] - first_match[1] >= 1000U &&
        now - last_match[1] <= 1500U;
    state = confirmed ? CHECK_CONFIRMED : CHECK_FAILED;
    chassis_state.default_mode_override = 0;
}

/* Keep the result available in Features until the next start or settings save. */
const char *drive_mode_check_status(void) {
    static const char *const labels[] = {"Next start", "Waiting", "Waiting", "Confirmed", "No confirm"};
    return labels[state];
}
#endif
