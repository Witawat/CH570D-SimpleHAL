#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_TIMER_MAX_INSTANCES 1
#define HAL_TIMER_MAX_PERIOD    67108864

/**
 * @brief   โหมดการทำงานของ Timer
 */
typedef enum {
    HAL_TIMER_MODE_PERIODIC = 0,  /* ทำงานซ้ำทุก period */
    HAL_TIMER_MODE_ONE_SHOT,      /* ทำงานครั้งเดียวแล้วหยุด */
} hal_timer_mode_t;

typedef struct hal_timer_obj *hal_timer_handle_t;

/**
 * @brief   เริ่มต้น Timer ด้วยคาบเวลา (ไมโครวินาที) และโหมด
 *
 * @param   period_us - คาบเวลาในหน่วยไมโครวินาที
 * @param   mode - โหมดการทำงาน (PERIODIC หรือ ONE_SHOT)
 *
 * @return  handle ของ Timer หรือ NULL หากผิดพลาด
 */
hal_timer_handle_t hal_timer_init(uint32_t period_us, hal_timer_mode_t mode);
/**
 * @brief   เริ่มจับเวลา (เปิด interrupt และ enable timer)
 *
 * @param   h - handle ของ Timer
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t       hal_timer_start(hal_timer_handle_t h);
/**
 * @brief   หยุดจับเวลา
 *
 * @param   h - handle ของ Timer
 */
void               hal_timer_stop(hal_timer_handle_t h);
/**
 * @brief   รีเซ็ต Timer กลับค่าเริ่มต้น
 *
 * @param   h - handle ของ Timer
 */
void               hal_timer_reset(hal_timer_handle_t h);
/**
 * @brief   เปลี่ยนคาบเวลา (ไมโครวินาที) ขณะทำงาน
 *
 * @param   h - handle ของ Timer
 * @param   period_us - คาบเวลาใหม่ในหน่วยไมโครวินาที
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t       hal_timer_set_period(hal_timer_handle_t h, uint32_t period_us);
/**
 * @brief   อ่านค่านับปัจจุบันของ Timer
 *
 * @param   h - handle ของ Timer
 *
 * @return  ค่านับปัจจุบัน
 */
uint32_t           hal_timer_get_count(hal_timer_handle_t h);
/**
 * @brief   ผูก callback สำหรับเมื่อ Timer ครบรอบ
 *
 * @param   h - handle ของ Timer
 * @param   cb - ฟังก์ชัน callback
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void               hal_timer_attach_cb(hal_timer_handle_t h, hal_callback_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif
