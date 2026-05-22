/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/22
 * Description        : ตัวอย่างที่ 15 — Non-blocking delay ด้วย hal_softimer
 *
 * สิ่งที่เรียนรู้:
 * - การใช้ hal_softimer_init() กำหนด mode ONESHOT / PERIODIC
 * - การใช้ hal_softimer_start() เริ่ม delay ที่ต้องการ
 * - การใช้ hal_softimer_expired() ตรวจสอบใน main loop
 * - การใช้ soft timer หลายตัวพร้อมกัน
 *
 * เปรียบเทียบกับตัวอย่างที่ 14:
 * - 14 ใช้ Timer hardware + DIY SysTick (ซับซ้อน)
 * - 15 ใช้ hal_softimer จาก core/ โดยเฉพาะ (ง่ายกว่า)
 *
 * ข้อดีของ hal_softimer:
 * - ไม่เปลือง hardware timer
 * - มีกี่ตัวก็ได้ (แค่ประกาศ hal_softimer_t)
 * - ไม่ต้อง calibrate — ใช้ SysTick อัตโนมัติ
 * - API กระชับ: init → start → expired
 *******************************************************************************/

#include "simple_hal.h"

/*
 * soft timer 2 ตัว:
 * - led_tmr: กระพริบ LED ทุก 500ms
 * - uart_tmr: ส่งข้อความทุก 1 วินาที
 */
static hal_softimer_t led_tmr;
static hal_softimer_t uart_tmr;
static hal_gpio_handle_t led;
static hal_uart_handle_t uart;
static uint32_t counter = 0;

int main()
{
    char msg[32];

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น GPIO + UART */
    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    uart = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_uart_send(uart, (const uint8_t *)"hal_softimer demo ready\r\n", 26);

    /*
     * เริ่มต้น soft timer:
     * - led_tmr: PERIODIC — วนซ้ำทุกครั้งที่ครบ
     * - uart_tmr: PERIODIC — วนซ้ำทุกครั้งที่ครบ
     *
     * โหมดที่รองรับ:
     * - HAL_SOFTIMER_PERIODIC: วนซ้ำอัตโนมัติ
     * - HAL_SOFTIMER_ONESHOT: ทำงานครั้งเดียวแล้วหยุด
     */
    hal_softimer_init(&led_tmr, HAL_SOFTIMER_PERIODIC);
    hal_softimer_start(&led_tmr, 500000);  // 500ms

    hal_softimer_init(&uart_tmr, HAL_SOFTIMER_PERIODIC);
    hal_softimer_start(&uart_tmr, 1000000);  // 1 วินาที

    /*
     * main loop:
     * - ไม่มี hal_delay_ms() เลย — CPU ไม่เคยหยุด
     * - UART echo ทำงานได้ตลอดเวลา
     * - สามารถเพิ่ม hal_ble_process(), อ่าน sensor ฯลฯ
     */
    while (1) {
        /* LED กระพริบทุก 500ms แบบ non-blocking */
        if (hal_softimer_expired(&led_tmr)) {
            hal_gpio_toggle(led);
        }

        /* ส่งข้อความทุก 1 วินาที แบบ non-blocking */
        if (hal_softimer_expired(&uart_tmr)) {
            sprintf(msg, "Count: %lu\r\n", counter++);
            hal_uart_send(uart, (uint8_t *)msg, strlen(msg));
        }

        /* UART echo (non-blocking) — ทำงานระหว่าง "รอ" */
        {
            uint8_t buf[64];
            uint16_t len;
            if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {
                hal_uart_send(uart, buf, len);
            }
        }
    }
}
