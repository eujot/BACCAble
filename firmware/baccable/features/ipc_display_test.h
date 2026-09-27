#ifndef BACCABLE_FEATURES_IPC_DISPLAY_TEST_H
#define BACCABLE_FEATURES_IPC_DISPLAY_TEST_H

#include <stdbool.h>
#include <stdint.h>

/* Reserved first payload byte in a BH screen packet; ordinary labels are printable. */
#define IPC_TEST_SENTINEL 0x01U
#define IPC_TEST_PATTERN_COUNT 4U
#define IPC_TEST_REFRESH_MS 3000U
#define IPC_TEST_LEASE_MS 5000U

bool ipc_display_test_select(uint8_t source, uint8_t pattern, uint32_t now);
void ipc_display_test_stop(void);
bool ipc_display_test_active(void);
bool ipc_display_test_expired(uint32_t now);
void ipc_display_test_restart(uint32_t now);
void ipc_display_test_refresh(uint32_t now);
bool ipc_display_test_peek(uint8_t frame[8]);
void ipc_display_test_accept(void);

#endif
