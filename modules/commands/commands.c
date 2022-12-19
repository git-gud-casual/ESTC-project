#include "commands.h"

typedef void (*command_handler)(char* args);

typedef struct {
    const char* command;
    command_handler handler;
    const char* help_str;
} cli_command_t;


static void send_msg_to_cli(const char* msg) {
    cli_write(msg, strlen(msg));
}

static char* get_begin_of_word(char* str, size_t word_pos) {
    bool was_space = true;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ' && was_space) {
            was_space = false;
            if (word_pos == 0) {
                return str + i;
            }
            word_pos--;
        }
        else if (!was_space && str[i] == ' ') {
            was_space = true;
        }
    }
    return NULL;
}

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

static void rgb_handler(char* args) {
    NRF_LOG_INFO("RGB args: %s", args);
    if (get_args_count(args) != 3) {
        send_msg_to_cli(INVALID_ARGUMENTS_MSG);
        return;
    }

    uint16_t rgb_vals[3];

    get_uint_ret_t ret;
    for (size_t i = 0; i < 3; i++) {
        ret = get_uint_from_str(args, i);
        if (ret.error || ret.value > 255) {
            send_msg_to_cli(INVALID_ARGUMENTS_MSG);
            return;
        }
        rgb_vals[i] = ret.value;
    }

    rgb_data_t rgb = new_rgb(rgb_vals[0], rgb_vals[1], rgb_vals[2]);
    set_led2_color_by_rgb(&rgb);
    send_msg_to_cli(COLOR_SET_MSG);
}

static void hsv_handler(char* args) {
    NRF_LOG_INFO("HSV args: %s", args);
    if (get_args_count(args) != 3) {
        send_msg_to_cli(INVALID_ARGUMENTS_MSG);
        return;
    }

    uint16_t hsv_vals[3];

    get_uint_ret_t ret;
    for (size_t i = 0; i < 3; i++) {
        ret = get_uint_from_str(args, i);
        if (ret.error || (i == 0 && ret.value > 359) ||
            (i > 0 && ret.value > 100)) 
        {
            send_msg_to_cli(INVALID_ARGUMENTS_MSG);
            return;
        }
        hsv_vals[i] = ret.value;
    }

    hsv_data_t hsv = new_hsv(hsv_vals[0], hsv_vals[1], hsv_vals[2]);
    rgb_data_t rgb = get_rgb_from_hsv(&hsv);
    set_led2_color_by_rgb(&rgb);
    send_msg_to_cli(COLOR_SET_MSG);
}

static void add_rgb_color(char* args) {
    NRF_LOG_INFO("add_rgb_color args %s: ", args);
    if (get_args_count(args) != 4) {
        send_msg_to_cli(INVALID_ARGUMENTS_MSG);
        return;
    }

    uint16_t rgb_vals[3];

    get_uint_ret_t ret;
    for (size_t i = 0; i < 3; i++) {
        ret = get_uint_from_str(args, i);
        if (ret.error || ret.value > 255) {
            send_msg_to_cli(INVALID_ARGUMENTS_MSG);
            return;
        }
        rgb_vals[i] = ret.value;
    }

    char* name = get_begin_of_word(args, 3);
    NRF_LOG_INFO("Color name %s", name);
    char* end_of_name = strchr(name, ' ');
    if (end_of_name == NULL) {
        end_of_name = strchr(name, '\0');
    }
    else {
        *end_of_name = '\0';
    }

    ptrdiff_t name_length = end_of_name - name;
    if (name_length > COLOR_NAME_SIZE) {
        send_msg_to_cli(COLOR_NAME_EXCEEDS_SIZE);
        return;
    }

    rgb_data_array_t rgb_array = get_last_saved_rgb_array();
    rgb_data_with_name_t rgb_data = new_rgb_with_name(new_rgb(rgb_vals[0], rgb_vals[1], rgb_vals[2]), name, name_length);
    put_rgb_in_array(&rgb_array, &rgb_data);
    save_colors_array(&rgb_array);
    send_msg_to_cli(COLOR_SAVED_MSG);
}

static void list_colors(char* args) {
    NRF_LOG_INFO("list_colors args: %s", args);
    if (get_args_count(args) > 0) {
        send_msg_to_cli(COMMAND_DOESNT_ACCEPT_ARGS_MSG);
        return;
    }

    rgb_data_array_t rgb_array = get_last_saved_rgb_array();
    NRF_LOG_INFO("Colors count %" PRIu32, rgb_array.count);
    if (rgb_array.count == 0) {
        send_msg_to_cli(CANT_FIND_ANY_SAVED_COLORS_MSG);
        return;
    }

    char* unformatted_str = "\r\nColor name: %s";
    char formatted_str[strlen(unformatted_str) + COLOR_NAME_SIZE];
    for (size_t i = 0; i < rgb_array.count; i++) {
        sprintf(formatted_str, unformatted_str, rgb_array.colors_array[i].color_name);
        cli_write(formatted_str, strlen(unformatted_str) + strlen(rgb_array.colors_array[i].color_name) - 1);
    }
}

