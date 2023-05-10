#include "led_color.h"
#include "../led_control/led_control.h"

static nrf_pwm_values_individual_t seq_values;

static nrf_pwm_sequence_t sequence = {
    .values.p_individual = &seq_values,
    .length = NRF_PWM_VALUES_LENGTH(seq_values),
    .repeats = 0,
    .end_delay = 0
};

static struct {
    uint16_t* const led2_red;
    uint16_t* const led2_green;
    uint16_t* const led2_blue;
    uint16_t* const led1;
} led_values_pointers_s = {.led1 = &seq_values.channel_3, .led2_blue = &seq_values.channel_2, 
                            .led2_green = &seq_values.channel_1, .led2_red = &seq_values.channel_0};


static hsv_data_t current_hsv_color = {.0};
static rgb_data_t current_rgb_color = {0};
static bool color_was_changed = false;


bool led_color_was_color_changed() {
    if (color_was_changed) {
        color_was_changed = false;
        return !color_was_changed;
    }
    return color_was_changed;
}


nrfx_pwm_t pwm_control_init() {
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
    return driver_instance;
}

hsv_data_t get_current_hsv_color() {
    return current_hsv_color;
}

rgb_data_t get_current_rgb_color() {
    return current_rgb_color;
}

void set_led2_color_by_rgb(const rgb_data_t* rgb) {
    *led_values_pointers_s.led2_red = rgb->r;
    *led_values_pointers_s.led2_blue = rgb->b;
    *led_values_pointers_s.led2_green = rgb->g;

    current_rgb_color = *rgb;
    color_was_changed = true;
    current_hsv_color = get_hsv_from_rgb(rgb);
}

void set_led2_color_by_hsv(const hsv_data_t* hsv) {
    rgb_data_t rgb = get_rgb_from_hsv(hsv);
    set_led2_color_by_rgb(&rgb);
}

void set_led1_brightness(uint8_t brightness) {
    *led_values_pointers_s.led1 = brightness;
}

uint16_t get_led1_brightness() {
    return *led_values_pointers_s.led1;
}
