#ifndef __HAL_TYPES_H__
#define __HAL_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK       = 0,
    HAL_ERROR    = 1,
    HAL_BUSY     = 2,
    HAL_TIMEOUT  = 3,
    HAL_INVALID  = 4,
} hal_status_t;

typedef void (*hal_callback_t)(void *arg);

#ifdef __cplusplus
}
#endif

#endif
