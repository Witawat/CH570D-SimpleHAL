#include "hal_pwr.h"
#include "CH57x_common.h"

void hal_pwr_idle(void)
{
    LowPower_Idle();
}

void hal_pwr_halt(void)
{
    LowPower_Halt();
}

void hal_pwr_sleep(uint16_t retention_mask)
{
    LowPower_Sleep(retention_mask);
}

void hal_pwr_shutdown(uint16_t retention_mask)
{
    LowPower_Shutdown(retention_mask);
}

void hal_pwr_wakeup_cfg(uint8_t source, hal_pwr_wake_delay_t delay)
{
    PWR_PeriphWakeUpCfg(ENABLE, source, (WakeUP_ModeypeDef)(uint8_t)delay);
}

void hal_pwr_periph_clk(uint8_t enable, uint16_t periph_mask)
{
    PWR_PeriphClkCfg(enable ? ENABLE : DISABLE, periph_mask);
}
