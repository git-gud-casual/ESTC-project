#include "color_types.h"
#include <math.h>

hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v) {
    return (hsv_data_t) {.h = h, .s = s, .v = v};
}

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (rgb_data_t) {.r = r, .g = g, .b = b};
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
    return new_rgb((r_component + m) * 255, (g_component + m) * 255, (b_component + m) * 255);
}
