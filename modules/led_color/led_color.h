#include "nrfx_pwm.h"
#include "../color_types/color_types.h"

#define PWM_TOP_VALUE 255


nrfx_pwm_t pwm_control_init();

void set_led2_color_by_rgb(const rgb_data_t* rgb);
void set_led2_color_by_hsv(const hsv_data_t* hsv);

void set_led1_brightness(uint8_t brightness);
uint16_t get_led1_brightness();

hsv_data_t get_current_hsv_color();
rgb_data_t get_current_rgb_color();

bool led_color_was_color_changed();