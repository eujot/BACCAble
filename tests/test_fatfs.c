#include "test_report.h"
#include "third_party/fatfs/ff.h"
#include "third_party/fatfs/diskio.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static BYTE disk[128 * 512];
static LBA_t sectors;
DSTATUS disk_status(BYTE drive) { return drive ? STA_NOINIT : 0; }
DSTATUS disk_initialize(BYTE drive) { return disk_status(drive); }
DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count) {
    assert(!drive && sector < sectors && count <= sectors - sector);
    memcpy(buffer, disk + sector * 512, count * 512); return RES_OK;
}
DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count) {
    assert(!drive && sector < sectors && count <= sectors - sector);
    memcpy(disk + sector * 512, buffer, count * 512); return RES_OK;
}
DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer) {
    assert(!drive);
    if (command == CTRL_SYNC) return RES_OK;
    if (command == GET_SECTOR_COUNT) *(LBA_t *)buffer = sectors;
    else if (command == GET_BLOCK_SIZE) *(DWORD *)buffer = 4;
    else return RES_PARERR;
    return RES_OK;
}
static void test_small_fat12(void) {
    for (unsigned size = 32; size <= 128; size += 8) {
        sectors = size; memset(disk, 0xff, sizeof(disk));
        FATFS fs;
        BYTE work[512];
        MKFS_PARM options = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .n_root = 32, .au_size = 512};
        assert(f_mount(&fs, "", 1) == FR_NO_FILESYSTEM);
        assert(f_mkfs("", &options, work, sizeof(work)) == FR_OK);
        assert(f_mount(&fs, "", 1) == FR_OK);
        assert(f_setlabel("BACCABLE C1") == FR_OK);
        FIL file; UINT count;
        assert(f_open(&file, "Version.txt", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK);
        assert(f_write(&file, "audit-test", 10, &count) == FR_OK && count == 10);
        assert(f_close(&file) == FR_OK);
        assert(f_mount(NULL, "", 0) == FR_OK);
        assert(f_mount(&fs, "", 1) == FR_OK);
        assert(f_open(&file, "Version.txt", FA_READ) == FR_OK);
        char result[10];
        assert(f_read(&file, result, sizeof(result), &count) == FR_OK && count == 10);
        assert(!memcmp(result, "audit-test", 10));
        assert(f_close(&file) == FR_OK);
        assert(f_mount(NULL, "", 0) == FR_OK);
    }
    puts("PASS: small FAT12 format, label, write, remount and read");
}

int main(void) {
    const HostTest tests[] = {HOST_TEST(test_small_fat12)};
    host_tests_run("fatfs", tests, sizeof(tests) / sizeof(tests[0]));
}
