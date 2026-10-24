#ifndef BACCABLE_RAIN_TEST_H
#define BACCABLE_RAIN_TEST_H
#include <stddef.h>
#include <stdint.h>

/* Rain-lights debug/test surface, compiled into normal firmware.
 * Single menu entry: force the 0x73E lamp bits and watch the C2 wiper
 * counters. Everything here is meant to be removed with rain_test.c. */

/* C1: open/close the test screen (close also stops any force). */
void rain_test_enter(void);
void rain_test_exit(void);
/* C1: toggle forced lamp bits on/off (30 s auto-off). */
void rain_test_toggle_force(void);
/* C1: poll C2 counters and expire the force; C2: no-op. */
void rain_test_process(void);
/* C1: store a debug reply from C2. */
void rain_test_reply(uint8_t cycles_lo, uint8_t cycles_hi, uint8_t flags, uint8_t tx);
/* C2: send its detection counters back to C1. */
void rain_test_debug_reply(uint8_t token);
/* C1: two-line display text. */
void rain_test_render(char *first, size_t first_size, char *second, size_t second_size);

#endif
