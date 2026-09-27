#include "app/application_state.h"
#include "features/usb_modes.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdio.h>

RuntimeState runtime_state;
USBD_HandleTypeDef hUsbDeviceFS;
uint32_t fake_primask;
void usb_device_status(uint8_t out[8]) { memset(out, 0, 8); out[2] = hUsbDeviceFS.dev_state; }
void board_uart_status(uint8_t out[5]) { memset(out, 0, 5); }
static uint32_t now;
static unsigned serial_starts, disk_starts, records;
static uint8_t presence;
static uint8_t ack[19];
static bool reject_status;

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
    if (size == 19 && data[1] == C1_CMD_USB_STATE) {
        if (reject_status) return 0;
        memcpy(ack, data, 19);
        return 1;
    }
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
    reject_status = true;
    usb_modes_request(1, 42);
    usb_modes_process();
    assert(ack[1] == 0);
    reject_status = false;
    usb_modes_process();
    assert(ack[1] == C1_CMD_USB_STATE && ack[3] == 1 && ack[5] == 42);
    unsigned before = serial_starts;
    usb_modes_request(1, 42); usb_modes_process();
    assert(serial_starts == before); /* Duplicate activation does not reenumerate. */
    hUsbDeviceFS.dev_state = 0; now += 10001; usb_modes_process();
    assert(!usb_modes_active());
    usb_modes_request(255, 42); usb_modes_process();
    assert(!usb_modes_active() && serial_starts == before); /* Status query cannot rearm. */
    puts("PASS: auxiliary USB capture, throughput, presence, expiry and rearm");
    return 0;
}
