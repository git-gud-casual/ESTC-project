#include <stdbool.h>
#include <stdint.h>


#include "modules/led_control/led_control.h"
#include "modules/button_control/button_control.h"
#include "nrfx_systick.h"

/* Logs libraries */
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"


#define SEQUENCE "RGB"
#define PWM_TURN_ON_TIME_US 25000


static volatile bool pause = false;


void light_up_with_brightness(char color, uint8_t brightness_percent, uint32_t time_us) {
    uint32_t led_id;
    switch (color) {
        case 'R':
            led_id = LED2_R_ID;
            break;
        case 'G':
            led_id = LED2_G_ID;
            break;
        case 'B':
            led_id = LED2_B_ID;
            break;
        default:
            return;
    }
    
    nrfx_systick_state_t systick_time;
    nrfx_systick_get(&systick_time);
    while (!nrfx_systick_test(&systick_time, time_us)) {
        pwm_write(led_id, brightness_percent);
    }
}

void double_click_handler() {
    NRF_LOG_INFO("DOUBLE_CLICK");
    pause = !pause;
}

void init_board() {
    leds_init();
    button_control_init();
    nrfx_systick_init();
    button_interrupt_init(BUTTON1_ID, NULL, double_click_handler);

    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void main_loop() {
    int8_t duty_cycle = 0;
    int8_t surplus_val = 1;
    size_t current_char_id = 0;

    while (true)
    {
        light_up_with_brightness(SEQUENCE[current_char_id], duty_cycle, PWM_TURN_ON_TIME_US);

        if (!pause) {
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
        }
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

int main(void) {
    init_board();
    main_loop();
}
