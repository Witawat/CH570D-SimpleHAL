/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 4 — กระพริบ LED ด้วย Timer interrupt
 *
 * สิ่งที่เรียนรู้:
 * - การใช้ Timer ในการสร้างเหตุการณ์ตามเวลา (periodic interrupt)
 * - การแนบ callback function สำหรับ Timer interrupt
 * - การใช้ __WFI() เพื่อประหยัดพลังงานขณะรอ interrupt
 * - ความแตกต่างระหว่าง polling (ตัวอย่าง 0) กับ interrupt (ตัวอย่างนี้)
 *
 * การต่อวงจร: ต่อ LED (พร้อม resistor 220Ω) ระหว่าง PA11 กับ GND
 *
 * เปรียบเทียบกับตัวอย่าง 00_blink_led:
 * - ตัวอย่าง 0: ใช้ hal_delay_ms() ซึ่งเป็นการ busy-wait (CPU ทำงานตลอด)
 * - ตัวอย่างนี้: ใช้ Timer interrupt — CPU หลับ (__WFI()) ระหว่างรอ
 *   ทำให้ประหยัดพลังงานกว่าและ CPU ว่างทำงานอื่นได้
 *******************************************************************************/

#include "simple_hal.h"

/*
 * handle ของ GPIO — ต้องเป็น global/static เพราะ Timer callback
 * ไม่มีทางรับพารามิเตอร์ที่เป็น handle ได้โดยตรงในตัวอย่างนี้
 *
 * static หมายถึงตัวแปรนี้มองเห็นได้เฉพาะในไฟล์นี้เท่านั้น
 */
static hal_gpio_handle_t led;

/*
 * ฟังก์ชัน callback สำหรับ Timer interrupt
 *
 * ฟังก์ชันนี้จะถูกเรียกโดยอัตโนมัติทุกครั้งที่ Timer นับครบตามเวลาที่กำหนด
 * (ในตัวอย่างนี้ทุก 500ms)
 *
 * พารามิเตอร์:
 * - arg: void pointer ส่งค่ามาจาก hal_timer_attach_cb
 *        (ใช้ NULL ในตัวอย่างนี้)
 *
 * ข้อสำคัญ: callback ห้ามใช้ hal_delay_ms() หรือ blocking operation
 * เพราะทำงานในบริบทของ interrupt — ควรสั้นและรวดเร็ว
 */
void timer_cb(void *arg)
{
    /*
     * (void)arg — บอก compiler ว่าจงใจไม่ใช้พารามิเตอร์นี้
     * ป้องกัน compiler warning
     */
    (void)arg;

    /*
     * hal_gpio_toggle(handle) — กลับค่าขา GPIO
     * ถ้าเป็น 1 → 0, ถ้าเป็น 0 → 1
     * สะดวกกว่าการใช้ hal_gpio_write() กรณีกระพริบ LED
     *
     * ฟังก์ชันนี้ถูกเรียกจาก Timer interrupt ทุก 500ms
     * ดังนั้น LED จะกระพริบที่ความถี่ 1 Hz (500ms ON + 500ms OFF)
     */
    hal_gpio_toggle(led);
}

/*
 * จุดเริ่มต้นโปรแกรม
 */
int main()
{
    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น GPIO PA11 เป็น output push-pull */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    /*
     * เริ่มต้น Timer
     *
     * hal_timer_init(period_us, mode)
     * - period_us: 500000 (500ms) — ระยะเวลาของ Timer ในหน่วยไมโครวินาที
     *   500000 µs = 500 ms
     * - mode: HAL_TIMER_MODE_PERIODIC — โหมดทำงานซ้ำ
     *
     * โหมด Timer:
     * - HAL_TIMER_MODE_PERIODIC: ทำงานซ้ำทุกๆ period_us
     * - HAL_TIMER_MODE_ONE_SHOT: ทำงานครั้งเดียวแล้วหยุด
     *
     * Timer นี้ใช้ฮาร์ดแวร์ Timer ของ CH572 (SysTick หรือ Timer peripheral)
     * ไม่ได้ใช้ hal_delay_ms() ซึ่งเป็น software delay
     */
    hal_timer_handle_t tmr = hal_timer_init(500000, HAL_TIMER_MODE_PERIODIC);

    /*
     * แนบ callback function
     *
     * hal_timer_attach_cb(timer_handle, callback_function, argument)
     * - tmr: handle ที่ได้จาก hal_timer_init
     * - timer_cb: ชื่อฟังก์ชันที่จะเรียกเมื่อ Timer ครบกำหนด
     * - NULL: อาร์กิวเมนต์ที่ส่งไปยัง callback (ไม่มีในตัวอย่างนี้)
     */
    hal_timer_attach_cb(tmr, timer_cb, NULL);

    /*
     * เริ่ม Timer — หลังจากนี้ Timer จะเริ่มนับและเรียก callback อัตโนมัติ
     * ทุก 500ms (500,000 µs)
     */
    hal_timer_start(tmr);

    /*
     * main loop
     *
     * __WFI() = Wait For Interrupt
     * เป็นคำสั่งพิเศษที่ทำให้ CPU หยุดทำงาน (sleep mode) จนกว่าจะมี interrupt เข้ามา
     * เมื่อ Timer interrupt เกิดขึ้น CPU จะตื่น เรียก timer_cb() แล้วกลับมาหลับต่อ
     *
     * ข้อดี: ประหยัดพลังงานมากกว่าการใช้ hal_delay_ms() ซึ่ง CPU ทำงานตลอดเวลา
     */
    while (1) {
        __WFI();    // รอ interrupt — CPU หลับ (ประหยัดไฟ)
    }
}
