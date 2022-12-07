#include "color_types.h"
#include <math.h>
#include <inttypes.h>

#include "nrfx_nvmc.h"
#include "nrf_dfu_types.h"

#include "nrf_log.h"

#define BOOTLOADER_ADDR 0xE0000
#define APP_DATA_ADDR (BOOTLOADER_ADDR - NRF_DFU_APP_DATA_AREA_SIZE)
#define GET_ADDRESS_BY_OFFSET(offset) (APP_DATA_ADDR + offset)
#define WORD_SIZE 4
#define DEVICE_ID_LAST_DIGITS 77
#define SIGNATURE 0b00101

static bool offset_defined = false;
static uint32_t last_hsv_offset;

hsv_data_t new_hsv(uint16_t h, uint8_t s, uint8_t v) {
    return (hsv_data_t) {.h = h, .s = s, .v = v, ._signature = SIGNATURE};
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

static hsv_data_t get_hsv_by_address(uint32_t address) {
    return (hsv_data_t) {*((uint32_t*) address)};
}

static void write_hsv(hsv_data_t* hsv) {
    if (offset_defined) {
        nrfx_nvmc_word_write(GET_ADDRESS_BY_OFFSET(last_hsv_offset), hsv->_value);
        while (!nrfx_nvmc_write_done_check());
    }
}

hsv_data_t get_last_saved_or_default_hsv_data() {
    if (offset_defined) {
        return get_hsv_by_address(GET_ADDRESS_BY_OFFSET(last_hsv_offset));
    }
    else {
        hsv_data_t hsv;
        for (uint32_t address = APP_DATA_ADDR; address < BOOTLOADER_ADDR; address += WORD_SIZE) {
            hsv = get_hsv_by_address(address);
            if (hsv._signature != SIGNATURE) {
                if (address == APP_DATA_ADDR) {
                    break;
                }

                NRF_LOG_INFO("Found hsv_data_t 0x%" PRIx32 " at address 0x%" PRIx32, hsv._value, address);
                hsv = get_hsv_by_address(address - WORD_SIZE);
                last_hsv_offset = address - APP_DATA_ADDR;
                offset_defined = true;
                return hsv;
            }
        }
        return new_hsv(360. * DEVICE_ID_LAST_DIGITS / 100, 100, 100);
    }
}

void save_hsv_data(hsv_data_t* hsv) {
    hsv->_is_writeable = 1;
    if (offset_defined &&
        nrfx_nvmc_word_writable_check(last_hsv_offset + APP_DATA_ADDR, hsv->_value)) {
        
        NRF_LOG_INFO("Attempt to rewrite hsv_data_t 0x%" PRIx32 " at address 0x%" PRIx32, hsv->_value, GET_ADDRESS_BY_OFFSET(last_hsv_offset));
        hsv->_is_writeable = 0;
        write_hsv(hsv);
        return;
    }
    
    if (!offset_defined) {
        last_hsv_offset = 0;
    }
    else {
        last_hsv_offset = (last_hsv_offset + WORD_SIZE) % NRF_DFU_APP_DATA_AREA_SIZE;
    }

    if (last_hsv_offset % CODE_PAGE_SIZE == 0) {
        nrfx_nvmc_page_erase(GET_ADDRESS_BY_OFFSET(last_hsv_offset));
    }

    NRF_LOG_INFO("Attempt to write hsv_data_t 0x%" PRIx32 " at address 0x%" PRIx32, hsv->_value, GET_ADDRESS_BY_OFFSET(last_hsv_offset));
    write_hsv(hsv);
    offset_defined = true;
}


void init_nmvc() {
    get_last_saved_or_default_hsv_data();
}