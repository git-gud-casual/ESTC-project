#include "button_control.h"
#include "nrf_gpio.h"

const static uint8_t buttons_array[BUTTONS_COUNT] = BUTTONS_ARRAY;

void buttons_init() {
    for (size_t i = 0; i < BUTTONS_COUNT; i++) {
        nrf_gpio_cfg_input(buttons_array[i], NRF_GPIO_PIN_PULLUP);
    }
}

bool button_pressed(uint32_t button_id) {
    return !nrf_gpio_pin_read(buttons_array[button_id]);
}