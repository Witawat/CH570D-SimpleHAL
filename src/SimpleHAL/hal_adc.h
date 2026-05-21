#ifndef __HAL_ADC_H__
#define __HAL_ADC_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_ADC_MAX_INSTANCES 1

typedef enum {
    HAL_ADC_4BIT = 0,
    HAL_ADC_8BIT,
    HAL_ADC_9BIT,
} hal_adc_resolution_t;

typedef struct hal_adc_obj *hal_adc_handle_t;

hal_adc_handle_t hal_adc_init(hal_adc_resolution_t res, uint8_t pin);
uint16_t         hal_adc_read(hal_adc_handle_t h);
uint32_t         hal_adc_read_mv(hal_adc_handle_t h, uint16_t vref_mv);
void             hal_adc_free(hal_adc_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
