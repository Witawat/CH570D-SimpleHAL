/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 9 — Power sleep และปลุกด้วย RTC
 *
 * สิ่งที่เรียนรู้:
 * - การใช้โหมดประหยัดพลังงาน (sleep) ของ CH572
 * - การกำหนดแหล่งปลุก (wake-up source)
 * - การปลุกด้วย RTC timer
 * - การ toggle LED ใน callback หลังตื่น
 *
 * ทฤษฎี:
 * CH572 มีโหมดประหยัดพลังงานหลายระดับ:
 * - Sleep: CPU หยุด, peripherals ยังทำงาน, ปลุกได้เร็ว
 * - Idle: คล้าย sleep แต่บาง module ยังทำงาน
 * - Shutdown: ประหยัดไฟมากที่สุด (RAM = 16K หรือ 32K)
 *
 * ตัวอย่างนี้ใช้โหมด sleep โดยให้ RTC timer (ทุก 1 วินาที) เป็นตัวปลุก
 * CPU จะหลับเกือบตลอดเวลา แล้วตื่นมาทำงานทุก 1 วินาทีเท่านั้น
 *******************************************************************************/

#include "simple_hal.h"

/*
 * ฟังก์ชัน callback เมื่อ RTC ปลุก MCU
 *
 * ฟังก์ชันนี้จะถูกเรียกหลังจาก CPU ตื่นจาก sleep
 * (RTC timer interrupt ทุก 1 วินาที)
 *
 * พารามิเตอร์:
 * - arg: ส่ง hal_gpio_handle_t (led) มาจาก hal_rtc_attach_cb()
 *
 * ข้อควรระวัง:
 * callback นี้ทำงานใน interrupt context
 * ควรสั้นและรวดเร็ว — ไม่ควรใช้ hal_delay_ms()
 */
void rtc_wake_cb(void *arg)
{
    /*
     * แคสต์ arg จาก void * → hal_gpio_handle_t
     * แล้วกลับค่า LED (toggle)
     *
     * LED จะกระพริบทุกๆ 1 วินาทีเมื่อ CPU ตื่น
     * แต่ระหว่างหลับ LED จะไม่กระพริบ (ไฟดับ)
     * การกระพริบแบบนี้เกิดจาก interrupt ไม่ใช่ polling
     */
    hal_gpio_toggle((hal_gpio_handle_t)arg);
}

int main()
{
    hal_gpio_handle_t led;

    /* ตั้งค่านาฬิกา — จำเป็นเพื่อให้ peripherals ทำงานถูกต้อง */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น GPIO PA11 เป็น output สำหรับ LED */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    /*
     * เริ่มต้น RTC
     *
     * Count_32 = ใช้ internal 32kHz oscillator แบบ calibrated
     * (แม่นยำกว่า Count_1)
     *
     * RTC ยังคงทำงานต่อไปแม้ CPU จะ sleep
     * เพราะ RTC ใช้ 32kHz oscillator แยกต่างหาก
     */
    hal_rtc_handle_t rtc = hal_rtc_init(Count_32);

    /*
     * ตั้ง RTC timer — ทุก 1 วินาที (RTC_TMR_CYC_S1)
     * timer นี้จะทำงานต่อเนื่องแม้ CPU อยู่ใน sleep
     * เมื่อครบกำหนด จะเกิด interrupt → ปลุก CPU
     */
    hal_rtc_set_timer(rtc, RTC_TMR_CYC_S1);

    /*
     * แนบ callback — ส่ง led handle ผ่าน arg
     * ทำไมส่งตรงๆ? เพราะเวลาตื่นจะได้ toggle LED ได้ทันที
     * โดยไม่ต้องประกาศเป็น global/static
     */
    hal_rtc_attach_cb(rtc, rtc_wake_cb, led);

    /*
     * กำหนดแหล่งปลุก (Wake-up Configuration)
     *
     * hal_pwr_wakeup_cfg(wake_source, delay)
     * - RB_SLP_RTC_WAKE: ให้ RTC สามารถปลุก MCU จาก sleep ได้
     *   (bit mask — ถ้าต้องการหลายแหล่งใช้ OR เช่น RB_SLP_RTC_WAKE | RB_SLP_GPIO_WAKE)
     * - HAL_PWR_WAKE_DELAY_4096: ระยะเวลารอหลังจากปลุก
     *   (รอบ 32kHz clock — 4096 รอบ ≈ 125ms ที่ 32kHz)
     *   ค่านี้ให้เวลา internal oscillator เสถียรก่อน CPU ทำงาน
     *
     * แหล่งปลุกอื่นๆ:
     * - RB_SLP_GPIO_WAKE: ปลุกเมื่อ GPIO เปลี่ยนสถานะ
     * - RB_SLP_RTC_WAKE: ปลุกเมื่อ RTC timer ครบ
     *
     * ค่า delay:
     * - HAL_PWR_WAKE_DELAY_4096: ~125ms (สำหรับ sleep/shutdown)
     * - ค่าน้อย: ปลุกเร็ว แต่ oscillator อาจไม่เสถียร
     * - ค่ามาก: ปลุกช้า แต่ oscillator เสถียร
     */
    hal_pwr_wakeup_cfg(RB_SLP_RTC_WAKE, HAL_PWR_WAKE_DELAY_4096);

    /*
     * main loop — sleep วนไปเรื่อยๆ
     *
     * hal_pwr_sleep(ram_mode)
     * - RB_PWR_RAM16K: เก็บ RAM ไว้ 16K (ประหยัดไฟมากกว่า)
     * - RB_PWR_RAM32K: เก็บ RAM ไว้ 32K (ข้อมูลคงอยู่ครบ)
     *
     * เมื่อเรียก hal_pwr_sleep():
     * 1. CPU หยุดทำงานทันที
     * 2. Peripherals บางตัวปิด (แต่ RTC ยังทำงาน)
     * 3. กระแสไฟลดลงมาก (จาก ~30mA เหลือ ~10µA)
     * 4. รอจนกว่าแหล่งปลุก (RTC timer) จะเกิด interrupt
     * 5. CPU ตื่น ทำงานต่อจากบรรทัดถัดไป
     *
     * ข้อสังเกต: ไม่ต้องเรียก __WFI() แยก — hal_pwr_sleep() ทำไปแล้ว
     */
    while (1) {
        hal_pwr_sleep(RB_PWR_RAM16K);
    }

    /*
     * สรุปการทำงาน:
     * ┌──────────┐     RTC 1 วินาที     ┌──────────┐
     * │  SLEEP   │ ──────────────────→ │  WAKE    │
     * │  ~10µA   │                     │  (callback) │
     * │         │ ←────────────────── │         │
     * └──────────┘     เข้า sleep      └──────────┘
     * CPU ใช้เวลาตื่นแค่ไม่กี่ µs แล้วกลับไป sleep ต่อ
     * ทำให้ประหยัดพลังงานได้มาก
     */
}
