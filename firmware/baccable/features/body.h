#ifndef BACCABLE_FEATURES_BODY_H
#define BACCABLE_FEATURES_BODY_H

#include "app/application_state.h"

/* Keep UART pace indices stable: 0=50 ms, 1=20 ms, 2=10 ms. */
#define IPC_DEFAULT_PACE 1U
#define IPC_DEFAULT_METHOD 0U

void body_display_options(uint8_t pace, uint8_t method);
void body_display_submit(const uint8_t *text);
void body_display_refresh(void);
void body_display_factory_frame(const uint8_t data[8], uint8_t dlc);
void body_init();
void body_process();
uint8_t mirror_positions_save();
uint16_t mirror_positions_read(uint8_t paramId);

#endif /* BACCABLE_FEATURES_BODY_H */
