#include "hal_timer.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

/**
 * @brief   โครงสร้างข้อมูลภายในของ Timer
 */
struct hal_timer_obj {
    uint8_t        used;       /**< flag ว่าถูกใช้งานแล้ว */
    volatile uint8_t running;  /**< กำลังทำงานอยู่ */
    uint8_t        mode;       /**< โหมด: PERIODIC หรือ ONE_SHOT */
    uint32_t       period_us;  /**< คาบเวลาในหน่วยไมโครวินาที */
    uint32_t       period_cycles; /**< คาบเวลาในหน่วยรอบสัญญาณนาฬิกา */
    hal_callback_t cb;         /**< callback เมื่อครบรอบ */
    void          *arg;        /**< อาร์กิวเมนต์ให้ callback */
};

static struct hal_timer_obj timer_instances[HAL_TIMER_MAX_INSTANCES];

/*********************************************************************
 * @fn      get_timer
 *
 * @brief   คืนพอยน์เตอร์ไปยัง instance แรก (มี instance เดียว)
 *
 * @return  พอยน์เตอร์ไปยัง timer instance
 */
static hal_timer_handle_t get_timer(void)
{
    return &timer_instances[0];
}

/*********************************************************************
 * @fn      hal_timer_init
 *
 * @brief   เริ่มต้น Timer: แปลง period_us เป็นรอบนาฬิกา ตั้งค่า hardware
 *
 * @param   period_us - คาบเวลาในหน่วยไมโครวินาที
 * @param   mode - โหมดการทำงาน (PERIODIC หรือ ONE_SHOT)
 *
 * @return  handle ของ Timer หรือ NULL หากผิดพลาด
 */
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

/*********************************************************************
 * @fn      hal_timer_start
 *
 * @brief   เริ่มนับเวลา: ล้าง flag, เปิด IRQ, เปิด Timer
 *
 * @param   h - handle ของ Timer
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t hal_timer_start(hal_timer_handle_t h)
{
    if (!h || !h->used) return HAL_INVALID;
    h->running = 1;
    TMR_ClearITFlag(TMR_IT_CYC_END);
    PFIC_EnableIRQ(TMR_IRQn);
    TMR_Enable();
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_timer_stop
 *
 * @brief   หยุดนับเวลา
 *
 * @param   h - handle ของ Timer
 */
void hal_timer_stop(hal_timer_handle_t h)
{
    if (!h || !h->used) return;
    h->running = 0;
    TMR_Disable();
}

/*********************************************************************
 * @fn      hal_timer_reset
 *
 * @brief   รีเซ็ต Timer: ปิด, ตั้งค่าใหม่, ถ้าเคย running ก็เปิดใหม่
 *
 * @param   h - handle ของ Timer
 */
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

/*********************************************************************
 * @fn      hal_timer_set_period
 *
 * @brief   เปลี่ยนคาบเวลา: อัปเดตค่า ปิด-เปิด Timer ถ้ากำลังทำงาน
 *
 * @param   h - handle ของ Timer
 * @param   period_us - คาบเวลาใหม่ในหน่วยไมโครวินาที
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
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

/*********************************************************************
 * @fn      hal_timer_get_count
 *
 * @brief   อ่านค่าปัจจุบันจาก hardware counter
 *
 * @param   h - handle ของ Timer
 *
 * @return  ค่าปัจจุบันจาก hardware counter
 */
uint32_t hal_timer_get_count(hal_timer_handle_t h)
{
    if (!h || !h->used) return 0;
    return TMR_GetCurrentCount();
}

/*********************************************************************
 * @fn      hal_timer_attach_cb
 *
 * @brief   ผูก callback ที่จะเรียกเมื่อ Timer ครบรอบ
 *
 * @param   h - handle ของ Timer
 * @param   cb - ฟังก์ชัน callback
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 */
void hal_timer_attach_cb(hal_timer_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->cb  = cb;
    h->arg = arg;
}

/*********************************************************************
 * @fn      TMR_IRQHandler
 *
 * @brief   ISR ของ Timer: ตรวจสอบ flag, เรียก callback, จัดการ one-shot
 */
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
