/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 0 — กระพริบ LED ด้วย GPIO
 *
 * สิ่งที่เรียนรู้:
 * - การตั้งค่านาฬิกา MCU (System Clock)
 * - การ init GPIO เป็น output
 * - การเขียนค่า HIGH/LOW ไปยังขา GPIO
 * - การหน่วงเวลาแบบ simple (busy wait)
 *
 * การต่อวงจร: ต่อ LED (พร้อม resistor 220Ω) ระหว่าง PA11 กับ GND
 *******************************************************************************/

/*
 * simple_hal.h คือไฟล์รวมทุกโมดูลของ SimpleHAL (GPIO, UART, SPI, ฯลฯ)
 * แค่ include ไฟล์เดียวก็ใช้ทุกฟังก์ชันได้
 * โดยไฟล์นี้จะ include CH57x_common.h และ header ของทุกโมดูลให้อัตโนมัติ
 */
#include "simple_hal.h"

/*
 * ฟังก์ชัน main — จุดเริ่มต้นของโปรแกรม
 * MCU จะเริ่มทำงานที่ฟังก์ชันนี้หลังจากรีเซ็ต
 */
int main()
{
    /*
     * ขั้นตอนที่ 1: ตั้งค่าความถี่นาฬิกา
     *
     * HSECFG_Capacitance(HSECap_18p) — กำหนดค่าโหลด capacitor
     * สำหรับ external crystal 16MHz ปกติใช้ 18pF
     *
     * SetSysClock(CLK_SOURCE_HSE_PLL_100MHz) — ตั้งสัญญาณนาฬิกา
     * ให้ MCU ทำงานที่ 100MHz โดยใช้ HSE (16MHz) + PLL x6.25
     * ค่านี้สำคัญมากเพราะส่งผลต่อความเร็วและค่าจาก hal_delay_ms
     */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * ขั้นตอนที่ 2: เริ่มต้น GPIO
     *
     * hal_gpio_init(pin, mode) — กำหนดขา GPIO และโหมด
     * - pin: PA11 (พิน 11 ของพอร์ต A) คือหมายเลขขาตาม datasheet
     * - mode: HAL_GPIO_OUTPUT_PP_5mA = output แบบ push-pull 5mA
     *
     * push-pull หมายถึง MCU สามารถดึงขาขึ้น HIGH หรือลง LOW ได้
     * 5mA คือกระแสสูงสุดที่ขานี้จะจ่ายได้ (เพียงพอสำหรับ LED)
     *
     * ฟังก์ชันคืนค่า handle ไว้ใช้ในการอ้างถึงขานี้ในคำสั่งถัดไป
     * ถ้าล้มเหลวจะคืน NULL (เช่น เรียก init ขาซ้ำ)
     */
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    /*
     * ขั้นตอนที่ 3: infinite loop — กระพริบ LED ตลอดไป
     *
     * while (1) หมายถึงทำงานซ้ำไม่สิ้นสุด
     * เป็นโครงสร้างหลักของโปรแกรม MCU ทั่วไป
     */
    while (1) {

        /*
         * hal_gpio_write(handle, value) — เขียนค่าลอจิกไปยังขา GPIO
         * ค่า 1 = HIGH = 3.3V (LED ติดเพราะ anode ต่อกับ PA11, cathode ต่อ GND)
         * ค่า 0 = LOW = 0V (LED ดับ)
         */
        hal_gpio_write(led, 1);     // LED ติด

        /*
         * hal_delay_ms(500) — หยุดรอ 500 มิลลิวินาที
         * ใช้การรอแบบ busy-wait (CPU ทำงานวนรอ ไม่ประหยัดไฟ)
         * ความแม่นยำขึ้นอยู่กับการตั้งค่า SetSysClock ข้างต้น
         */
        hal_delay_ms(500);

        hal_gpio_write(led, 0);     // LED ดับ
        hal_delay_ms(500);
    }
}
