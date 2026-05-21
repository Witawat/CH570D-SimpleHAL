#ifndef __HAL_CLK_H__
#define __HAL_CLK_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_CLK_LSI            = 0xC0,
    HAL_CLK_HSE_16MHz      = 0x02,
    HAL_CLK_HSE_8MHz       = 0x04,
    HAL_CLK_HSE_4MHz       = 0x08,
    HAL_CLK_PLL_100MHz     = 0x46,
    HAL_CLK_PLL_75MHz      = 0x48,
    HAL_CLK_PLL_60MHz      = 0x4A,
    HAL_CLK_PLL_50MHz      = 0x4C,
    HAL_CLK_PLL_40MHz      = 0x4F,
    HAL_CLK_PLL_30MHz      = 0x54,
    HAL_CLK_PLL_25MHz      = 0x58,
    HAL_CLK_PLL_24MHz      = 0x59,
    HAL_CLK_PLL_20MHz      = 0x5E,
} hal_sysclk_t;

void     hal_clk_set_sysclock(hal_sysclk_t clk);
uint32_t hal_clk_get_sysclock(void);
void     hal_clk_hse_cfg_cap(uint8_t cap);
void     hal_clk_hse_cfg_current(uint8_t cur);

#ifdef __cplusplus
}
#endif

#endif
