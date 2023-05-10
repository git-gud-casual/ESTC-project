#include "fs.h"

#include "nrf_log.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "nrf_soc.h"
#include <string.h>
#include <inttypes.h>

#define FS_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define FS_MAX(a, b) (((a) < (b) ? (b) : (a)))

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage_instance) = {
    .evt_handler = NULL,
    .start_addr = APP_DATA_ADDR,
    .end_addr = BOOTLOADER_ADDR
};

/* Wait function */
static void fs_wait() {
    while (nrf_fstorage_is_busy(&fstorage_instance)) {
        sd_app_evt_wait();
    }
}


// Id starts from 1. 
static uint8_t max_id = 0;
static int8_t curr_page = -1;

/*
    Crc8 functions impl
*/

static uint8_t crc8(uint8_t *data_block, size_t length) {
    uint8_t crc = 0xFF;
    uint8_t i;

    while (length--) {
        crc ^= *data_block++;

        for (i = 0; i < 8; i++) {
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
        }
    }

    return crc;
}

static bool check_crc(fs_header_t *phead) {
    uint8_t crc = crc8((uint8_t*)phead + FS_HEADER_SIZE_BYTES, phead->length);
    return phead->_crc8 == crc;
}

/*
    Next fs_header_t function impl
*/

static size_t get_rounded_length(size_t length) {
    /*
        Rounds length to top number multiples of 4
    */

    if (length % 4 == 0) {
        return length;
    }

   return length + (4 - length % 4);
}

static bool is_page_erased(uint32_t page_addr);

static void init_page() {
    ret_code_t err_code;

    fs_header_t *phead;
    for (int8_t page_index = 0; page_index < 3; page_index++) {
        phead = (fs_header_t*)(APP_DATA_ADDR + CODE_PAGE_SIZE * page_index);
        if (!(phead->id & phead->nid) && check_crc(phead)) {
            curr_page = page_index;
        }
    }

    if (curr_page == -1) {
        NRF_LOG_INFO("Page %" PRIi8, curr_page);
        if (!is_page_erased(APP_DATA_ADDR)) {
            NRF_LOG_INFO("Erased");
            err_code = nrf_fstorage_erase(&fstorage_instance, APP_DATA_ADDR, 1, NULL);
            APP_ERROR_CHECK(err_code);
        }
        curr_page = 0;
    }
}

static fs_header_t *next_header(fs_header_t *phead) {
    if (curr_page == -1) {
        NRF_LOG_INFO("Init page");
        init_page();
    }
    if (phead == NULL) {
        phead = (fs_header_t*)(APP_DATA_ADDR + CODE_PAGE_SIZE * curr_page);
    }
    else {
        phead = (fs_header_t*)((uint8_t*)phead + FS_HEADER_SIZE_BYTES + get_rounded_length(phead->length));

        if ((uintptr_t) phead >= BOOTLOADER_ADDR || (uintptr_t) phead < APP_DATA_ADDR) {
            return NULL;
        }
    }

    if (!(phead->id & phead->nid) && check_crc(phead)) {
        NRF_LOG_INFO("fs_next_header: Find header at 0x%" PRIXPTR " with name \"%s\"", (uintptr_t) phead, phead->record_name);
        return phead;
    }
    return NULL;
}

/*
    Find record function impl
*/


fs_header_t *fs_find_record(char *name) {
    NRF_LOG_INFO("fs_find_record: Try to find record \"%s\"", name);

    fs_header_t *phead = NULL;
    for (fs_header_t *curr_phead = next_header(NULL); curr_phead != NULL; curr_phead = next_header(curr_phead)) {
        if ((phead != NULL && phead->id == curr_phead->id) ||
            strcmp(curr_phead->record_name, name) == 0) {
            phead = curr_phead;
        }
    }
    
    if (phead != NULL && phead->length == 0) {
        return NULL;
    }
    return phead;
}

ret_code_t fs_read(fs_header_t *phead, void *dest, size_t bytes_count) {
    if ((uintptr_t) phead >= APP_DATA_ADDR && (uintptr_t) phead < BOOTLOADER_ADDR) {
        NRF_LOG_INFO("fs_read: Reading data");
        bytes_count = FS_MIN(bytes_count, phead->length);

        memcpy(dest, (void*)((uint8_t*)phead + FS_HEADER_SIZE_BYTES), bytes_count);
        return NRF_SUCCESS;
    }
    NRF_LOG_INFO("fs_read: Invalid pointer to fs_header_t");
    return NRF_ERROR_INVALID_PARAM;
}

/* 
    Write funtion impl
*/

static fs_header_t *get_last_record() {
    NRF_LOG_INFO("fs_get_last_record: Try to find last_record");

    fs_header_t *last_phead = NULL;
    for (fs_header_t *curr_phead = next_header(NULL); curr_phead != NULL; curr_phead = next_header(curr_phead)) {
        last_phead = curr_phead;
    }
    return last_phead;
}

static bool is_page_erased(uint32_t page_addr) {
    for (uint32_t word_addr = page_addr; word_addr - page_addr < CODE_PAGE_SIZE; word_addr += WORD_SIZE) {
        if (*(uint32_t*)word_addr != 0xffffffff) {
            return false;
        }
    }
    NRF_LOG_INFO("page erased")
    return true;
}

