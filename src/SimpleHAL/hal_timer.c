#include "hal_timer.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

struct hal_timer_obj {
    uint8_t        used;
    volatile uint8_t running;
    uint8_t        mode;
    uint32_t       period_us;
    uint32_t       period_cycles;
    hal_callback_t cb;
    void          *arg;
};

static struct hal_timer_obj timer_instances[HAL_TIMER_MAX_INSTANCES];

static hal_timer_handle_t get_timer(void)
{
    return &timer_instances[0];
}

hal_timer_handle_t hal_timer_init(uint32_t period_us, hal_timer_mode_t mode)
{
    hal_timer_handle_t h = get_timer();
    if (h->used) return h;

    h->used      = 1;
    h->mode      = (uint8_t)mode;
    h->running   = 0;
    h->cb        = NULL;
    h->arg       = NULL;

    uint64_t cycles = (uint64_t)period_us * (FREQ_SYS / 1000000);
    if (cycles > HAL_TIMER_MAX_PERIOD) cycles = HAL_TIMER_MAX_PERIOD;
    h->period_us     = period_us;
    h->period_cycles = (uint32_t)cycles;

    TMR_TimerInit(h->period_cycles);
    TMR_ITCfg(ENABLE, TMR_IT_CYC_END);
    return h;
}

hal_status_t hal_timer_start(hal_timer_handle_t h)
{
    if (!h || !h->used) return HAL_INVALID;
    h->running = 1;
    TMR_ClearITFlag(TMR_IT_CYC_END);
    PFIC_EnableIRQ(TMR_IRQn);
    TMR_Enable();
    return HAL_OK;
}

void hal_timer_stop(hal_timer_handle_t h)
{
    if (!h || !h->used) return;
    h->running = 0;
    TMR_Disable();
}

void hal_timer_reset(hal_timer_handle_t h)
{
    if (!h || !h->used) return;
    TMR_Disable();
    TMR_TimerInit(h->period_cycles);
    TMR_ClearITFlag(TMR_IT_CYC_END);
    if (h->running) {
        TMR_Enable();
    }
}

hal_status_t hal_timer_set_period(hal_timer_handle_t h, uint32_t period_us)
{
    if (!h || !h->used) return HAL_INVALID;

    uint64_t cycles = (uint64_t)period_us * (FREQ_SYS / 1000000);
    if (cycles > HAL_TIMER_MAX_PERIOD) cycles = HAL_TIMER_MAX_PERIOD;
    h->period_us     = period_us;
    h->period_cycles = (uint32_t)cycles;

    TMR_Disable();
    TMR_TimerInit(h->period_cycles);
    TMR_ClearITFlag(TMR_IT_CYC_END);
    if (h->running) {
        TMR_Enable();
    }
    return HAL_OK;
}

uint32_t hal_timer_get_count(hal_timer_handle_t h)
{
    if (!h || !h->used) return 0;
    return TMR_GetCurrentCount();
}

void hal_timer_attach_cb(hal_timer_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->cb  = cb;
    h->arg = arg;
}

__INTERRUPT __HIGH_CODE void TMR_IRQHandler(void)
{
    if (TMR_GetITFlag(TMR_IT_CYC_END)) {
        hal_timer_handle_t h = get_timer();
        if (h && h->used) {
            TMR_ClearITFlag(TMR_IT_CYC_END);

            if (h->mode == HAL_TIMER_MODE_ONE_SHOT) {
                TMR_Disable();
                h->running = 0;
            }

            if (h->cb) {
                h->cb(h->arg);
            }
        }
    }
}
