#ifndef __HAL_SYS_H__
#define __HAL_SYS_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void     hal_delay_ms(uint16_t ms);
void     hal_delay_us(uint16_t us);
void     hal_reset(void);
uint32_t hal_get_sys_tick(void);
uint8_t  hal_get_reset_status(void);
void     hal_wdt_init(uint8_t counter);
void     hal_wdt_feed(uint8_t counter);
void     hal_wdt_enable_irq(uint8_t enable);
void     hal_wdt_enable_reset(uint8_t enable);
void     hal_wdt_clear_flag(void);

#ifdef __cplusplus
}
#endif

#endif
