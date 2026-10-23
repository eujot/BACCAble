#ifndef BACCABLE_FEATURES_DNA_SELECTOR_H
#define BACCABLE_FEATURES_DNA_SELECTOR_H
#include "app/application_state.h"

/* Arm one startup selection, preserve it on resync, or cancel it on OFF/save. */
void dna_selector_configure(uint8_t mode, uint8_t action, uint8_t token);
void dna_selector_configure_default(uint8_t mode, uint8_t action, uint8_t token, uint8_t softer_suspension);
/* Observe the physical BH selector without altering its factory report. */
void dna_selector_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
/* Observe the suspension state reported with the current BH drive mode. */
void dna_selector_suspension_status(const uint8_t *data);
/* Record the actual BH driving mode before any display customization. */
void dna_selector_mode(uint8_t mode);
/* Advance a bounded press/release gesture and report its outcome to C1. */
void dna_selector_process(void);
#endif
