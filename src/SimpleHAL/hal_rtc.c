/* ต้อง include CH57x_common.h ก่อน hal_rtc.h เพราะ hal_rtc.h ใช้ types จาก WCH */
#include "CH57x_common.h"
#include "hal_rtc.h"

struct hal_rtc_obj {
    uint8_t        used;
    hal_callback_t cb;
    void          *arg;
    uint8_t        event;
};

static struct hal_rtc_obj rtc_instances[HAL_RTC_MAX_INSTANCES];

static hal_rtc_handle_t get_rtc(void)
{
    return &rtc_instances[0];
}

hal_rtc_handle_t hal_rtc_init(RTC_OSCCntTypeDef osc_cycles)
{
    hal_rtc_handle_t h = get_rtc();
    if (h->used) return h;
    h->used  = 1;
    h->cb    = NULL;
    h->arg   = NULL;
    h->event = 0;
    RTC_InitClock(osc_cycles);
    PFIC_EnableIRQ(RTC_IRQn);
    return h;
}

hal_status_t hal_rtc_set_time(hal_rtc_handle_t h, const hal_rtc_time_t *t)
{
    if (!h || !h->used || !t) return HAL_INVALID;
    RTC_InitTime(t->year, t->month, t->day, t->hour, t->minute, t->second);
    return HAL_OK;
}

hal_status_t hal_rtc_get_time(hal_rtc_handle_t h, hal_rtc_time_t *t)
{
    if (!h || !h->used || !t) return HAL_INVALID;
    RTC_GetTime(&t->year, &t->month, &t->day, &t->hour, &t->minute, &t->second);
    return HAL_OK;
}

hal_status_t hal_rtc_set_trigger(hal_rtc_handle_t h, uint32_t cycles)
{
    if (!h || !h->used) return HAL_INVALID;
    RTC_TRIGFunCfg(cycles);
    h->event = RTC_TRIG_EVENT;
    return HAL_OK;
}

hal_status_t hal_rtc_set_timer(hal_rtc_handle_t h, RTC_TMRCycTypeDef period)
{
    if (!h || !h->used) return HAL_INVALID;
    RTC_TMRFunCfg(period);
    h->event = RTC_TMR_EVENT;
    return HAL_OK;
}

void hal_rtc_attach_cb(hal_rtc_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->cb  = cb;
    h->arg = arg;
}

uint32_t hal_rtc_get_cycle_lsi(hal_rtc_handle_t h)
{
    (void)h;
    return RTC_GetCycleLSI();
}

__INTERRUPT __HIGH_CODE void RTC_IRQHandler(void)
{
    hal_rtc_handle_t h = get_rtc();
    if (!h || !h->used) return;

    uint8_t flag = RTC_GetITFlag(h->event);
    if (flag) {
        RTC_ClearITFlag(h->event);
        if (h->cb) {
            h->cb(h->arg);
        }
    }
}
