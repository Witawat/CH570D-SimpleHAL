#ifndef __HAL_UART_H__
#define __HAL_UART_H__

#include <stdint.h>
#include "core/hal_types.h"
#include "core/hal_ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_DEFAULT_TX_PIN    UART_TX_REMAP_PA3
#define UART_DEFAULT_RX_PIN    UART_RX_REMAP_PA2
#define HAL_UART_MAX_INSTANCES 1

typedef struct hal_uart_obj *hal_uart_handle_t;

hal_uart_handle_t hal_uart_init(uint32_t baudrate, uint8_t tx_remap, uint8_t rx_remap);
hal_status_t      hal_uart_send(hal_uart_handle_t h, const uint8_t *data, uint16_t len);
hal_status_t      hal_uart_send_byte(hal_uart_handle_t h, uint8_t data);
hal_status_t      hal_uart_send_async(hal_uart_handle_t h, const uint8_t *data, uint16_t len, hal_callback_t cb, void *arg);
hal_status_t      hal_uart_recv(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, uint16_t *out_len);
uint8_t           hal_uart_recv_byte(hal_uart_handle_t h);
hal_status_t      hal_uart_recv_async(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, hal_callback_t cb, void *arg);
uint16_t          hal_uart_available(hal_uart_handle_t h);
void              hal_uart_flush(hal_uart_handle_t h);
void              hal_uart_set_trig_bytes(hal_uart_handle_t h, uint8_t bytes);
void              hal_uart_attach_rx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg);
void              hal_uart_attach_tx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg);
void              hal_uart_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif
