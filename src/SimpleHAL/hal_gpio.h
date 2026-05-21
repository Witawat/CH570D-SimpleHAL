#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_GPIO_MAX_PINS  16

#define PA0   0
#define PA1   1
#define PA2   2
#define PA3   3
#define PA4   4
#define PA5   5
#define PA6   6
#define PA7   7
#define PA8   8
#define PA9   9
#define PA10  10
#define PA11  11
#define PA12  12
#define PA13  13
#define PA14  14
#define PA15  15

typedef enum {
    HAL_GPIO_INPUT_FLOATING = 0,
    HAL_GPIO_INPUT_PULLUP,
    HAL_GPIO_INPUT_PULLDOWN,
    HAL_GPIO_OUTPUT_PP_5mA,
    HAL_GPIO_OUTPUT_PP_20mA,
} hal_gpio_mode_t;

typedef enum {
    HAL_GPIO_IRQ_LOW_LEVEL  = 0,
    HAL_GPIO_IRQ_HIGH_LEVEL,
    HAL_GPIO_IRQ_FALLING,
    HAL_GPIO_IRQ_RISING,
} hal_gpio_irq_mode_t;

typedef struct hal_gpio_obj *hal_gpio_handle_t;

hal_gpio_handle_t hal_gpio_init(uint8_t pin, hal_gpio_mode_t mode);
void              hal_gpio_write(hal_gpio_handle_t h, uint8_t val);
uint8_t           hal_gpio_read(hal_gpio_handle_t h);
void              hal_gpio_toggle(hal_gpio_handle_t h);
void              hal_gpio_set(hal_gpio_handle_t h);
void              hal_gpio_reset(hal_gpio_handle_t h);
hal_status_t      hal_gpio_attach_irq(hal_gpio_handle_t h, hal_gpio_irq_mode_t mode, hal_callback_t cb, void *arg);
void              hal_gpio_detach_irq(hal_gpio_handle_t h);
void              hal_gpio_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif
