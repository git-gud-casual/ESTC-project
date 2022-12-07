#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>


#include "modules/led_control/led_control.h"
#include "modules/button_control/button_control.h"
#include "modules/color_types/color_types.h"
#include "modules/led_color/led_color.h"

#include "nrfx_pwm.h"
#include "app_timer.h"
#include "nrfx_systick.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"


#define HUE_MODIFICATION_LED1_BLINK_TIME_MS 10
#define SATURATION_MODIFICATION_LED1_BLINK_TIME_MS 1

#define PWM_STEP 1
#define HUE_STEP 1
#define SATURATION_STEP 1
#define VALUE_STEP 1

#define CHANGE_COLOR_SPEED_TIME_US 20000


typedef enum {
    STATE_NO_INPUT,
    STATE_HUE_MODIFICATION,
    STATE_SATURATION_MODIFICATION,
    STATE_BRIGHTNESS_MODIFICATION
} input_states_t;


static volatile int8_t pwm_step = PWM_STEP;

static input_states_t current_input_state = STATE_NO_INPUT;
static volatile bool should_change_color = false;
static volatile bool should_save_data = false;

static nrfx_systick_state_t change_color_speed_timer;
APP_TIMER_DEF(led1_blink_timer);


void click_handler(uint8_t clicks_count) {
    NRF_LOG_INFO("CLICK HANDLER %" PRIu8 " clicks", clicks_count);
    if (clicks_count == 2) {
        switch (current_input_state) {
            case STATE_NO_INPUT:
                current_input_state = STATE_HUE_MODIFICATION;
                app_timer_start(led1_blink_timer, APP_TIMER_TICKS(HUE_MODIFICATION_LED1_BLINK_TIME_MS), NULL);
                break;
            case STATE_HUE_MODIFICATION:
                current_input_state = STATE_SATURATION_MODIFICATION;
                app_timer_stop(led1_blink_timer);
                app_timer_start(led1_blink_timer, APP_TIMER_TICKS(SATURATION_MODIFICATION_LED1_BLINK_TIME_MS), NULL);
                break;
            case STATE_SATURATION_MODIFICATION:
                current_input_state = STATE_BRIGHTNESS_MODIFICATION;
                app_timer_stop(led1_blink_timer);

                set_led1_brightness(PWM_TOP_VALUE);
                break;
            case STATE_BRIGHTNESS_MODIFICATION:
                current_input_state = STATE_NO_INPUT;
                should_save_data = true;

                set_led1_brightness(0);
                break;
        }
        
    }
    else if (clicks_count == 1) {
        should_change_color = true;
        nrfx_systick_get(&change_color_speed_timer);
    }
}

void release_handler() {
    NRF_LOG_INFO("RELEASE HANDLER");
    should_change_color = false;
} 

void init_board() {
    app_timer_init();
    nrfx_systick_init();

    button_control_init();
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


void led1_blink_timer_handler(void* p_context) {
    uint16_t led1_brightness = get_led1_brightness();
    if (led1_brightness + pwm_step > PWM_TOP_VALUE || led1_brightness  + pwm_step < 0) {
        pwm_step *= -1;
    }
    set_led1_brightness(led1_brightness + pwm_step);
}


void main_loop() {
    nrfx_pwm_t driver_instance = pwm_control_init();
    UNUSED_VARIABLE(driver_instance);
    
    app_timer_create(&led1_blink_timer, APP_TIMER_MODE_REPEATED, led1_blink_timer_handler);

    button_interrupt_init(BUTTON1_ID, click_handler, release_handler);

    hsv_data_t hsv =  get_last_saved_or_default_hsv_data();
    set_led2_color_by_hsv(&hsv);

    int8_t hue_step = HUE_STEP;
    int8_t saturation_step = SATURATION_STEP;
    int8_t value_step = VALUE_STEP;

    while (true) {
        if (current_input_state == STATE_NO_INPUT && should_save_data) {
            save_hsv_data(&hsv);
            should_save_data = false;
        }
        else if (should_change_color && 
                 nrfx_systick_test(&change_color_speed_timer, CHANGE_COLOR_SPEED_TIME_US)) {

            switch (current_input_state) {
                case STATE_HUE_MODIFICATION:
                    hsv.h = (hsv.h + hue_step) % 360;
                    break;
                case STATE_SATURATION_MODIFICATION:
                    if (saturation_step + hsv.s > 100 || hsv.s + saturation_step < 0) {
                        saturation_step *= -1;
                    }
                    hsv.s += saturation_step;
                    break;
                case STATE_BRIGHTNESS_MODIFICATION:
                    if (hsv.v + value_step > 100 || hsv.v + value_step < 0) {
                        value_step *= -1;
                    }
                    hsv.v += value_step;
                    break;
                case STATE_NO_INPUT:
                    break;
            }
            set_led2_color_by_hsv(&hsv);
            nrfx_systick_get(&change_color_speed_timer);
        }
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
    
}

int main(void) {
    init_board();
    main_loop();
}
