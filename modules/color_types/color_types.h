#ifndef _COLOR_TYPES
#define _COLOR_TYPES

#include <stdint.h>

typedef union {
    uint32_t _value;
    
    struct {
        uint8_t _is_writeable : 1;
        uint8_t _signature : 5;
        uint16_t h : 10;
        uint8_t s;
        uint8_t v;
    };
} hsv_data_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_data_t;

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b);
hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v);
rgb_data_t get_rgb_from_hsv(const hsv_data_t* hsv_data);
hsv_data_t get_last_saved_or_default_hsv_data();
void save_hsv_data(hsv_data_t* hsv);


#endif