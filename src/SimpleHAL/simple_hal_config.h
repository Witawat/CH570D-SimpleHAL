/*
 * simple_hal_config.h -- ไฟล์คอนฟิกกลางของ SimpleHAL
 *
 * - กำหนดขนาด ring buffer สำหรับ UART
 * - กำหนดขาสัญญาณเริ่มต้น
 * - เปิด/ปิดโมดูล BLE/RF
 */

#ifndef __SIMPLE_HAL_CONFIG_H__
#define __SIMPLE_HAL_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ขนาด ring buffer สำหรับรับ UART */
#ifndef HAL_UART_RB_SIZE
#define HAL_UART_RB_SIZE     128
#endif

/* ขนาด ring buffer สำหรับส่ง UART */
#ifndef HAL_UART_TX_RB_SIZE
#define HAL_UART_TX_RB_SIZE  128
#endif

/* เปิด/ปิดโมดูล BLE (0=ปิด) */
#ifndef HAL_ENABLE_BLE
#define HAL_ENABLE_BLE      0
#endif
/* เปิด/ปิดโมดูล RF (0=ปิด) */
#ifndef HAL_ENABLE_RF
#define HAL_ENABLE_RF       0
#endif

/* ขา TX เริ่มต้นของ UART0 */
#ifndef HAL_GPIO_BTXD_0
#define HAL_GPIO_BTXD_0     GPIO_Pin_3
#endif
/* ขา RX เริ่มต้นของ UART0 */
#ifndef HAL_GPIO_BRXD_0
#define HAL_GPIO_BRXD_0     GPIO_Pin_2
#endif

#ifdef __cplusplus
}
#endif

#endif
