#ifndef __HAL_SOFTIMER_H__
#define __HAL_SOFTIMER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   โหมดการทำงานของ soft timer
 */
typedef enum {
    HAL_SOFTIMER_ONESHOT  = 0,  /**< ทำงานครั้งเดียวแล้วหยุด */
    HAL_SOFTIMER_PERIODIC = 1,  /**< วนซ้ำทุกครั้งที่ครบกำหนด */
} hal_softimer_mode_t;

/**
 * @brief   โครงสร้าง soft timer (ใช้ hal_get_sys_tick() เป็นฐาน)
 *
 * ผู้ใช้ประกาศตัวแปร static หรือ global แล้วส่งพอยน์เตอร์ไปยังฟังก์ชัน
 * ไม่ต้อง init ฟังก์ชัน ใช้ hal_softimer_init() ก่อน start
 *
 * ตัวอย่าง:
 *   static hal_softimer_t tmr;
 *   hal_softimer_init(&tmr, HAL_SOFTIMER_PERIODIC);
 *   hal_softimer_start(&tmr, 500000);  // 500ms
 *   if (hal_softimer_expired(&tmr)) { hal_gpio_toggle(led); }
 */
typedef struct {
    uint32_t expire_tick;         /**< ค่า SysTick เป้าหมาย */
    uint32_t reload_ticks;        /**< จำนวน ticks reload (PERIODIC) */
    uint8_t  mode;                /**< HAL_SOFTIMER_ONESHOT / PERIODIC */
    uint8_t  running;             /**< 1 = กำลังนับ, 0 = หยุด */
} hal_softimer_t;

/**
 * @brief   เริ่มต้น soft timer
 *
 * @param   t    - พอยน์เตอร์ไปยัง hal_softimer_t
 * @param   mode - โหมด HAL_SOFTIMER_ONESHOT หรือ HAL_SOFTIMER_PERIODIC
 */
void hal_softimer_init(hal_softimer_t *t, hal_softimer_mode_t mode);

/**
 * @brief   เริ่มนับเวลา (ไม่บล็อก — CPU ทำงานต่อได้ทันที)
 *
 * @param   t        - พอยน์เตอร์ไปยัง hal_softimer_t
 * @param   delay_us - ระยะเวลาในหน่วยไมโครวินาที
 */
void hal_softimer_start(hal_softimer_t *t, uint32_t delay_us);

/**
 * @brief   หยุด soft timer (ยกเลิกการนับ)
 *
 * @param   t - พอยน์เตอร์ไปยัง hal_softimer_t
 */
void hal_softimer_stop(hal_softimer_t *t);

/**
 * @brief   ตรวจสอบว่า soft timer ครบกำหนดหรือยัง
 *
 * เรียกใน main loop ทุกรอบ
 * - ถ้าโหมด PERIODIC จะเริ่มนับรอบถัดไปอัตโนมัติ
 * - ถ้าโหมด ONESHOT จะหยุดนับและต้องเรียก hal_softimer_start() อีกครั้ง
 *
 * @param   t - พอยน์เตอร์ไปยัง hal_softimer_t
 *
 * @return  0 = ยังไม่ครบ, 1 = ครบกำหนด
 */
uint8_t hal_softimer_expired(hal_softimer_t *t);

#ifdef __cplusplus
}
#endif

#endif
