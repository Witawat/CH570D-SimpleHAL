/**
 * @file PCF8574.h
 * @brief PCF8574 I2C GPIO Expander Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับควบคุม GPIO ผ่าน PCF8574 I2C GPIO Expander (8-bit)
 * ขยาย GPIO ได้ 8 ขาผ่าน I2C โดยใช้ address ที่กำหนดด้วย A0-A2
 *
 * **Address Map:**
 * ```
 *   PCF8574  : 0x20 – 0x27  (ขึ้นอยู่กับ A2, A1, A0)
 *   PCF8574A : 0x38 – 0x3F  (ขึ้นอยู่กับ A2, A1, A0)
 *
 *   Address = base | (A2<<2) | (A1<<1) | A0
 *   ตัวอย่าง: A2=0, A1=0, A0=0 → addr = 0x20 (PCF8574) หรือ 0x38 (PCF8574A)
 * ```
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          PCF8574
 *   SCL (PC2) ------> SCL
 *   SDA (PC1) <-----> SDA
 *   3.3V      ------> VCC
 *   GND       ------> GND
 *   GND       ------> A0, A1, A2  (address = 0x20)
 * ```
 *
 * @note Input ต้องตั้ง pin เป็น OUTPUT HIGH ก่อน แล้วจึงอ่านค่าได้
 *       (PCF8574 ใช้ quasi-bidirectional I/O)
 *
 * @example
 * PCF8574_Instance expander;
 * PCF8574_Init(&expander, 0x20);
 * PCF8574_PinMode(&expander, 0, PCF8574_OUTPUT);
 * PCF8574_Write(&expander, 0, HIGH);
 *
 * PCF8574_PinMode(&expander, 7, PCF8574_INPUT);
 * uint8_t val = PCF8574_Read(&expander, 7);
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __PCF8574_H
#define __PCF8574_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Default Addresses ========== */

#define PCF8574_ADDR_BASE   0x20  /**< PCF8574 base address */
#define PCF8574A_ADDR_BASE  0x38  /**< PCF8574A base address */

/* ========== Type Definitions ========== */

/**
 * @brief PCF8574 status codes
 */
typedef enum {
    PCF8574_OK          = 0,
    PCF8574_ERROR_PARAM = 1,
    PCF8574_ERROR_I2C   = 2
} PCF8574_Status;

/**
 * @brief Pin mode สำหรับ PCF8574
 */
typedef enum {
    PCF8574_OUTPUT = 0,  /**< Output mode — ควบคุม state โดยตรง */
    PCF8574_INPUT  = 1   /**< Input mode  — เซต HIGH แล้วอ่านค่า */
} PCF8574_Mode;

/**
 * @brief PCF8574 instance
 */
typedef struct {
    uint8_t  i2c_addr;      /**< I2C address (7-bit) */
    uint8_t  port_state;    /**< สถานะปัจจุบันของ port (bit mask) */
    uint8_t  initialized;   /**< Init flag */
} PCF8574_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น PCF8574
 * @param pcf       pointer ไปยัง PCF8574_Instance
 * @param i2c_addr  I2C address ของ device (เช่น 0x20)
 * @return PCF8574_OK หรือ error
 *
 * @example
 * PCF8574_Instance pcf;
 * PCF8574_Init(&pcf, 0x20);
 */
PCF8574_Status PCF8574_Init(PCF8574_Instance* pcf, uint8_t i2c_addr);

/**
 * @brief ตั้ง pin mode (OUTPUT หรือ INPUT)
 * @param pcf  pointer ไปยัง PCF8574_Instance
 * @param pin  หมายเลข pin (0–7)
 * @param mode PCF8574_OUTPUT หรือ PCF8574_INPUT
 * @return PCF8574_OK หรือ error
 *
 * @note INPUT mode จะ write HIGH ไปที่ pin นั้น (quasi-bidirectional)
 */
PCF8574_Status PCF8574_PinMode(PCF8574_Instance* pcf, uint8_t pin, PCF8574_Mode mode);

/**
 * @brief เขียนค่า HIGH/LOW ไปยัง pin
 * @param pcf   pointer ไปยัง PCF8574_Instance
 * @param pin   หมายเลข pin (0–7)
 * @param value HIGH (1) หรือ LOW (0)
 * @return PCF8574_OK หรือ error
 */
PCF8574_Status PCF8574_Write(PCF8574_Instance* pcf, uint8_t pin, uint8_t value);

/**
 * @brief อ่านค่า pin (ต้อง PinMode เป็น INPUT ก่อน)
 * @param pcf pointer ไปยัง PCF8574_Instance
 * @param pin หมายเลข pin (0–7)
 * @return HIGH (1) หรือ LOW (0), หรือ 0 ถ้า error
 */
uint8_t PCF8574_Read(PCF8574_Instance* pcf, uint8_t pin);

/**
 * @brief เขียนค่าทั้ง port (8-bit) พร้อมกัน
 * @param pcf   pointer ไปยัง PCF8574_Instance
 * @param value 8-bit value (bit0 = pin0, bit7 = pin7)
 * @return PCF8574_OK หรือ error
 */
PCF8574_Status PCF8574_WritePort(PCF8574_Instance* pcf, uint8_t value);

/**
 * @brief อ่านค่าทั้ง port (8-bit) พร้อมกัน
 * @param pcf   pointer ไปยัง PCF8574_Instance
 * @param value pointer สำหรับรับค่า 8-bit
 * @return PCF8574_OK หรือ error
 */
PCF8574_Status PCF8574_ReadPort(PCF8574_Instance* pcf, uint8_t* value);

/**
 * @brief Toggle pin
 * @param pcf pointer ไปยัง PCF8574_Instance
 * @param pin หมายเลข pin (0–7)
 * @return PCF8574_OK หรือ error
 */
PCF8574_Status PCF8574_Toggle(PCF8574_Instance* pcf, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif  /* __PCF8574_H */
