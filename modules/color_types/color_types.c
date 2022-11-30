#include "color_types.h"
#include <math.h>
#include <inttypes.h>

#include "nrfx_nvmc.h"
#include "crc32.h"
#include "nrf_dfu_types.h"
#include "nrf_log.h"

#define APP_DATA_ADDR BOOTLOADER_ADDRESS - NRF_DFU_APP_DATA_AREA_SIZE
#define WORD_SIZE 4
#define HSV_DATA_SIZE 4
#define DEVICE_ID_LAST_DIGITS 77
static bool last_record_address_is_valid = false;
static uint32_t last_record_address;

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

static uint32_t get_crc32(hsv_data_t* hsv) {
    return crc32_compute(hsv->_data, sizeof(hsv_data_t) - 4, NULL);
}

hsv_data_t get_last_saved_or_default_hsv_data() {
    hsv_data_t* hsv;
    if (!last_record_address_is_valid) {
        for (uint32_t address = APP_DATA_ADDR; address <= BOOTLOADER_ADDRESS; address += sizeof(hsv_data_t)) {
            hsv = (hsv_data_t*) address;
            if (get_crc32(hsv) != hsv->_crc32) {
                if (address != APP_DATA_ADDR) {
                    last_record_address = address - sizeof(hsv_data_t);
                    last_record_address_is_valid = true;
                }
                else {
                    return new_hsv(360. * DEVICE_ID_LAST_DIGITS / 100, 100, 100);
                }
            }
        }
    }
    hsv = (hsv_data_t*) last_record_address;
    return new_hsv(hsv->h, hsv->s, hsv->v);
}

void save_hsv_data(hsv_data_t hsv) {
    hsv._crc32 = get_crc32(&hsv);
    if (!last_record_address_is_valid || last_record_address == BOOTLOADER_ADDRESS - 2 * WORD_SIZE) {
        last_record_address = APP_DATA_ADDR;
    }
    else {
        last_record_address += WORD_SIZE * 2;
    }

    if ((last_record_address - APP_DATA_ADDR) % CODE_PAGE_SIZE == 0) {
        nrfx_nvmc_page_erase(last_record_address);
    }

    nrfx_nvmc_bytes_write(last_record_address, hsv._data, 4);
    while (!nrfx_nvmc_write_done_check());
    nrfx_nvmc_word_write(last_record_address + WORD_SIZE, hsv._crc32);
    while (!nrfx_nvmc_write_done_check());
    last_record_address_is_valid = true;
}