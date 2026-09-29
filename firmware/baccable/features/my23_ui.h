#ifndef BACCABLE_MY23_UI_H
#define BACCABLE_MY23_UI_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define MY23_L1_VISIBLE 14U
#define MY23_L2_VISIBLE 22U
#define MY23_PACKET_MARKER 2U
#define MY23_PACKET_SIZE (1U + MY23_L1_VISIBLE + MY23_L2_VISIBLE)
typedef enum { IPC_LEGACY, IPC_MY23 } ipc_type_t;
typedef enum { UI_MODE_LIST, UI_MODE_EDIT, UI_MODE_CONFIRM, UI_MODE_PARAMETER, UI_MODE_NOTICE } UiMode;
/* Compact UART tokens, translated into big-endian UCS-2 only by BH. */
#define MY23_ACTIVE "\x80"
#define MY23_BULLET "\x81"
#define MY23_UP "\x82"
#define MY23_DOWN "\x83"
#define MY23_SAVED "\x84"
#define MY23_FAILED "\x85"
#define MY23_ARROW "\x86"
#define MY23_ELLIPSIS "\x87"
uint16_t my23_codepoint(uint8_t token);
size_t my23_text_encode(uint8_t *out, size_t limit, const char *utf8);
void my23_packet_mode(uint8_t out[MY23_PACKET_SIZE], UiMode mode, const char *first, const char *second);
void my23_packet(uint8_t out[MY23_PACKET_SIZE], const char *first, const char *second);
#endif
