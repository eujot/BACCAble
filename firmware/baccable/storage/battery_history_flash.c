#include "app/build_config.h"
#if defined(BACCABLE_C1)
#include "storage/battery_history_store.h"
#include "storage/flash_records.h"
#include "stm32f0xx_hal.h"
static bool erase(unsigned page) {
    if (page >= HISTORY_PAGES || __get_IPSR() || HAL_FLASH_Unlock() != HAL_OK) return false;
    FLASH_EraseInitTypeDef op = {.TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = HISTORY_FLASH_START + page * HISTORY_PAGE_SIZE, .NbPages = 1};
    uint32_t error;
    bool ok = HAL_FLASHEx_Erase(&op, &error) == HAL_OK;
    HAL_FLASH_Lock(); return ok;
}
static bool program(size_t offset, uint16_t value) {
    if (offset >= HISTORY_PAGES * HISTORY_PAGE_SIZE || (offset & 1) || __get_IPSR() ||
        HAL_FLASH_Unlock() != HAL_OK) return false;
    bool ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, HISTORY_FLASH_START + offset, value) == HAL_OK;
    HAL_FLASH_Lock(); return ok;
}
HistoryStorage history_flash_storage(void) {
    return (HistoryStorage){flash_storage_available() ? (const uint8_t *)HISTORY_FLASH_START : NULL, erase, program};
}
#endif
