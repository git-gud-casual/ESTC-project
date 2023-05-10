#include "color_types.h"
#include <math.h>
#include <inttypes.h>

#include "nrf_dfu_types.h"

#include "nrf_log.h"
#include <string.h>


#define GET_MAX(a, b) (((a) > (b))? (a) : (b))
#define GET_MIN(a, b) (((a) > (b))? (b) : (a))


hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v) {
    return (hsv_data_t) {.h = h, .s = s, .v = v};
}

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (rgb_data_t) {.r = r, .g = g, .b = b};
}

rgb_data_with_name_t new_rgb_with_name(rgb_data_t rgb, char* color_name, size_t name_length) {
    ASSERT(name_length <= COLOR_NAME_SIZE);

    rgb_data_with_name_t rgb_with_name = {.rgb = rgb};
    strncpy(rgb_with_name.color_name, color_name, name_length);
    return rgb_with_name;
}

rgb_data_t get_rgb_from_hsv(const hsv_data_t* hsv_data) {
    float c = (float)(hsv_data->v * hsv_data->s) / 10000;
    float x = c * (1 - fabsf(fmodf((float)hsv_data->h / 60, 2) - 1));
    float m = (float)hsv_data->v / 100 - c; 
    float r_component, g_component, b_component;

    if (hsv_data->h < 60) {
        r_component = c;
        g_component = x;
        b_component = 0;
    }
    else if (hsv_data->h < 120) {
        r_component = x;
        g_component = c;
        b_component = 0;
    }
    else if (hsv_data->h < 180) {
        r_component = 0;
        g_component = c;
        b_component = x;
    }
    else if (hsv_data->h < 240) {
        r_component = 0;
        g_component = x;
        b_component = c;
    }
    else if (hsv_data->h < 300) {
        r_component = x;
        g_component = 0;
        b_component = c;
    }
    else {
        r_component = c;
        g_component = 0;
        b_component = x;
    }
    return new_rgb((uint8_t)((r_component + m) * 255) % 256, (uint8_t)((g_component + m) * 255) % 256, (uint8_t)((b_component + m) * 255) % 256);
}

hsv_data_t get_hsv_from_rgb(const rgb_data_t* rgb_data) {
    float r = (float) rgb_data->r / 255;
    float g = (float) rgb_data->g / 255;
    float b = (float) rgb_data->b / 255;

    float max, min, delta;
    max = GET_MAX(GET_MAX(r, g), b);
    min = GET_MIN(GET_MIN(r, g), b);
    delta = max - min;

    float h = 0;
    if (delta == 0) {
        h = 0;
    } else {
        if (r == max) {
            h = 60 * fmod(fabs((g - b) / delta), 6.);
        } else if (g == max) {
            h = 60 * (fabs((b - r) / delta) + 2);
        }

        if (b == max) {
            h = 60 * (fabs((r - g) / delta) + 4);
        }
    }

    float s = max == 0 ? 0 : delta / max * 100;
    float v = max * 100;

    return new_hsv((uint16_t) ceilf(h) % 360, (uint8_t) ceilf(s) % 101, (uint8_t) ceilf(v) % 101);

}


void put_rgb_in_array(rgb_data_array_t* rgb_array, rgb_data_with_name_t* rgb_data) {
    for (size_t i = 0; i < COLORS_COUNT; i++) {
        if (strncmp(rgb_array->colors_array[i].color_name, rgb_data->color_name, COLOR_NAME_SIZE) == 0) {
            rgb_array->colors_array[i].rgb = rgb_data->rgb;
            NRF_LOG_INFO("Changed color %s at index %d", rgb_data->color_name, i);
            return;
        }
    }

    if (rgb_array->count >= COLORS_COUNT) {
        for (size_t i = 1; i < COLORS_COUNT; i++) {
            rgb_array->colors_array[i - 1] = rgb_array->colors_array[i];
        }
        rgb_array->count--;
    }
    rgb_array->colors_array[rgb_array->count] = *rgb_data;
    rgb_array->count++;

    NRF_LOG_INFO("Added rgb_data at rgb_array. Colors count %" PRIu32, rgb_array->count);
}

void delete_color_from_array(rgb_data_array_t* rgb_array, size_t color_index) {
    bool is_deleted = false;
    for (size_t i = 0; i < rgb_array->count; i++) {
        if (i == color_index) {
            is_deleted = true;
        }
        else if (is_deleted) {
            rgb_array->colors_array[i - 1] = rgb_array->colors_array[i];
        }
    }
    
    if (is_deleted) {
        rgb_array->count--;
    }

    NRF_LOG_INFO("Deleted color");
}