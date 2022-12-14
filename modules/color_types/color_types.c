#include "color_types.h"
#include <math.h>
#include <inttypes.h>

#include "nrfx_nvmc.h"
#include "nrf_dfu_types.h"

#include "nrf_log.h"
#include <string.h>

#define BOOTLOADER_ADDR 0xE0000
#define APP_DATA_ADDR (BOOTLOADER_ADDR - NRF_DFU_APP_DATA_AREA_SIZE)
#define GET_ADDRESS_BY_OFFSET(offset) (APP_DATA_ADDR + offset)
#define WORD_SIZE 4
#define COLORS_ARRAY_SIZE (COLORS_COUNT * RGB_DATA_WITH_NAME_SIZE)
#define DEVICE_ID_LAST_DIGITS 77
#define SIGNATURE 0b00101

static bool offset_defined = false;
static uint32_t last_colors_array_offset;

static rgb_data_with_name_t colors_array[COLORS_COUNT] = {.0};

rgb_data_with_name_t* get_colors_array() {
    return &colors_array[0];
}

hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v) {
    return (hsv_data_t) {.h = h, .s = s, .v = v, ._signature = SIGNATURE};
}

rgb_data_t new_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (rgb_data_t) {.r = r, .g = g, .b = b};
}

rgb_data_with_name_t new_rgb_with_name(uint8_t r, uint8_t g, uint8_t b, char* name, size_t name_length) {
    rgb_data_with_name_t rgb_with_name = {.r = r, .g = g, .b = b, ._signature = RGB_DATA_WITH_NAME_SIGNATURE};
    strncpy(rgb_with_name.color_name, name, name_length);
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
    return new_rgb((r_component + m) * 255, (g_component + m) * 255, (b_component + m) * 255);
}


static rgb_data_with_name_t get_rgb_data_with_name_by_address(uint32_t address) {
    rgb_data_with_name_t rgb_data;
    memcpy(&rgb_data, (uint32_t*) address, RGB_DATA_WITH_NAME_SIZE);
    return rgb_data;
}

static void set_colors_array(uint32_t address) {
    rgb_data_with_name_t* rgb_array = (rgb_data_with_name_t*) address;
    memcpy(colors_array, rgb_array, COLORS_ARRAY_SIZE);
}


void put_rgb_with_name_in_color_array(rgb_data_with_name_t* rgb_data) {
    for (size_t i = 0; i < COLORS_COUNT; i++) {
        if (colors_array[i]._signature != RGB_DATA_WITH_NAME_SIGNATURE) {
            colors_array[i] = *rgb_data;
            NRF_LOG_INFO("Added color at index %d", i);
            return;
        }
        else if (strncmp(colors_array[i].color_name, rgb_data->color_name, COLOR_NAME_SIZE) == 0) {
            colors_array[i].r = rgb_data->r;
            colors_array[i].g = rgb_data->g;
            colors_array[i].b = rgb_data->b;
            NRF_LOG_INFO("Changed color %s at index %d", colors_array[i].color_name, i);
            return;
        }
    }

    rgb_data_with_name_t new_colors_array[COLORS_COUNT] = {.0};
    for (size_t i = 1; i < COLORS_COUNT; i++) {
        new_colors_array[i - 1] = colors_array[i];
    }
    new_colors_array[COLORS_COUNT - 1] = *rgb_data;

    memcpy(colors_array, new_colors_array, COLORS_ARRAY_SIZE);
    NRF_LOG_INFO("Added value at end")
}

static void write_color_array() {
    if (offset_defined) {
        for (size_t i = 0; i < COLORS_COUNT; i++) {
            for (size_t j = 0; j < RGB_DATA_WITH_NAME_SIZE / 4; j++) {
                if (last_colors_array_offset % CODE_PAGE_SIZE == 0) {
                    nrfx_nvmc_page_erase(GET_ADDRESS_BY_OFFSET(last_colors_array_offset));
                }
                nrfx_nvmc_word_write(GET_ADDRESS_BY_OFFSET(last_colors_array_offset), colors_array[i]._value[j]);
                while (!nrfx_nvmc_write_done_check());
                last_colors_array_offset += 4;
            }
        }
        last_colors_array_offset -= COLORS_ARRAY_SIZE - 4;
        NRF_LOG_INFO("Data has been written");
    }
}

void init_last_saved_colors_array() {
    rgb_data_with_name_t rgb_data;
    for (uint32_t address = APP_DATA_ADDR; address <= BOOTLOADER_ADDR - COLORS_ARRAY_SIZE; address += COLORS_ARRAY_SIZE) {
        rgb_data = get_rgb_data_with_name_by_address(address);
        NRF_LOG_INFO("rgb_data_with_name_t address is 0x%" PRIx32, address);
        NRF_LOG_INFO("rgb_data_with_name_t signature is 0x%" PRIx8, rgb_data._signature);
        if (rgb_data._signature != RGB_DATA_WITH_NAME_SIGNATURE) {
            NRF_LOG_INFO("Found invalid data at address 0x%" PRIx32, address);
            if (address == APP_DATA_ADDR) {
                return;
            }

            set_colors_array(address - COLORS_ARRAY_SIZE);
            NRF_LOG_INFO("Found rgb_data_with_name_t at address 0x%" PRIx32, address - COLORS_ARRAY_SIZE);
            last_colors_array_offset = address - APP_DATA_ADDR - COLORS_ARRAY_SIZE;
            offset_defined = true;
                
            return;
        }
    }
}

void save_colors_array() {
    if (!offset_defined) {
        last_colors_array_offset = 0;
        offset_defined = true;
    }
    else {
        last_colors_array_offset = (last_colors_array_offset + COLORS_ARRAY_SIZE) % NRF_DFU_APP_DATA_AREA_SIZE;
    }

    NRF_LOG_INFO("Attempt to write colors_array at address 0x%" PRIx32, GET_ADDRESS_BY_OFFSET(last_colors_array_offset));
    write_color_array();
}


void init_nmvc() {
    init_last_saved_colors_array();
}