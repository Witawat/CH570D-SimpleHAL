#ifndef __HAL_PWR_H__
#define __HAL_PWR_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   โหมดหน่วงเวลาหลังตื่นจาก sleep — ค่าตัวเลข = รอบนาฬิกา
 */
typedef enum {
    HAL_PWR_WAKE_DELAY_3584 = 0,
    HAL_PWR_WAKE_DELAY_512,
    HAL_PWR_WAKE_DELAY_64,
    HAL_PWR_WAKE_DELAY_1,
    HAL_PWR_WAKE_DELAY_8191,
    HAL_PWR_WAKE_DELAY_7168,
    HAL_PWR_WAKE_DELAY_6144,
    HAL_PWR_WAKE_DELAY_4096,
} hal_pwr_wake_delay_t;

/**
 * @brief   โหมดประหยัดพลังงาน Idle (หยุด CPU แต่ peri ทำงานต่อ)
 *
 * @return  ไม่มี
 */
void hal_pwr_idle(void);
/**
 * @brief   โหมด Halt (หยุด CPU และ peri)
 *
 * @return  ไม่มี
 */
void hal_pwr_halt(void);
/**
 * @brief   โหมด Sleep (เตรียมข้อมูลไว้ก่อนหลับ)
 *
 * @param   retention_mask - บิตมาสก์บอกข้อมูลที่ต้องเก็บไว้ก่อนหลับ
 *
 * @return  ไม่มี
 */
void hal_pwr_sleep(uint16_t retention_mask);
/**
 * @brief   โหมด Shutdown (ปิดระบบ เหลือแค่ RTC เท่านั้น)
 *
 * @param   retention_mask - บิตมาสก์บอกข้อมูลที่ต้องเก็บไว้
 *
 * @return  ไม่มี
 */
void hal_pwr_shutdown(uint16_t retention_mask);
/**
 * @brief   กำหนดแหล่งปลุกและระยะหน่วงก่อนปลุก
 *
 * @param   source - แหล่งสัญญาณที่สามารถปลุก
 * @param   delay - ระยะหน่วงก่อนปลุก
 *
 * @return  ไม่มี
 */
void hal_pwr_wakeup_cfg(uint8_t source, hal_pwr_wake_delay_t delay);
/**
 * @brief   เปิด/ปิดสัญญาณนาฬิกาให้ peri ที่เลือก
 *
 * @param   enable - 1 = เปิด, 0 = ปิด
 * @param   periph_mask - บิตมาสก์บอก peri ที่ต้องการกำหนด
 *
 * @return  ไม่มี
 */
void hal_pwr_periph_clk(uint8_t enable, uint16_t periph_mask);

#ifdef __cplusplus
}
#endif

#endif
