#ifndef BACCABLE_FEATURES_DRIVE_MODE_CHECK_H
#define BACCABLE_FEATURES_DRIVE_MODE_CHECK_H

#include <stdint.h>

/* Start a new ignition check, or clear it after OFF, sleep or a settings save. */
void drive_mode_check_start(uint8_t enabled);
/* Start the reply timeout after the engine-ready request enters the UART queue. */
void drive_mode_check_engine_started(void);
/* Begin observation only after BH finishes its gesture for this startup token. */
void drive_mode_check_reply(uint8_t token, uint8_t result);
/* Identify the current startup request without persisting another setting. */
uint8_t drive_mode_check_token(void);
/* Observe an unmodified DNA (0) or transmission (1) mode report. */
void drive_mode_check_observe(uint8_t source, uint8_t value);
/* Finish the bounded check even when no further CAN reports arrive. */
void drive_mode_check_process(void);
/* Describe the last check without implying an ECU acknowledgement. */
const char *drive_mode_check_status(void);

#endif
