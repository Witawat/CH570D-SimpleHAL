#ifndef __HAL_RF_H__
#define __HAL_RF_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_RF_MAX_INSTANCES 1

/**
 * @brief   โหมด PHY ของ RF
 */
typedef enum {
    HAL_RF_PHY_1M  = 0,
    HAL_RF_PHY_2M  = 1,
    HAL_RF_PHY_2G4 = 2,
} hal_rf_phy_mode_t;

typedef struct hal_rf_obj *hal_rf_handle_t;

/**
 * @brief   เริ่มต้น RF: กำหนดความถี่, PHY, กำลังส่ง
 *
 * @param   frequency_khz - ความถี่ในหน่วย kHz
 * @param   phy - โหมด PHY
 * @param   tx_power_dbm - กำลังส่งในหน่วย dBm
 *
 * @return  handle ของ RF
 */
hal_rf_handle_t hal_rf_init(uint32_t frequency_khz, hal_rf_phy_mode_t phy, int8_t tx_power_dbm);
/**
 * @brief   ส่งข้อมูล (blocking)
 *
 * @param   h - handle ของ RF
 * @param   data - ข้อมูลที่จะส่ง
 * @param   len - ความยาวข้อมูล
 *
 * @return  สถานะการส่ง
 */
hal_status_t    hal_rf_send(hal_rf_handle_t h, const uint8_t *data, uint16_t len);
/**
 * @brief   รับข้อมูล (blocking, ระบุ timeout)
 *
 * @param   h - handle ของ RF
 * @param   buf - buffer สำหรับรับข้อมูล
 * @param   max_len - ขนาดสูงสุดของ buffer
 * @param   out_len - ตัวชี้สำหรับเก็บความยาวที่รับได้
 * @param   rssi - ตัวชี้สำหรับเก็บค่า RSSI
 * @param   timeout_us - timeout ในหน่วยไมโครวินาที
 *
 * @return  สถานะการรับ
 */
hal_status_t    hal_rf_recv(hal_rf_handle_t h, uint8_t *buf, uint16_t max_len, uint16_t *out_len, int8_t *rssi, uint32_t timeout_us);
/**
 * @brief   เปลี่ยนกำลังส่ง
 *
 * @param   h - handle ของ RF
 * @param   power_dbm - กำลังส่งในหน่วย dBm
 *
 * @return  ไม่มี
 */
void            hal_rf_set_tx_power(hal_rf_handle_t h, int8_t power_dbm);
/**
 * @brief   ผูก callback เมื่อได้รับข้อมูล
 *
 * @param   h - handle ของ RF
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void            hal_rf_attach_rx_cb(hal_rf_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   หยุด RF
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void            hal_rf_stop(hal_rf_handle_t h);
/**
 * @brief   สั่ง RF เข้าโหมด sleep
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void            hal_rf_sleep(hal_rf_handle_t h);
/**
 * @brief   ปลุก RF จาก sleep
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void            hal_rf_wake(hal_rf_handle_t h);
/**
 * @brief   เปลี่ยนช่องความถี่
 *
 * @param   h - handle ของ RF
 * @param   frequency_khz - ความถี่ในหน่วย kHz
 *
 * @return  ไม่มี
 */
void            hal_rf_set_channel(hal_rf_handle_t h, uint32_t frequency_khz);
/**
 * @brief   calibrate RF
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void            hal_rf_calibrate(hal_rf_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
