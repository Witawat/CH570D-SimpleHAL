/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.0
 * Date               : 2026/05/22
 * Description        : ตัวอย่างที่ 17 — USB Device HID Keyboard
 *
 * สิ่งที่เรียนรู้:
 * - การใช้ hal_usbdev_init() เริ่มต้น USB Device
 * - การใช้ hal_usbdev_send() ส่ง HID report ไปยัง Host
 * - การใช้ hal_usbdev_attach_cb() รับเหตุการณ์ reset/config
 * - การส่ง key press/release ผ่าน USB HID
 *
 * การต่อวงจร:
 * - CH570D ต่อ USB กับ PC (ใช้ USB micro B หรือ USB-C)
 * - กดปุ่ม PA8 → PC เห็นเป็นกดคีย์ 'A'
 * - UART แสดงสถานะที่ 115200 baud
 * - LED PA11 ติดเมื่อกด, ดับเมื่อปล่อย
 *******************************************************************************/

#include "simple_hal.h"

/* handles สำหรับ modules ต่างๆ */
static hal_usbdev_handle_t usbdev;
static hal_gpio_handle_t led;
static hal_gpio_handle_t btn_pin;   /* PA8 — ปุ่มกด */
static hal_uart_handle_t uart;

static uint8_t prev_btn = 1;        /* สถานะปุ่มครั้งก่อน (1 = ไม่กด) */

/*********************************************************************
 * @fn      usbdev_cb
 *
 * @brief   callback เมื่อเกิดเหตุการณ์ USB (reset / configured)
 *
 * @param   h       - handle ของ USB Device
 * @param   evt     - เหตุการณ์ที่เกิดขึ้น
 * @param   arg     - อาร์กิวเมนต์ (ไม่ได้ใช้)
 *
 * @return  ไม่มี
 */
void usbdev_cb(hal_usbdev_handle_t h, hal_usbdev_evt_t evt, void *arg)
{
    (void)arg;
    if (evt == HAL_USBDEV_EVT_RESET) {
        hal_uart_send(uart, (const uint8_t *)"USB reset\r\n", 11);
    } else if (evt == HAL_USBDEV_EVT_CONFIGURED) {
        hal_uart_send(uart, (const uint8_t *)"USB configured\r\n", 16);
    }
}

int main()
{
    /* ปิด debug pin — ให้ขา PA14/PA15 เป็น GPIO ปกติ */
    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    /* ตั้งค่านาฬิกาที่ 100MHz จาก HSE PLL */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /* เริ่มต้น UART, LED (PA11), และปุ่มกด (PA8 พร้อม pull-up) */
    uart    = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    led     = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    btn_pin = hal_gpio_init(PA8, HAL_GPIO_INPUT_PULLUP);

    hal_uart_send(uart, (const uint8_t *)"USB Device Keyboard Demo\r\n", 25);

    /* เริ่มต้น USB Device และผูก callback */
    usbdev = hal_usbdev_init();
    hal_usbdev_attach_cb(usbdev, usbdev_cb, 0);

    /* HID report: modifier + reserved + 6 key codes (เริ่มต้น = 0) */
    uint8_t kbd_report[8] = { 0 };

    while (1) {
        uint8_t btn = hal_gpio_read(btn_pin);   /* อ่านปุ่มกด (0 = กด) */

        if (btn == 0 && prev_btn == 1) {
            /* ปุ่มเพิ่งถูกกด — ส่ง key code ของ 'A' (0x04) */
            kbd_report[2] = 0x04;
            hal_usbdev_send(usbdev, 1, kbd_report, 8);
            hal_gpio_write(led, 1);
            hal_uart_send(uart, (const uint8_t *)"Key A pressed\r\n", 15);
        }

        if (btn == 1 && prev_btn == 0) {
            /* ปุ่มเพิ่งปล่อย — ส่ง report ว่าง (ปล่อยคีย์ทั้งหมด) */
            kbd_report[2] = 0;
            hal_usbdev_send(usbdev, 1, kbd_report, 8);
            hal_gpio_write(led, 0);
            hal_uart_send(uart, (const uint8_t *)"Key A released\r\n", 16);
        }

        prev_btn = btn;
        hal_delay_ms(10);   /* ป้องกัน debounce */
    }
}
