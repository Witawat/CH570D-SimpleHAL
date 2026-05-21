#include "hal_clk.h"
#include "CH57x_common.h"

/*********************************************************************
 * @fn      hal_clk_set_sysclock
 *
 * @brief   ตั้งค่าความถี่นาฬิการะบบตามที่ระบุ
 *
 * @param   clk - ค่าความถี่นาฬิกา
 *
 * @return  ไม่มี
 */
void hal_clk_set_sysclock(hal_sysclk_t clk)
{
    SetSysClock((SYS_CLKTypeDef)(uint8_t)clk);
}

/*********************************************************************
 * @fn      hal_clk_get_sysclock
 *
 * @brief   อ่านความถี่นาฬิการะบบปัจจุบัน คืนค่า Hz
 *
 * @return  ความถี่นาฬิกาปัจจุบัน (Hz)
 */
uint32_t hal_clk_get_sysclock(void)
{
    return GetSysClock();
}

/*********************************************************************
 * @fn      hal_clk_hse_cfg_cap
 *
 * @brief   กำหนดค่าความจุ capacitor ของ HSE
 *
 * @param   cap - ค่าความจุ
 *
 * @return  ไม่มี
 */
void hal_clk_hse_cfg_cap(uint8_t cap)
{
    HSECFG_Capacitance((HSECapTypeDef)cap);
}

/*********************************************************************
 * @fn      hal_clk_hse_cfg_current
 *
 * @brief   กำหนดค่ากระแสของ HSE oscillator
 *
 * @param   cur - ค่ากระแส
 *
 * @return  ไม่มี
 */
void hal_clk_hse_cfg_current(uint8_t cur)
{
    HSECFG_Current((HSECurrentTypeDef)cur);
}
