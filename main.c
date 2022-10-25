#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

#define DEVICE_ID 6577

void blink_led_n_times(uint32_t led_id, uint32_t times) {
    for (uint32_t i = 0; i < times; i++) {
        bsp_board_led_on(led_id);
        nrf_delay_ms(500);
        bsp_board_led_off(led_id);
        nrf_delay_ms(500);
    }
}

void blink_led_by_device_id(uint32_t led_id) {
    uint32_t blinks_count;
    for (int i = 1000; i > 0; i /= 10) {
        blinks_count = DEVICE_ID / i % 10;
        blink_led_n_times(led_id, blinks_count);
        nrf_delay_ms(1000);
    }
}

int main(void)
{
    bsp_board_init(BSP_INIT_LEDS);
    
    while (true)
    {
        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            blink_led_by_device_id(i);
        }
    }
}
