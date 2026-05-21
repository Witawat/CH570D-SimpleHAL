#ifndef __HAL_BLE_H__
#define __HAL_BLE_H__

#include <stdint.h>
#include "core/hal_types.h"
#include "CH572BLEPeri_LIB.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_BLE_MAX_INSTANCES 1
#define HAL_BLE_DEVICE_NAME_MAX 16

typedef enum {
    HAL_BLE_ADV_MODE_CONNECTABLE = 0,
    HAL_BLE_ADV_MODE_NON_CONNECTABLE,
} hal_ble_adv_mode_t;

typedef struct hal_ble_obj *hal_ble_handle_t;

hal_ble_handle_t hal_ble_init(const char *device_name, uint8_t tx_power);
void             hal_ble_advertise_start(hal_ble_handle_t h, hal_ble_adv_mode_t mode);
void             hal_ble_advertise_stop(hal_ble_handle_t h);
hal_status_t     hal_ble_send(hal_ble_handle_t h, const uint8_t *data, uint16_t len);
void             hal_ble_attach_connect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
void             hal_ble_attach_disconnect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
void             hal_ble_attach_data_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
uint8_t          hal_ble_is_connected(hal_ble_handle_t h);
void             hal_ble_process(void);

#ifdef __cplusplus
}
#endif

#endif
