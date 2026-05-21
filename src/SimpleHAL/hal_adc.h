#ifndef __HAL_ADC_H__
#define __HAL_ADC_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_ADC_MAX_INSTANCES 1

/** @brief   ความละเอียด ADC */
typedef enum {
    HAL_ADC_4BIT = 0,
    HAL_ADC_8BIT,
    HAL_ADC_9BIT,
} hal_adc_resolution_t;

typedef struct hal_adc_obj *hal_adc_handle_t;

/**
 * @brief   เริ่มต้น ADC กำหนดความละเอียดและขาที่ใช้วัด
 *
 * @param   res - ความละเอียด ADC
 * @param   pin - ขาที่ใช้วัด
 *
 * @return  handle ของ ADC
 */
hal_adc_handle_t hal_adc_init(hal_adc_resolution_t res, uint8_t pin);
/**
 * @brief   อ่านค่า ADC ดิบ (ตามความละเอียดที่กำหนด)
 *
 * @param   h - handle ของ ADC
 *
 * @return  ค่า ADC ดิบ
 */
uint16_t         hal_adc_read(hal_adc_handle_t h);
/**
 * @brief   อ่านค่า ADC เป็นแรงดันไฟฟ้า (mV)
 *
 * @param   h - handle ของ ADC
 * @param   vref_mv - แรงดันอ้างอิง (mV)
 *
 * @return  แรงดันไฟฟ้า (mV)
 */
uint32_t         hal_adc_read_mv(hal_adc_handle_t h, uint16_t vref_mv);
/**
 * @brief   คืนทรัพยากร ADC
 *
 * @param   h - handle ของ ADC
 */
void             hal_adc_free(hal_adc_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
