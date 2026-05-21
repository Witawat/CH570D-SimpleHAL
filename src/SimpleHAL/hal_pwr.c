#include "hal_pwr.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      hal_pwr_idle
 *
 * @brief   โหมด Idle: หยุด CPU, peri ทั้งหมดทำงานปกติ
 *
 * @return  ไม่มี
 */
void hal_pwr_idle(void)
{
    LowPower_Idle();
}

/*********************************************************************
 * @fn      hal_pwr_halt
 *
 * @brief   โหมด Halt: หยุด CPU และ peri ทั้งหมด
 *
 * @return  ไม่มี
 */
void hal_pwr_halt(void)
{
    LowPower_Halt();
}

/*********************************************************************
 * @fn      hal_pwr_sleep
 *
 * @brief   โหมด Sleep: เก็บข้อมูลสำคัญใน retention_mask ก่อนหลับ
 *
 * @param   retention_mask - บิตมาสก์บอกข้อมูลที่ต้องเก็บไว้
 *
 * @return  ไม่มี
 */
void hal_pwr_sleep(uint16_t retention_mask)
{
    LowPower_Sleep(retention_mask);
}

/*********************************************************************
 * @fn      hal_pwr_shutdown
 *
 * @brief   โหมด Shutdown: ปิดระบบทั้งหมดยกเว้น RTC
 *
 * @param   retention_mask - บิตมาสก์บอกข้อมูลที่ต้องเก็บไว้
 *
 * @return  ไม่มี
 */
void hal_pwr_shutdown(uint16_t retention_mask)
{
    LowPower_Shutdown(retention_mask);
}

/*********************************************************************
 * @fn      hal_pwr_wakeup_cfg
 *
 * @brief   กำหนดแหล่งสัญญาณที่สามารถปลุก และหน่วงเวลาก่อนปลุก
 *
 * @param   source - แหล่งสัญญาณที่สามารถปลุก
 * @param   delay - ระยะหน่วงก่อนปลุก
 *
 * @return  ไม่มี
 */
void hal_pwr_wakeup_cfg(uint8_t source, hal_pwr_wake_delay_t delay)
{
    PWR_PeriphWakeUpCfg(ENABLE, source, (WakeUP_ModeypeDef)(uint8_t)delay);
}

/*********************************************************************
 * @fn      hal_pwr_periph_clk
 *
 * @brief   เปิดหรือปิดนาฬิกาให้ peri ที่ระบุ
 *
 * @param   enable - 1 = เปิด, 0 = ปิด
 * @param   periph_mask - บิตมาสก์บอก peri ที่ต้องการกำหนด
 *
 * @return  ไม่มี
 */
void hal_pwr_periph_clk(uint8_t enable, uint16_t periph_mask)
{
    PWR_PeriphClkCfg(enable ? ENABLE : DISABLE, periph_mask);
}
