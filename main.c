#include <stdbool.h>
#include <stdint.h>


#include "modules/led_control/led_control.h"
#include "modules/button_control/button_control.h"
#include "nrfx_systick.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"


#define SEQUENCE "RGB"
#define FREQUENCY 1000
#define DUTY_CYCLE_CHANGE_TIME_US 25000


static volatile bool pause = false;


uint32_t get_led_id(char color) {
    switch (color) {
        case 'R':
            return LED2_R_ID;
        case 'G':
            return LED2_G_ID;
        case 'B':
            return LED2_B_ID;
        default:
            return 0;
    }
}

void double_click_handler(uint8_t clicks_count) {
    if (clicks_count == 2) {
        NRF_LOG_INFO("DOUBLE_CLICK");
        pause = !pause;
    }
}

void init_board() {
    leds_init();
    button_control_init();
    nrfx_systick_init();
    button_interrupt_init(BUTTON1_ID, double_click_handler);

    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void main_loop() {
    int8_t duty_cycle = 0;
    int8_t surplus_val = 1;
    size_t current_char_id = 0;
    pwm_config_t pwm_conf = create_pwm_config(FREQUENCY);

    nrfx_systick_state_t duty_cycle_change_time;
    nrfx_systick_get(&duty_cycle_change_time);

    while (true)
    {
        pwm_write(get_led_id(SEQUENCE[current_char_id]), &pwm_conf);

        if (!pause && nrfx_systick_test(&duty_cycle_change_time, DUTY_CYCLE_CHANGE_TIME_US)) {
            duty_cycle += surplus_val;
            if (duty_cycle == -1) {
                surplus_val = 1;
                duty_cycle = 0;

                if (SEQUENCE[current_char_id + 1] != '\0') {
                    current_char_id++;
                }
                else {
                    current_char_id = 0;
                }
            } 
            else if (duty_cycle == 101) {
                surplus_val = -1;
                duty_cycle = 100;
            }
            set_duty_cycle(&pwm_conf, duty_cycle);
            nrfx_systick_get(&duty_cycle_change_time);
        }
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

int main(void) {
    init_board();
    main_loop();
}
