#ifndef MYSERVICE_
#define MYSERVICE_

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

#define ESTC_BASE_UUID { 0xF6, 0xCE, 0x0F, 0xC4, 0xCE, 0x9F, /* - */ 0xC3, 0x99, /* - */ 0xF7, 0x4D, /* - */ 0xDB, 0xB9, /* - */ 0x00, 0x00, 0xEC, 0x39 }
#define ESTC_SERVICE_UUID 0x5bda

#define ESTC_COLOR_WRITE_CHAR_UUID 0x0001
#define ESTC_COLOR_READ_CHAR_UUID  0x0002

#define ESTC_COLOR_READ_CHAR_DESC  "LED color read"
#define ESTC_COLOR_WRITE_CHAR_DESC "LED color write"

#define ESTC_READ_PROPERTY 0b00000001
#define ESTC_WRITE_PROPERTY (ESTC_READ_PROPERTY << 1)
#define ESTC_NOTIFY_PROPERTY (ESTC_READ_PROPERTY << 2)
#define ESTC_INDICATE_PROPERTY (ESTC_READ_PROPERTY << 3)

typedef struct
{
    uint16_t service_handle;

    ble_gatts_char_handles_t color_write_char;
    ble_gatts_char_handles_t color_read_char;
} ble_estc_service_t;

ret_code_t estc_ble_service_init(ble_estc_service_t *service);
ret_code_t estc_ble_update_char(uint16_t conn_handle, uint16_t char_handle, 
                                uint8_t type, uint8_t *p_val, uint16_t length);
void estc_ble_indication_confirms();

#endif