static fs_header_t *transition_to_new_page() {
    /*
        Endures newest records to new page.
        Returns pointer to last record.
    */
    uint8_t new_page = (curr_page + 1) % 3;
    uintptr_t new_page_addr = APP_DATA_ADDR + CODE_PAGE_SIZE * new_page;

    ret_code_t err_code;

    if (!is_page_erased(new_page_addr)) {
        nrf_fstorage_erase(&fstorage_instance, new_page_addr, 1, NULL);
        fs_wait();
    }

    uint32_t ids_was[256 / 32];
    memset(ids_was, 0, sizeof(ids_was));

    fs_header_t *phead = next_header(NULL);
    fs_header_t *last_file_version;
    fs_header_t *last_record = NULL;

    while (phead != NULL) {
        last_file_version = fs_find_record(phead->record_name);
        
        if (last_file_version != NULL) {
            size_t index = last_file_version->id / 32;
            uint8_t shift = last_file_version->id % 32;

            if ((ids_was[index] >> shift & 1) == 0) {
                ids_was[index] = ids_was[index] | (1 << shift);

                err_code = nrf_fstorage_write(&fstorage_instance, new_page_addr, (uint32_t*) last_file_version->_val, FS_HEADER_SIZE_BYTES, NULL);
                APP_ERROR_CHECK(err_code);
                fs_wait();
                last_record = (fs_header_t*) new_page_addr;
                new_page_addr += FS_HEADER_SIZE_BYTES;

                uint32_t *words = (uint32_t*)((uint8_t*)last_file_version + FS_HEADER_SIZE_BYTES);
                err_code = nrf_fstorage_write(&fstorage_instance, new_page_addr, words, get_rounded_length(last_file_version->length), NULL);
                APP_ERROR_CHECK(err_code);
                fs_wait();

                new_page_addr += get_rounded_length(last_file_version->length);
            }
        }
        phead = next_header(phead);
    }
    err_code = nrf_fstorage_erase(&fstorage_instance, APP_DATA_ADDR + CODE_PAGE_SIZE * curr_page, 1, NULL);
    APP_ERROR_CHECK(err_code);
    fs_wait();
    curr_page = new_page;
    return last_record;
}

static bool is_enough_space(fs_header_t *last_record, size_t bytes_to_write) {
    uintptr_t new_addr = (uintptr_t)((uint8_t*)last_record + get_rounded_length(last_record->length) + FS_HEADER_SIZE_BYTES * 2 
        + get_rounded_length(bytes_to_write));

    uint32_t addr_right_border = curr_page < 2 ? CODE_PAGE_SIZE * (curr_page + 1) + APP_DATA_ADDR : BOOTLOADER_ADDR;
    
    return new_addr + bytes_to_write < addr_right_border;
}

static uint8_t get_record_max_id() {
    if (max_id == 0) {
        for (fs_header_t *phead = next_header(NULL); phead != NULL; phead = next_header(phead)) {
            max_id = FS_MAX(max_id, phead->id);
        }
    }
    return max_id;
}

static fs_header_t new_header(char *name, void *src, size_t bytes_count) {
    fs_header_t head = {0};

    fs_header_t *record_to_rewrite;
    if ((record_to_rewrite = fs_find_record(name)) == NULL) {
        if (get_record_max_id() == 0xFF) {
            NRF_LOG_INFO("fs_write: MAX_ID is 255");
        }
        else {
            head.id = get_record_max_id() + 1;
        }
    }
    else {
        head.id = record_to_rewrite->id;
    }
    
    head.nid = ~head.id;
    head.length = bytes_count;
    head._crc8 = crc8((uint8_t*) src, bytes_count);
    strcpy(head.record_name, name);

    return head;
}

fs_header_t *fs_write(char* record_name, void *src, size_t bytes_count) {
    if (strlen(record_name) > RECORDNAME_MAX_LENGTH) {
        NRF_LOG_INFO("fs_write: name \"%s\" length exceeds RECORDNAME_MAX_LENGTH", record_name);
        return NULL;
    }

    fs_header_t *last_record = get_last_record();
    if (last_record != NULL && !is_enough_space(last_record, bytes_count)) {
        last_record = transition_to_new_page();
        if (!is_enough_space(last_record, bytes_count)) {
            NRF_LOG_WARNING("fs_write: Not enough space");
            return NULL;
        }
    }

    ret_code_t err_code;
    fs_header_t head = new_header(record_name, src, bytes_count);
    uint32_t write_addr = last_record != NULL ? (uint32_t)last_record + get_rounded_length(last_record->length) + FS_HEADER_SIZE_BYTES : APP_DATA_ADDR + CODE_PAGE_SIZE * curr_page;

    fs_header_t *phead = (fs_header_t*) write_addr;
    
    err_code = nrf_fstorage_write(&fstorage_instance, write_addr, head._val, FS_HEADER_SIZE_BYTES, NULL);
    APP_ERROR_CHECK(err_code);
    fs_wait();

    write_addr += FS_HEADER_SIZE_BYTES;

    if (bytes_count > 0) {
        uint32_t words[get_rounded_length(bytes_count) / WORD_SIZE];
        memcpy(&words, src, bytes_count);
        err_code = nrf_fstorage_write(&fstorage_instance, write_addr, words, get_rounded_length(bytes_count), NULL);
        APP_ERROR_CHECK(err_code);
        fs_wait();
    }
    
    if (head.id > get_record_max_id()) {
        max_id = head.id;
    }
    return phead;
}


ret_code_t fs_delete(fs_header_t* header) {
    if ((uintptr_t) header < APP_DATA_ADDR || (uintptr_t) header >= BOOTLOADER_ADDR) {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (fs_write(header->record_name, NULL, 0) != NULL) {
        return NRF_SUCCESS;
    }
    return NRF_ERROR_BASE_NUM;
}

ret_code_t fs_format() {
    ret_code_t err_code = nrf_fstorage_erase(&fstorage_instance, APP_DATA_ADDR, 3, NULL);
    fs_wait();
    return err_code;
}

ret_code_t fs_init() {
    return nrf_fstorage_init(&fstorage_instance, &nrf_fstorage_sd, NULL);
}