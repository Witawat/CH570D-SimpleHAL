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

/**
 * @brief   โหมดการโฆษณา BLE
 */
typedef enum {
    HAL_BLE_ADV_MODE_CONNECTABLE = 0,
    HAL_BLE_ADV_MODE_NON_CONNECTABLE,
} hal_ble_adv_mode_t;

typedef struct hal_ble_obj *hal_ble_handle_t;

/**
 * @brief   เริ่มต้น BLE Peripheral: ตั้งชื่ออุปกรณ์และกำลังส่ง
 *
 * @param   device_name - ชื่ออุปกรณ์
 * @param   tx_power - กำลังส่ง
 *
 * @return  handle ของ BLE
 */
hal_ble_handle_t hal_ble_init(const char *device_name, uint8_t tx_power);
/**
 * @brief   เริ่มโฆษณาตัวเอง (advertise)
 *
 * @param   h - handle ของ BLE
 * @param   mode - โหมดการโฆษณา
 *
 * @return  ไม่มี
 */
void             hal_ble_advertise_start(hal_ble_handle_t h, hal_ble_adv_mode_t mode);
/**
 * @brief   หยุดโฆษณา
 *
 * @param   h - handle ของ BLE
 *
 * @return  ไม่มี
 */
void             hal_ble_advertise_stop(hal_ble_handle_t h);
/**
 * @brief   ส่งข้อมูลไปยังอุปกรณ์ที่เชื่อมต่ออยู่
 *
 * @param   h - handle ของ BLE
 * @param   data - ข้อมูลที่จะส่ง
 * @param   len - ความยาวข้อมูล
 *
 * @return  สถานะการส่ง
 */
hal_status_t     hal_ble_send(hal_ble_handle_t h, const uint8_t *data, uint16_t len);
/**
 * @brief   ผูก callback เมื่อมีการเชื่อมต่อ
 *
 * @param   h - handle ของ BLE
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void             hal_ble_attach_connect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   ผูก callback เมื่อขาดการเชื่อมต่อ
 *
 * @param   h - handle ของ BLE
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void             hal_ble_attach_disconnect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   ผูก callback เมื่อได้รับข้อมูล
 *
 * @param   h - handle ของ BLE
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void             hal_ble_attach_data_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   ตรวจสอบว่ากำลังเชื่อมต่ออยู่หรือไม่
 *
 * @param   h - handle ของ BLE
 *
 * @return  1 = เชื่อมต่ออยู่, 0 = ไม่ได้เชื่อมต่อ
 */
uint8_t          hal_ble_is_connected(hal_ble_handle_t h);
/**
 * @brief   ประมวลผล event BLE (เรียกใน loop หลัก)
 *
 * @return  ไม่มี
 */
void             hal_ble_process(void);

#ifdef __cplusplus
}
#endif

#endif
