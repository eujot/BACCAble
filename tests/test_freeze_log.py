"""Run the production logger against bounded storage and clock substitutes."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as name:
    base = Path(name)
    common = r'''
#ifndef TEST_LOG_COMMON
#define TEST_LOG_COMMON
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#define _FW_VERSION "test"
#define FR_OK 0
#define FA_OPEN_ALWAYS 1
#define FA_WRITE 2
#define USBD_STATE_CONFIGURED 3
#define snprintf_ snprintf
#define f_size(file) (file_size)
typedef unsigned UINT;
typedef int FATFS;
typedef struct { unsigned position; } FIL;
typedef struct { int dev_state; } USBD_HandleTypeDef;
typedef struct { uint32_t CSR; } RCC_Type;
typedef struct { uint32_t ESR; } CAN_Regs;
typedef struct { CAN_Regs *Instance; } CAN_Handle;
extern RCC_Type fake_rcc;
#define RCC (&fake_rcc)
extern struct Runtime { uint32_t last_received_can_msg_time; unsigned low_consume_is_active; } runtime_state;
extern unsigned file_size;
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) {}
static inline void __set_PRIMASK(uint32_t value) { (void)value; }
uint32_t HAL_GetTick(void);
uint32_t __get_MSP(void);
CAN_Handle *can_gethandle(void);
bool usb_modes_active(void);
bool flash_storage_available(void);
unsigned elmlink_is_enabled(void);
int f_mount(FATFS *, const char *, int);
int f_open(FIL *, const char *, int);
int f_lseek(FIL *, unsigned);
int f_write(FIL *, const void *, unsigned, UINT *);
int f_sync(FIL *);
int f_close(FIL *);
int f_unmount(const char *);
#endif
'''
    (base / 'common.h').write_text(common)
    for path in ['app/build_config.h', 'app/main.h', 'third_party/fatfs/ff.h', 'usb_device.h',
                 'transport/diagnostic_link.h', 'features/usb_modes.h', 'storage/flash_records.h',
                 'third_party/printf/printf.h']:
        p = base / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#include "common.h"\n')
    (base / 'test.c').write_text(r'''
#include "common.h"
#include "features/freeze_log.h"
FATFS diagnostic_filesystem;
USBD_HandleTypeDef hUsbDeviceFS;
RCC_Type fake_rcc = {123};
struct Runtime runtime_state;
uint32_t board_uart_timeouts;
unsigned file_size, writes, mounts, now, gap = 2048;
static int mount_error;
static char disk[24 * 256];
uint32_t HAL_GetTick(void) { return now; }
uint32_t __get_MSP(void) { return 4096 + gap; }
uint32_t freeze_heap_boundary(void) { return 4096; }
CAN_Handle *can_gethandle(void) { static CAN_Regs r; static CAN_Handle h = {&r}; return &h; }
bool usb_modes_active(void) { return false; }
bool flash_storage_available(void) { return true; }
unsigned elmlink_is_enabled(void) { return 0; }
int f_mount(FATFS *fs, const char *p, int opt) { ++mounts; assert(freeze_disk_busy); return mount_error; }
int f_open(FIL *f, const char *p, int flags) { assert(!strcmp(p, "FREEZE.LOG")); return 0; }
int f_lseek(FIL *f, unsigned n) { assert(n < sizeof(disk)); f->position = n; return 0; }
int f_write(FIL *f, const void *p, unsigned n, UINT *done) {
    assert(freeze_disk_busy && n == 256 && f->position + n <= sizeof(disk));
    memcpy(disk + f->position, p, n); *done = n; ++writes;
    if (file_size < f->position + n) file_size = f->position + n;
    return 0;
}
int f_sync(FIL *f) { return 0; }
int f_close(FIL *f) { return 0; }
int f_unmount(const char *p) { return 0; }
int main(void) {
    freeze_log_boot();
    now = 9999; freeze_log_process(); assert(writes == 0);
    now = 10000; freeze_log_process(); assert(writes == 1 && !freeze_disk_busy);
    assert(strstr(disk, "reset=0000007b") && strstr(disk, "temp=NA"));
    ++board_uart_timeouts;
    now += 59999; freeze_log_process(); assert(writes == 1);
    ++now; freeze_log_process(); assert(writes == 2);
    hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
    now += 300000; freeze_log_process(); assert(writes == 2);
    hUsbDeviceFS.dev_state = 0;
    mount_error = 1; unsigned before = mounts;
    freeze_log_process(); freeze_log_process();
    assert(mounts == before + 1 && !freeze_disk_busy);
    mount_error = 0;
    for (unsigned i = 0; i < 30; ++i) { now += 300000; freeze_log_process(); }
    assert(file_size == sizeof(disk) && writes == 32);
    gap = 100; now += 300000; freeze_log_process(); assert(writes == 32);
    puts("PASS: log bounds, cadence, UART events, disk ownership, failed mount retry and low-memory guard");
}
''')
    subprocess.run(['clang', '-std=c11', '-fsanitize=address,undefined', '-DFREEZE_DIAGNOSTICS',
                    '-I' + str(base), '-I' + str(ROOT / 'firmware/baccable'),
                    str(ROOT / 'firmware/baccable/features/freeze_log.c'), str(base / 'test.c'),
                    '-o', str(base / 'test')], check=True)
    subprocess.run([str(base / 'test')], check=True)
