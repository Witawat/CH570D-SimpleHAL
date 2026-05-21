/*********************************************************************
 * @brief   ต้อง include CH57x_common.h ก่อน hal_rtc.h เพราะ hal_rtc.h ใช้ types จาก WCH
 */
#include "CH57x_common.h"
#include "hal_rtc.h"

/*********************************************************************
 * @brief   โครงสร้างข้อมูลภายในของ RTC
 */
struct hal_rtc_obj {
    uint8_t        used;
    hal_callback_t cb;
    void          *arg;
    uint8_t        event;  /* RTC_TRIG_EVENT หรือ RTC_TMR_EVENT */
};

static struct hal_rtc_obj rtc_instances[HAL_RTC_MAX_INSTANCES];

/*********************************************************************
 * @fn      get_rtc
 *
 * @brief   คืนพอยน์เตอร์ไปยัง instance RTC
 *
 * @return  พอยน์เตอร์ไปยัง instance RTC แรก
 */
static hal_rtc_handle_t get_rtc(void)
{
    return &rtc_instances[0];
}

/*********************************************************************
 * @fn      hal_rtc_init
 *
 * @brief   เริ่มต้น RTC: ตั้งค่า oscillator และเปิด IRQ
 *
 * @param   osc_cycles - จำนวนรอบ oscillator
 *
 * @return  handle ของ RTC
 */
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

/*********************************************************************
 * @fn      hal_rtc_set_time
 *
 * @brief   ตั้งค่าเวลาให้ RTC (ปี เดือน วัน ชั่วโมง นาที วินาที)
 *
 * @param   h - handle ของ RTC
 * @param   t - เวลาที่จะตั้ง
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t hal_rtc_set_time(hal_rtc_handle_t h, const hal_rtc_time_t *t)
{
    if (!h || !h->used || !t) return HAL_INVALID;
    RTC_InitTime(t->year, t->month, t->day, t->hour, t->minute, t->second);
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_rtc_get_time
 *
 * @brief   อ่านเวลาปัจจุบันจาก RTC
 *
 * @param   h - handle ของ RTC
 * @param   t - บัฟเฟอร์รับเวลา
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t hal_rtc_get_time(hal_rtc_handle_t h, hal_rtc_time_t *t)
{
    if (!h || !h->used || !t) return HAL_INVALID;
    RTC_GetTime(&t->year, &t->month, &t->day, &t->hour, &t->minute, &t->second);
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_rtc_set_trigger
 *
 * @brief   ตั้งค่า RTC trigger: ขัดจังหวะเมื่อนับครบตาม cycles
 *
 * @param   h - handle ของ RTC
 * @param   cycles - จำนวนรอบ
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t hal_rtc_set_trigger(hal_rtc_handle_t h, uint32_t cycles)
{
    if (!h || !h->used) return HAL_INVALID;
    RTC_TRIGFunCfg(cycles);
    h->event = RTC_TRIG_EVENT;
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_rtc_set_timer
 *
 * @brief   ตั้งค่า RTC timer: ขัดจังหวะเป็นรอบตาม period
 *
 * @param   h - handle ของ RTC
 * @param   period - คาบเวลา
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t hal_rtc_set_timer(hal_rtc_handle_t h, RTC_TMRCycTypeDef period)
{
    if (!h || !h->used) return HAL_INVALID;
    RTC_TMRFunCfg(period);
    h->event = RTC_TMR_EVENT;
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_rtc_attach_cb
 *
 * @brief   ผูก callback สำหรับ RTC event (trigger หรือ timer)
 *
 * @param   h - handle ของ RTC
 * @param   cb - ฟังก์ชัน callback
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void hal_rtc_attach_cb(hal_rtc_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->cb  = cb;
    h->arg = arg;
}

/*********************************************************************
 * @fn      hal_rtc_get_cycle_lsi
 *
 * @brief   อ่านค่ารอบ LSI ปัจจุบัน
 *
 * @param   h - handle ของ RTC
 *
 * @return  ค่ารอบ LSI ปัจจุบัน
 */
uint32_t hal_rtc_get_cycle_lsi(hal_rtc_handle_t h)
{
    (void)h;
    return RTC_GetCycleLSI();
}

/*********************************************************************
 * @fn      RTC_IRQHandler
 *
 * @brief   ISR ของ RTC: ตรวจสอบ flag, ล้าง flag, เรียก callback
 */
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
