#ifndef __HAL_CLK_H__
#define __HAL_CLK_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   แหล่งสัญญาณนาฬิกาสำหรับระบบ
 */
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

/**
 * @brief   ตั้งค่าความถี่นาฬิการะบบ
 *
 * @param   clk - ค่าความถี่นาฬิกาที่ต้องการตั้ง
 *
 * @return  ไม่มี
 */
void     hal_clk_set_sysclock(hal_sysclk_t clk);
/**
 * @brief   อ่านความถี่นาฬิการะบบปัจจุบัน
 *
 * @return  ความถี่นาฬิกาปัจจุบัน (หน่วย Hz)
 */
uint32_t hal_clk_get_sysclock(void);
/**
 * @brief   กำหนดค่าความจุ capacitor สำหรับ HSE oscillator
 *
 * @param   cap - ค่าความจุที่ต้องการกำหนด
 *
 * @return  ไม่มี
 */
void     hal_clk_hse_cfg_cap(uint8_t cap);
/**
 * @brief   กำหนดค่ากระแสสำหรับ HSE oscillator
 *
 * @param   cur - ค่ากระแสที่ต้องการกำหนด
 *
 * @return  ไม่มี
 */
void     hal_clk_hse_cfg_current(uint8_t cur);

#ifdef __cplusplus
}
#endif

#endif
