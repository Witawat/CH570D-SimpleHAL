#include "simple_hal.h"

static hal_usbdev_handle_t usbdev;
static hal_gpio_handle_t led;
static hal_gpio_handle_t btn_pin;
static hal_uart_handle_t uart;

static uint8_t prev_btn = 1;

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
    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    uart    = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    led     = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    btn_pin = hal_gpio_init(PA8, HAL_GPIO_INPUT_PULLUP);

    hal_uart_send(uart, (const uint8_t *)"USB Device Keyboard Demo\r\n", 25);

    usbdev = hal_usbdev_init();
    hal_usbdev_attach_cb(usbdev, usbdev_cb, 0);

    uint8_t kbd_report[8] = { 0 };

    while (1) {
        uint8_t btn = hal_gpio_read(btn_pin);
        if (btn == 0 && prev_btn == 1) {
            kbd_report[2] = 0x04;
            hal_usbdev_send(usbdev, 1, kbd_report, 8);
            hal_gpio_write(led, 1);
            hal_uart_send(uart, (const uint8_t *)"Key A pressed\r\n", 15);
        }
        if (btn == 1 && prev_btn == 0) {
            kbd_report[2] = 0;
            hal_usbdev_send(usbdev, 1, kbd_report, 8);
            hal_gpio_write(led, 0);
            hal_uart_send(uart, (const uint8_t *)"Key A released\r\n", 16);
        }
        prev_btn = btn;
        hal_delay_ms(10);
    }
}
