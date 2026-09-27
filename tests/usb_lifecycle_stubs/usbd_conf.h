#ifndef TEST_REAL_USBD_CONF_H
#define TEST_REAL_USBD_CONF_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#undef __weak
#define __weak __attribute__((weak))
#define UNUSED(x) ((void)(x))
#define __IO volatile
typedef struct { struct { uint32_t maxpacket; } IN_ep[8]; } PCD_HandleTypeDef;
#define DEVICE_FS 0
#define USBD_MAX_NUM_INTERFACES 1
#define USBD_MAX_NUM_CONFIGURATION 1
#define USBD_MAX_STR_DESC_SIZ 512
#define USBD_DEBUG_LEVEL 0
#define USBD_LPM_ENABLED 0
#define USBD_SELF_POWERED 1
#define USBD_SUPPORT_USER_STRING_DESC 0
#define USBD_malloc USBD_static_malloc
#define USBD_free USBD_static_free
#define USBD_memset memset
#define USBD_memcpy memcpy
#define USBD_UsrLog(...)
#define USBD_ErrLog(...)
#define USBD_DbgLog(...)
void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);
#endif
