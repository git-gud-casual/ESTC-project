#ifndef _FS
#define _FS


#include "nrf_dfu_types.h"


#include <stdbool.h>
#include <stdint.h>


#define BOOTLOADER_ADDR 0xE0000
#define APP_DATA_ADDR (BOOTLOADER_ADDR - NRF_DFU_APP_DATA_AREA_SIZE)
#define WORD_SIZE 4

#define RECORDNAME_MAX_LENGTH 24
#define FS_HEADER_SIZE_BYTES 36


typedef union {
    uint8_t _val[FS_HEADER_SIZE_BYTES];

    struct {
        uint8_t id;
        uint8_t nid; // inverted id
        char record_name[RECORDNAME_MAX_LENGTH + 1]; // 1 byte for \0
        size_t length;
        uint8_t _crc8;
    };
} fs_header_t;


fs_header_t *fs_find_record(char *record_name);
ret_code_t fs_read(fs_header_t *header, void* dest, size_t bytes_count);
fs_header_t *fs_write(char *record_name, void *src, size_t bytes_count);
ret_code_t fs_delete(fs_header_t *header);
ret_code_t fs_format();

ret_code_t fs_init();


#endif