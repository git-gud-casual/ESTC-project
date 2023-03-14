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
#define RGB_DATA_ARRAY_SIZE (RGB_DATA_ARRAY_SIZE_IN_WORDS * WORD_SIZE)
#define DEVICE_ID_LAST_DIGITS 77
#define HSV_DATA_SIGNATURE 0b001011
#define RGB_DATA_ARRAY_SIGNATURE 0b01011011

#define GET_MAX(a, b) (((a) > (b))? (a) : (b))
#define GET_MIN(a, b) (((a) > (b))? (b) : (a))

static bool offset_defined = false;
static uint32_t mem_offset;


hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v) {
    return (hsv_data_t) {.h = h, .s = s, .v = v, ._signature = HSV_DATA_SIGNATURE};
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

static rgb_data_array_t get_rgb_array_by_address(uint32_t address) {
    rgb_data_array_t rgb_data;
    memcpy(&rgb_data, (uint32_t*) address, RGB_DATA_ARRAY_SIZE);
    return rgb_data;
}

rgb_data_array_t get_last_saved_rgb_array() {
    if (offset_defined) {
        return get_rgb_array_by_address(GET_ADDRESS_BY_OFFSET(mem_offset));
    }

    rgb_data_array_t* rgb_array_ptr;
    for (uint32_t address = APP_DATA_ADDR + CODE_PAGE_SIZE; address <= BOOTLOADER_ADDR - RGB_DATA_ARRAY_SIZE; address += RGB_DATA_ARRAY_SIZE) {
        rgb_array_ptr = (rgb_data_array_t*) address;
        NRF_LOG_INFO("rgb_data_array_t address is 0x%" PRIx32, address);
        NRF_LOG_INFO("rgb_data_array_t signature is 0x%" PRIx8, rgb_array_ptr->_signature);

        if (rgb_array_ptr->_signature != RGB_DATA_ARRAY_SIGNATURE) {
            NRF_LOG_INFO("Found invalid data at address 0x%" PRIx32, address);
            if (address == APP_DATA_ADDR + CODE_PAGE_SIZE) {
                break;
            }

            NRF_LOG_INFO("Found rgb_data_array_t at address 0x%" PRIx32, address - RGB_DATA_ARRAY_SIZE);
            mem_offset = address - APP_DATA_ADDR - RGB_DATA_ARRAY_SIZE;
            offset_defined = true;
                
            
            return get_rgb_array_by_address(GET_ADDRESS_BY_OFFSET(mem_offset));
        }
    }

    return (rgb_data_array_t) {._signature = RGB_DATA_ARRAY_SIGNATURE, .count = 0};
}

void save_colors_array(rgb_data_array_t* rgb_data_array) {
    if (!offset_defined || GET_ADDRESS_BY_OFFSET(mem_offset + RGB_DATA_ARRAY_SIZE) > BOOTLOADER_ADDR) {
        mem_offset = CODE_PAGE_SIZE;
        offset_defined = true;
    }
    else {
        mem_offset += RGB_DATA_ARRAY_SIZE;
    }

    NRF_LOG_INFO("Attempt to write colors_array at address 0x%" PRIx32, GET_ADDRESS_BY_OFFSET(mem_offset));
    for (size_t i = 0; i < RGB_DATA_ARRAY_SIZE_IN_WORDS; i++) {
        if (mem_offset % CODE_PAGE_SIZE == 0) {
            nrfx_nvmc_page_erase(GET_ADDRESS_BY_OFFSET(mem_offset));
        }

        nrfx_nvmc_word_write(GET_ADDRESS_BY_OFFSET(mem_offset), rgb_data_array->_value[i]);
        while (!nrfx_nvmc_write_done_check());

        mem_offset += WORD_SIZE;
    }
    mem_offset -= RGB_DATA_ARRAY_SIZE;
    NRF_LOG_INFO("Colors array wrote at address 0x%" PRIx32, GET_ADDRESS_BY_OFFSET(mem_offset));
}

static uint32_t hsv_offset = 0;
static bool hsv_offset_defined = false;

hsv_data_t get_last_saved_hsv_data() {
    if (offset_defined) {
        hsv_data_t hsv;
        memcpy(&hsv, (void*) GET_ADDRESS_BY_OFFSET(hsv_offset), WORD_SIZE);
        return hsv;
    }

    for (uint32_t address = APP_DATA_ADDR; address <= GET_ADDRESS_BY_OFFSET(CODE_PAGE_SIZE) - WORD_SIZE; address += WORD_SIZE) {
        hsv_data_t* hsv_ptr = (hsv_data_t*) address;
        NRF_LOG_INFO("hsv_data_t address is 0x%" PRIx32, address);

        if (hsv_ptr->_signature != HSV_DATA_SIGNATURE) {
            NRF_LOG_INFO("Found invalid data at address 0x%" PRIx32, address);
            if (address == APP_DATA_ADDR) {
                break;
            }

            
            hsv_offset = address - APP_DATA_ADDR - WORD_SIZE;
            hsv_offset_defined = true;

            hsv_data_t hsv;
            memcpy(&hsv, (void*) GET_ADDRESS_BY_OFFSET(hsv_offset), WORD_SIZE);
            NRF_LOG_INFO("Found hsv_data_t 0x%" PRIx32 " at address 0x%" PRIx32, hsv._value, address - WORD_SIZE);
            
            return hsv;
        }
    }
    return new_hsv(360. * DEVICE_ID_LAST_DIGITS / 100, 100, 100);
}

void write_hsv_data(hsv_data_t* hsv_ptr) {
    if (!hsv_offset_defined) {
        hsv_offset = 0;
        hsv_offset_defined = true;
    }
    else {
        hsv_offset = (hsv_offset + WORD_SIZE) % CODE_PAGE_SIZE;
    }

    if (hsv_offset % CODE_PAGE_SIZE == 0) {
        nrfx_nvmc_page_erase(GET_ADDRESS_BY_OFFSET(hsv_offset));
    }

    NRF_LOG_INFO("Attempt to write hsv_data_t 0x%" PRIx32 " at address 0x%" PRIx32, hsv_ptr->_value, GET_ADDRESS_BY_OFFSET(hsv_offset));
    nrfx_nvmc_word_write(GET_ADDRESS_BY_OFFSET(hsv_offset), hsv_ptr->_value);
    while(!nrfx_nvmc_write_done_check());
}


void init_nmvc() {
    get_last_saved_rgb_array();
}
