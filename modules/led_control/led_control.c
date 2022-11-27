#include <stdint.h>
#include "nrf_gpio.h"
#include "led_control.h"
#include "nrfx_systick.h"

#define US_IN_SECOND 1000000

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
    nrfx_systick_init();
}

void led_off(uint32_t led_id) {
    nrf_gpio_pin_write(leds_array[led_id], 1);
}

void led_on(uint32_t led_id) {
    nrf_gpio_pin_write(leds_array[led_id], 0);
}

void led_toggle(uint32_t led_id) {
    nrf_gpio_pin_toggle(leds_array[led_id]);
}

static uint32_t get_period_time_us(uint32_t frequency) {
    return (uint32_t)((float)US_IN_SECOND / frequency);
}

static uint32_t get_time_on_us(uint32_t period_time_us, uint8_t duty_cycle) {
    return (uint32_t)((float)(period_time_us) / 100 * duty_cycle);
}

pwm_config_t create_pwm_config(uint32_t frequency) {
    pwm_config_t pwm_conf = {.is_on = false, .period_time_us = get_period_time_us(frequency),
    .time_on_us = 0};
    return pwm_conf;
}

void set_duty_cycle(pwm_config_t* pwm_conf, uint8_t duty_cycle) {
    pwm_conf->time_on_us = get_time_on_us(pwm_conf->period_time_us, duty_cycle);
}

void pwm_write(uint32_t led_id, pwm_config_t* pwm_conf) {
    if (pwm_conf->is_on) {
        if (nrfx_systick_test(&pwm_conf->started, pwm_conf->time_on_us)) {
            led_off(led_id);
            pwm_conf->is_on = false;
            nrfx_systick_get(&pwm_conf->started);
        }
    }
    else {
        if (nrfx_systick_test(&pwm_conf->started, pwm_conf->period_time_us - pwm_conf->time_on_us)) {
            led_on(led_id);
            pwm_conf->is_on = true;
            nrfx_systick_get(&pwm_conf->started);
        } 
    }
}