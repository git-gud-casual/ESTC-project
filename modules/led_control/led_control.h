#ifndef _LED_CONTROL
#define _LED_CONTROL

#include <stdint.h>
#include "nrf_gpio.h"
#include "nrfx_systick.h"

#define LEDS_COUNT 3

#define LED2_R NRF_GPIO_PIN_MAP(0, 8)
#define LED2_R_ID 0

#define LED2_G NRF_GPIO_PIN_MAP(1, 9)
#define LED2_G_ID 1

#define LED2_B NRF_GPIO_PIN_MAP(0, 12)
#define LED2_B_ID 2

#define LEDS_ARRAY {LED2_R, LED2_G, LED2_B};

typedef struct {
    bool is_on;
    nrfx_systick_state_t started;
    uint32_t time_on_us;
    uint32_t period_time_us;
} pwm_config_t;

void leds_init();

void led_on(uint32_t led_id);
void led_off(uint32_t led_id);
void led_toggle(uint32_t led_id);

void set_duty_cycle(pwm_config_t* pwm_conf, uint8_t duty_cycle);
pwm_config_t create_pwm_config(uint32_t frequency);
void pwm_write(uint32_t led_id, pwm_config_t* pwm_conf);

#endif
