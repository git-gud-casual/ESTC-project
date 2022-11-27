#include "button_control.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "nrfx_gpiote.h"
#include "nrf_log.h"
#include <inttypes.h>

#define DEBOUNCING_TIMEOUT_MS 50
#define CLICKS_COUNT_TIMEOUT_MS 400

APP_TIMER_DEF(debouncing_timer);
APP_TIMER_DEF(clicks_count_timer);

static struct {
    uint32_t button_id;
    uint8_t button_clicks_count;
    bool debounce_proccessing;
    bool click_count_timer_is_started;
    bool should_call_release_handler;
    click_handler_t click_handler;
    release_handler_t release_handler;
} button_config_s;

const static uint8_t buttons_array[BUTTONS_COUNT] = BUTTONS_ARRAY;

static void button_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    if (!button_config_s.debounce_proccessing) {
        NRF_LOG_INFO("Button toggled");
        button_config_s.debounce_proccessing = true;
        app_timer_start(debouncing_timer, APP_TIMER_TICKS(DEBOUNCING_TIMEOUT_MS), NULL);
    }
}

void button_interrupt_init(uint32_t button_id, click_handler_t click_handler, release_handler_t release_handler) {
    button_config_s.button_id = button_id;
    button_config_s.click_handler = click_handler;
    button_config_s.release_handler = release_handler;
    button_config_s.button_clicks_count = 0;
    button_config_s.debounce_proccessing = false;
    button_config_s.click_count_timer_is_started = false;
    button_config_s.should_call_release_handler = false;

    nrfx_gpiote_in_config_t button_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    button_config.pull = NRF_GPIO_PIN_PULLUP;
    nrfx_gpiote_in_init(buttons_array[button_id], &button_config, button_handler);
    nrfx_gpiote_in_event_enable(buttons_array[button_id], true);
}

static void debouncing_timer_handler(void* p_context) {
    if (button_pressed(button_config_s.button_id)) {
        NRF_LOG_INFO("Button pressed");
        button_config_s.button_clicks_count += 1;
        app_timer_stop(clicks_count_timer);
        app_timer_start(clicks_count_timer, APP_TIMER_TICKS(CLICKS_COUNT_TIMEOUT_MS), NULL);
        button_config_s.click_count_timer_is_started = true;
        button_config_s.should_call_release_handler = false;
    }
    else if (button_config_s.release_handler != NULL) {
        NRF_LOG_INFO("Button released");
        if (!(button_config_s.should_call_release_handler = button_config_s.click_count_timer_is_started)) {
            button_config_s.release_handler();
        }
    }
    button_config_s.debounce_proccessing = false;
}

static void clicks_count_timer_handler(void* p_context) {
    NRF_LOG_INFO("Clicks count: %" PRIu8, button_config_s.button_clicks_count);
    if (button_config_s.click_handler != NULL) {
        button_config_s.click_handler(button_config_s.button_clicks_count);
    }
    if (button_config_s.should_call_release_handler) {
        button_config_s.release_handler();
        button_config_s.should_call_release_handler = false;
    }
    button_config_s.click_count_timer_is_started = false;
    button_config_s.button_clicks_count = 0;
}

static void timers_init() {
    app_timer_init();

    app_timer_create(&debouncing_timer, APP_TIMER_MODE_SINGLE_SHOT, debouncing_timer_handler);
    app_timer_create(&clicks_count_timer, APP_TIMER_MODE_SINGLE_SHOT, clicks_count_timer_handler);
}

void button_control_init() {
    timers_init();
    nrfx_gpiote_init();
}

bool button_pressed(uint32_t button_id) {
    return !nrf_gpio_pin_read(buttons_array[button_id]);
}

