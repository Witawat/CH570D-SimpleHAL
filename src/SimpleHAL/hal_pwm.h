#ifndef __HAL_PWM_H__
#define __HAL_PWM_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_PWM_MAX_CHANNELS 5

typedef enum {
    HAL_PWM_CH1 = 0x01,
    HAL_PWM_CH2 = 0x02,
    HAL_PWM_CH3 = 0x04,
    HAL_PWM_CH4 = 0x08,
    HAL_PWM_CH5 = 0x10,
} hal_pwm_channel_t;

typedef struct hal_pwm_obj {
    uint8_t  used;
    uint8_t  channel;
    uint8_t  mode_16bit;
    uint16_t freq_hz;
    uint16_t duty_raw;
    uint16_t cycle_16bit;
    uint8_t  polarity;
    uint8_t  running;
} *hal_pwm_handle_t;

hal_pwm_handle_t hal_pwm_init(hal_pwm_channel_t ch, uint32_t freq_hz, uint8_t duty_pct);
hal_pwm_handle_t hal_pwm_init_16bit(hal_pwm_channel_t ch, uint32_t freq_hz, uint16_t duty);
void             hal_pwm_set_duty(hal_pwm_handle_t h, uint8_t duty_pct);
void             hal_pwm_set_duty_16bit(hal_pwm_handle_t h, uint16_t duty);
void             hal_pwm_set_freq(hal_pwm_handle_t h, uint32_t freq_hz);
void             hal_pwm_start(hal_pwm_handle_t h);
void             hal_pwm_stop(hal_pwm_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
