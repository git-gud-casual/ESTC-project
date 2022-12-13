#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>


#if ESTC_USB_CLI_ENABLED == 1
    #include "modules/cli/cli.h"
    #include "modules/commands/commands.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"


void init_board() {
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    #if ESTC_USB_CLI_ENABLED == 1
        cli_init(commands_cli_listener);
        commands_init();
    #endif
}


void main_loop() {
    
    while (true) {
        #if ESTC_USB_CLI_ENABLED == 1
            cli_process();
            commands_process();
        #endif
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
    
}

int main(void) {
    init_board();
    main_loop();
}
