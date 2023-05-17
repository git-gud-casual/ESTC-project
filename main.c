/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup estc_adverts main.c
 * @{
 * @ingroup estc_templates
 * @brief ESTC Advertisments template app.
 *
 * This file contains a template for creating a new BLE application with GATT services. It has
 * the code necessary to advertise, get a connection, restart advertising on disconnect.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrfx_pwm.h"
#include "nrfx_systick.h"
#include "nrfx_gpiote.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "modules/ble_service/ble_service.h"
#if ESTC_USB_CLI_ENABLED == 1
    #include "modules/cli/cli.h"
    #include "modules/commands/commands.h"
#endif
#include "modules/fs/fs.h"
#include "modules/button_control/button_control.h"
#include "modules/color_types/color_types.h"
#include "modules/led_color/led_color.h"


#define HUE_MODIFICATION_LED1_BLINK_TIME_MS 10
#define SATURATION_MODIFICATION_LED1_BLINK_TIME_MS 1

#define PWM_STEP 1
#define HUE_STEP 1
#define SATURATION_STEP 1
#define VALUE_STEP 1
#define DEVICE_ID 77

#define CHANGE_COLOR_SPEED_TIME_US 2000

#define DEVICE_NAME                     "BLE LED Service"        /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                   /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION                18000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

/**< Universally unique service identifiers. */ 
static ble_uuid_t m_adv_uuids[] =                                             
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
    {ESTC_SERVICE_UUID, BLE_UUID_TYPE_VENDOR_BEGIN}
};

ble_estc_service_t m_service_example; /**< ESTC example BLE service */


APP_TIMER_DEF(led1_blink_timer);                                                /**< Timer to blink led1 when changing led2 color*/

/* Code to changing led2 color with button press*/
typedef enum {
    STATE_NO_INPUT,
    STATE_HUE_MODIFICATION,
    STATE_SATURATION_MODIFICATION,
    STATE_BRIGHTNESS_MODIFICATION
} input_states_t;


static volatile int8_t pwm_step = PWM_STEP;

static input_states_t current_input_state = STATE_NO_INPUT;
static volatile bool should_change_color = false;

static nrfx_systick_state_t change_color_speed_timer;

static hsv_data_t hsv;

void click_handler(uint8_t clicks_count) {
    NRF_LOG_INFO("CLICK HANDLER %" PRIu8 " clicks", clicks_count);
    hsv = get_current_hsv_color();
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

void led1_blink_timer_handler(void* p_context) {
    uint16_t led1_brightness = get_led1_brightness();
    if (led1_brightness + pwm_step > PWM_TOP_VALUE || led1_brightness  + pwm_step < 0) {
        pwm_step *= -1;
    }
    set_led1_brightness(led1_brightness + pwm_step);
}

void ble_write_evt(ble_evt_t const * p_ble_evt, void * p_context) {
    ble_gatts_evt_write_t const* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	uint16_t handle = p_evt_write->handle;

    if (handle == m_service_example.color_write_char.value_handle && p_evt_write->len > 2) {
        NRF_LOG_INFO("Set led2 by rgb on write event");
        rgb_data_t rgb = {.r = p_evt_write->data[0], .g = p_evt_write->data[1], .b = p_evt_write->data[2]};

        set_led2_color_by_rgb(&rgb);
    }
}

/* --- */

static void advertising_start(void);


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */


static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&led1_blink_timer, APP_TIMER_MODE_REPEATED, led1_blink_timer_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);
	APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    err_code = estc_ble_service_init(&m_service_example);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    /* Nothing to start */
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            NRF_LOG_INFO("Advert data: %s", m_advertising.adv_data.adv_data.p_data);
            NRF_LOG_INFO("SRP data %s", m_advertising.adv_data.scan_rsp_data.p_data);
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;
    rgb_data_t curr_rgb;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected.");
            // LED indication will be changed when advertising starts.
            break;

        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);

            // Update char
            curr_rgb = get_current_rgb_color();
            estc_ble_update_char(m_conn_handle, m_service_example.color_read_char.value_handle, 
                                 BLE_GATT_HVX_NOTIFICATION, (uint8_t*) &curr_rgb, sizeof(curr_rgb));
            break;
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_GATTS_EVT_WRITE:
            NRF_LOG_DEBUG("BLE Write event");
            ble_write_evt(p_ble_evt, p_context);
            break;
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
	init.srdata.uuids_complete.p_uuids = m_adv_uuids;
    
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
	LOG_BACKEND_USB_PROCESS();
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

void buttons_init() {
    button_control_init();
    button_interrupt_init(BUTTON1_ID, click_handler, release_handler);
}


void pm_evt_handler(pm_evt_t const * p_evt) {
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start();
            break;
        default:
            break;
    }
}

/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    log_init();
    timers_init();
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    nrfx_systick_init();
    pwm_control_init();
    nrfx_gpiote_init();
    buttons_init();
    fs_init();
    #if ESTC_USB_CLI_ENABLED == 1
        cli_init(commands_cli_listener);
        commands_init();
    #endif

    // Start execution.
    application_timers_start();
    advertising_start();
    
    /* Getting last saved hsv if it exists.*/
    fs_header_t *last_hsv_header = fs_find_record("last_hsv");
    hsv_data_t hsv;
    if (last_hsv_header == NULL) {
        hsv = new_hsv(360. * 77 / 100, 100, 100);
    } else {
       fs_read(last_hsv_header, &hsv, last_hsv_header->length);
    }
    set_led2_color_by_hsv(&hsv);
    led_color_was_color_changed(); /* Set color_was_changed = false */

    /* Init steps for hsv editing */
    int8_t hue_step = HUE_STEP;
    int8_t saturation_step = SATURATION_STEP;
    int8_t value_step = VALUE_STEP;

    // Enter main loop.
    for (;;)
    {
        #if ESTC_USB_CLI_ENABLED == 1
            cli_process();
            commands_process();
        #endif

        /* Hsv editing process */
        if (current_input_state == STATE_NO_INPUT) {
            if (led_color_was_color_changed()) {
                hsv = get_current_hsv_color();
                fs_write("last_hsv", &hsv, sizeof(hsv));

                if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
                    rgb_data_t curr_rgb = get_current_rgb_color();
                    estc_ble_update_char(m_conn_handle, m_service_example.color_read_char.value_handle, 
                                         BLE_GATT_HVX_NOTIFICATION, (uint8_t*) &curr_rgb, sizeof(curr_rgb));
                }
            }

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
        
        idle_state_handle();
    }
}


/**
 * @}
 */