/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usb_device.c
 * @version        : v2.0_Cube
 * @brief          : This file implements the USB Device
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Runtime USB lifecycle shared by disk and serial builds. */
#include "usb_device.h"
#include "app/build_config.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_ioreq.h"
#if !defined(ACT_AS_CANABLE)
#include "features/usb_modes.h"
#endif
#include "stm32f0xx_hal.h"
#include <string.h>

USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t initialized, serial_mode, stage, last_error, attempts;
static uint32_t deadline, reset_flags;
static volatile uint8_t pending_fault;
void usb_device_fault(uint8_t error) { pending_fault = error; }

uint8_t usb_device_is_serial(void) { return serial_mode; }
void usb_device_record_reset(uint32_t flags) { reset_flags = flags; }

/* Quiesce callbacks before freeing class memory. Only the USB IRQ is masked; UART and
 * SysTick must continue. USBD_DeInit already closes the class and stops USB. */
void usb_device_stop(void) {
    HAL_NVIC_DisableIRQ(USB_IRQn);
    if (stage == USB_STAGE_RESET)
        __HAL_RCC_USB_RELEASE_RESET();
    if (initialized) {
        USBD_DeInit(&hUsbDeviceFS);
        initialized = 0;
    }
    HAL_NVIC_ClearPendingIRQ(USB_IRQn);
    memset(&hUsbDeviceFS, 0, sizeof(hUsbDeviceFS));
    pending_fault = 0;
    stage = USB_STAGE_OFF;
}

/* Requests are idempotent while the same transition is pending/running. */
void usb_device_start(uint8_t serial) {
    if (stage != USB_STAGE_OFF && stage != USB_STAGE_FAILED && serial_mode == !!serial)
        return;
    usb_device_stop();
    serial_mode = !!serial;
    attempts = 0;
    stage = USB_STAGE_DETACH;
    deadline = HAL_GetTick() + 100;
}

static void failed(uint8_t error) {
    last_error = error;
    usb_device_stop();
    stage = attempts < 3 ? USB_STAGE_DETACH : USB_STAGE_FAILED;
    deadline = HAL_GetTick() + 250;
}

/* No delay loops and no fatal error handler on a recoverable USB failure. */
void usb_device_process(void) {
    if (pending_fault) {
        failed(pending_fault);
        return;
    }
    if (stage != USB_STAGE_DETACH && stage != USB_STAGE_RESET)
        return;
    if ((int32_t)(HAL_GetTick() - deadline) < 0)
        return;
    if (stage == USB_STAGE_DETACH) {
        __HAL_RCC_USB_FORCE_RESET();
        stage = USB_STAGE_RESET;
        deadline = HAL_GetTick() + 2;
        return;
    }
    __HAL_RCC_USB_RELEASE_RESET();
    ++attempts;
    /* MSP initialization must leave the IRQ masked until class/EP0 state exists. */
    initialized = 1;
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
        failed(1);
        return;
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, serial_mode ? &USBD_CDC : &USBD_MSC) != USBD_OK) {
        failed(2);
        return;
    }
    uint8_t result = serial_mode ? USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS)
                                : USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS);
    if (result != USBD_OK) {
        failed(3);
        return;
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        failed(4);
        return;
    }
    stage = USB_STAGE_READY;
    HAL_NVIC_ClearPendingIRQ(USB_IRQn);
    HAL_NVIC_EnableIRQ(USB_IRQn);
}

void usb_device_status(uint8_t out[8]) {
    out[0] = stage;
    out[1] = last_error;
    out[2] = hUsbDeviceFS.dev_state;
    out[3] = attempts;
    for (unsigned i = 0; i < 4; ++i)
        out[4 + i] = reset_flags >> (8 * i);
}

void MX_USB_DEVICE_Init(void) {
#ifdef ENABLE_USB_MASS_STORAGE
    usb_device_start(0);
#else
    usb_device_start(1);
#endif
}

/* Device-recipient vendor IN request 0x5a: fixed 64-byte versioned snapshot.
 * No interface claim, CDC text injection, or capture stream modification. */
USBD_StatusTypeDef USBD_VendorRequest(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    static uint8_t reply[64];
    if (req->bmRequest != 0xc0 || req->bRequest != 0x5a || req->wValue || req->wIndex || !req->wLength)
        return USBD_FAIL;
#if !defined(ACT_AS_CANABLE)
    usb_modes_debug_read(reply);
#else
    memset(reply, 0, sizeof(reply));
    reply[0] = 1;
    reply[1] = 3;
    reply[2] = 1;
    usb_device_status(reply + 19);
#endif
    return USBD_CtlSendData(pdev, reply, req->wLength < sizeof(reply) ? req->wLength : sizeof(reply));
}
