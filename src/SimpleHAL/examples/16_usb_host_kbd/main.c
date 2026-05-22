#include "simple_hal.h"

static hal_uart_handle_t uart;
static hal_gpio_handle_t led;

void usb_cb(hal_usbhost_handle_t h, hal_usbhost_evt_t evt, void *arg)
{
    (void)arg;
    if (evt == HAL_USBHOST_EVT_READY) {
        hal_usbhost_dev_type_t t = hal_usbhost_get_device_type(h);
        hal_uart_send(uart, (const uint8_t *)"USB: ", 5);
        switch (t) {
        case HAL_USBHOST_DEV_KEYBOARD:
            hal_uart_send(uart, (const uint8_t *)"Keyboard\r\n", 10); break;
        case HAL_USBHOST_DEV_MOUSE:
            hal_uart_send(uart, (const uint8_t *)"Mouse\r\n", 7); break;
        default:
            hal_uart_send(uart, (const uint8_t *)"Other\r\n", 7); break;
        }
    } else if (evt == HAL_USBHOST_EVT_DISCONNECT) {
        hal_uart_send(uart, (const uint8_t *)"USB: removed\r\n", 14);
    }
}

static void print_hex(uint8_t val)
{
    const char hex[] = "0123456789ABCDEF";
    char buf[3] = { hex[val >> 4], hex[val & 0xF], ' ' };
    hal_uart_send(uart, (const uint8_t *)buf, 3);
}

int main()
{
    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    uart = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    led  = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    hal_uart_send(uart, (const uint8_t *)"USB Host Keyboard Demo\r\n", 23);

    hal_usbhost_handle_t host = hal_usbhost_init();
    hal_usbhost_attach_cb(host, usb_cb, 0);

    uint8_t ready = 0;
    hal_usbhost_kbd_report_t report;

    while (1) {
        hal_usbhost_evt_t evt = hal_usbhost_poll(host);

        if (evt == HAL_USBHOST_EVT_READY) ready = 1;
        else if (evt == HAL_USBHOST_EVT_DISCONNECT) ready = 0;

        if (ready && hal_usbhost_get_device_type(host) == HAL_USBHOST_DEV_KEYBOARD) {
            if (hal_usbhost_kbd_read(host, &report) == HAL_OK) {
                hal_gpio_toggle(led);
                for (int i = 0; i < 8; i++) print_hex(((uint8_t *)&report)[i]);
                hal_uart_send(uart, (const uint8_t *)"\r\n", 2);
            }
        }

        hal_delay_ms(10);
    }
}
