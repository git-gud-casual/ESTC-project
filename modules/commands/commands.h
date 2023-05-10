#ifndef _COMMANDS_MODULE
#define _COMMANDS_MODULE

#include <stdlib.h>
#include <inttypes.h>

#include "nrf_log.h"

#include "../color_types/color_types.h"
#include "../cli/cli.h"
#include "../led_color/led_color.h"
#include "../fs/fs.h"


#define COMMANDS_COUNT 8

#define INVALID_ARGUMENTS_MSG "\r\nInvalid arguments"
#define UNKNOWN_COMMAND_MSG "\r\nUnknown command"
#define COMMAND_DOESNT_ACCEPT_ARGS_MSG "\r\nCommand doesn`t accept any arguments"
#define COLOR_DOESNT_FOUND_MSG "\r\nColor doesn`t found"
#define COLOR_DELETED_MSG "\r\nColor deleted"
#define COLOR_SET_MSG "\r\nColor set"
#define COLOR_SAVED_MSG "\r\nColor saved"
#define COLOR_NAME_EXCEEDS_SIZE "\r\nColor name size can`t be bigger than 31"
#define CANT_FIND_ANY_SAVED_COLORS_MSG "\r\nCan`t find any saved colors"

#define HELP_COMMAND_NAME "help"
#define HELP_HELP_MSG "\r\nhelp - print information about available commands"

#define RGB_COMMAND_NAME "RGB"
#define RGB_HELP_MSG "\r\nRGB <r> <g> <b> - set led2 color by RGB model"

#define HSV_COMMAND_NAME "HSV"
#define HSV_HELP_MSG "\r\nHSV <h> <s> <v> - set led2 color by HSV model"

#define ADD_RGB_COMMAND_NAME "add_rgb_color"
#define ADD_RGB_HELP_MSG "\r\nadd_rgb_color <r> <g> <b> <color_name> - save rgb color with name"

#define LIST_COLORS_COMMAND_NAME "list_colors"
#define LIST_COLORS_HELP_MSG "\r\nlist_colors - print every saved colors"

#define ADD_CURRENT_COLOR_COMMAND_NAME "add_current_color"
#define ADD_CURRENT_COLOR_HELP_MSG "\r\nadd_current_color <color_name> - save current color with name"

#define APPLY_COLOR_COMMAND_NAME "apply_color"
#define APPLY_COLOR_HELP_MSG "\r\napply_color <color_name> - set led2 with saved <color_name> color"

#define DEL_COLOR_COMMAND_NAME "del_color"
#define DEL_COLOR_HELP_MSG "\r\ndel_color <color_name> - delete <color_name> color"



void commands_init();
void commands_cli_listener(char* line);
void commands_process();

#endif