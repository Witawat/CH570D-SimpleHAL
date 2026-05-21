#ifndef __HAL_PWR_H__
#define __HAL_PWR_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_PWR_WAKE_DELAY_3584 = 0,
    HAL_PWR_WAKE_DELAY_512,
    HAL_PWR_WAKE_DELAY_64,
    HAL_PWR_WAKE_DELAY_1,
    HAL_PWR_WAKE_DELAY_8191,
    HAL_PWR_WAKE_DELAY_7168,
    HAL_PWR_WAKE_DELAY_6144,
    HAL_PWR_WAKE_DELAY_4096,
} hal_pwr_wake_delay_t;

void hal_pwr_idle(void);
void hal_pwr_halt(void);
void hal_pwr_sleep(uint16_t retention_mask);
void hal_pwr_shutdown(uint16_t retention_mask);
void hal_pwr_wakeup_cfg(uint8_t source, hal_pwr_wake_delay_t delay);
void hal_pwr_periph_clk(uint8_t enable, uint16_t periph_mask);

#ifdef __cplusplus
}
#endif

#endif
