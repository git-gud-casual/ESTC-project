#include "cli.h"
#include "nrf_log.h"
#include <string.h>
#include <inttypes.h>

#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event);
                           
APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                            usb_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_NONE);


#define READ_SIZE 1
static char m_rx_buffer[READ_SIZE];

static struct line_buffer_s {
    char buff[LINE_BUFFER_SIZE];
    uint8_t current_index;
} line_buff_s = {.current_index = 0};

static void clear_line_buffer() {
    memset(line_buff_s.buff, '\0', LINE_BUFFER_SIZE);
    line_buff_s.current_index = 0;
}

static char line_copy[LINE_BUFFER_SIZE];


static newline_handler_t newline_handler;

void cli_init(newline_handler_t handler) {
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
    ret_code_t ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    newline_handler = handler;
}

void cli_process() {
    while (app_usbd_event_queue_process());
}

static bool is_writing;
void cli_write(const char* buff, size_t count) {
    is_writing = true;

    ret_code_t ret;
    do {
        ret = app_usbd_cdc_acm_write(&usb_cdc_acm, buff, count);
    } while (ret == NRF_ERROR_BUSY);
    while (is_writing) {
        cli_process();
    }
}

void cli_end_of_command_write() {
    cli_write("\r\nboard>", 8);
}


#define BACKSPACE_ASCII_CODE 127

static volatile bool is_first_rx_done;

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                    app_usbd_cdc_acm_user_event_t event)
{
    switch (event)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
    {
        NRF_LOG_INFO("USB OPENED");
        ret_code_t ret;
        ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
        UNUSED_VARIABLE(ret);

        is_first_rx_done = true;
        clear_line_buffer();
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
    {
        NRF_LOG_INFO("USB CLOSED");
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
    {
        NRF_LOG_INFO("TX DONE");
        is_writing = false;
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    {
        if (is_first_rx_done) {
            app_usbd_cdc_acm_write(&usb_cdc_acm, "board>", 6);
            is_first_rx_done = false;
        }
        NRF_LOG_INFO("RX DONE");
        ret_code_t ret;
        uint8_t offset = line_buff_s.current_index;
        bool is_newline = false;
        
        do
        {
            NRF_LOG_INFO("Message: %s", line_buff_s.buff);
            NRF_LOG_INFO("Current char %d", m_rx_buffer[0]);

            if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
            {
                is_newline = true;
            }
            else if (m_rx_buffer[0] == BACKSPACE_ASCII_CODE) {
                if (line_buff_s.current_index > 0) {
                    offset -= 1;
                    line_buff_s.buff[--line_buff_s.current_index] = '\0';
                    ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\b \b", 3);
                }
            }
            else if (line_buff_s.current_index + 1 < LINE_BUFFER_SIZE) {
                line_buff_s.buff[line_buff_s.current_index++] = m_rx_buffer[0];
            }

            ret = app_usbd_cdc_acm_read(&usb_cdc_acm,
                                        m_rx_buffer,
                                        READ_SIZE);
        } while (ret == NRF_SUCCESS);

        if (line_buff_s.current_index > offset) {
            ret = app_usbd_cdc_acm_write(&usb_cdc_acm,
                                        line_buff_s.buff + offset,
                                        line_buff_s.current_index - offset);
        }

        if (is_newline) {
            memcpy(line_copy, line_buff_s.buff, LINE_BUFFER_SIZE);
            newline_handler(line_copy);
            clear_line_buffer();
        }
        
        break;
    }
    default:
        break;
    }
}

