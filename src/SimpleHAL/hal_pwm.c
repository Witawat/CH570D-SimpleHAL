#include "hal_pwm.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

static struct hal_pwm_obj pwm_instances[HAL_PWM_MAX_CHANNELS];
static uint8_t pwm_global_init = 0;

static uint8_t ch_to_index(uint8_t ch)
{
    if (ch == CH_PWM1) return 0;
    if (ch == CH_PWM2) return 1;
    if (ch == CH_PWM3) return 2;
    if (ch == CH_PWM4) return 3;
    if (ch == CH_PWM5) return 4;
    return 0xFF;
}

static uint8_t index_to_ch(uint8_t idx)
{
    static const uint8_t map[] = { CH_PWM1, CH_PWM2, CH_PWM3, CH_PWM4, CH_PWM5 };
    if (idx >= HAL_PWM_MAX_CHANNELS) return 0;
    return map[idx];
}

static void pwm_global_init_once(void)
{
    if (!pwm_global_init) {
        pwm_global_init = 1;
        for (int i = 0; i < HAL_PWM_MAX_CHANNELS; i++) {
            pwm_instances[i].used = 0;
        }
    }
}

uint32_t hal_pwm_calc_div(uint32_t freq_hz, uint32_t cycle)
{
    if (freq_hz == 0) return 255;
    uint32_t div = FREQ_SYS / (cycle * freq_hz);
    if (div < 1) div = 1;
    if (div > 65535) div = 65535;
    return div;
}

hal_pwm_handle_t hal_pwm_init(hal_pwm_channel_t ch, uint32_t freq_hz, uint8_t duty_pct)
{
    pwm_global_init_once();

    uint8_t idx = ch_to_index((uint8_t)ch);
    if (idx >= HAL_PWM_MAX_CHANNELS) return NULL;

    hal_pwm_handle_t h = &pwm_instances[idx];
    if (h->used) return h;

    h->used      = 1;
    h->channel   = (uint8_t)ch;
    h->mode_16bit = 0;
    h->freq_hz   = (uint16_t)freq_hz;
    h->polarity  = 0;
    h->running   = 0;

    if (freq_hz == 0) freq_hz = 1000;

    PWM_16bit_CycleDisable();

    uint8_t cycle_sel = 0;
    PWMX_CycleTypeDef cycle_cfg[] = {
        PWMX_Cycle_256, PWMX_Cycle_255, PWMX_Cycle_128,
        PWMX_Cycle_127, PWMX_Cycle_64,  PWMX_Cycle_63
    };
    uint16_t cycle_vals[] = { 256, 255, 128, 127, 64, 63 };

    uint16_t best_cycle = 256;
    uint32_t best_div = 1;

    for (int i = 0; i < 6; i++) {
        uint32_t div = FREQ_SYS / (cycle_vals[i] * freq_hz);
        if (div < 1) div = 1;
        if (div > 65535) div = 65535;
        if (i == 0 || div < best_div) {
            best_div = div;
            best_cycle = cycle_vals[i];
            cycle_sel = i;
        }
    }

    R16_PWM_CLOCK_DIV = (uint16_t)best_div;
    PWMX_CycleCfg(cycle_cfg[cycle_sel]);

    uint8_t duty_val = (uint8_t)((uint32_t)duty_pct * best_cycle / 100);
    h->duty_raw = duty_val;

    PWMX_ACTOUT(h->channel, duty_val, High_Level, DISABLE);
    return h;
}

