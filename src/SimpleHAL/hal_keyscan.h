#ifndef __HAL_KEYSCAN_H__
#define __HAL_KEYSCAN_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_KEYSCAN_MAX_INSTANCES 1

/**
 * @brief   ค่า divider สำหรับ Key Scan clock
 */
typedef enum {
    HAL_KEYSCAN_DIV1  = 0x00,
    HAL_KEYSCAN_DIV2  = 0x10,
    HAL_KEYSCAN_DIV4  = 0x30,
    HAL_KEYSCAN_DIV8  = 0x70,
    HAL_KEYSCAN_DIV16 = 0xF0,
} hal_keyscan_div_t;

typedef struct hal_keyscan_obj *hal_keyscan_handle_t;

/**
 * @brief   เริ่มต้น Key Scan: กำหนดขา, clock divider, ค่า repeat
 *
 * @param   pins - ขาที่ใช้สำหรับ Key Scan
 * @param   clk_div - ค่า clock divider
 * @param   repeat - ค่า repeat
 *
 * @return  handle ของ Key Scan
 */
hal_keyscan_handle_t hal_keyscan_init(uint16_t pins, uint8_t clk_div, uint8_t repeat);
/**
 * @brief   อ่านค่าปุ่มที่กดล่าสุด
 *
 * @param   h - handle ของ Key Scan
 *
 * @return  ค่าปุ่มที่กดล่าสุด
 */
uint32_t             hal_keyscan_get_value(hal_keyscan_handle_t h);
/**
 * @brief   อ่านจำนวนครั้งที่กดปุ่ม
 *
 * @param   h - handle ของ Key Scan
 *
 * @return  จำนวนครั้งที่กดปุ่ม
 */
uint32_t             hal_keyscan_get_count(hal_keyscan_handle_t h);
/**
 * @brief   ผูก callback เมื่อมีการกดปุ่ม
 *
 * @param   h - handle ของ Key Scan
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void                 hal_keyscan_attach_cb(hal_keyscan_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   เปิด/ปิดการปลุกจากปุ่มกด
 *
 * @param   h - handle ของ Key Scan
 * @param   enable - 1 = เปิด, 0 = ปิด
 *
 * @return  ไม่มี
 */
void                 hal_keyscan_enable_wakeup(hal_keyscan_handle_t h, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif
