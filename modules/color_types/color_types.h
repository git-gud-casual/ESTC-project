#ifndef _COLOR_TYPES
#define _COLOR_TYPES

#include <stdint.h>
#include <stddef.h>

#define COLOR_NAME_SIZE 32
#define COLORS_COUNT 10
#define RGB_DATA_WITH_NAME_SIGNATURE 0b0101101

// RGB_DATA_WITH_NAME_SIZE should be multiple of 4
#define RGB_DATA_WITH_NAME_SIZE (COLOR_NAME_SIZE + 4)

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

typedef union {
    uint32_t _value[RGB_DATA_WITH_NAME_SIZE / 4];
    
    struct {
        uint8_t _signature : 7;
        uint8_t _is_correct : 1;
        char color_name[COLOR_NAME_SIZE];
        rgb_data_t rgb_data;
    };

} rgb_data_with_name_t;

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b);
hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v);
rgb_data_t get_rgb_from_hsv(const hsv_data_t* hsv_data);
rgb_data_with_name_t* get_colors_array();
rgb_data_with_name_t new_rgb_with_name(uint8_t r, uint8_t g, uint8_t b, char* name, size_t name_length);
void delete_color_from_color_array(size_t color_index);
void init_last_saved_colors_array();
void save_colors_array();
void put_rgb_with_name_in_color_array();
void init_nmvc();


#endif