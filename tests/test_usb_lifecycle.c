/* Exercise the actual ST USB core, MSC/BOT and CDC classes, stubbing only hardware. */
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_msc.h"
#include "usbd_msc_bot.h"
#include "usbd_cdc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
extern USBD_HandleTypeDef hUsbDeviceFS;
USBD_DescriptorsTypeDef FS_Desc;
void usb_modes_debug_read(uint8_t out[64]) { memset(out, 0, 64); out[0] = 1; }
static uint32_t now;
static unsigned allocated, freed, starts, fail_init, fail_start;
static int irq_enabled, reset_asserted;
static uint8_t rx[64];
static uint16_t control_length;
static uint8_t control_data[64];
static unsigned stalls;
static union { USBD_MSC_BOT_HandleTypeDef msc; USBD_CDC_HandleTypeDef cdc; } memory;
void *USBD_static_malloc(uint32_t n) { assert(n <= sizeof(memory)); ++allocated; return &memory; }
void USBD_static_free(void *p) { assert(p == &memory); ++freed; }
uint32_t HAL_GetTick(void) { return now; }
void HAL_Delay(uint32_t ms) { now += ms; }
void HAL_NVIC_DisableIRQ(int irq) { (void)irq; irq_enabled = 0; }
void HAL_NVIC_EnableIRQ(int irq) { (void)irq; irq_enabled = 1; }
void HAL_NVIC_ClearPendingIRQ(int irq) { (void)irq; }
void test_usb_reset(int v) { assert(!irq_enabled); reset_asserted = v; }
static int8_t storage_init(uint8_t lun) { (void)lun; return 0; }
static int8_t capacity(uint8_t lun, uint32_t *blocks, uint16_t *size) { (void)lun; *blocks=32; *size=512; return 0; }
static int8_t storage_io(uint8_t lun, uint8_t *buf, uint32_t block, uint16_t n) { (void)lun; (void)buf; (void)block; (void)n; return 0; }
static int8_t max_lun(void) { return 0; }
static int8_t inquiry[36];
USBD_StorageTypeDef USBD_Storage_Interface_fops_FS = {storage_init, capacity, storage_init, storage_init, storage_io, storage_io, max_lun, inquiry};
static int8_t cdc_init(void) { USBD_CDC_SetTxBuffer(&hUsbDeviceFS, rx, 0); USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx); return 0; }
static int8_t cdc_deinit(void) { return 0; }
static int8_t cdc_control(uint8_t cmd, uint8_t *buf, uint16_t n) { (void)cmd; (void)buf; (void)n; return 0; }
static int8_t cdc_receive(uint8_t *buf, uint32_t *n) { (void)buf; (void)n; return 0; }
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {cdc_init, cdc_deinit, cdc_control, cdc_receive};
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *p) { (void)p; assert(!irq_enabled); return fail_init ? USBD_FAIL : USBD_OK; }
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *p) { (void)p; return USBD_OK; }
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *p) { (void)p; ++starts; return fail_start ? USBD_FAIL : USBD_OK; }
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *p) { (void)p; return USBD_OK; }
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *p, uint8_t a, uint8_t t, uint16_t n) { (void)p; (void)a; (void)t; (void)n; return USBD_OK; }
#define EP_FN(name) USBD_StatusTypeDef name(USBD_HandleTypeDef *p,uint8_t a) { (void)p; (void)a; return USBD_OK; }
EP_FN(USBD_LL_CloseEP) EP_FN(USBD_LL_FlushEP) EP_FN(USBD_LL_ClearStallEP) EP_FN(USBD_LL_SetUSBAddress)
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *p, uint8_t a) { (void)p; (void)a; ++stalls; return USBD_OK; }
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *p,uint8_t a) { (void)p; (void)a; return 0; }
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *p,uint8_t a,uint8_t *b,uint16_t n) { (void)p; if (a == 0) { control_length = n; assert(n <= 64); if (n) memcpy(control_data,b,n); } return USBD_OK; }
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *p,uint8_t a,uint8_t *b,uint16_t n) { return USBD_LL_Transmit(p,a,b,n); }
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *p,uint8_t a) { (void)p; (void)a; return 0; }
void USBD_LL_Delay(uint32_t ms) { now += ms; }
static void advance(uint32_t ms) { now += ms; usb_device_process(); }
static void boot(uint8_t serial) { usb_device_start(serial); advance(100); assert(reset_asserted); advance(2); assert(!reset_asserted && irq_enabled); }
static void configure(void) { USBD_LL_SetSpeed(&hUsbDeviceFS,USBD_SPEED_FULL); USBD_LL_Reset(&hUsbDeviceFS); assert(USBD_SetClassConfig(&hUsbDeviceFS,1) == USBD_OK); hUsbDeviceFS.dev_state=USBD_STATE_CONFIGURED; }
int main(int argc, char **argv) {
    (void)argv;
    boot(0); configure();
    if (argc > 1) {
        /* Reproduce the exact beta-13/16 shutdown sequence against the original
         * vendor sources. ASan/UBSan must reject its second MSC deinitialization. */
        USBD_Stop(&hUsbDeviceFS);
        USBD_DeInit(&hUsbDeviceFS);
        return 0;
    }
    unsigned before=freed;
    usb_device_start(1);
    assert(freed == before+1 && !irq_enabled);
    advance(99); assert(!irq_enabled);
    advance(1); advance(2); configure();
    assert(usb_device_is_serial() && allocated == freed+1);
    uint8_t setup[8] = {0xc0, 0x5a, 0, 0, 0, 0, 64, 0};
    USBD_LL_SetupStage(&hUsbDeviceFS, setup);
    assert(control_length == 64 && control_data[0] == 1);
    setup[6] = 8; USBD_LL_SetupStage(&hUsbDeviceFS, setup);
    assert(control_length == 8);
    unsigned old_stalls = stalls;
    setup[0] = 0x40; USBD_LL_SetupStage(&hUsbDeviceFS, setup);
    assert(stalls > old_stalls); /* No host writes through the status endpoint. */
    for(unsigned i=0;i<30;++i) {
        boot(0); configure();
        USBD_LL_Reset(&hUsbDeviceFS); /* reset after enumeration, before switch */
        boot(1); configure();
    }
    usb_device_stop(); usb_device_stop(); assert(allocated == freed);
    boot(0); /* never configured MSC is safe to stop */
    USBD_MSC.DeInit(&hUsbDeviceFS, 0);
    USBD_MSC.DeInit(&hUsbDeviceFS, 0); /* Host deconfiguration before allocation. */
    usb_device_stop(); assert(allocated == freed);
    fail_init=1; usb_device_start(1);
    advance(100); advance(2);
    advance(250); advance(2);
    advance(250); advance(2);
    uint8_t status[8]; usb_device_status(status);
    assert(status[0] == USB_STAGE_FAILED && status[1] == 1 && status[3] == 3);
    unsigned previous=starts; advance(10000); assert(starts == previous);
    fail_init=0; fail_start=1; usb_device_start(1); advance(100); advance(2);
    usb_device_status(status); assert(status[1] == 4 && !irq_enabled);
    fail_start=0; advance(250); advance(2); configure();
    usb_device_record_reset(0xaabbccdd); usb_device_status(status);
    assert(status[0] == USB_STAGE_READY && status[1] == 4 && status[3] == 2 && status[4] == 0xdd);
    usb_device_stop(); assert(allocated == freed);
    now=UINT32_MAX-50; boot(1); usb_device_stop();
    usb_device_start(0); advance(100); assert(reset_asserted);
    usb_device_stop(); assert(!reset_asserted); /* OFF cancels an in-progress reset. */
    puts("PASS: real MSC/CDC lifecycle, reset, unconfigured stop, bounded failures/retry and tick wrap");
}
