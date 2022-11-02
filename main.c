#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"

#include "led_control/led_control.h"
#include "button_control/button_control.h"

#define SEQUENCE "RRGGGB"


void led2_toggle_by_color(char color) {
    switch (color) {
        case 'R':
            led_toggle(LED2_R_ID);
            break;
        case 'G':
            led_toggle(LED2_G_ID);
            break;
        case 'B':
            led_toggle(LED2_B_ID);
            break;
    }
}

void init_board() {
    leds_init();
    buttons_init();
}

void main_loop() {
    size_t current_sequnce_char_index = 0;
    uint16_t ms_count = 0;

    while (true) {
        while (button_pressed(BUTTON1_ID)) {
            nrf_delay_ms(10);
            ms_count += 10;
            if (ms_count > 500) {
                ms_count = 0;

                if (SEQUENCE[current_sequnce_char_index / 2] == '\0') {
                    current_sequnce_char_index = 0;
                }
                
                led2_toggle_by_color(SEQUENCE[current_sequnce_char_index++ / 2]);
            }
        }
        ms_count = 0;
    }
}

int main(void) {
    init_board();
    main_loop();
}
