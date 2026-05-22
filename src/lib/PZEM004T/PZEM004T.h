/**
 * @file PZEM004T.h
 * @brief PZEM-004T (v1/v2) AC Power Monitor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับวัด Voltage, Current, Power, Energy AC ด้วย PZEM-004T (เวอร์ชัน 1/2)
 * ใช้โปรโตคอล Custom Serial (ไม่ใช่ Modbus) ที่ความเร็ว 9600 baud
 *
 * ช่วงการวัด:
 * - Voltage: 80-260V AC, ความละเอียด 0.1V
 * - Current: 0-100A, ความละเอียด 0.01A (ต้องใช้ CT clamp ภายนอก)
 * - Power: 0-22kW, ความละเอียด 0.1W
 * - Energy: 0-9999kWh, ความละเอียด 1Wh
 *
 * @note สำหรับ PZEM-004T v3 (Modbus RTU) ให้ใช้ PZEM004Tv3 library แทน
 *
 * **วงจร:**
 * ```
 *   CH57x              PZEM-004T
 *   PD5 (TX) ──────────> RX
 *   PD6 (RX) <────────── TX
 *   5V ─────────────────> VCC (⚠️ PZEM ต้องการ 5V)
 *   GND ────────────────> GND
 *
 *   ⚠️ แรงดัน RX ของ CH57x รับ 5V tolerant → ต่อตรงได้
 *   ⚠️ ต่อ L, N, ขั้ว CT ตามคู่มือ PZEM อย่างระมัดระวัง!
 * ```
 *
 * **การใช้งาน:**
 * @code
 * PZEM004T pzem;
 * PZEM004T_Init(&pzem, USART_PINS_DEFAULT);
 *
 * float voltage = PZEM004T_GetVoltage(&pzem);
 * float current = PZEM004T_GetCurrent(&pzem);
 * float power   = PZEM004T_GetPower(&pzem);
 * uint32_t energy = PZEM004T_GetEnergy(&pzem);
 *
 * // อ่านทุกค่าพร้อมกัน
 * PZEM004T_Data data;
 * if (PZEM004T_ReadAll(&pzem, &data) == PZEM004T_OK) {
 *     // ใช้ data.voltage, data.current, data.power, data.energy
 * }
 * @endcode
 *
 * @author CH57x Library Team
 */

#ifndef __PZEM004T_H
#define __PZEM004T_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Timeout รอรับ response (ms) */
#ifndef PZEM004T_TIMEOUT_MS
#define PZEM004T_TIMEOUT_MS     1000
#endif

/** @brief Baud rate สำหรับ PZEM-004T */
#define PZEM004T_BAUD           BAUD_9600

/* ========== Internal Constants ========== */

#define PZEM004T_CMD_VOLTAGE    0xB0
#define PZEM004T_CMD_CURRENT    0xB1
#define PZEM004T_CMD_POWER      0xB2
#define PZEM004T_CMD_ENERGY     0xB3
#define PZEM004T_CMD_ADDR       0xB4
#define PZEM004T_CMD_ALARM      0xB5

#define PZEM004T_RESP_VOLTAGE   0xA0
#define PZEM004T_RESP_CURRENT   0xA1
#define PZEM004T_RESP_POWER     0xA2
#define PZEM004T_RESP_ENERGY    0xA3
#define PZEM004T_RESP_ADDR      0xA4
#define PZEM004T_RESP_ALARM     0xA5

#define PZEM004T_CMD_LEN        6
#define PZEM004T_RESP_LEN       7

/* ========== Default Address ========== */

/** @brief Default address ของ PZEM-004T = 192.168.1.1 */
#define PZEM004T_DEFAULT_ADDR   {0xC0, 0xA8, 0x01, 0x01}

/* ========== Types ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    PZEM004T_OK              = 0,
    PZEM004T_ERROR_PARAM     = 1,
    PZEM004T_ERROR_TIMEOUT   = 2,   /**< ไม่ได้รับ response */
    PZEM004T_ERROR_CHECKSUM  = 3,   /**< Checksum ไม่ตรง */
    PZEM004T_ERROR_RESP      = 4    /**< Response byte ไม่ถูกต้อง */
} PZEM004T_Status;

/**
 * @brief ข้อมูลที่อ่านได้ทั้งหมด
 */
typedef struct {
    float    voltage;        /**< แรงดัน (V), -1 ถ้า error */
    float    current;        /**< กระแส (A), -1 ถ้า error */
    float    power;          /**< กำลัง (W), -1 ถ้า error */
    uint32_t energy;         /**< พลังงานสะสม (Wh), 0xFFFFFFFF ถ้า error */
    uint8_t  alarm;          /**< 1 = power alarm triggered */
} PZEM004T_Data;

/**
 * @brief PZEM-004T Instance
 */
typedef struct {
    uint8_t  addr[4];        /**< Device address (4 bytes) */
    uint8_t  pin_config;     /**< USART pin configuration */
    uint8_t  initialized;
} PZEM004T;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น PZEM-004T และ USART1 ที่ 9600 baud
 * @param pzem       ตัวแปร instance
 * @param pin_config USART pin: USART_PINS_DEFAULT / USART_PINS_REMAP1 / USART_PINS_REMAP2
 * @return PZEM004T_OK
 * @note เรียก SystemCoreClockUpdate() และ Timer_Init() ก่อนเสมอ
 */
PZEM004T_Status PZEM004T_Init(PZEM004T* pzem, uint8_t pin_config);

/**
 * @brief ตั้งค่า device address (default = 192.168.1.1)
 * @param pzem  instance
 * @param addr  array 4 bytes เช่น {0xC0, 0xA8, 0x01, 0x02}
 */
void PZEM004T_SetAddress(PZEM004T* pzem, uint8_t addr[4]);

/**
 * @brief อ่านแรงดันไฟฟ้า
 * @param pzem instance
 * @return แรงดัน (V), -1.0 ถ้า error
 */
float PZEM004T_GetVoltage(PZEM004T* pzem);

/**
 * @brief อ่านกระแสไฟฟ้า
 * @param pzem instance
 * @return กระแส (A), -1.0 ถ้า error
 */
float PZEM004T_GetCurrent(PZEM004T* pzem);

/**
 * @brief อ่านกำลังไฟฟ้าแอคทีฟ
 * @param pzem instance
 * @return กำลัง (W), -1.0 ถ้า error
 */
float PZEM004T_GetPower(PZEM004T* pzem);

/**
 * @brief อ่านพลังงานสะสม
 * @param pzem instance
 * @return พลังงาน (Wh), 0xFFFFFFFF ถ้า error
 */
uint32_t PZEM004T_GetEnergy(PZEM004T* pzem);

/**
 * @brief อ่านค่าทั้งหมดในครั้งเดียว (4 requests แยกกัน)
 * @param pzem instance
 * @param data ตัวแปรรับผลลัพธ์
 * @return PZEM004T_OK ถ้าอ่านได้อย่างน้อยหนึ่งค่า
 */
PZEM004T_Status PZEM004T_ReadAll(PZEM004T* pzem, PZEM004T_Data* data);

/**
 * @brief ตั้ง Power Alarm threshold
 * @param pzem      instance
 * @param watts     threshold (W), 0 = ปิด alarm
 * @return PZEM004T_OK หรือ error
 */
PZEM004T_Status PZEM004T_SetAlarm(PZEM004T* pzem, uint16_t watts);

#ifdef __cplusplus
}
#endif

#endif /* __PZEM004T_H */
