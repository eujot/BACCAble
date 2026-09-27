#include "features/periodic.h"
/* Runtime capture behavior adapted from gaucho1978 BACCAble, August 2026. */
#include "features/usb_modes.h"
#if !defined(ACT_AS_CANABLE)
    #include "app/powertrain.h"
    #include "protocol/elm327.h"
    #include "transport/diagnostic_link.h"
    #include "usb_device.h"
    #include "usbd_cdc_if.h"
    #include "diagnostics/fault_reader.h"
    #include "features/ibs_override.h"
    #include "diagnostics/parameter_request.h"
    #include "diagnostics/parameter_cache.h"
extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t requested, active;
static uint32_t activation, last_flush;
static uint8_t command_token;
static uint8_t debug_snapshot[64];
    #if defined(BACCABLE_C1)
static uint8_t generation, pending_mask, attempts[2], alternate[2];
static uint8_t peer_status[2][16], status_seen;
static uint32_t status_time[2];
    #else
static bool status_pending;
    #endif

/* This cached snapshot proves main-loop progress separately from enumeration.
 * Published with IRQs masked; EP0 reads never drive the UART or alter capture. */
void usb_modes_debug_read(uint8_t out[64]) { memcpy(out, debug_snapshot, 64); }

static void local_status(uint8_t out[16]) {
    out[0] = requested;
    out[1] = active;
    out[2] = command_token;
    usb_device_status(out + 3);
    board_uart_status(out + 11);
}

void usb_modes_request(uint8_t mode, uint8_t token) {
    #if !defined(BACCABLE_C1)
    if (mode <= 1) {
        requested = mode;
        command_token = token;
    }
    status_pending = true;
    #else
    (void)mode;
    (void)token;
    #endif
}

void usb_modes_ack(uint8_t peer, const uint8_t status[16]) {
    #if defined(BACCABLE_C1)
    if (peer > 1 || status[0] > 1 || status[1] > 1 || status[3] > 4 || status[5] > 4)
        return;
    memcpy(peer_status[peer], status, 16);
    status_seen |= 1U << peer;
    status_time[peer] = currentTime;
    if (status[0] == (requested == 1) && status[2] == generation)
        pending_mask &= ~(1U << peer);
    #else
    (void)peer;
    (void)status;
    #endif
}

/* Replace every other version poll, preserving the shared link's traffic budget.
 * Pending activation gets three addressed deliveries. A query never rearms an
 * expired session or resets a failed USB controller. Rejected enqueue attempts
 * do not consume the bounded command delivery budget. */
uint8_t usb_modes_poll(uint8_t peer, uint8_t command[4]) {
    #if defined(BACCABLE_C1)
    command[0] = peer ? BhBusID : C2BusID;
    if ((pending_mask & (1U << peer)) || (alternate[peer] ^= 1)) {
        command[1] = BOARD_CMD_USB_STATE;
        command[2] = 255;
        command[3] = generation;
        if ((pending_mask & (1U << peer)) && attempts[peer] < 3) {
            command[2] = requested == 1;
        }
        return 4;
    }
    command[0] = peer ? BhBusIDgetStatus : C2BusID;
    command[1] = peer ? 0 : C2cmdGetStatus;
    return 2;
    #else
    (void)peer;
    (void)command;
    return 0;
    #endif
}

void usb_modes_poll_queued(uint8_t peer, const uint8_t command[4]) {
    #if defined(BACCABLE_C1)
    if (peer < 2 && command[1] == BOARD_CMD_USB_STATE && command[2] <= 1 && attempts[peer] < 3)
        ++attempts[peer];
    #else
    (void)peer;
    (void)command;
    #endif
}

