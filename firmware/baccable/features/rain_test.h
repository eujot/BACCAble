#ifndef BACCABLE_RAIN_TEST_H
#define BACCABLE_RAIN_TEST_H
#include "stm32f0xx_hal.h"
#include <stddef.h>
#include <stdint.h>

/* Rain-lights debug/test surface, compiled into normal firmware.
 * Single menu entry: force the 0x73E lamp bits and watch C2 wiper counters
 * or BH light-responsive bytes. Remove its call sites with rain_test.c. */

/* C1: open/close the test screen (close also stops any force). */
void rain_test_enter(void);
void rain_test_exit(void);
/* C1: toggle forced lamp bits on/off (30 s auto-off). */
void rain_test_toggle_force(void);
/* C1: poll the visible page and maintain the force lease; C2: no-op. */
void rain_test_process(void);
/* C1: store a debug reply from C2. */
void rain_test_reply(uint8_t cycles_lo, uint8_t cycles_hi, uint8_t flags, uint8_t tx,
                     uint8_t token);
/* C2: send its detection counters back to C1. */
void rain_test_debug_reply(uint8_t token);
/* BH: remember only exact standard 0x738/DLC-8 frames and answer C1 polls. */
void rain_test_sensor_observe(const CAN_RxHeaderTypeDef *header, const uint8_t data[8]);
void rain_test_sensor_debug_reply(uint8_t token);
/* C1: accept a matching BH reply and switch between diagnostic pages. */
void rain_test_sensor_reply(uint8_t first, uint8_t second, uint8_t valid, uint8_t token,
                            uint8_t tx_count);
void rain_test_move_page(void);
/* C1: report whether the temporary lamp force needs the faster UART lease. */
uint8_t rain_test_forcing(void);
/* C1: report whether the diagnostic screen is using UART poll slots. */
uint8_t rain_test_visible(void);
/* C1: two-line display text. */
void rain_test_render(char *first, size_t first_size, char *second, size_t second_size);

#endif
