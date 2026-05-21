#ifndef __HAL_SPI_H__
#define __HAL_SPI_H__

#include <stdint.h>
#include "core/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_SPI_MAX_INSTANCES 1

/**
 * @brief   โหมด SPI
 */
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

/**
 * @brief   เริ่มต้น SPI Master ด้วยความถี่, โหมด และลำดับบิต
 *
 * @param   clock_hz - ความถี่ SPI
 * @param   mode - โหมด SPI (MODE0/MODE3)
 * @param   order - ลำดับบิต (MSB/LSB)
 *
 * @return  handle ของ SPI หรือ NULL หากล้มเหลว
 */
hal_spi_handle_t hal_spi_init(uint32_t clock_hz, hal_spi_mode_t mode, hal_spi_bit_order_t order);
/**
 * @brief   ส่ง-รับข้อมูลแบบ Full-duplex (tx/rx อาจเป็น NULL ได้)
 *
 * @param   h - handle ของ SPI
 * @param   tx - ข้อมูลที่จะส่ง (NULL = ส่ง 0xFF)
 * @param   rx - บัฟเฟอร์รับข้อมูล (NULL = ไม่รับ)
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t     hal_spi_transfer(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len);
/**
 * @brief   ส่ง-รับข้อมูลผ่าน DMA (มี callback เมื่อเสร็จ)
 *
 * @param   h - handle ของ SPI
 * @param   tx - ข้อมูลที่จะส่ง (tx หรือ rx ตัวใดตัวหนึ่งต้องเป็น NULL)
 * @param   rx - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 * @param   cb - callback เมื่อเสร็จสิ้น
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  HAL_OK หากสำเร็จ, HAL_BUSY หาก tx และ rx ไม่เป็น NULL
 */
hal_status_t     hal_spi_transfer_dma(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len, hal_callback_t cb, void *arg);
/**
 * @brief   ส่ง 1 ไบต์
 *
 * @param   h - handle ของ SPI
 * @param   data - ข้อมูล 1 ไบต์ที่จะส่ง
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t     hal_spi_send_byte(hal_spi_handle_t h, uint8_t data);
/**
 * @brief   รับ 1 ไบต์ (ส่ง dummy byte ออกไป)
 *
 * @param   h - handle ของ SPI
 *
 * @return  ข้อมูล 1 ไบต์ที่รับได้
 */
uint8_t          hal_spi_recv_byte(hal_spi_handle_t h);
/**
 * @brief   เปลี่ยนความถี่ SPI
 *
 * @param   h - handle ของ SPI
 * @param   clock_hz - ความถี่ใหม่
 */
void             hal_spi_set_speed(hal_spi_handle_t h, uint32_t clock_hz);
/**
 * @brief   ควบคุม CS (Chip Select): 0 = เลือก, 1 = ไม่เลือก
 *
 * @param   h - handle ของ SPI
 * @param   level - ระดับสัญญาณ CS (0 = เลือก, 1 = ไม่เลือก)
 */
void             hal_spi_chip_select(hal_spi_handle_t h, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif
