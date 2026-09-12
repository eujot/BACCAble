#include "features/freeze_log.h"
#if defined(FREEZE_DIAGNOSTICS) && !defined(ACT_AS_CANABLE)
    #include "app/main.h"
    #include "third_party/fatfs/ff.h"
    #include "usb_device.h"
    #include "transport/diagnostic_link.h"
    #include "features/usb_modes.h"
    #include "storage/flash_records.h"
    #include "third_party/printf/printf.h"
    #include <string.h>
extern FATFS diagnostic_filesystem;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint32_t board_uart_timeouts;
volatile uint32_t freeze_alloc_failures;
volatile uint8_t freeze_disk_busy;
static uint32_t reset_flags, loops, last_write, saved_timeouts, minimum_gap = UINT32_MAX;
static uint16_t slot;
static bool opened;

/* Keep the reset cause before initialization clears the hardware flags. */
void freeze_log_boot(void) { reset_flags = RCC->CSR; }

/* Store a bounded breadcrumb trail without sharing the mounted disk with a USB host. */
void freeze_log_process(void) {
    ++loops;
    uint32_t now = HAL_GetTick();
    uint32_t sp = __get_MSP();
    uint32_t heap = freeze_heap_boundary();
    uint32_t gap = sp > heap ? sp - heap : 0;
    if (gap < minimum_gap)
        minimum_gap = gap;
    /* No Flash writes while USB is configured or a diagnostic USB role owns the device. */
    if (gap < 768 || !flash_storage_available() || usb_modes_active() ||
        hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
        return;
    if (now < 10000 || (last_write && now - last_write < 300000 &&
                        !(board_uart_timeouts != saved_timeouts && now - last_write >= 60000)))
        return;
    last_write = now; /* Throttle storage failures as well as successful writes. */
    /* Close the race with USB enumeration before claiming the filesystem. */
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
        __set_PRIMASK(irq);
        return;
    }
    freeze_disk_busy = 1;
    __set_PRIMASK(irq);
    if (f_mount(&diagnostic_filesystem, "", 1) != FR_OK) {
        f_unmount("");
        freeze_disk_busy = 0;
        return;
    }
    FIL file;
    if (f_open(&file, "FREEZE.LOG", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK) {
        if (!opened)
            slot = (f_size(&file) / 256) % 24;
        char record[256];
        memset(record, ' ', sizeof(record));
        int length =
            snprintf_(record, sizeof(record),
                      "%s ms=%lu loop=%lu reset=%08lx uart_to=%lu gap=%lu min_gap=%lu can_age=%lu sleep=%u "
                      "diag=%u oom=%lu esr=%08lx temp=NA",
                      _FW_VERSION, (unsigned long)now, (unsigned long)loops, (unsigned long)reset_flags,
                      (unsigned long)board_uart_timeouts, (unsigned long)gap, (unsigned long)minimum_gap,
                      (unsigned long)(now - runtime_state.last_received_can_msg_time),
                      runtime_state.low_consume_is_active, elmlink_is_enabled(),
                      (unsigned long)freeze_alloc_failures, (unsigned long)can_gethandle()->Instance->ESR);
        if (length >= 0 && length < (int)sizeof(record))
            record[length] = ' ';
        record[254] = '\r';
        record[255] = '\n';
        UINT written = 0;
        if (f_lseek(&file, slot * sizeof(record)) == FR_OK &&
            f_write(&file, record, sizeof(record), &written) == FR_OK && written == sizeof(record) &&
            f_sync(&file) == FR_OK) {
            slot = (slot + 1) % 24;
            saved_timeouts = board_uart_timeouts;
        }
        f_close(&file);
    }
    opened = true;
    f_unmount("");
    freeze_disk_busy = 0;
}
#endif
