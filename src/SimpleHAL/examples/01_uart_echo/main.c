/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 1 — UART echo ผ่าน ring buffer
 *
 * สิ่งที่เรียนรู้:
 * - การ init UART และตั้งค่า baudrate
 * - การส่งข้อความ (string) ผ่าน UART
 * - การรับข้อมูลจาก UART แบบไม่บล็อก (non-blocking)
 * - การทำงานของ ring buffer สำหรับรับข้อมูล
 *
 * การต่อวงจร: ต่อ UART (PA3=TX, PA2=RX) กับ USB-UART converter
 * ตั้งโปรแกรม terminal ที่ 115200 baud, 8N1
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /*
     * ประกาศตัวแปรสำหรับรับข้อมูล
     * buf[64] — บัฟเฟอร์ขนาด 64 ไบต์สำหรับเก็บข้อมูลที่รับมา
     * len — ตัวแปรรับจำนวนไบต์ที่อ่านได้จริงจาก hal_uart_recv
     */
    uint8_t buf[64];
    uint16_t len;

    /* ตั้งค่านาฬิกา — จำเป็นเสมอสำหรับการทำงานของ MCU */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น UART
     *
     * hal_uart_init(baudrate, tx_pin, rx_pin)
     * - baudrate: 115200 bps — ความเร็วในการสื่อสาร
     * - tx_pin: UART_DEFAULT_TX_PIN (= PA3) — ขาส่งข้อมูล
     * - rx_pin: UART_DEFAULT_RX_PIN (= PA2) — ขารับข้อมูล
     *
     * UART_REMAP: CH572 สามารถเปลี่ยนตำแหน่งขา UART ได้
     * ถ้าต้องการสลับขา TX/RX สามารถใช้ UART_TX_REMAP_PA2 + UART_RX_REMAP_PA3
     *
     * เมื่อ init สำเร็จ UART จะสร้าง ring buffer สำหรับรับข้อมูล
     * และเปิด interrupt รับข้อมูลอัตโนมัติ — ผู้ใช้ไม่ต้องจัดการ interrupt เอง
     */
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    /*
     * ส่งข้อความต้อนรับ
     * (const uint8_t *) — แคสต์จาก string literal เป็น pointer to byte
     * 18 — ความยาวของข้อความ (นับรวม \r\n)
     *
     * hal_uart_send() เป็นแบบ blocking (รอจนส่งครบทุกไบต์)
     */
    hal_uart_send(uart, (const uint8_t *)"UART Echo Ready\r\n", 18);

    /*
     * main loop: รับ → ส่งกลับ (echo)
     *
     * hal_uart_recv() เป็นแบบ non-blocking:
     * - ถ้ามีข้อมูลใน ring buffer → คัดลอกไป buf และคืน HAL_OK
     * - ถ้าไม่มีข้อมูล → คืน HAL_BUSY ทันที (ไม่รอ)
     *
     * ข้อดี: CPU ไม่ต้องรอข้อมูล สามารถทำงานอย่างอื่นระหว่างรอได้
     */
    while (1) {

        /*
         * hal_uart_recv(handle, buffer, max_len, &out_len)
         * - h: handle UART
         * - buf: พอยน์เตอร์ไปยังบัฟเฟอร์สำหรับเก็บข้อมูล
         * - sizeof(buf): จำนวนไบต์สูงสุดที่อ่านได้ (64)
         * - &len: พอยน์เตอร์ไปยังตัวแปรรับจำนวนไบต์ที่อ่านได้จริง
         *
         * คืนค่า HAL_OK ถ้ามีข้อมูล, HAL_BUSY ถ้าไม่มี
         * ใช้ if เช็ค返回值เพื่อให้แน่ใจว่ามีข้อมูลก่อนส่งกลับ
         */
        if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {

            /*
             * ส่งข้อมูลกลับ (echo)
             * ส่งเฉพาะจำนวนไบต์ที่รับมา (len) ไม่ใช่ sizeof(buf)
             * เพื่อไม่ให้ส่งข้อมูลขยะไป
             */
            hal_uart_send(uart, buf, len);
        }

        /*
         * ถ้าไม่มีข้อมูล ก็วน loop ต่อไปเรื่อยๆ
         * CPU ว่างและสามารถทำงานอื่นในอนาคตได้
         */
    }
}
