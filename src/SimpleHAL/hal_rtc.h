#ifndef __HAL_RTC_H__
#define __HAL_RTC_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_RTC_MAX_INSTANCES 1

/**
 * @brief   โครงสร้างเวลาสำหรับ RTC
 */
typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
} hal_rtc_time_t;

typedef struct hal_rtc_obj *hal_rtc_handle_t;

/**
 * @brief   เริ่มต้น RTC ด้วยจำนวนรอบ oscillator
 *
 * @param   osc_cycles - จำนวนรอบ oscillator
 *
 * @return  handle ของ RTC หรือ NULL หากล้มเหลว
 */
hal_rtc_handle_t hal_rtc_init(RTC_OSCCntTypeDef osc_cycles);
/**
 * @brief   ตั้งค่าเวลา RTC
 *
 * @param   h - handle ของ RTC
 * @param   time - โครงสร้างเวลาที่จะตั้ง
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t     hal_rtc_set_time(hal_rtc_handle_t h, const hal_rtc_time_t *time);
/**
 * @brief   อ่านเวลาปัจจุบันจาก RTC
 *
 * @param   h - handle ของ RTC
 * @param   time - บัฟเฟอร์รับเวลา
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t     hal_rtc_get_time(hal_rtc_handle_t h, hal_rtc_time_t *time);
/**
 * @brief   ตั้งค่า Trigger (ปลุกเมื่อนับครบ cycles)
 *
 * @param   h - handle ของ RTC
 * @param   cycles - จำนวนรอบที่จะปลุก
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t     hal_rtc_set_trigger(hal_rtc_handle_t h, uint32_t cycles);
/**
 * @brief   ตั้งค่า Timer (ทำงานเป็นรอบตาม period ที่กำหนด)
 *
 * @param   h - handle ของ RTC
 * @param   period - คาบเวลา
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t     hal_rtc_set_timer(hal_rtc_handle_t h, RTC_TMRCycTypeDef period);
/**
 * @brief   ผูก callback สำหรับ RTC event (trigger/timer)
 *
 * @param   h - handle ของ RTC
 * @param   cb - ฟังก์ชัน callback
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void             hal_rtc_attach_cb(hal_rtc_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   อ่านค่ารอบ LSI ปัจจุบันของ RTC
 *
 * @param   h - handle ของ RTC
 *
 * @return  ค่ารอบ LSI ปัจจุบัน
 */
uint32_t         hal_rtc_get_cycle_lsi(hal_rtc_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
