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
 * - LED กระพริบที่ PA11 ทุก 500ms
 * - ปุ่มกดที่ PA8 (กดลงกราวด์) ส่งข้อความผ่าน UART
 * - UART echo ที่ PA3=TX, PA2=RX 115200 baud 8N1
 */

#include "simple_hal.h"

/** @brief   ขา LED (PA11) */
#define LED_PIN         PA11
/** @brief   ขาปุ่มกด (PA8) กดลงกราวด์  */
#define BTN_PIN         PA8

/**
 * @brief   ฟังก์ชันหลัก: เริ่มต้นระบบ, LED กระพริบ, UART echo, อ่านปุ่ม
 *
 * - ตั้งนาฬิกา 100MHz
 * - เริ่ม UART 115200 baud PA3=TX PA2=RX
 * - เริ่ม GPIO LED PA11 ขาเอาต์พุต
 * - เริ่ม GPIO ปุ่ม PA8 ขาอินพุตพุลอัป
 * - ลูปหลัก: กระพริบ LED, echo UART, ส่งข้อความเมื่อกดปุ่ม
 *
 * @return  ไม่มีการคืนค่า
 */
int main()
{
    uint8_t prev_btn = 1;

    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    hal_clk_hse_cfg_cap(HSECap_18p);
    hal_clk_set_sysclock(HAL_CLK_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_gpio_handle_t led = hal_gpio_init(LED_PIN, HAL_GPIO_OUTPUT_PP_5mA);
    hal_gpio_handle_t btn = hal_gpio_init(BTN_PIN, HAL_GPIO_INPUT_PULLUP);

    hal_uart_send(uart, (const uint8_t *)"Ready (LED PA11, BTN PA8)\r\n", 28);

    uint8_t buf[64];
    uint16_t len;
    while (1) {
        hal_gpio_write(led, 0);
        hal_delay_ms(500);
        hal_gpio_write(led, 1);
        hal_delay_ms(500);

        if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {
            hal_uart_send(uart, buf, len);
        }

        uint8_t btn_val = hal_gpio_read(btn);
        if (btn_val == 0 && prev_btn == 1) {
            hal_uart_send(uart, (const uint8_t *)"Button pressed\r\n", 17);
        }
        prev_btn = btn_val;
    }
}
