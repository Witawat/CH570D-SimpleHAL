/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : UART echo demo using SimpleHAL abstraction layer
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 * ตัวอย่างการใช้งาน SimpleHAL เบื้องต้น
 * - ตั้งค่านาฬิกาที่ 100MHz จาก HSE PLL
 * - เริ่มต้น UART ผ่าน hal_uart_init() ซึ่งจัดการ GPIO, Remap, Interrupt ให้อัตโนมัติ
 * - ส่งข้อความต้อนรับ แล้วรับ-ส่งกลับ (echo) แบบ non-blocking ผ่าน ring buffer
 *
 * การต่อวงจร: UART (PA3=TX, PA2=RX) กับ USB-UART converter ที่ 115200 baud 8N1
 */

#include "simple_hal.h"

int main()
{
    /* ปิดขา debug เพื่อประหยัดพลังงาน */
    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    /* ตั้งค่านาฬิกาที่ 100MHz จาก HSE PLL */
    hal_clk_hse_cfg_cap(HSECap_18p);
    hal_clk_set_sysclock(HAL_CLK_PLL_100MHz);

    /*
     * เริ่มต้น UART
     * - ความเร็ว 115200 baud
     * - ขา TX = PA3, RX = PA2
     * - ฟังก์ชันนี้จัดการ GPIO, Remap, เปิด Interrupt ให้อัตโนมัติ
     */
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    /* ส่งข้อความต้อนรับ */
    hal_uart_send(uart, (const uint8_t *)"UART Echo Ready\r\n", 18);

    /*
     * ลูปหลัก: รับข้อมูลแบบ non-blocking แล้วส่งกลับ (echo)
     * - hal_uart_recv() อ่านจาก ring buffer ไม่ต้องรอข้อมูล
     * - ถ้ามีข้อมูลให้ส่งกลับทันที
     */
    uint8_t buf[64];
    uint16_t len;
    while (1) {
        if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {
            hal_uart_send(uart, buf, len);
        }
    }
}
