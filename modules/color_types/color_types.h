#ifndef _COLOR_TYPES
#define _COLOR_TYPES

#include <stdint.h>
#include <stddef.h>

#define COLOR_NAME_SIZE 32
#define COLORS_COUNT 10

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



typedef struct {
    rgb_data_t rgb;
    char color_name[COLOR_NAME_SIZE];
} rgb_data_with_name_t;


typedef struct {
    rgb_data_with_name_t colors_array[COLORS_COUNT];
    size_t count;

} rgb_data_array_t;

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b);
hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v);
rgb_data_t get_rgb_from_hsv(const hsv_data_t* hsv_data);
rgb_data_with_name_t new_rgb_with_name(rgb_data_t rgb, char* color_name, size_t name_length);
hsv_data_t get_hsv_from_rgb(const rgb_data_t* rgb_data);
void put_rgb_in_array(rgb_data_array_t* rgb_array, rgb_data_with_name_t* rgb_data);
void delete_color_from_array(rgb_data_array_t* rgb_array, size_t color_index);


#endif