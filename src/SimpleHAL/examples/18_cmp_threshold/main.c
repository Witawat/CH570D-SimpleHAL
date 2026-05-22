/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.0
 * Date               : 2026/05/22
 * Description        : ตัวอย่างที่ 18 — CMP Voltage Threshold Detector
 *
 * สิ่งที่เรียนรู้:
 * - การใช้ hal_cmp_init() เลือกขาเข้า (+) และ VREF
 * - การใช้ hal_cmp_enable() เปิด Comparator
 * - การใช้ hal_cmp_read() อ่านผลลัพธ์เปรียบเทียบ
 * - การตรวจจับการเปลี่ยนแปลง threshold แบบ polling
 *
 * การต่อวงจร:
 * - PA7 (CMP_P0+) ── สัญญาณที่ต้องการวัด (0-3.3V)
 * - PA2 (CMP_N-)  ── แรงดันอ้างอิงจากภายนอก
 *   หรือใช้ VREF ภายใน (500mV ในตัวอย่างนี้)
 * - UART แสดง HIGH/LOW ที่ 115200 baud
 * - LED PA11 ติดเมื่อแรงดัน > 500mV, ดับเมื่อ < 500mV
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /* ปิด debug pin — ให้ขา PA14/PA15 เป็น GPIO ปกติ */
    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    /* ตั้งค่านาฬิกาที่ 100MHz จาก HSE PLL */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น UART (115200 baud) และ LED (PA11) */
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    hal_uart_send(uart, (const uint8_t *)"CMP Threshold Demo\r\n", 20);

    /*
     * เริ่มต้น Comparator:
     * - (+) = PA7, (-) = VREF ภายใน 500mV
     * - ถ้าแรงดัน PA7 > 500mV → output = 1
     * - ถ้าแรงดัน PA7 < 500mV → output = 0
     */
    hal_cmp_handle_t cmp = hal_cmp_init(
        HAL_CMP_INPUT_PA7_PA2, HAL_CMP_VREF_500MV);
    hal_cmp_enable(cmp);

    uint8_t prev = 0;   /* ค่า output ครั้งก่อน */

    while (1) {
        uint8_t val = hal_cmp_read(cmp);   /* อ่าน output ปัจจุบัน */

        /* แสดงผลเฉพาะเมื่อค่าเปลี่ยน (ลดการส่ง UART ซ้ำ) */
        if (val != prev) {
            prev = val;
            if (val) {
                hal_gpio_write(led, 1);
                hal_uart_send(uart,
                    (const uint8_t *)"HIGH (>500mV on PA7)\r\n", 21);
            } else {
                hal_gpio_write(led, 0);
                hal_uart_send(uart,
                    (const uint8_t *)"LOW (<500mV on PA7)\r\n", 20);
            }
        }

        hal_delay_ms(50);   /* อ่านค่าทุก 50ms — ลดการใช้ CPU */
    }
}
