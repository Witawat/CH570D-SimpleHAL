#ifndef __HAL_SYS_H__
#define __HAL_SYS_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   หน่วงเวลาเป็นมิลลิวินาที
 *
 * @param   ms - จำนวนมิลลิวินาทีที่ต้องการหน่วง
 */
void     hal_delay_ms(uint16_t ms);
/**
 * @brief   หน่วงเวลาเป็นไมโครวินาที
 *
 * @param   us - จำนวนไมโครวินาทีที่ต้องการหน่วง
 */
void     hal_delay_us(uint16_t us);
/**
 * @brief   รีเซ็ตระบบ
 */
void     hal_reset(void);
/**
 * @brief   อ่านค่านับเวลาระบบ (sys tick)
 *
 * @return  ค่า sys tick ปัจจุบัน
 */
uint32_t hal_get_sys_tick(void);
/**
 * @brief   อ่านสถานะการรีเซ็ตครั้งล่าสุด
 *
 * @return  สถานะการรีเซ็ต
 */
uint8_t  hal_get_reset_status(void);
/**
 * @brief   เริ่มต้น watchdog timer ด้วยค่า counter เริ่มต้น
 *
 * @param   counter - ค่าเริ่มต้นให้ counter
 */
void     hal_wdt_init(uint8_t counter);
/**
 * @brief   ป้อนค่า counter ให้ watchdog (กัน reset)
 *
 * @param   counter - ค่า counter ที่ต้องการป้อน
 */
void     hal_wdt_feed(uint8_t counter);
/**
 * @brief   เปิด/ปิดขัดจังหวะ watchdog
 *
 * @param   enable - 1 = เปิด, 0 = ปิด
 */
void     hal_wdt_enable_irq(uint8_t enable);
/**
 * @brief   เปิด/ปิดการรีเซ็ตเมื่อ watchdog timeout
 *
 * @param   enable - 1 = เปิด, 0 = ปิด
 */
void     hal_wdt_enable_reset(uint8_t enable);
/**
 * @brief   ล้าง flag ของ watchdog
 */
void     hal_wdt_clear_flag(void);

#ifdef __cplusplus
}
#endif

#endif
