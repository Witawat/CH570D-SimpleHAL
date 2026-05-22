/*
 * simple_hal_config.h -- ไฟล์คอนฟิกกลางของ SimpleHAL
 *
 * - กำหนดขนาด ring buffer สำหรับ UART
 * - กำหนดขาสัญญาณเริ่มต้น
 * - เปิด/ปิดโมดูล BLE/RF
 *
 * === การเปิด/ปิดโมดูล BLE/RF ===
 *
 * โมดูล BLE และ RF ถูกปิดโดยค่าเริ่มต้น (HAL_ENABLE_BLE=0, HAL_ENABLE_RF=0)
 * เพื่อประหยัดทรัพยากร หากต้องการใช้งาน ให้เปลี่ยนค่าเป็น 1
 *
 * ข้อกำหนดในการเปิด BLE:
 *   1. คัดลอกไฟล์จาก SDK ของ WCH ไปยังโปรเจกต์:
 *      - StdPeriphDriver/inc/CH572BLEPeri_LIB.h    (header)
 *      - StdPeriphDriver/libCH572BLE_PERI.a         (library)
 *   2. ตั้งค่า HAL_ENABLE_BLE = 1
 *   3. ไฟล์ hal_ble.c/h จะถูกคอมไพล์และ include
 *
 * ข้อกำหนดในการเปิด RF:
 *   1. คัดลอกไฟล์จาก SDK ของ WCH ไปยังโปรเจกต์:
 *      - StdPeriphDriver/inc/CH572rf.h               (header)
 *      - StdPeriphDriver/libCH57xRF.a                (library)
 *   2. ตั้งค่า HAL_ENABLE_RF = 1
 *   3. ไฟล์ hal_rf.c/h จะถูกคอมไพล์และ include
 *
 * หมายเหตุ:
 *   - ไฟล์ header (.h) และ library (.a) ข้างต้นมีอยู่ในโฟลเดอร์
 *     D:\XSoFTz\Download\Compressed\ch570-main\ch570-main\EVT\EXAM\
 *     BLE/LIB/ และ RF/LIB/ ตามลำดับ และได้คัดลอกไว้ให้แล้ว
 *   - หากเปิดโมดูลใดแล้วต้องเพิ่ม -lCH572BLE_PERI หรือ -lCH57xRF
 *     ใน linker flags ของ build script หรือ IDE ด้วย
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
/* เปิด/ปิดโมดูล USB Host (0=ปิด, 1=เปิด) — ห้ามใช้พร้อม USB Device */
#ifndef HAL_ENABLE_USBHOST
#define HAL_ENABLE_USBHOST  0
#endif
/* เปิด/ปิดโมดูล USB Device (0=ปิด, 1=เปิด) — ห้ามใช้พร้อม USB Host */
#ifndef HAL_ENABLE_USBDEV
#define HAL_ENABLE_USBDEV   0
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
