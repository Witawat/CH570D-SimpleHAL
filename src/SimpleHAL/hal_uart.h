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

/**
 * @brief   เริ่มต้น UART
 *
 * @param   baudrate - อัตราบอด
 * @param   tx_remap - ค่า remap ขาส่ง
 * @param   rx_remap - ค่า remap ขารับ
 *
 * @return  handle ของ UART
 */
hal_uart_handle_t hal_uart_init(uint32_t baudrate, uint8_t tx_remap, uint8_t rx_remap);
/**
 * @brief   ส่งข้อมูล (blocking)
 *
 * @param   h - handle ของ UART
 * @param   data - ข้อมูลที่จะส่ง
 * @param   len - ความยาวข้อมูล
 *
 * @return  สถานะการส่ง
 */
hal_status_t      hal_uart_send(hal_uart_handle_t h, const uint8_t *data, uint16_t len);
/**
 * @brief   ส่ง 1 ไบต์ (blocking)
 *
 * @param   h - handle ของ UART
 * @param   data - ข้อมูล 1 ไบต์
 *
 * @return  สถานะการส่ง
 */
hal_status_t      hal_uart_send_byte(hal_uart_handle_t h, uint8_t data);
/**
 * @brief   ส่งข้อมูลแบบไม่บล็อก (ใช้ interrupt)
 *
 * @param   h - handle ของ UART
 * @param   data - ข้อมูลที่จะส่ง
 * @param   len - ความยาวข้อมูล
 * @param   cb - callback เมื่อส่งเสร็จ
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  สถานะการส่ง
 */
hal_status_t      hal_uart_send_async(hal_uart_handle_t h, const uint8_t *data, uint16_t len, hal_callback_t cb, void *arg);
/**
 * @brief   รับข้อมูล (blocking)
 *
 * @param   h - handle ของ UART
 * @param   data - buffer รับข้อมูล
 * @param   max_len - ขนาดสูงสุดของ buffer
 * @param   out_len - ตัวชี้สำหรับเก็บความยาวที่รับได้
 *
 * @return  สถานะการรับ
 */
hal_status_t      hal_uart_recv(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, uint16_t *out_len);
/**
 * @brief   รับ 1 ไบต์
 *
 * @param   h - handle ของ UART
 *
 * @return  ข้อมูล 1 ไบต์ที่รับได้
 */
uint8_t           hal_uart_recv_byte(hal_uart_handle_t h);
/**
 * @brief   รับข้อมูลแบบไม่บล็อก (ใช้ interrupt)
 *
 * @param   h - handle ของ UART
 * @param   data - buffer รับข้อมูล
 * @param   max_len - ขนาดสูงสุดของ buffer
 * @param   cb - callback เมื่อรับครบ
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  สถานะการรับ
 */
hal_status_t      hal_uart_recv_async(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, hal_callback_t cb, void *arg);
/**
 * @brief   จำนวนไบต์ที่รออ่านใน buffer
 *
 * @param   h - handle ของ UART
 *
 * @return  จำนวนไบต์
 */
uint16_t          hal_uart_available(hal_uart_handle_t h);
/**
 * @brief   ล้าง buffer รับ-ส่ง
 *
 * @param   h - handle ของ UART
 */
void              hal_uart_flush(hal_uart_handle_t h);
/**
 * @brief   กำหนดจำนวนไบต์ที่ trigger interrupt
 *
 * @param   h - handle ของ UART
 * @param   bytes - จำนวนไบต์
 */
void              hal_uart_set_trig_bytes(hal_uart_handle_t h, uint8_t bytes);
/**
 * @brief   ผูก callback เมื่อมีข้อมูลรับเข้า
 *
 * @param   h - handle ของ UART
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 */
void              hal_uart_attach_rx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   ผูก callback เมื่อส่งข้อมูลเสร็จ
 *
 * @param   h - handle ของ UART
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 */
void              hal_uart_attach_tx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg);
/**
 * @brief   ตัวจัดการ interrupt UART
 */
void              hal_uart_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif
