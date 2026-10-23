#ifndef BACCABLE_EXPERIMENTS_H
#define BACCABLE_EXPERIMENTS_H
#include "app/build_config.h"
#include <stddef.h>
#include <stdint.h>
#include "stm32f0xx_hal.h"

#define EXPERIMENT_GROUP_COUNT 9U
#define EXPERIMENT_SIGNAL_COUNT 64U

const char *experiment_group_name(uint8_t group);
uint8_t experiment_group_count(uint8_t group);
uint8_t experiment_group_signal(uint8_t group, uint8_t position);
const char *experiment_signal_name(uint8_t signal);
void experiment_select(uint8_t signal);
void experiment_cancel(void);
void experiment_process(void);
void experiment_receive(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
void experiment_format(uint8_t signal, char *output, size_t size);
#endif
