#include "ble_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"


static ret_code_t estc_ble_add_characteristics(ble_estc_service_t*, ble_gatts_char_handles_t*, 
                                               uint16_t, uint8_t*, uint16_t, uint8_t, char*);

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;
    uint8_t rgb_default_data[3] = {0};

    ble_uuid_t service_uuid;
    service_uuid.uuid = ESTC_SERVICE_UUID;
    
    // Add service` UUID to BLE stack table
    ble_uuid128_t ble_uuid = {.uuid128=ESTC_BASE_UUID};
    error_code = sd_ble_uuid_vs_add(&ble_uuid, &service_uuid.type);
    APP_ERROR_CHECK(error_code);

    // Add service to BLE stack
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle);
    APP_ERROR_CHECK(error_code);

    NRF_LOG_DEBUG("Service UUID: 0x%04x", service_uuid.uuid);
    NRF_LOG_DEBUG("Service UUID type: 0x%02x", service_uuid.type);
    NRF_LOG_DEBUG("Service handle: 0x%04x", service->service_handle);

    // Configure led_color_read_char
    error_code = estc_ble_add_characteristics(service, &service->color_read_char, ESTC_COLOR_READ_CHAR_UUID, rgb_default_data, sizeof(rgb_default_data), 
                                              ESTC_READ_PROPERTY | ESTC_NOTIFY_PROPERTY, ESTC_COLOR_READ_CHAR_DESC);
    APP_ERROR_CHECK(error_code);

    // Configure led_color_write_char
    error_code = estc_ble_add_characteristics(service, &service->color_write_char, ESTC_COLOR_WRITE_CHAR_UUID, rgb_default_data, sizeof(rgb_default_data), 
                                              ESTC_WRITE_PROPERTY, ESTC_COLOR_WRITE_CHAR_DESC);
    APP_ERROR_CHECK(error_code);

    return error_code;
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, ble_gatts_char_handles_t *char_handle, uint16_t uuid,
                                               uint8_t *p_val, uint16_t val_len, uint8_t char_properties, char *cdesc)
{
    ret_code_t error_code = NRF_SUCCESS;

    // Set char metadata
    ble_gatts_char_md_t char_md = {0};
    char_md.char_props.read = char_properties & ESTC_READ_PROPERTY ? 1 : 0;
    char_md.char_props.write = char_properties & ESTC_WRITE_PROPERTY ? 1 : 0;
    char_md.char_props.indicate = char_properties & ESTC_INDICATE_PROPERTY ? 1 : 0;
    char_md.char_props.notify = char_properties & ESTC_NOTIFY_PROPERTY ? 1 : 0;

    // Set char description
    if (cdesc != NULL) {
        ble_srv_utf8_str_t utf8_str = {0};
        ble_srv_ascii_to_utf8(&utf8_str, cdesc);
        char_md.p_char_user_desc = utf8_str.p_str;
        char_md.char_user_desc_size = utf8_str.length;
        char_md.char_user_desc_max_size = utf8_str.length;
    }

    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_STACK;

    // Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // Configure uuid, default value
    ble_uuid_t char_uuid = {.uuid = uuid, .type=BLE_UUID_TYPE_VENDOR_BEGIN};
    ble_gatts_attr_t attr_char_value = {.init_len = val_len, .max_len = val_len, .p_value = p_val};
    attr_char_value.p_uuid = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;
    char_md.p_cccd_md = &attr_md;

    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`
    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, char_handle);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}

static bool _can_send_indication = true;

void estc_ble_indication_confirms() {
    _can_send_indication = true;
}

ret_code_t estc_ble_update_char(uint16_t conn_handle, uint16_t char_handle, 
                                uint8_t type, uint8_t *p_val, uint16_t length) {
    if (!_can_send_indication && type == BLE_GATT_HVX_INDICATION) {
        return 0;
    }
    ble_gatts_hvx_params_t hvx_params = {
        .type = type,
        .p_len = &length,
        .p_data = p_val,
        .offset = 0,
        .handle = char_handle
    };

    ret_code_t err_code = sd_ble_gatts_hvx(conn_handle, &hvx_params);
    _can_send_indication =  err_code != NRF_SUCCESS;

    return err_code;
}