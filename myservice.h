#ifndef MYSERVICE_
#define MYSERVICE_

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

// TODO: 1. Generate random BLE UUID (Version 4 UUID) and define it in the following format:
// #define ESTC_BASE_UUID { 0xF6, 0xCE, 0x0F, 0xC4, 0xCE, 0x9F, /* - */ 0xC3, 0x99, /* - */ 0xF7, 0x4D, /* - */ 0xDB, 0xB9, /* - */ 0x00, 0x00, 0xEC, 0x39 } // UUID: EC39xxxx-B9DB-4DF7-99C3-9FCEC40FCEF6

#define ESTC_BASE_UUID {0x42, 0xff, 0x53, 0x72, /* - */ 0x83, 0xe1, /* - */ 0x4d, 0x91, /* - */ 0xb5, 0x30, /* - */ 0x6b, 0x16, 0x60, 0x75, 0xfb, 0xaf}

// TODO: 2. Pick a random service 16-bit UUID and define it:
// #define ESTC_SERVICE_UUID 0xabcd
#define ESTC_SERVICE_UUID 0x5bda


typedef struct
{
    uint16_t service_handle;
} ble_estc_service_t;

ret_code_t estc_ble_service_init(ble_estc_service_t *service);

#endif