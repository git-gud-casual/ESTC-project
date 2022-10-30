#include <stdint.h>
#include "nrf_gpio.h"
#include "led_control.h"

static const uint8_t leds_array[LEDS_COUNT] = LEDS_ARRAY;

static void gpio_output_voltage_setup(void)
{
    // Configure UICR_REGOUT0 register only if it is set to default value.
    if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
        (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))
    {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

        NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
                            (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos);

        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

        // System reset is needed to update UICR registers.
        NVIC_SystemReset();
    }
}

void leds_init() {
    if (NRF_POWER->MAINREGSTATUS &
       (POWER_MAINREGSTATUS_MAINREGSTATUS_High << POWER_MAINREGSTATUS_MAINREGSTATUS_Pos)) {
        gpio_output_voltage_setup();
    }

    for (size_t i = 0; i < LEDS_COUNT; i++) {
        nrf_gpio_cfg_output(leds_array[i]);
        led_off(i);
    }
}

void led_off(size_t led_id) {
    nrf_gpio_pin_write(leds_array[led_id], 1);
}

void led_on(size_t led_id) {
    nrf_gpio_pin_write(leds_array[led_id], 0);
}

void led_toggle(size_t led_id) {
    nrf_gpio_pin_toggle(leds_array[led_id]);
}
