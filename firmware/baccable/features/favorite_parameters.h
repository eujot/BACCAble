#ifndef BACCABLE_FAVORITE_PARAMETERS_H
#define BACCABLE_FAVORITE_PARAMETERS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define FAVORITE_MAX_PARAMS 5U
#define FAVORITE_VISIBLE_PARAMS 2U
#define FAVORITE_SET_COUNT 6U
#define FAVORITE_EMPTY 255U
#define FAVORITE_STORAGE_SIZE 62U
typedef struct { uint8_t params[FAVORITE_MAX_PARAMS]; } FavoriteParameters;
const char *favorite_parameter_name(uint8_t engine, bool v6, uint8_t id);
bool favorite_parameter_supported(uint8_t engine, bool v6, uint8_t id);
void favorite_parameter_segment(uint8_t engine, uint8_t id, float value, bool tiny, char *out, size_t size);
const char *favorite_parameter_label(uint8_t engine, bool v6, uint8_t id, bool compact);
void favorite_parameters_render(uint8_t engine, const FavoriteParameters *favorite, uint32_t now,
                                char first[15], char second[23]);
#endif
