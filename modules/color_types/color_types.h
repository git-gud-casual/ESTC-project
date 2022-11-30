#ifndef _COLOR_TYPES
#define _COLOR_TYPES

#include <stdint.h>

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
} hsv_data_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_data_t;

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b);
hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v);
rgb_data_t get_rgb_from_hsv(const hsv_data_t* hsv_data);


#endif