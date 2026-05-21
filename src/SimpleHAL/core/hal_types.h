#ifndef __HAL_TYPES_H__
#define __HAL_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   สถานะผลลัพธ์การทำงานของ HAL
 */
typedef enum {
    HAL_OK       = 0,  /**< ดำเนินการสำเร็จ */
    HAL_ERROR    = 1,  /**< เกิดข้อผิดพลาด */
    HAL_BUSY     = 2,  /**< กำลังทำงาน ไม่สามารถรับคำขอใหม่ได้ */
    HAL_TIMEOUT  = 3,  /**< รอการตอบสนองเกินเวลาที่กำหนด */
    HAL_INVALID  = 4,  /**< พารามิเตอร์หรือ handle ไม่ถูกต้อง */
} hal_status_t;

/**
 * @brief   ตัวชี้ฟังก์ชัน callback ทั่วไป
 * @param   arg     อาร์กิวเมนต์ที่ส่งให้ callback (กำหนดโดยผู้ใช้)
 */
typedef void (*hal_callback_t)(void *arg);

#ifdef __cplusplus
}
#endif

#endif
