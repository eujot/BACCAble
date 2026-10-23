#ifndef BACCABLE_VEHICLE_DRIVE_MODE_H
#define BACCABLE_VEHICLE_DRIVE_MODE_H

#include "app/application_state.h"
#include "platform/system.h"

void vehicle_handle_drive_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_lane_state(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void lane_assist_button_observe(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void lane_assist_button_process(void);
void driver_aids_configure(uint8_t mode, uint8_t disable_lka, uint8_t action);

#endif
