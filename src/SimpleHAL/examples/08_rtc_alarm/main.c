/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 8 — RTC ตั้งเวลาและ alarm
 *
 * สิ่งที่เรียนรู้:
 * - การ init RTC (Real-Time Clock)
 * - การตั้งค่าเวลาปัจจุบัน
 * - การอ่านเวลาปัจจุบัน
 * - การใช้ RTC timer interrupt
 *
 * RTC คืออะไร:
 * RTC (Real-Time Clock) = นาฬิกาที่เดินต่อเนื่องแม้ MCU ดับ
 * ใช้ 32kHz oscillator (ภายในหรือภายนอก) เพื่อรักษาเวลา
 * มีประโยชน์ในการ:
 * - บอกเวลาปัจจุบัน (วันที่/เวลา)
 * - ตั้ง alarm ปลุกตามเวลาที่กำหนด
 * - สร้างสัญญาณ interrupt เป็นระยะ (periodic timer)
 *******************************************************************************/

#include "simple_hal.h"

/*
 * handle ของ GPIO ต้องเป็น static (ระดับโมดูล)
 * เพราะ RTC callback ไม่รับพารามิเตอร์ที่เป็น handle โดยตรง
 * (จริงๆ ตัวอย่างนี้ส่งผ่าน arg = NULL)
 */
static hal_gpio_handle_t led;

/*
 * ฟังก์ชัน callback สำหรับ RTC timer interrupt
 *
 * RTC timer callback — ทำงานแตกต่างจาก Timer ปกติ:
 * - Timer ปกติ (SysTick): ทำงานทุกๆ ไมโครวินาทีตามที่ตั้ง
 * - RTC timer: ทำงานทุกๆ 1 วินาที (จาก RTC_TMR_CYC_S1)
 *
 * ฟังก์ชันนี้จะถูกเรียกโดยอัตโนมัติทุกวินาที
 * จาก interrupt ของ RTC
 *
 * ข้อควรระวัง: callback ต้องสั้นและรวดเร็ว
 * เนื่องจากทำงานในบริบท interrupt
 */
void rtc_cb(void *arg)
{
    (void)arg;              // ไม่ได้ใช้ arg ในตัวอย่างนี้
    hal_gpio_toggle(led);   // กลับค่า LED ทุกวินาที
}

int main()
{
    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น GPIO PA11 เป็น output */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    /*
     * เริ่มต้น RTC
     *
     * hal_rtc_init(clock_source)
     * - Count_1 = ใช้ internal 32kHz oscillator (32000 Hz)
     * - Count_32 = ใช้ internal oscillator ปรับ calibrated (32768 Hz)
     *
     * RTC ต้องมี clock source ที่稳定 (32kHz) เพื่อให้เวลาถูกต้อง
     * ถ้าต้องการ precision สูง ควรใช้ external 32kHz crystal
     */
    hal_rtc_handle_t rtc = hal_rtc_init(Count_1);

    /*
     * ตั้งค่าเวลาปัจจุบันให้ RTC
     *
     * hal_rtc_time_t t คือ struct ที่เก็บ:
     * - year: ปี (เช่น 2026)
     * - month: เดือน (1-12)
     * - day: วัน (1-31)
     * - hour: ชั่วโมง (0-23)
     * - minute: นาที (0-59)
     * - second: วินาที (0-59)
     *
     * การตั้งค่าครั้งแรก: กำหนดเวลาเริ่มต้น
     * หลังจากนี้ RTC จะนับเวลาต่อจากจุดนี้
     */
    hal_rtc_time_t t = {2026, 5, 16, 10, 30, 0};

    /*
     * hal_rtc_set_time(handle, &time_struct)
     * เขียนเวลาไปยัง RTC
     * RTC จะเริ่มนับจากเวลานี้
     *
     * หมายเหตุ: ถ้า MCU รีเซ็ต RTC จะถูกรีเซ็ตด้วย
     * ต้อง set time ใหม่ทุกครั้งที่ boot
     * (ถ้าต้องการให้จำเวลาได้ ต้องใช้ external RTC + battery backup)
     */
    hal_rtc_set_time(rtc, &t);

    /*
     * ตั้ง RTC timer ให้ทำงานทุก 1 วินาที
     *
     * hal_rtc_set_timer(handle, cycle)
     * - RTC_TMR_CYC_S1 = ทุก 1 วินาที
     *
     * ค่า cycle ที่เป็นไปได้:
     * - RTC_TMR_CYC_S0_5 = ทุก 0.5 วินาที
     * - RTC_TMR_CYC_S1   = ทุก 1 วินาที
     * - RTC_TMR_CYC_S2   = ทุก 2 วินาที
     * - RTC_TMR_CYC_S4   = ทุก 4 วินาที
     * (แล้วแต่รุ่น MCU อาจมีค่าเพิ่มเติม)
     *
     * ข้อแตกต่าง: timer นี้เป็น interrupt-based ไม่ใช่ polling
     * CPU ไม่ต้องคอยตรวจสอบเวลา — RTC จะแจ้งเมื่อครบกำหนด
     */
    hal_rtc_set_timer(rtc, RTC_TMR_CYC_S1);

    /*
     * แนบ callback กับ RTC timer
     * ทุกครั้งที่ timer ครบ 1 วินาที rtc_cb() จะถูกเรียก
     * ซึ่งจะไปกลับค่า LED ทำให้ LED กระพริบทุกวินาที
     */
    hal_rtc_attach_cb(rtc, rtc_cb, NULL);

    /*
     * main loop
     *
     * ทุกครั้งที่วน loop:
     * 1. อ่านเวลาปัจจุบันจาก RTC มาไว้ใน struct t
     * 2. เรียก __WFI() ให้ CPU หลับรอ interrupt
     *
     * hal_rtc_get_time(handle, &time_struct) — อ่านเวลาปัจจุบัน
     * เวลาจะถูกเขียนไปยัง struct t (ปี เดือน วัน ชั่วโมง นาที วินาที)
     *
     * __WFI() — CPU หลับจนกว่าจะมี interrupt
     * (ในที่นี้คือ RTC timer interrupt ทุก 1 วินาที)
     *
     * ข้อดีของ RTC: กินกระแสน้อยมากเมื่อเทียบกับการใช้ Timer ปกติ
     * เหมาะกับอุปกรณ์ที่ใช้แบตเตอรี่
     */
    while (1) {
        hal_rtc_get_time(rtc, &t);   // อ่านเวลาปัจจุบัน
        __WFI();                       // หลับรอ interrupt
    }
}
