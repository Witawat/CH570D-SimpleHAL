#include "hal_clk.h"
#include "CH57x_common.h"

void hal_clk_set_sysclock(hal_sysclk_t clk)
{
    SetSysClock((SYS_CLKTypeDef)(uint8_t)clk);
}

uint32_t hal_clk_get_sysclock(void)
{
    return GetSysClock();
}

void hal_clk_hse_cfg_cap(uint8_t cap)
{
    HSECFG_Capacitance((HSECapTypeDef)cap);
}

void hal_clk_hse_cfg_current(uint8_t cur)
{
    HSECFG_Current((HSECurrentTypeDef)cur);
}
