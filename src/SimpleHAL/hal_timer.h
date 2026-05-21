#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_TIMER_MAX_INSTANCES 1
#define HAL_TIMER_MAX_PERIOD    67108864

typedef enum {
    HAL_TIMER_MODE_PERIODIC = 0,
    HAL_TIMER_MODE_ONE_SHOT,
} hal_timer_mode_t;

typedef struct hal_timer_obj *hal_timer_handle_t;

hal_timer_handle_t hal_timer_init(uint32_t period_us, hal_timer_mode_t mode);
hal_status_t       hal_timer_start(hal_timer_handle_t h);
void               hal_timer_stop(hal_timer_handle_t h);
void               hal_timer_reset(hal_timer_handle_t h);
hal_status_t       hal_timer_set_period(hal_timer_handle_t h, uint32_t period_us);
uint32_t           hal_timer_get_count(hal_timer_handle_t h);
void               hal_timer_attach_cb(hal_timer_handle_t h, hal_callback_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif
