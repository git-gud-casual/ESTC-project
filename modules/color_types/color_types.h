#ifndef _COLOR_TYPES
#define _COLOR_TYPES

#include <stdint.h>
#include <stddef.h>

#define COLOR_NAME_SIZE 32
#define COLORS_COUNT 10
#define RGB_DATA_ARRAY_SIZE_IN_WORDS 89

typedef union {
    uint32_t _value;
    
    struct {
        uint8_t _signature : 6;
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



typedef struct {
    rgb_data_t rgb;
    char color_name[COLOR_NAME_SIZE];
} rgb_data_with_name_t;


typedef union {
    uint32_t _value[RGB_DATA_ARRAY_SIZE_IN_WORDS];

    struct {
        uint8_t _signature;
        rgb_data_with_name_t colors_array[COLORS_COUNT];
        size_t count;
    };
} rgb_data_array_t;

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b);
hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v);
rgb_data_t get_rgb_from_hsv(const hsv_data_t* hsv_data);
rgb_data_with_name_t new_rgb_with_name(rgb_data_t rgb, char* color_name, size_t name_length);
hsv_data_t get_hsv_from_rgb(const rgb_data_t* rgb_data);

rgb_data_array_t get_last_saved_rgb_array();
void save_colors_array(rgb_data_array_t* rgb_data_array);
void put_rgb_in_array(rgb_data_array_t* rgb_array, rgb_data_with_name_t* rgb_data);
void delete_color_from_array(rgb_data_array_t* rgb_array, size_t color_index);

hsv_data_t get_last_saved_hsv_data();
void write_hsv_data(hsv_data_t* hsv_ptr);

void init_nmvc();

#endif