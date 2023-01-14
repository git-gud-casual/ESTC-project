#ifndef _CLI_MODULE
#define _CLI_MODULE

#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_cdc_acm.h"

#define LINE_BUFFER_SIZE 100


typedef void (*newline_handler_t)(char* line);

void cli_init(newline_handler_t);
void cli_process();
void cli_write(const char* buff, size_t count);
void cli_end_of_command_write();
#endif