static void add_current_color(char* args) {
    NRF_LOG_INFO("add_current_color args: %s", args);
    if (get_args_count(args) != 1) {
        send_msg_to_cli(INVALID_ARGUMENTS_MSG);
        return;
    }

    char* name = get_begin_of_word(args, 0);
    NRF_LOG_INFO("Color name %s", name);
    char* end_of_name = strchr(name, ' ');
    if (end_of_name == NULL) {
        end_of_name = strchr(name, '\0');
    }
    else {
        *end_of_name = '\0';
    }

    ptrdiff_t name_length = end_of_name - name;
    if (name_length > COLOR_NAME_SIZE - 1) {
        send_msg_to_cli(COLOR_NAME_EXCEEDS_SIZE);
        return;
    }

    rgb_data_array_t rgb_array = get_last_saved_rgb_array();
    hsv_data_t hsv_color = get_current_color();

    rgb_data_with_name_t rgb_data = new_rgb_with_name(get_rgb_from_hsv(&hsv_color), name, name_length);
    put_rgb_in_array(&rgb_array, &rgb_data);
    save_colors_array(&rgb_array);
    send_msg_to_cli(COLOR_SAVED_MSG);
}

static void apply_color(char* args) {
    NRF_LOG_INFO("apply_color args: %s", args);
    if (get_args_count(args) != 1) {
        send_msg_to_cli(INVALID_ARGUMENTS_MSG);
        return;
    }

    char* name = get_begin_of_word(args, 0);
    NRF_LOG_INFO("Color name %s", name);
    char* end_of_name = strchr(name, ' ');
    if (end_of_name == NULL) {
        end_of_name = strchr(name, '\0');
    }

    ptrdiff_t name_length = end_of_name - name;
    rgb_data_array_t rgb_array = get_last_saved_rgb_array();
    for (size_t i = 0; i < rgb_array.count; i++) {
        if (strncmp(rgb_array.colors_array[i].color_name, name, name_length) == 0 && 
            (name[name_length] == ' ' || name[name_length] == '\0') && 
            strlen(name) >= strlen(rgb_array.colors_array[i].color_name)) 
            {
                
                set_led2_color_by_rgb(&rgb_array.colors_array[i].rgb);
                send_msg_to_cli(COLOR_SET_MSG);
                return;
            }
    }
    send_msg_to_cli(COLOR_DOESNT_FOUND_MSG);
}

static void del_color(char* args) {
    NRF_LOG_INFO("del_color args: %s", args);
    if (get_args_count(args) != 1) {
        send_msg_to_cli(INVALID_ARGUMENTS_MSG);
        return;
    }

    char* name = get_begin_of_word(args, 0);
    NRF_LOG_INFO("Color name %s", name);
    char* end_of_name = strchr(name, ' ');
    if (end_of_name == NULL) {
        end_of_name = strchr(name, '\0');
    }

    ptrdiff_t name_length = end_of_name - name;
    rgb_data_array_t rgb_array = get_last_saved_rgb_array();
    for (size_t i = 0; i < rgb_array.count; i++) {
        if (strncmp(rgb_array.colors_array[i].color_name, name, name_length) == 0 && 
            (name[name_length] == ' ' || name[name_length] == '\0') && strlen(name) >= strlen(rgb_array.colors_array[i].color_name)) 
            {
                delete_color_from_array(&rgb_array, i);
                save_colors_array(&rgb_array);
                send_msg_to_cli(COLOR_DELETED_MSG);
                return;
            }
    }
    send_msg_to_cli(COLOR_DOESNT_FOUND_MSG);
}

static void help_handler(char* args);

static cli_command_t commands[COMMANDS_COUNT] = {
    {
        .command = HELP_COMMAND_NAME,
        .handler = help_handler,
        .help_str = HELP_HELP_MSG
    },
    {
        .command = RGB_COMMAND_NAME,
        .handler = rgb_handler,
        .help_str = RGB_HELP_MSG
    },
    {
        .command = HSV_COMMAND_NAME,
        .handler = hsv_handler,
        .help_str = HSV_HELP_MSG
    },
    {
        .command = ADD_RGB_COMMAND_NAME,
        .handler = add_rgb_color,
        .help_str = ADD_RGB_HELP_MSG
    },
    {
        .command = LIST_COLORS_COMMAND_NAME,
        .handler = list_colors,
        .help_str = LIST_COLORS_HELP_MSG
    },
    {
        .command = ADD_CURRENT_COLOR_COMMAND_NAME,
        .handler = add_current_color,
        .help_str = ADD_CURRENT_COLOR_HELP_MSG
    },
    {
        .command = APPLY_COLOR_COMMAND_NAME,
        .handler = apply_color,
        .help_str = APPLY_COLOR_HELP_MSG
    },
    {
        .command = DEL_COLOR_COMMAND_NAME,
        .handler = del_color,
        .help_str = DEL_COLOR_HELP_MSG
    }
};


static void help_handler(char* args) {
    NRF_LOG_INFO("Help args: %s", args);
    if (get_args_count(args) > 0) {
        send_msg_to_cli(COMMAND_DOESNT_ACCEPT_ARGS_MSG);
        return;
    }
    
    for (size_t i = 0; i < COMMANDS_COUNT; i++) {
        send_msg_to_cli(commands[i].help_str);
    }
}

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
    cli_write(UNKNOWN_COMMAND_MSG, strlen(UNKNOWN_COMMAND_MSG));
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
    init_nmvc();
}