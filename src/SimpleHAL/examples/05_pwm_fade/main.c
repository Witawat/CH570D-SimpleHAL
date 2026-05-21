/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 5 — PWM fade LED
 *
 * สิ่งที่เรียนรู้:
 * - การสร้างสัญญาณ PWM (Pulse Width Modulation)
 * - การปรับความสว่าง LED ด้วย duty cycle
 * - การใช้ for loop ค่อยๆ เปลี่ยนค่า duty cycle
 *
 * ทฤษฎี PWM:
 * PWM = การจำลองสัญญาณแอนะล็อกด้วยดิจิตอล
 * โดยการเปิด-ปิดสัญญาณความถี่สูง แล้วปรับช่วงเวลาที่เปิด (duty cycle)
 *   - duty 0%   = LED ดับสนิท
 *   - duty 50%  = LED สว่างครึ่งเดียว
 *   - duty 100% = LED สว่างเต็มที่
 *
 * การต่อวงจร: ต่อ LED (พร้อม resistor 220Ω) ระหว่าง PWM output pin กับ GND
 * PA4 = PWM CH1 (สำหรับ nRF51 BLINK, ดู pinout ของบอร์ด)
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /*
     * duty — ตัวแปรเก็บค่า duty cycle (0-100%)
     * ใช้ uint8_t เพราะค่าน้อยกว่า 256
     */
    uint8_t duty;

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น PWM
     *
     * hal_pwm_init(channel, freq_hz, duty_pct)
     * - ch: HAL_PWM_CH1 — เลือกช่อง PWM 1
     *   CH572 มีทั้งหมด 5 ช่อง: CH1-CH5
     *   แต่ละช่องเชื่อมกับขา GPIO ที่แตกต่างกัน
     * - freq_hz: 1000 (1 kHz) — ความถี่ PWM
     *   ค่าน้อยไปจะเห็น LED กระพริบ, มากไปจะลดความละเอียด
     *   - 1 kHz = รอบ 1ms — เหมาะกับ LED ทั่วไป
     *   - 20 kHz = เหมาะกับ motor drive (ไม่เกิดเสียง)
     * - duty: 0 — เริ่มต้นที่ 0% (LED ดับ)
     *
     * หมายเหตุ: PWM ใช้โหมด 8-bit (duty 0-100%) โดยค่าเริ่มต้น
     * ถ้าต้องการความละเอียดสูงขึ้น มีฟังก์ชัน hal_pwm_init_16bit()
     */
    hal_pwm_handle_t pwm = hal_pwm_init(HAL_PWM_CH1, 1000, 0);

    /*
     * hal_pwm_start(handle) — เริ่มส่งสัญญาณ PWM ออกขา
     * หลังจากเรียกฟังก์ชันนี้ ขาจะเริ่มสร้างสัญญาณ PWM
     * (ไม่ต้องกลัว LED จะเสียหาย เพราะ duty = 0% = ดับ)
     */
    hal_pwm_start(pwm);

    /* main loop: fade วนไปเรื่อยๆ */
    while (1) {

        /*
         * Fade-in: ค่อยๆ เพิ่มความสว่างจาก 0% → 99%
         *
         * for loop:
         * - duty = 0: เริ่มต้น
         * - duty < 100: ทำงานตราบใดที่ duty น้อยกว่า 100
         * - duty++: เพิ่ม duty ทีละ 1 ทุกรอบ
         *
         * แต่ละรอบ: ตั้งค่า duty แล้วรอ 10ms
         * ใช้เวลาทั้งหมด 100 × 10ms = 1 วินาที สำหรับ fade-in
         */
        for (duty = 0; duty < 100; duty++) {

            /*
             * hal_pwm_set_duty(handle, duty_pct)
             * เปลี่ยน duty cycle แบบ 8-bit (0-100%)
             * ฟังก์ชันนี้เปลี่ยนค่าทันที — PWM ยังทำงานต่อเนื่อง
             */
            hal_pwm_set_duty(pwm, duty);

            /*
             * รอ 10ms ก่อนเพิ่มค่า duty ถัดไป
             * ทำให้เห็นการเปลี่ยนแปลงอย่างนุ่มนวล
             */
            hal_delay_ms(10);
        }

        /*
         * Fade-out: ค่อยๆ ลดความสว่างจาก 99% → 0%
         *
         * for loop แบบถอยหลัง:
         * - duty = 99: เริ่มจากเกือบเต็ม
         * - duty > 0: ทำงานตราบใดที่ duty มากกว่า 0
         * - duty--: ลด duty ทีละ 1 ทุกรอบ
         *
         * สังเกตว่าใช้ > ไม่ใช่ >= — ถ้าใช้ >= 0 จะได้ 0 แล้วยังลดอีก
         * ซึ่ง uint8_t จะ underflow เป็น 255 ทำให้เกิด loop ไม่รู้จบ
         */
        for (duty = 100; duty > 0; duty--) {
            hal_pwm_set_duty(pwm, duty);
            hal_delay_ms(10);
        }

        /*
         * เมื่อจบ fade-in และ fade-out 1 รอบ จะวนไปทำ fade-in ใหม่
         * ทำให้เห็น LED ค่อยๆ สว่างขึ้น แล้วค่อยๆ ดับลง ตลอดไป
         */
    }
}
