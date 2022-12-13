#ifndef _COMMANDS_MODULE
#define _COMMANDS_MODULE

#include <stdlib.h>

#include "nrf_log.h"

#include "../color_types/color_types.h"
#include "../cli/cli.h"
#include "../led_color/led_color.h"


#define COMMANDS_COUNT 3

void commands_init();
void commands_cli_listener(char* line);
void commands_process();

#endif