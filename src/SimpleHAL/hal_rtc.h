#ifndef __HAL_RTC_H__
#define __HAL_RTC_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_RTC_MAX_INSTANCES 1

typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
} hal_rtc_time_t;

typedef struct hal_rtc_obj *hal_rtc_handle_t;

hal_rtc_handle_t hal_rtc_init(RTC_OSCCntTypeDef osc_cycles);
hal_status_t     hal_rtc_set_time(hal_rtc_handle_t h, const hal_rtc_time_t *time);
hal_status_t     hal_rtc_get_time(hal_rtc_handle_t h, hal_rtc_time_t *time);
hal_status_t     hal_rtc_set_trigger(hal_rtc_handle_t h, uint32_t cycles);
hal_status_t     hal_rtc_set_timer(hal_rtc_handle_t h, RTC_TMRCycTypeDef period);
void             hal_rtc_attach_cb(hal_rtc_handle_t h, hal_callback_t cb, void *arg);
uint32_t         hal_rtc_get_cycle_lsi(hal_rtc_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
