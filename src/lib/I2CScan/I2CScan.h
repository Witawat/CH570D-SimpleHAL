/**
 * @file I2CScan.h
 * @brief I2C Address Scanner Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-25
 *
 * @details
 * Library นี้ scan หา I2C device ที่ต่ออยู่บน bus แล้วแสดงผล
 * เป็นตารางผ่าน USART คล้ายกับ i2cdetect บน Linux
 *
 * **รูปแบบผลลัพธ์:**
 * @code
 * I2C_SCAN: Starting I2C scan...
 *      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
 * 00:                                                  
 * 10:                                                  
 * 20:                                                  
 * 30:                            -- -- 3c -- -- 3f -- --
 * 40:                                                  
 * 50:                                                  
 * 60:                            68                   
 * 70:                                                  
 * Scan finished.
 * @endcode
 *
 * **การใช้งาน:**
 * @code
 * #include "User/Lib/I2CScan/I2CScan.h"
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Delay_Init();
 *
 *     USART_SimpleInit(BAUD_115200, USART_PINS_DEFAULT);
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     I2C_Scan();
 * }
 * @endcode
 *
 * @note
 * - ต้องเรียก I2C_SimpleInit() และ USART_SimpleInit() ก่อนใช้งาน
 * - ต้องต่อ pull-up resistor (4.7kΩ) ที่ SDA และ SCL
 * - Address 0x00–0x07 และ 0x78–0x7F เป็น reserved addresses จะถูกข้ามไป
 */

#ifndef __I2C_SCAN_H
#define __I2C_SCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Timeout ในการรอ ACK (milliseconds) */
#define I2C_SCAN_TIMEOUT_MS     10

/** @brief แสดง "--" สำหรับ address ที่ไม่พบ device (1=แสดง, 0=แสดงช่องว่าง) */
#define I2C_SCAN_SHOW_EMPTY     0

/**
 * @brief เลือก Output backend สำหรับแสดงผล
 *
 * ค่าที่รองรับ:
 *   I2C_SCAN_OUTPUT_USART  (0) — ใช้ USART_Print() จาก SimpleUSART (default)
 *   I2C_SCAN_OUTPUT_PRINTF (1) — ใช้ printf() จาก <stdio.h>
 *
 * เปลี่ยนได้ที่นี่ หรือ override ด้วย -D flag ตอน compile:
 *   -DI2C_SCAN_OUTPUT=I2C_SCAN_OUTPUT_PRINTF
 */
#define I2C_SCAN_OUTPUT_USART   0
#define I2C_SCAN_OUTPUT_PRINTF  1

#ifndef I2C_SCAN_OUTPUT
#  define I2C_SCAN_OUTPUT   I2C_SCAN_OUTPUT_PRINTF
#endif

#if (I2C_SCAN_OUTPUT == I2C_SCAN_OUTPUT_USART)
#  include "../../SimpleHAL/SimpleUSART.h"
#  define I2C_SCAN_PRINT(s)       USART_Print(s)
#  define I2C_SCAN_PRINT_NUM(n)   USART_PrintNum((int32_t)(n))
#  define I2C_SCAN_PRINTCHAR(c)   USART_WriteByte((uint8_t)(c))
#else
#  include <stdio.h>
#  define I2C_SCAN_PRINT(s)       printf("%s", (s))
#  define I2C_SCAN_PRINT_NUM(n)   printf("%d", (int)(n))
#  define I2C_SCAN_PRINTCHAR(c)   printf("%c", (int)(c))
#endif

/* ========== Function Prototypes ========== */

/**
 * @brief Scan I2C bus และแสดงผลเป็นตารางผ่าน USART
 *
 * @details
 * ฟังก์ชันนี้จะ:
 * 1. Probe แต่ละ address ตั้งแต่ 0x08 ถึง 0x77
 * 2. ส่ง START → address → ตรวจ ACK/NACK → ส่ง STOP
 * 3. แสดงผลเป็นตารางขนาด 8 แถว × 16 คอลัมน์
 *
 * @note ต้องเรียก I2C_SimpleInit() และ USART_SimpleInit() ก่อนใช้งาน
 *
 * @example
 * I2CScan_Run();
 */
void I2CScan_Run(void);

/**
 * @brief Probe I2C address เพียง address เดียว
 *
 * @param addr 7-bit I2C address (0x00–0x7F)
 * @return 1 ถ้าพบ device ตอบ ACK, 0 ถ้าไม่พบ
 *
 * @example
 * if(I2C_Probe(0x3C)) {
 *     USART_Print("OLED found!\r\n");
 * }
 */
uint8_t I2C_Probe(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_SCAN_H */
