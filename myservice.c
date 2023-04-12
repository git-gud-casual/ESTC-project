#include "myservice.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    service->service_uuid.uuid = ESTC_SERVICE_UUID;
    
    // TODO: 3. Add service UUIDs to the BLE stack table using `sd_ble_uuid_vs_add`
    ble_uuid128_t ble_uuid = {.uuid128=ESTC_BASE_UUID};
    error_code = sd_ble_uuid_vs_add(&ble_uuid, &service->service_uuid.type);
    APP_ERROR_CHECK(error_code);

    // TODO: 4. Add service to the BLE stack using `sd_ble_gatts_service_add`
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service->service_uuid, &service->service_handle);
    APP_ERROR_CHECK(error_code);

    NRF_LOG_DEBUG("Service UUID: 0x%04x", service->service_uuid.uuid);
    NRF_LOG_DEBUG("Service UUID type: 0x%02x", service->service_uuid.type);
    NRF_LOG_DEBUG("Service handle: 0x%04x", service->service_handle);

    return estc_ble_add_characteristics(service);
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    // TODO: 6.5. Configure Characteristic metadata (enable read and write)
    ble_gatts_char_md_t char_md = {0};
    char_md.char_props = (ble_gatt_char_props_t) {.read = 1, .write = 1};

    ble_srv_utf8_str_t utf8_str = {0};
    ble_srv_ascii_to_utf8(&utf8_str, ESTC_CHAR_1_DESC);
    char_md.p_char_user_desc = utf8_str.p_str;
    char_md.char_user_desc_size = utf8_str.length;
    char_md.char_user_desc_max_size = utf8_str.length;

    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_STACK;

    // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);


    // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    ble_uuid_t char_uuid = {.uuid = ESTC_GATT_CHAR_1_UUID, .type = BLE_UUID_TYPE_BLE};

    ble_gatts_attr_t attr_char_value = { 0 };

    attr_char_value.p_uuid = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;


    // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure
    attr_char_value.init_len = 2;
    attr_char_value.max_len = 2;

    uint8_t val[2] = {0xC0, 0xDE};
    attr_char_value.p_value = val;


    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`
    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->char1_handle);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}