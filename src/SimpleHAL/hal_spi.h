#ifndef __HAL_SPI_H__
#define __HAL_SPI_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_SPI_MAX_INSTANCES 1

typedef enum {
    HAL_SPI_MODE0 = 0,
    HAL_SPI_MODE3 = 2,
} hal_spi_mode_t;

typedef enum {
    HAL_SPI_MSB_FIRST = 0,
    HAL_SPI_LSB_FIRST,
} hal_spi_bit_order_t;

typedef enum {
    HAL_SPI_FULL_DUPLEX = 0,
    HAL_SPI_HALF_DUPLEX,
} hal_spi_duplex_t;

typedef struct hal_spi_obj *hal_spi_handle_t;

hal_spi_handle_t hal_spi_init(uint32_t clock_hz, hal_spi_mode_t mode, hal_spi_bit_order_t order);
hal_status_t     hal_spi_transfer(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len);
hal_status_t     hal_spi_transfer_dma(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len, hal_callback_t cb, void *arg);
hal_status_t     hal_spi_send_byte(hal_spi_handle_t h, uint8_t data);
uint8_t          hal_spi_recv_byte(hal_spi_handle_t h);
void             hal_spi_set_speed(hal_spi_handle_t h, uint32_t clock_hz);
void             hal_spi_chip_select(hal_spi_handle_t h, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif
