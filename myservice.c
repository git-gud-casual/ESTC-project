#include "myservice.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    ble_uuid_t service_uuid;
    service_uuid.uuid = ESTC_SERVICE_UUID;
    
    // TODO: 3. Add service UUIDs to the BLE stack table using `sd_ble_uuid_vs_add`
    ble_uuid128_t ble_uuid = {.uuid128=ESTC_BASE_UUID};
    error_code = sd_ble_uuid_vs_add(&ble_uuid, &service_uuid.type);
    APP_ERROR_CHECK(error_code);

    // TODO: 4. Add service to the BLE stack using `sd_ble_gatts_service_add`
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle);
    APP_ERROR_CHECK(error_code);

    NRF_LOG_DEBUG("Service UUID: 0x%04x", service_uuid.uuid);
    NRF_LOG_DEBUG("Service UUID type: 0x%02x", service_uuid.type);
    NRF_LOG_DEBUG("Service handle: 0x%04x", service->service_handle);

    return error_code;
}
