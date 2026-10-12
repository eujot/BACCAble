#ifndef BACCABLE_VEHICLE_DRIVE_MODE_H
#define BACCABLE_VEHICLE_DRIVE_MODE_H

#include "app/application_state.h"
#include "platform/system.h"

/* Frame encodings the default-drive-mode override knows how to rewrite. */
#define DRIVE_MODE_ENC_CAN 0  /* C1/C2 0x384, mode in byte 1 bits 6..2 */
#define DRIVE_MODE_ENC_BODY 1 /* BH 0x46C, mode in byte 7 bits 4..0 */
#define DRIVE_MODE_ENC_IPC 2  /* BH 0x25A, mode in byte 0 bits 6..2 */

void vehicle_handle_drive_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_lane_state(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void lane_assist_button_observe(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void lane_assist_button_process(void);

/* Raw field value that encodes the configured default mode for one frame layout. */
uint8_t drive_mode_default_field(uint8_t encoding);

/* Check the observed mode before offering the one start-up frame. */
uint8_t drive_mode_default_should_apply(uint8_t raw_mode);

/* Mark one start-up frame as queued only after CAN accepts it. */
void drive_mode_default_queued(uint8_t encoding);

/* True until this frame has been queued in the current start-up session. */
uint8_t drive_mode_default_pending(uint8_t encoding);

#endif
