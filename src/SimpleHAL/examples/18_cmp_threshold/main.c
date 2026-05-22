#include "simple_hal.h"

int main()
{
    R16_PIN_ALTERNATE &= ~RB_PIN_DEBUG_EN;

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    hal_uart_send(uart, (const uint8_t *)"CMP Threshold Demo\r\n", 20);

    hal_cmp_handle_t cmp = hal_cmp_init(
        HAL_CMP_INPUT_PA7_PA2, HAL_CMP_VREF_500MV);
    hal_cmp_enable(cmp);

    uint8_t prev = 0;

    while (1) {
        uint8_t val = hal_cmp_read(cmp);
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
        hal_delay_ms(50);
    }
}
