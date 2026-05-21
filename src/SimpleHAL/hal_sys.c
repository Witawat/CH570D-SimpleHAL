#include "hal_sys.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      hal_delay_ms
 *
 * @brief   หน่วงเวลาตามจำนวนมิลลิวินาทีที่กำหนด
 *
 * @param   ms - จำนวนมิลลิวินาทีที่ต้องการหน่วง
 */
void hal_delay_ms(uint16_t ms)
{
    mDelaymS(ms);
}

/*********************************************************************
 * @fn      hal_delay_us
 *
 * @brief   หน่วงเวลาตามจำนวนไมโครวินาทีที่กำหนด
 *
 * @param   us - จำนวนไมโครวินาทีที่ต้องการหน่วง
 */
void hal_delay_us(uint16_t us)
{
    mDelayuS(us);
}

/*********************************************************************
 * @fn      hal_reset
 *
 * @brief   รีเซ็ตชิพโดยเรียก SYS_ResetExecute
 */
void hal_reset(void)
{
    SYS_ResetExecute();
}

/*********************************************************************
 * @fn      hal_get_sys_tick
 *
 * @brief   อ่านค่า sys tick ปัจจุบัน
 *
 * @return  ค่า sys tick ปัจจุบัน
 */
uint32_t hal_get_sys_tick(void)
{
    return SYS_GetSysTickCnt();
}

/*********************************************************************
 * @fn      hal_get_reset_status
 *
 * @brief   อ่านสาเหตุการรีเซ็ตครั้งล่าสุด
 *
 * @return  สาเหตุการรีเซ็ตครั้งล่าสุด
 */
uint8_t hal_get_reset_status(void)
{
    return SYS_GetLastResetSta();
}

/*********************************************************************
 * @fn      hal_wdt_init
 *
 * @brief   เริ่มต้น watchdog: กำหนด counter เริ่มต้น
 *
 * @param   counter - ค่าเริ่มต้นให้ counter
 */
void hal_wdt_init(uint8_t counter)
{
    WWDG_SetCounter(counter);
}

/*********************************************************************
 * @fn      hal_wdt_feed
 *
 * @brief   ป้อนค่า watchdog ใหม่เพื่อกันไม่ให้รีเซ็ต
 *
 * @param   counter - ค่า counter ที่ต้องการป้อน
 */
void hal_wdt_feed(uint8_t counter)
{
    WWDG_SetCounter(counter);
}

/*********************************************************************
 * @fn      hal_wdt_enable_irq
 *
 * @brief   เปิดหรือปิดการขัดจังหวะของ watchdog
 *
 * @param   enable - 1 = เปิด, 0 = ปิด
 */
void hal_wdt_enable_irq(uint8_t enable)
{
    WWDG_ITCfg(enable ? ENABLE : DISABLE);
}

/*********************************************************************
 * @fn      hal_wdt_enable_reset
 *
 * @brief   เปิดหรือปิดการรีเซ็ตเมื่อ watchdog timeout
 *
 * @param   enable - 1 = เปิด, 0 = ปิด
 */
void hal_wdt_enable_reset(uint8_t enable)
{
    WWDG_ResetCfg(enable ? ENABLE : DISABLE);
}

/*********************************************************************
 * @fn      hal_wdt_clear_flag
 *
 * @brief   ล้าง flag สถานะของ watchdog
 */
void hal_wdt_clear_flag(void)
{
    WWDG_ClearFlag();
}