static void publish_status(void) {
    uint8_t next[64] = {1, 0, 1, 0};
    uint32_t now = currentTime;
    for (unsigned i = 0; i < 4; ++i)
        next[4 + i] = now >> (8 * i);
    #if defined(BACCABLE_C1)
    next[3] = pending_mask;
    next[12] = attempts[0];
    next[13] = attempts[1];
    next[14] = generation;
    local_status(next + 16);
    for (unsigned peer = 0; peer < 2; ++peer) {
        uint32_t age = now - status_time[peer];
        if ((status_seen & (1U << peer)) && age < 5000)
            next[2] |= 2U << peer;
        uint16_t ms = !(status_seen & (1U << peer)) || age > 65535 ? 65535 : age;
        next[8 + peer * 2] = ms;
        next[9 + peer * 2] = ms >> 8;
        memcpy(next + 32 + peer * 16, peer_status[peer], 16);
    }
    #else
        #ifdef BACCABLE_BH
    next[1] = 2;
        #else
    next[1] = 1;
        #endif
    next[2] = 1U << next[1];
    local_status(next + 16 + next[1] * 16);
    if (status_pending) {
        uint8_t reply[19] = {C1BusID, C1_CMD_USB_STATE, next[1] - 1};
        memcpy(reply + 3, next + 16 + next[1] * 16, 16);
        if (board_uart_send(reply, sizeof(reply)))
            status_pending = false;
    }
    #endif
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    memcpy(debug_snapshot, next, sizeof(next));
    __set_PRIMASK(irq);
}

static uint8_t frames[16][16], head, tail, count;
static uint16_t dropped;
    #if defined(BACCABLE_C1)
static uint8_t peer_mask;
static uint32_t peer_seen[2];
    #else
static uint32_t last_presence;
static bool was_connected;
    #endif

/* Report whether a runtime USB function is using this board. */
bool usb_modes_active(void) { return active != 0; }

/* Remember which auxiliary USB connections currently need the boards to stay awake. */
void usb_modes_peer(uint8_t peer, bool connected) {
    #if defined(BACCABLE_C1)
    if (peer > 1)
        return;
    if (connected)
        peer_mask |= 1U << peer;
    else
        peer_mask &= ~(1U << peer);
    peer_seen[peer] = currentTime;
    #else
    (void)peer;
    (void)connected;
    #endif
}

/* Request binary capture on an auxiliary board. */
void usb_modes_set_sniffer(bool enabled) { requested = enabled ? 1 : 0; }

/* Compare preferences with the request, including sessions that have expired. */
static uint8_t selected_mode(void) {
    #if defined(BACCABLE_C1)
    if (settings_state.usb_sniffer)
        return 1;
        #ifdef ACT_AS_ELM327
    if (settings_state.usb_elm327)
        return 2;
        #endif
    #endif
    return 0;
}

bool usb_modes_needs_apply(void) { return selected_mode() != requested; }

/* Apply saved USB preferences after the user finishes editing settings. */
void usb_modes_apply(void) {
    #if defined(BACCABLE_C1)
    requested = selected_mode();
    ++generation;
    command_token = generation;
    pending_mask = 3;
    attempts[0] = attempts[1] = 0;
    #endif
}

/* Retain a capture record until USB accepts its complete contents. */
static void capture_push(const uint8_t *record) {
    memcpy(frames[head], record, 16);
    head = (head + 1) % 16;
    ++count;
}

/* Capture incoming CAN data using upstream's fixed sixteen-byte binary format. */
void usb_sniffer_observe(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
    if (active != 1 || h->DLC > 8 || h->RTR != CAN_RTR_DATA)
        return;
    uint8_t record[16] = {0};
    record[1] = currentTime;
    record[2] = currentTime >> 8;
    record[3] = currentTime >> 16;
    if (dropped && count <= 14) {
        record[0] = 0xaf;
        record[4] = dropped;
        record[5] = dropped >> 8;
        capture_push(record);
        dropped = 0;
    }
    if (count == 16) {
        if (dropped < UINT16_MAX)
            ++dropped;
        return;
    }
    uint32_t id = h->IDE == CAN_ID_EXT ? h->ExtId : h->StdId;
    record[0] = 0xa0 | h->DLC;
    for (unsigned i = 0; i < 4; ++i)
        record[4 + i] = id >> (8 * i);
    memcpy(record + 8, data, h->DLC);
    capture_push(record);
}

