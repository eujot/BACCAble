#include "app/application_state.h"
#include "features/usb_modes.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdio.h>

RuntimeState runtime_state;
USBD_HandleTypeDef hUsbDeviceFS;
static uint32_t now;
static unsigned serial_starts, disk_starts, records;
static uint8_t presence;

uint32_t HAL_GetTick(void) { return now; }
void HAL_GPIO_DeInit(void *port, uint32_t pins) { (void)port; (void)pins; }
void usb_device_start(uint8_t serial) {
    assert(serial);
    ++serial_starts;
    hUsbDeviceFS.dev_state = 0;
}
void usb_device_stop(void) { hUsbDeviceFS.dev_state = 0; }
void MX_USB_DEVICE_Init(void) { ++disk_starts; }
void cdc_process(void) {}
uint8_t board_uart_send(const uint8_t *data, size_t size) {
    assert(size == 4 && data[0] == C1BusID && data[1] == C1_CMD_USB_PRESENCE);
#ifdef BACCABLE_BH
    assert(data[2] == 1);
#else
    assert(data[2] == 0);
#endif
    presence = data[3];
    return 1;
}
uint8_t CDC_Transmit_FS(uint8_t *data, uint16_t size) {
    assert(size && size <= 64 && size % 16 == 0);
    for (unsigned i = 0; i < size; i += 16)
        assert(data[i] == 0xa8);
    records += size / 16;
    return USBD_OK;
}

int main(void) {
    usb_modes_set_sniffer(true);
    usb_modes_process();
    assert(serial_starts == 1 && usb_modes_active());
    hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
    now = 500;
    runtime_state.we_can_send_a_message_reply = 123;
    usb_modes_process();
    assert(presence == 1 && runtime_state.usb_connected_to_slave);
    assert(runtime_state.we_can_send_a_message_reply == 123);
    CAN_RxHeaderTypeDef h = {.StdId = 0x123, .DLC = 8};
    uint8_t data[8] = {0};
    for (unsigned pass = 0; pass < 50; ++pass) {
        for (unsigned i = 0; i < 8; ++i)
            usb_sniffer_observe(&h, data);
        now += 1000;
        usb_modes_process();
        assert(runtime_state.we_can_send_a_message_reply == 123);
    }
    assert(records == 400 && serial_starts == 1);
    hUsbDeviceFS.dev_state = 0;
    now += 10001;
    usb_modes_process();
    assert(!usb_modes_active() && presence == 0 && disk_starts == 1);
    usb_modes_set_sniffer(true);
    usb_modes_process();
    assert(serial_starts == 2 && usb_modes_active());
    usb_modes_set_sniffer(false);
    usb_modes_process();
    assert(!usb_modes_active() && disk_starts == 2);
    puts("PASS: auxiliary USB capture, throughput, presence, expiry and rearm");
    return 0;
}
