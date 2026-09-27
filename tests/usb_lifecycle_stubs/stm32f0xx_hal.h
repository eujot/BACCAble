#ifndef TEST_LIFECYCLE_HAL_H
#define TEST_LIFECYCLE_HAL_H
#include <stdint.h>
typedef struct { uint32_t unused; } CAN_RxHeaderTypeDef;
#define USB_IRQn 31
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
void HAL_NVIC_DisableIRQ(int irq);
void HAL_NVIC_EnableIRQ(int irq);
void HAL_NVIC_ClearPendingIRQ(int irq);
void test_usb_reset(int asserted);
#define __HAL_RCC_USB_FORCE_RESET() test_usb_reset(1)
#define __HAL_RCC_USB_RELEASE_RESET() test_usb_reset(0)
#endif
