#include "commands.h"


typedef void (*command_handler)(char* args);

typedef struct {
    const char* command;
    command_handler handler;
} cli_command_t;


static size_t get_args_count(char* str) {
    size_t count = 0;
    bool was_char_except_for_space = false;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ') {
            was_char_except_for_space = true;
        }
        else if (was_char_except_for_space && str[i] == ' ') {
            count++;
            was_char_except_for_space = false;
        }
    }

    if (was_char_except_for_space) {
        count++;
    }
    return count;
}

typedef struct {
    uint32_t value;
    bool error;
} get_uint_ret_t;

static get_uint_ret_t get_uint_from_str(char* str, size_t uint_pos) {
    char* last_symbol_after_digit = str;
    uint32_t value = 0;
    while (*last_symbol_after_digit != '\0') {
        value = strtoul(last_symbol_after_digit, &last_symbol_after_digit, 10);
        if ((value != 0 || last_symbol_after_digit != str) &&
            (*last_symbol_after_digit == ' ' || *last_symbol_after_digit == '\0')) {
            if (uint_pos == 0) {
                return (get_uint_ret_t) {value, false};
            }
            uint_pos--;
        }
        else {
            break;
        }
        str = last_symbol_after_digit;
    }

    return (get_uint_ret_t) {value, true};
}


static void help_handler(char* args) {
    NRF_LOG_INFO("Help args: %s", args);
    if (get_args_count(args) > 0) {
        cli_write("\r\nCommand help doesn`t accept any arguments", 43);
        return;
    }
    cli_write("\r\nRGB <r> <g> <b> - set led2 color by RGB model"
              "\r\nHSV <h> <s> <v> - set led2 color by HSV model", 94);
}

static void rgb_handler(char* args) {
    NRF_LOG_INFO("RGB args: %s", args);
    if (get_args_count(args) != 3) {
        cli_write("\r\nInvalid arguments", 19);
        return;
    }

    uint16_t rgb_vals[3];

    get_uint_ret_t ret;
    for (size_t i = 0; i < 3; i++) {
        ret = get_uint_from_str(args, i);
        if (ret.error || ret.value > 255) {
            cli_write("\r\nInvalid arguments", 19);
            return;
        }
        rgb_vals[i] = ret.value;
    }

    rgb_data_t rgb = new_rgb(rgb_vals[0], rgb_vals[1], rgb_vals[2]);
    set_led2_color_by_rgb(&rgb);
    cli_write("\r\nColor set", 11);
}

static void hsv_handler(char* args) {
    NRF_LOG_INFO("HSV args: %s", args);
    if (get_args_count(args) != 3) {
        cli_write("\r\nInvalid arguments", 19);
        return;
    }

    uint16_t hsv_vals[3];

    get_uint_ret_t ret;
    for (size_t i = 0; i < 3; i++) {
        ret = get_uint_from_str(args, i);
        if (ret.error || (i == 0 && ret.value > 359) ||
            (i > 0 && ret.value > 100)) 
        {
            cli_write("\r\nInvalid arguments", 19);
            return;
        }
        hsv_vals[i] = ret.value;
    }

    hsv_data_t hsv = new_hsv(hsv_vals[0], hsv_vals[1], hsv_vals[2]);
    set_led2_color_by_hsv(&hsv);
    cli_write("\r\nColor set", 11);
}

static cli_command_t commands[COMMANDS_COUNT] = {
    {
        .command = "help",
        .handler = help_handler
    },
    {
        .command = "RGB",
        .handler = rgb_handler
    },
    {
        .command = "HSV",
        .handler = hsv_handler
    }
};

static char* command;
void commands_cli_listener(char* line) {
    command = line;
}

static void parse_command() {
    size_t command_name_size;

    for (size_t i = 0; i < COMMANDS_COUNT; i++) {
        command_name_size = strlen(commands[i].command);
        if (strncmp(command, commands[i].command, command_name_size) == 0 && 
            strlen(command) >= command_name_size && 
            (command[command_name_size] == ' ' || command[command_name_size] == '\0')) 
        {
            NRF_LOG_INFO("Execute command %s", command);
            commands[i].handler(command + command_name_size + 1);
            return;
        }
    }
    cli_write("\r\nUnknown command", 17);
}

void commands_process() {
    if (command != NULL) {
        if (command[0] != '\0') {
            parse_command();
        }
        cli_end_of_command_write();
        command = NULL;
    }
}

void commands_init() {
    pwm_control_init();
}