#include "hal_sys.h"
#include "CH57x_common.h"

void hal_delay_ms(uint16_t ms)
{
    mDelaymS(ms);
}

void hal_delay_us(uint16_t us)
{
    mDelayuS(us);
}

void hal_reset(void)
{
    SYS_ResetExecute();
}

uint32_t hal_get_sys_tick(void)
{
    return SYS_GetSysTickCnt();
}

uint8_t hal_get_reset_status(void)
{
    return SYS_GetLastResetSta();
}

void hal_wdt_init(uint8_t counter)
{
    WWDG_SetCounter(counter);
}

void hal_wdt_feed(uint8_t counter)
{
    WWDG_SetCounter(counter);
}

void hal_wdt_enable_irq(uint8_t enable)
{
    WWDG_ITCfg(enable ? ENABLE : DISABLE);
}

void hal_wdt_enable_reset(uint8_t enable)
{
    WWDG_ResetCfg(enable ? ENABLE : DISABLE);
}

void hal_wdt_clear_flag(void)
{
    WWDG_ClearFlag();
}
