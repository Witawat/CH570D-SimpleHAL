#include "hal_softimer.h"
#include "hal_sys.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      hal_softimer_init
 *
 * @brief   เริ่มต้น soft timer กำหนดโหมด ONESHOT หรือ PERIODIC
 *
 * @param   t    - พอยน์เตอร์ไปยัง hal_softimer_t
 * @param   mode - โหมดการทำงาน
 */
void hal_softimer_init(hal_softimer_t *t, hal_softimer_mode_t mode)
{
    t->expire_tick  = 0;
    t->reload_ticks = 0;
    t->mode         = (uint8_t)mode;
    t->running      = 0;
}

/*********************************************************************
 * @fn      hal_softimer_start
 *
 * @brief   เริ่มนับเวลา (ไม่บล็อก)
 *
 * แปลง delay_us เป็นจำนวน ticks โดยใช้ FREQ_SYS เพื่อคำนวณ
 * ticks ต่อไมโครวินาที แล้วบันทึกค่า SysTick เป้าหมาย
 *
 * @param   t        - พอยน์เตอร์ไปยัง hal_softimer_t
 * @param   delay_us - ระยะเวลาในหน่วยไมโครวินาที
 */
void hal_softimer_start(hal_softimer_t *t, uint32_t delay_us)
{
    uint32_t ticks_per_us = FREQ_SYS / 1000000;
    uint32_t delay_ticks  = delay_us * ticks_per_us;

    t->expire_tick  = hal_get_sys_tick() + delay_ticks;
    t->reload_ticks = delay_ticks;
    t->running      = 1;
}

/*********************************************************************
 * @fn      hal_softimer_stop
 *
 * @brief   หยุด soft timer
 *
 * @param   t - พอยน์เตอร์ไปยัง hal_softimer_t
 */
void hal_softimer_stop(hal_softimer_t *t)
{
    t->running = 0;
}

/*********************************************************************
 * @fn      hal_softimer_expired
 *
 * @brief   ตรวจสอบว่า soft timer ครบกำหนดหรือยัง
 *
 * ใช้ (int32_t)(now - expire_tick) >= 0 เพื่อรองรับ wraparound
 * ของ SysTick counter ที่ 32-bit (~43 วินาทีที่ 100MHz)
 *
 * @param   t - พอยน์เตอร์ไปยัง hal_softimer_t
 *
 * @return  0 = ยังไม่ครบ, 1 = ครบกำหนด
 */
uint8_t hal_softimer_expired(hal_softimer_t *t)
{
    if (!t->running) return 0;

    uint32_t now = hal_get_sys_tick();
    if ((int32_t)(now - t->expire_tick) >= 0) {
        if (t->mode == HAL_SOFTIMER_PERIODIC) {
            t->expire_tick = now + t->reload_ticks;
        } else {
            t->running = 0;
        }
        return 1;
    }
    return 0;
}
