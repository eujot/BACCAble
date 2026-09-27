#ifndef BACCABLE_USB_MODES_H
#define BACCABLE_USB_MODES_H
#include "app/build_config.h"
#include "stm32f0xx_hal.h"
#include <stdbool.h>
void usb_modes_apply(void);
/* Addressed status polls share the existing C1 scheduler and reply windows. */
uint8_t usb_modes_poll(uint8_t peer, uint8_t command[4]);
void usb_modes_poll_queued(uint8_t peer, const uint8_t command[4]);
void usb_modes_ack(uint8_t peer, const uint8_t status[16]);
void usb_modes_request(uint8_t mode, uint8_t token);
void usb_modes_debug_read(uint8_t out[64]);
bool usb_modes_needs_apply(void);
void usb_modes_process(void);
void usb_modes_set_sniffer(bool enabled);
void usb_modes_peer(uint8_t peer, bool connected);
bool usb_modes_active(void);
void usb_sniffer_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
#endif
