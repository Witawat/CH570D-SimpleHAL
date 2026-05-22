/**
 * @file HC05.h
 * @brief HC-05 Bluetooth Module Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับใช้งาน HC-05 Bluetooth Classic module ผ่าน USART
 * รองรับทั้ง AT command mode (ตั้งค่า) และ Data mode (รับ/ส่งข้อมูล)
 *
 * **โหมดการทำงาน:**
 * - **Data Mode**: ส่ง/รับข้อมูลแบบ Serial ปกติ (default)
 * - **AT Mode**: ตั้งค่า module (ชื่อ, baudrate, PIN, master/slave)
 *   ต้องกด EN/KEY pin ค้างตอน power-on จึงจะเข้า AT mode
 *
 * **วงจร:**
 * ```
 *   CH57x          HC-05
 *   PD5 (TX) ──────> RXD  (⚠️ HC-05 RXD = 3.3V หรือ voltage divider ถ้า 5V)
 *   PD6 (RX) <────── TXD
 *   3.3V/5V ────────> VCC  (3.3V-5V, module ทนได้)
 *   GND ────────────> GND
 *   (GPIO ─────────> EN/KEY  สำหรับเข้า AT mode)
 *   (GPIO <─────────STATE   1=connected, 0=disconnected)
 *
 *   Voltage divider สำหรับ TX → RXD ถ้า MCU 3.3V → HC-05 5V:
 *   TX ──[1kΩ]── RXD
 *                 │
 *               [2kΩ]
 *                 │
 *                GND
 *   (จริงๆ HC-05 รุ่นส่วนใหญ่รับ 3.3V ได้เลย)
 * ```
 *
 * @example
 * // Data Mode: รับ/ส่งข้อมูล
 * HC05_Instance bt;
 * HC05_Init(&bt, USART1, 9600);
 * HC05_SendString(&bt, "Hello from CH57x!\r\n");
 *
 * char buf[64];
 * if (HC05_ReadLine(&bt, buf, 64, 1000) == HC05_OK)
 *     printf("BT received: %s\r\n", buf);
 *
 * @author CH57x Library Team
 */

#ifndef __HC05_H
#define __HC05_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <string.h>

/* ========== Configuration ========== */

/** @brief USART RX buffer size (bytes) — ปรับตาม RAM ที่มี */
#ifndef HC05_RX_BUFFER_SIZE
#define HC05_RX_BUFFER_SIZE   64
#endif

/* ========== Type Definitions ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    HC05_OK           = 0,
    HC05_ERROR_PARAM  = 1,
    HC05_ERROR_TIMEOUT = 2,  /**< ไม่ได้รับข้อมูลภายใน timeout */
    HC05_ERROR_USART  = 3
} HC05_Status;

/**
 * @brief HC-05 Instance
 */
typedef struct {
    uint32_t  baudrate;
    uint8_t   rx_buf[HC05_RX_BUFFER_SIZE];
    uint8_t   rx_head;
    uint8_t   rx_tail;
    uint8_t   initialized;
} HC05_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น HC-05 (USART)
 * @param bt       ตัวแปร instance
 * @param baudrate baud rate (9600 default สำหรับ data mode, 38400 สำหรับ AT mode)
 * @return HC05_OK หรือ error code
 *
 * @note ใช้ USART1 (PD5=TX, PD6=RX) ตาม SimpleUSART default
 * @note ต้องเรียก SimpleHAL.h และ SimpleUSART.h ก่อน
 *
 * @example
 * HC05_Init(&bt, 9600);  // data mode
 */
HC05_Status HC05_Init(HC05_Instance* bt, uint32_t baudrate);

/**
 * @brief ส่ง byte เดียว
 * @param bt  ตัวแปร instance
 * @param byte ข้อมูลที่ต้องการส่ง
 */
void HC05_SendByte(HC05_Instance* bt, uint8_t byte);

/**
 * @brief ส่ง buffer
 * @param bt   ตัวแปร instance
 * @param data ข้อมูล
 * @param len  จำนวน bytes
 */
HC05_Status HC05_Send(HC05_Instance* bt, const uint8_t* data, uint16_t len);

/**
 * @brief ส่ง string (null-terminated)
 * @param bt  ตัวแปร instance
 * @param str string ที่ต้องการส่ง
 *
 * @example HC05_SendString(&bt, "Hello!\r\n");
 */
HC05_Status HC05_SendString(HC05_Instance* bt, const char* str);

/**
 * @brief ตรวจสอบว่ามีข้อมูลรอรับหรือเปล่า
 * @return จำนวน bytes ที่รอรับ
 */
uint8_t HC05_Available(HC05_Instance* bt);

/**
 * @brief อ่าน byte เดียว
 * @param bt   ตัวแปร instance
 * @param byte ตัวแปรรับค่า
 * @return HC05_OK หรือ HC05_ERROR_TIMEOUT ถ้าไม่มีข้อมูล
 */
HC05_Status HC05_ReadByte(HC05_Instance* bt, uint8_t* byte);

/**
 * @brief อ่านจนถึง newline ('\n') หรือ timeout
 * @param bt         ตัวแปร instance
 * @param buf        buffer รับข้อมูล
 * @param max_len    ขนาด buffer
 * @param timeout_ms timeout (ms)
 * @return HC05_OK หรือ HC05_ERROR_TIMEOUT
 *
 * @note null-terminate ให้อัตโนมัติ, ตัด '\r' ออก
 */
HC05_Status HC05_ReadLine(HC05_Instance* bt, char* buf,
                           uint16_t max_len, uint32_t timeout_ms);

/**
 * @brief ส่ง AT command และรอรับ response (สำหรับ AT mode)
 * @param bt         ตัวแปร instance
 * @param cmd        AT command เช่น "AT", "AT+NAME=MyCH32"
 * @param resp       buffer รับ response
 * @param resp_len   ขนาด buffer
 * @param timeout_ms timeout (ms)
 * @return HC05_OK หรือ error code
 *
 * @note ต้องอยู่ใน AT mode ก่อน (EN pin = HIGH ตอน power-on)
 * @example
 * char resp[32];
 * HC05_ATCommand(&bt, "AT", resp, 32, 1000);  // ควรได้ "OK"
 * HC05_ATCommand(&bt, "AT+NAME=MyCH32", resp, 32, 1000);
 */
HC05_Status HC05_ATCommand(HC05_Instance* bt, const char* cmd,
                            char* resp, uint16_t resp_len, uint32_t timeout_ms);

/**
 * @brief Flush RX buffer
 */
void HC05_Flush(HC05_Instance* bt);

#ifdef __cplusplus
}
#endif

#endif /* __HC05_H */
