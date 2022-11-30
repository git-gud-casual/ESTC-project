#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>


#include "modules/led_control/led_control.h"
#include "modules/button_control/button_control.h"
#include "modules/color_types/color_types.h"

#include "nrfx_pwm.h"
#include "app_timer.h"
#include "nrfx_systick.h"
#include "nrfx_nvmc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#define DEVICE_ID_LAST_DIGITS 77
#define HUE_MODIFICATION_LED1_BLINK_TIME_MS 10
#define SATURATION_MODIFICATION_LED1_BLINK_TIME_MS 1
#define PWM_TOP_VALUE 255
#define PWM_STEP 1
#define HUE_STEP 1
#define SATURATION_STEP 1
#define VALUE_STEP 1
#define CHANGE_COLOR_SPEED_TIME_US 20000


typedef enum {
    NO_INPUT,
    HUE_MODIFICATION,
    SATURATION_MODIFICATION,
    BRIGHTNESS_MODIFICATION
} input_states_t;

static input_states_t current_input_state = NO_INPUT;
static nrf_pwm_values_individual_t seq_values;

static volatile int8_t pwm_step = PWM_STEP;
static volatile bool should_change_color = false;
static volatile bool should_save_data = false;
static nrfx_systick_state_t change_color_speed_timer;
APP_TIMER_DEF(led1_blink_timer);


static nrf_pwm_sequence_t sequence = {
    .values.p_individual = &seq_values,
    .length = NRF_PWM_VALUES_LENGTH(seq_values),
    .repeats = 0,
    .end_delay = 0
};


void click_handler(uint8_t clicks_count) {
    NRF_LOG_INFO("CLICK HANDLER %" PRIu8 " clicks", clicks_count);
    if (clicks_count == 2) {
        switch (current_input_state) {
            case NO_INPUT:
                current_input_state = HUE_MODIFICATION;
                app_timer_start(led1_blink_timer, APP_TIMER_TICKS(HUE_MODIFICATION_LED1_BLINK_TIME_MS), NULL);
                break;
            case HUE_MODIFICATION:
                current_input_state = SATURATION_MODIFICATION;
                app_timer_stop(led1_blink_timer);
                app_timer_start(led1_blink_timer, APP_TIMER_TICKS(SATURATION_MODIFICATION_LED1_BLINK_TIME_MS), NULL);
                break;
            case SATURATION_MODIFICATION:
                current_input_state = BRIGHTNESS_MODIFICATION;
                app_timer_stop(led1_blink_timer);
                seq_values.channel_3 = PWM_TOP_VALUE;
                break;
            case BRIGHTNESS_MODIFICATION:
                current_input_state = NO_INPUT;
                should_save_data = true;
                seq_values.channel_3 = 0;
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
    if (seq_values.channel_3 + pwm_step > PWM_TOP_VALUE || seq_values.channel_3 + pwm_step < 0) {
        pwm_step *= -1;
    }
    seq_values.channel_3 += pwm_step;
}


void main_loop() {
    nrfx_pwm_t driver_instance = NRFX_PWM_INSTANCE(1);
    nrfx_pwm_config_t pwm_conf = NRFX_PWM_DEFAULT_CONFIG;
    pwm_conf.load_mode = NRF_PWM_LOAD_INDIVIDUAL;
    pwm_conf.output_pins[0] = LED2_R;
    pwm_conf.output_pins[1] = LED2_G;
    pwm_conf.output_pins[2] = LED2_B;
    pwm_conf.output_pins[3] = LED1;
    pwm_conf.top_value = PWM_TOP_VALUE;
    nrfx_pwm_init(&driver_instance, &pwm_conf, NULL);
    nrfx_pwm_simple_playback(&driver_instance, &sequence, 1, NRFX_PWM_FLAG_LOOP);

    app_timer_create(&led1_blink_timer, APP_TIMER_MODE_REPEATED, led1_blink_timer_handler);

    hsv_data_t hsv = get_last_saved_or_default_hsv_data();
    rgb_data_t rgb = get_rgb_from_hsv(&hsv);

    seq_values.channel_0 = rgb.r;
    seq_values.channel_1 = rgb.g;
    seq_values.channel_2 = rgb.b;

    button_interrupt_init(BUTTON1_ID, click_handler, release_handler);

    int8_t hue_step = HUE_STEP;
    int8_t saturation_step = SATURATION_STEP;
    int8_t value_step = VALUE_STEP;

    while (true) {
        if (current_input_state == NO_INPUT && should_save_data) {
            save_hsv_data(hsv);
            should_save_data = false;
        }
        else if (should_change_color && nrfx_systick_test(&change_color_speed_timer, CHANGE_COLOR_SPEED_TIME_US)) {
            switch (current_input_state) {
                case HUE_MODIFICATION:
                    hsv.h = (hsv.h + hue_step) % 360;
                    break;
                case SATURATION_MODIFICATION:
                    if (saturation_step + hsv.s > 100 || hsv.s + saturation_step < 0) {
                        saturation_step *= -1;
                    }
                    hsv.s += saturation_step;
                    break;
                case BRIGHTNESS_MODIFICATION:
                    if (hsv.v + value_step > 100 || hsv.v + value_step < 0) {
                        value_step *= -1;
                    }
                    hsv.v += value_step;
                    break;
                case NO_INPUT:
                    break;
            }
            rgb = get_rgb_from_hsv(&hsv);
            seq_values.channel_0 = rgb.r;
            seq_values.channel_1 = rgb.g;
            seq_values.channel_2 = rgb.b;
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
