/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 7 — อ่าน Flash unique ID
 *
 * สิ่งที่เรียนรู้:
 * - การอ่าน Unique ID ของ MCU จาก Flash
 * - การอ่านค่า Configuration Word จาก Flash
 * - การแปลงค่า binary เป็น hexadecimal string
 * - การส่งข้อมูลผ่าน UART
 *
 * Flash ใน CH572 แบ่งเป็น:
 * - โปรแกรม (Program memory): สำหรับเก็บโค้ด
 * - Data Flash: สำหรับเก็บข้อมูล (สามารถเขียนลบได้)
 * - Configuration: ค่าตั้งค่าเริ่มต้นของ MCU
 * - Unique ID: หมายเลขประจำตัว 8 ไบต์ที่ไม่ซ้ำกัน (burned-in)
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /*
     * uid[8] — เก็บ Unique ID 8 ไบต์
     * config — เก็บค่า Configuration Word (16-bit)
     * buf[64] — บัฟเฟอร์สำหรับข้อความที่จะส่งผ่าน UART
     */
    uint8_t uid[8];
    uint16_t config;
    uint8_t buf[64];

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น UART สำหรับแสดงผล
     *
     * ใช้ UART_DEFAULT_TX_PIN (= PA3) และ UART_DEFAULT_RX_PIN (= PA2)
     * ความเร็ว 115200 baud
     * ต่อ USB-UART converter เพื่อดูผลลัพธ์บน PC
     */
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    /*
     * hal_flash_get_unique_id(buffer)
     * อ่าน Unique ID 8 ไบต์ของ MCU
     * CH572 แต่ละตัวจะมี ID นี้ไม่ซ้ำกัน (ใช้สำหรับ product serial number)
     *
     * ข้อมูลนี้ถูกโปรแกรมจากโรงงาน ไม่สามารถเปลี่ยนแปลงได้
     */
    hal_flash_get_unique_id(uid);

    /*
     * hal_flash_read_config(&config)
     * อ่านค่า Configuration Word
     * เป็นค่า 16-bit ที่ใช้ config การทำงานเริ่มต้นของ MCU
     * เช่น บิต boot, watchdog, ฯลฯ
     */
    hal_flash_read_config(&config);

    /*
     * ส่งข้อความ "Flash UID: " ไปยัง UART
     * (const uint8_t *) — แคสต์ string literal เป็น uint8_t pointer
     * 11 = ความยาว "Flash UID: " (10 ตัว + space)
     */
    hal_uart_send(uart, (const uint8_t *)"Flash UID: ", 11);

    /*
     * ส่ง UID ที่อ่านได้ ผ่าน UART ในรูปแบบ hexadecimal
     *
     * วิธีการแปลง byte → hex:
     * "0123456789ABCDEF" คือ lookup table
     * - uid[i] >> 4: ดึง 4 บิตบน (high nibble) → ได้ index 0-15
     * - uid[i] & 0x0F: ดึง 4 บิตล่าง (low nibble) → ได้ index 0-15
     *
     * ตัวอย่าง: uid[0] = 0x3A
     *   - 0x3A >> 4 = 0x03 → "0123456789ABCDEF"[3] = '3'
     *   - 0x3A & 0x0F = 0x0A → "0123456789ABCDEF"[10] = 'A'
     *   ได้ "3A"
     */
    for (uint8_t i = 0; i < 8; i++) {

        /*
         * buf[0] = อักขระ hex ตัวแรก (high nibble)
         * buf[1] = อักขระ hex ตัวที่สอง (low nibble)
         * buf[2] = ถ้าไม่ใช่ตัวสุดท้าย ให้คั่นด้วย ':' หรือ '\r' ถ้าตัวสุดท้าย
         *
         * Ternary operator: (i < 7) ? ':' : '\r'
         * ถ้า i < 7 (ไม่ใช่ตัวสุดท้าย) → ใช้ ':'
         * ถ้า i >= 7 (ตัวสุดท้าย) → ใช้ '\r'
         */
        buf[0] = "0123456789ABCDEF"[uid[i] >> 4];
        buf[1] = "0123456789ABCDEF"[uid[i] & 0x0F];
        buf[2] = (i < 7) ? ':' : '\r';

        /*
         * ส่ง UART — จำนวนไบต์ขึ้นกับว่าเป็นตัวสุดท้ายหรือไม่
         * ถ้าไม่ใช่: ส่ง 3 ไบต์ (2 hex + ':')
         * ถ้าใช่: ส่ง 2 ไบต์ (2 hex + '\r')
         */
        hal_uart_send(uart, buf, (i < 7) ? 3 : 2);
    }

    /*
     * ส่ง '\n' เพื่อขึ้นบรรทัดใหม่
     * ทำให้ terminal แสดงผลเป็นระเบียบ
     */
    hal_uart_send(uart, (const uint8_t *)"\n", 1);

    /*
     * infinite loop — หยุดโปรแกรมไว้ที่นี้
     * หลังจากแสดง UID เสร็จแล้ว ไม่ต้องทำอะไรอีก
     * while (1); = หยุดตรงนี้ตลอดไป
     *
     * ถ้าต้องการให้ CPU หลับ: while (1) { __WFI(); }
     */
    while (1);
}