/* Change USB roles while releasing the previous session and shared hardware pins. */
static void switch_mode(uint8_t mode) {
    #ifdef ACT_AS_ELM327
    if (active == 2)
        elm327_set_enabled(0);
    #endif
    if (active || mode)
        usb_device_stop();
    #if defined(BACCABLE_C1)
        #if defined(DEBUG_MODE) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER) ||                         \
            defined(ENABLE_USB_MASS_STORAGE)
    led_strip_set_usb(true);
        #else
    led_strip_set_usb(mode != 0);
        #endif
    if (mode) {
        parameter_request_cancel();
        fault_reader_cancel();
        ibs_override_enable(false);
        /* Awake boards may already be transmitting: resetting HAL's UART state
         * then would strand TXE. Only resume a UART paused by low-power entry. */
        if (runtime_state.low_consume_is_active) {
            power_wake();
            runtime_state.low_consume_is_active = 0;
            uart_resume(&huart2);
        }
    }
    #endif
    active = mode;

    activation = last_flush = currentTime;
    head = tail = count = 0;
    dropped = 0;
    if (mode) {
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
        usb_device_start(1);
    #ifdef ACT_AS_ELM327
        if (mode == 2)
            elm327_set_enabled(1);
    #endif
    } else {
    #if defined(ENABLE_USB_MASS_STORAGE) || defined(DEBUG_MODE) ||                                           \
        defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
        MX_USB_DEVICE_Init();
    #endif
    #if defined(BACCABLE_C1)
        parameter_cache_reset();
        board_sync_restart();
        runtime_state.last_received_can_msg_time = currentTime;
    #endif
    }
}

/* Maintain USB sessions and return to normal operation after disconnection or inactivity. */
void usb_modes_process(void) {
    publish_status();
    bool connected = hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
    #if defined(BACCABLE_C1)
    for (unsigned i = 0; i < 2; ++i)
        if (currentTime - peer_seen[i] > 4000)
            peer_mask &= ~(1U << i);
    runtime_state.usb_connected_to_slave = peer_mask != 0;
    #else
    if ((connected || was_connected) && (connected != was_connected || currentTime - last_presence >= 1000)) {
        uint8_t presence[] = {C1BusID, C1_CMD_USB_PRESENCE,
        #ifdef BACCABLE_BH
                              1,
        #else
                              0,
        #endif
                              connected};
        if (board_uart_send(presence, sizeof(presence))) {
            /* Queue presence until C1 grants this board a reply window.
             * USB activity must not grant unsolicited access to the shared UART. */
            last_presence = currentTime;
            was_connected = connected;
        }
    }
    runtime_state.usb_connected_to_slave = connected;
    #endif
    if (requested != active)
        switch_mode(requested);
    if (!active)
        return;
    connected = hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
    if (connected) {
        activation = currentTime;
    }
    bool peer_capture = false;
    #if defined(BACCABLE_C1)
    for (unsigned peer = 0; peer < 2; ++peer)
        if (active == 1 && (status_seen & (1U << peer)) && currentTime - status_time[peer] < 5000 &&
            peer_status[peer][1] == 1 && peer_status[peer][5] == USBD_STATE_CONFIGURED)
            peer_capture = true;
    #endif
    bool expired = !connected && !peer_capture && (currentTime - activation >= 10000);
    #ifdef ACT_AS_ELM327
    expired |= active == 2 && !elm327_is_enabled();
    #endif
    if (expired) {
        requested = 0;
    #if defined(BACCABLE_C1)
        settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
        usb_modes_apply(); /* Cancel any unacknowledged activation with an OFF generation. */
    #endif
        switch_mode(0);
        return;
    }
    if (!connected) {
    #ifdef ACT_AS_ELM327
        if (active == 2)
            elm327_port_reset();
    #endif
        return;
    }
    cdc_process();
    /* Drain the bounded ring while CDC has space: CAN can deliver eight frames
     * per main-loop pass, so sending only four here causes avoidable overflow. */
    while (active == 1 && count && (count >= 4 || currentTime - last_flush >= 20)) {
        unsigned n = count < 4 ? count : 4;
        if (n > 16U - tail)
            n = 16U - tail;
        if (CDC_Transmit_FS(frames[tail], n * 16) == USBD_OK) {
            tail = (tail + n) % 16;
            count -= n;
            last_flush = currentTime;
        } else
            break;
    }
}
#endif
