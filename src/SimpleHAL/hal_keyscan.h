#ifndef __HAL_KEYSCAN_H__
#define __HAL_KEYSCAN_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_KEYSCAN_MAX_INSTANCES 1

typedef enum {
    HAL_KEYSCAN_DIV1  = 0x00,
    HAL_KEYSCAN_DIV2  = 0x10,
    HAL_KEYSCAN_DIV4  = 0x30,
    HAL_KEYSCAN_DIV8  = 0x70,
    HAL_KEYSCAN_DIV16 = 0xF0,
} hal_keyscan_div_t;

typedef struct hal_keyscan_obj *hal_keyscan_handle_t;

hal_keyscan_handle_t hal_keyscan_init(uint16_t pins, uint8_t clk_div, uint8_t repeat);
uint32_t             hal_keyscan_get_value(hal_keyscan_handle_t h);
uint32_t             hal_keyscan_get_count(hal_keyscan_handle_t h);
void                 hal_keyscan_attach_cb(hal_keyscan_handle_t h, hal_callback_t cb, void *arg);
void                 hal_keyscan_enable_wakeup(hal_keyscan_handle_t h, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif
