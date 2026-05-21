#include "hal_uart.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

struct hal_uart_obj {
    uint8_t        used;
    uint32_t       baudrate;
    uint8_t        tx_remap;
    uint8_t        rx_remap;
    uint8_t        trig_bytes;
    uint8_t        rx_buf[HAL_UART_RB_SIZE];
    uint8_t        tx_buf[HAL_UART_TX_RB_SIZE];
    hal_ringbuf_t  rx_rb;
    hal_ringbuf_t  tx_rb;
    hal_callback_t rx_cb;
    void          *rx_arg;
    hal_callback_t tx_cb;
    void          *tx_arg;
    uint8_t       *async_recv_buf;
    uint16_t       async_recv_max;
    uint16_t       async_recv_cnt;
    uint8_t        async_recv_active;
    hal_callback_t async_recv_cb;
    void          *async_recv_arg;
};

static struct hal_uart_obj uart_instances[HAL_UART_MAX_INSTANCES];

static hal_uart_handle_t get_uart(void)
{
    return &uart_instances[0];
}

hal_uart_handle_t hal_uart_init(uint32_t baudrate, uint8_t tx_remap, uint8_t rx_remap)
{
    hal_uart_handle_t h = get_uart();
    if (h->used) return h;

    h->used       = 1;
    h->baudrate   = baudrate;
    h->tx_remap   = tx_remap;
    h->rx_remap   = rx_remap;
    h->trig_bytes = 1;
    h->rx_cb      = NULL;
    h->rx_arg     = NULL;
    h->tx_cb      = NULL;
    h->tx_arg     = NULL;
    h->async_recv_active = 0;

    hal_ringbuf_init(&h->rx_rb, h->rx_buf, HAL_UART_RB_SIZE);
    hal_ringbuf_init(&h->tx_rb, h->tx_buf, HAL_UART_TX_RB_SIZE);

    /* กำหนดขา UART (ค่าเริ่มต้น PA3=TX, PA2=RX) */
    GPIOA_SetBits(HAL_GPIO_BTXD_0);
    GPIOA_ModeCfg(HAL_GPIO_BRXD_0, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(HAL_GPIO_BTXD_0, GPIO_ModeOut_PP_5mA);
    UART_Remap(ENABLE, tx_remap, rx_remap);
    UART_DefInit();
    if (baudrate != 115200) {
        UART_BaudRateCfg(baudrate);
    }

    UART_ByteTrigCfg(UART_1BYTE_TRIG);
    UART_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
    PFIC_EnableIRQ(UART_IRQn);

    return h;
}

hal_status_t hal_uart_send_byte(hal_uart_handle_t h, uint8_t data)
{
    if (!h || !h->used) return HAL_INVALID;
    while (!(UART_GetLinSTA() & STA_TXFIFO_EMP));
    UART_SendByte(data);
    return HAL_OK;
}

hal_status_t hal_uart_send(hal_uart_handle_t h, const uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !len) return HAL_INVALID;
    for (uint16_t i = 0; i < len; i++) {
        while (!(UART_GetLinSTA() & STA_TXFIFO_EMP));
        UART_SendByte(data[i]);
    }
    return HAL_OK;
}

static void uart_start_tx_int(hal_uart_handle_t h)
{
    uint8_t byte;
    if (hal_ringbuf_get(&h->tx_rb, &byte) == HAL_OK) {
        UART_SendByte(byte);
        UART_INTCfg(ENABLE, RB_IER_THR_EMPTY);
    }
}

hal_status_t hal_uart_send_async(hal_uart_handle_t h, const uint8_t *data, uint16_t len, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !len) return HAL_INVALID;

    if (hal_ringbuf_free_space(&h->tx_rb) < len) {
        return HAL_BUSY;
    }

    for (uint16_t i = 0; i < len; i++) {
        hal_ringbuf_put(&h->tx_rb, data[i]);
    }

    h->tx_cb = cb;
    h->tx_arg = arg;
    uart_start_tx_int(h);
    return HAL_OK;
}

hal_status_t hal_uart_recv(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, uint16_t *out_len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !max_len) return HAL_INVALID;

    uint16_t cnt = 0;
    for (uint16_t i = 0; i < max_len; i++) {
        if (hal_ringbuf_get(&h->rx_rb, &data[cnt]) != HAL_OK) break;
        cnt++;
    }
    if (out_len) *out_len = cnt;
    return (cnt > 0) ? HAL_OK : HAL_BUSY;
}