hal_pwm_handle_t hal_pwm_init_16bit(hal_pwm_channel_t ch, uint32_t freq_hz, uint16_t duty)
{
    pwm_global_init_once();

    uint8_t idx = ch_to_index((uint8_t)ch);
    if (idx >= HAL_PWM_MAX_CHANNELS) return NULL;

    hal_pwm_handle_t h = &pwm_instances[idx];
    if (h->used) return h;

    h->used       = 1;
    h->channel    = (uint8_t)ch;
    h->mode_16bit = 1;
    h->freq_hz    = (uint16_t)freq_hz;
    h->duty_raw   = duty;
    h->polarity   = 0;
    h->running    = 0;

    if (freq_hz == 0) freq_hz = 1000;

    uint16_t cycle = (uint16_t)(FREQ_SYS / freq_hz);
    if (cycle > 65534) cycle = 65534;
    if (cycle < 2) cycle = 2;
    h->cycle_16bit = cycle;

    R16_PWM_CLOCK_DIV = 1;
    PWM_16bit_CycleEnable();
    PWMX_16bit_CycleCfg(h->channel, cycle);

    if (duty > cycle) duty = cycle;

    PWMX_16bit_ACTOUT(h->channel, duty, High_Level, DISABLE);
    return h;
}

void hal_pwm_set_duty(hal_pwm_handle_t h, uint8_t duty_pct)
{
    if (!h || !h->used) return;

    if (h->mode_16bit) {
        uint16_t cycle = h->cycle_16bit;
        uint16_t duty = (uint16_t)((uint32_t)duty_pct * cycle / 100);
        if (duty > cycle) duty = cycle;
        h->duty_raw = duty;
        PWMX_16bit_ACTOUT(h->channel, duty, High_Level, h->running ? ENABLE : DISABLE);
    } else {
        uint8_t duty = (uint8_t)((uint32_t)duty_pct * 256 / 100);
        h->duty_raw = duty;
        PWMX_ACTOUT(h->channel, duty, High_Level, h->running ? ENABLE : DISABLE);
    }
}

void hal_pwm_set_duty_16bit(hal_pwm_handle_t h, uint16_t duty)
{
    if (!h || !h->used || !h->mode_16bit) return;
    h->duty_raw = duty;
    PWMX_16bit_ACTOUT(h->channel, duty, High_Level, h->running ? ENABLE : DISABLE);
}

void hal_pwm_set_freq(hal_pwm_handle_t h, uint32_t freq_hz)
{
    if (!h || !h->used) return;
    h->freq_hz = (uint16_t)freq_hz;
    if (freq_hz == 0) freq_hz = 1000;

    if (h->mode_16bit) {
        uint16_t cycle = (uint16_t)(FREQ_SYS / freq_hz);
        if (cycle > 65534) cycle = 65534;
        if (cycle < 2) cycle = 2;
        h->cycle_16bit = cycle;
        PWMX_16bit_CycleCfg(h->channel, cycle);
        if (h->duty_raw > cycle) h->duty_raw = cycle;
        PWMX_16bit_ACTOUT(h->channel, h->duty_raw, High_Level, h->running ? ENABLE : DISABLE);
    } else {
        uint32_t div = FREQ_SYS / (256 * freq_hz);
        if (div < 1) div = 1;
        if (div > 65535) div = 65535;
        R16_PWM_CLOCK_DIV = (uint16_t)div;
        PWMX_ACTOUT(h->channel, (uint8_t)h->duty_raw, High_Level, h->running ? ENABLE : DISABLE);
    }
}

void hal_pwm_start(hal_pwm_handle_t h)
{
    if (!h || !h->used) return;
    h->running = 1;
    if (h->mode_16bit) {
        PWMX_16bit_ACTOUT(h->channel, h->duty_raw, High_Level, ENABLE);
    } else {
        PWMX_ACTOUT(h->channel, (uint8_t)h->duty_raw, High_Level, ENABLE);
    }
}

void hal_pwm_stop(hal_pwm_handle_t h)
{
    if (!h || !h->used) return;
    h->running = 0;
    if (h->mode_16bit) {
        PWMX_16bit_ACTOUT(h->channel, 0, High_Level, DISABLE);
    } else {
        PWMX_ACTOUT(h->channel, 0, High_Level, DISABLE);
    }
}