uint8_t hal_uart_recv_byte(hal_uart_handle_t h)
{
    uint8_t data = 0;
    hal_ringbuf_get(&h->rx_rb, &data);
    return data;
}

hal_status_t hal_uart_recv_async(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !max_len || !cb) return HAL_INVALID;

    h->async_recv_buf = data;
    h->async_recv_max = max_len;
    h->async_recv_cnt = 0;
    h->async_recv_active = 1;
    h->async_recv_cb = cb;
    h->async_recv_arg = arg;
    return HAL_OK;
}

uint16_t hal_uart_available(hal_uart_handle_t h)
{
    if (!h || !h->used) return 0;
    return hal_ringbuf_available(&h->rx_rb);
}

void hal_uart_flush(hal_uart_handle_t h)
{
    if (!h || !h->used) return;
    hal_ringbuf_flush(&h->rx_rb);
    hal_ringbuf_flush(&h->tx_rb);
}

void hal_uart_set_trig_bytes(hal_uart_handle_t h, uint8_t bytes)
{
    if (!h || !h->used) return;
    static const UARTByteTRIGTypeDef trig_map[] = {
        UART_1BYTE_TRIG, UART_2BYTE_TRIG, UART_4BYTE_TRIG, UART_7BYTE_TRIG
    };
    uint8_t idx = (bytes <= 1) ? 0 : (bytes <= 2) ? 1 : (bytes <= 4) ? 2 : 3;
    h->trig_bytes = bytes;
    UART_ByteTrigCfg(trig_map[idx]);
}

void hal_uart_attach_rx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->rx_cb = cb;
    h->rx_arg = arg;
}

void hal_uart_attach_tx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->tx_cb = cb;
    h->tx_arg = arg;
}

void hal_uart_int_handler(void)
{
    hal_uart_handle_t h = get_uart();
    if (!h || !h->used) return;

    uint8_t iir = UART_GetITFlag();

    switch (iir) {
    case UART_II_LINE_STAT:
        UART_GetLinSTA();
        break;

    case UART_II_RECV_RDY: {
        uint8_t cnt = h->trig_bytes;
        for (uint8_t i = 0; i < cnt; i++) {
            uint8_t byte = UART_RecvByte();
            hal_ringbuf_put(&h->rx_rb, byte);

            if (h->async_recv_active && h->async_recv_cnt < h->async_recv_max) {
                h->async_recv_buf[h->async_recv_cnt++] = byte;
                if (h->async_recv_cnt >= h->async_recv_max) {
                    h->async_recv_active = 0;
                    if (h->async_recv_cb) {
                        h->async_recv_cb(h->async_recv_arg);
                    }
                }
            }
        }
        if (h->rx_cb) {
            h->rx_cb(h->rx_arg);
        }
        break;
    }

    case UART_II_RECV_TOUT: {
        uint8_t byte;
        while (UART_GetLinSTA() & STA_RECV_DATA) {
            byte = UART_RecvByte();
            hal_ringbuf_put(&h->rx_rb, byte);

            if (h->async_recv_active && h->async_recv_cnt < h->async_recv_max) {
                h->async_recv_buf[h->async_recv_cnt++] = byte;
                if (h->async_recv_cnt >= h->async_recv_max) {
                    h->async_recv_active = 0;
                    if (h->async_recv_cb) {
                        h->async_recv_cb(h->async_recv_arg);
                    }
                }
            }
        }
        if (h->rx_cb) {
            h->rx_cb(h->rx_arg);
        }
        break;
    }

    case UART_II_THR_EMPTY: {
        uint8_t byte;
        if (hal_ringbuf_get(&h->tx_rb, &byte) == HAL_OK) {
            UART_SendByte(byte);
        } else {
            UART_INTCfg(DISABLE, RB_IER_THR_EMPTY);
            if (h->tx_cb) {
                hal_callback_t cb = h->tx_cb;
                void *arg = h->tx_arg;
                h->tx_cb = NULL;
                h->tx_arg = NULL;
                cb(arg);
            }
        }
        break;
    }

    default:
        break;
    }
}

__INTERRUPT __HIGH_CODE void UART_IRQHandler(void)
{
    hal_uart_int_handler();
}
