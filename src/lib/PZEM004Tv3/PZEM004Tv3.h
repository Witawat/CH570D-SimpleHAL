/**
 * @file PZEM004Tv3.h
 * @brief PZEM-004T v3 AC Power Monitor Library สำหรับ CH57x (Modbus RTU)
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับวัด Voltage, Current, Power, Energy, Frequency, Power Factor AC
 * ด้วย PZEM-004T v3 ซึ่งใช้โปรโตคอล **Modbus RTU** ที่ 9600 baud
 *
 * ช่วงการวัด:
 * - Voltage:      80-260V AC, ความละเอียด 0.1V
 * - Current:      0-100A, ความละเอียด 0.001A (ต้องใช้ CT clamp ภายนอก)
 * - Active Power: 0-22kW, ความละเอียด 0.1W
 * - Energy:       0-9999kWh, ความละเอียด 1Wh (รีเซ็ตได้)
 * - Frequency:    45-65Hz, ความละเอียด 0.1Hz
 * - Power Factor: 0.00-1.00, ความละเอียด 0.01
 *
 * @note สำหรับ PZEM-004T v1/v2 (Custom protocol) ให้ใช้ PZEM004T library แทน
 * @note ตรวจสอบ version โดยดูที่ PCB: v3 จะมีสีขาว/ใหม่กว่า
 *
 * **วงจร:**
 * ```
 *   CH57x              PZEM-004T v3
 *   PD5 (TX) ──────────> RX
 *   PD6 (RX) <────────── TX
 *   5V ─────────────────> VCC (⚠️ PZEM ต้องการ 5V)
 *   GND ────────────────> GND
 *
 *   ⚠️ ต่อ L, N, ขั้ว CT ตามคู่มือ PZEM อย่างระมัดระวัง!
 *   ⚠️ PZEM-004T v3 รองรับ Modbus address 0x01-0xF7
 * ```
 *
 * **Modbus Register Map:**
 * ```
 *   Address  | Parameter     | Scale  | Unit
 *   0x0000   | Voltage       | 0.1    | V
 *   0x0001   | Current (Low) | 0.001  | A
 *   0x0002   | Current (High)| 0.001  | A
 *   0x0003   | Power (Low)   | 0.1    | W
 *   0x0004   | Power (High)  | 0.1    | W
 *   0x0005   | Energy (Low)  | 1      | Wh
 *   0x0006   | Energy (High) | 1      | Wh
 *   0x0007   | Frequency     | 0.1    | Hz
 *   0x0008   | Power Factor  | 0.01   | --
 *   0x0009   | Alarm Status  | --     | 0/1
 * ```
 *
 * **การใช้งาน:**
 * @code
 * PZEM004Tv3 pzem;
 * PZEM004Tv3_Init(&pzem, 0x01, USART_PINS_DEFAULT);  // Modbus address=1
 *
 * // อ่านทุกค่าในครั้งเดียว (1 Modbus request)
 * PZEM004Tv3_Data data;
 * if (PZEM004Tv3_ReadAll(&pzem, &data) == PZEM004Tv3_OK) {
 *     // data.voltage, data.current, data.power, data.energy
 *     // data.frequency, data.power_factor, data.alarm
 * }
 *
 * // รีเซ็ต energy counter
 * PZEM004Tv3_ResetEnergy(&pzem);
 * @endcode
 *
 * @author CH57x Library Team
 */

#ifndef __PZEM004TV3_H
#define __PZEM004TV3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Timeout รอรับ response (ms) */
#ifndef PZEM004TV3_TIMEOUT_MS
#define PZEM004TV3_TIMEOUT_MS   1000
#endif

/** @brief Baud rate สำหรับ PZEM-004T v3 */
#define PZEM004TV3_BAUD         BAUD_9600

/** @brief Broadcast address (อ่านได้ แต่ไม่ควรใช้กับหลาย device) */
#define PZEM004TV3_ADDR_BROADCAST  0xF8

/* ========== Modbus Constants ========== */

#define PZEM004TV3_FC_READ      0x04    /**< Function code: Read Input Registers */
#define PZEM004TV3_FC_WRITE     0x06    /**< Function code: Write Single Register */
#define PZEM004TV3_FC_RESET     0x42    /**< Function code: Reset Energy (special) */

#define PZEM004TV3_REG_VOLTAGE  0x0000  /**< Voltage register */
#define PZEM004TV3_REG_CURRENT  0x0001  /**< Current register (low word) */
#define PZEM004TV3_REG_POWER    0x0003  /**< Power register (low word) */
#define PZEM004TV3_REG_ENERGY   0x0005  /**< Energy register (low word) */
#define PZEM004TV3_REG_FREQ     0x0007  /**< Frequency register */
#define PZEM004TV3_REG_PF       0x0008  /**< Power Factor register */
#define PZEM004TV3_REG_ALARM    0x0009  /**< Alarm Status register */
#define PZEM004TV3_REG_COUNT    10      /**< จำนวน register ที่อ่านพร้อมกัน */

/* Holding register สำหรับ set address */
#define PZEM004TV3_HREG_ADDR    0x0002  /**< Device address register */

/* ========== Types ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    PZEM004Tv3_OK            = 0,
    PZEM004Tv3_ERROR_PARAM   = 1,
    PZEM004Tv3_ERROR_TIMEOUT = 2,   /**< ไม่ได้รับ response */
    PZEM004Tv3_ERROR_CRC     = 3,   /**< CRC Modbus ไม่ตรง */
    PZEM004Tv3_ERROR_RESP    = 4,   /**< Response address/function code ไม่ถูกต้อง */
    PZEM004Tv3_ERROR_EXCEPT  = 5    /**< Modbus exception response */
} PZEM004Tv3_Status;

/**
 * @brief ข้อมูลที่อ่านได้ทั้งหมด (จาก 1 Modbus request)
 */
typedef struct {
    float    voltage;        /**< แรงดัน (V) */
    float    current;        /**< กระแส (A) */
    float    power;          /**< กำลัง Active (W) */
    uint32_t energy;         /**< พลังงานสะสม (Wh) */
    float    frequency;      /**< ความถี่ (Hz) */
    float    power_factor;   /**< Power Factor (0.00-1.00) */
    uint8_t  alarm;          /**< 1 = power alarm triggered */
} PZEM004Tv3_Data;

/**
 * @brief PZEM-004T v3 Instance
 */
typedef struct {
    uint8_t  modbus_addr;    /**< Modbus slave address (0x01-0xF7) */
    uint8_t  pin_config;     /**< USART pin configuration */
    uint8_t  initialized;
} PZEM004Tv3;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น PZEM-004T v3 และ USART1 ที่ 9600 baud
 * @param pzem        ตัวแปร instance
 * @param modbus_addr Modbus slave address (0x01-0xF7), default = 0x01
 * @param pin_config  USART pin: USART_PINS_DEFAULT / REMAP1 / REMAP2
 * @return PZEM004Tv3_OK
 * @note เรียก SystemCoreClockUpdate() และ Timer_Init() ก่อนเสมอ
 */
PZEM004Tv3_Status PZEM004Tv3_Init(PZEM004Tv3* pzem, uint8_t modbus_addr,
                                    uint8_t pin_config);

/**
 * @brief อ่านทุกพารามิเตอร์ในครั้งเดียว (1 Modbus request)
 * @param pzem instance
 * @param data ตัวแปรรับผลลัพธ์
 * @return PZEM004Tv3_OK หรือ error code
 */
PZEM004Tv3_Status PZEM004Tv3_ReadAll(PZEM004Tv3* pzem, PZEM004Tv3_Data* data);

/**
 * @brief อ่านแรงดันไฟฟ้า
 * @return แรงดัน (V), -1.0 ถ้า error
 */
float PZEM004Tv3_GetVoltage(PZEM004Tv3* pzem);

/**
 * @brief อ่านกระแสไฟฟ้า
 * @return กระแส (A), -1.0 ถ้า error
 */
float PZEM004Tv3_GetCurrent(PZEM004Tv3* pzem);

/**
 * @brief อ่านกำลังไฟฟ้าแอคทีฟ
 * @return กำลัง (W), -1.0 ถ้า error
 */
float PZEM004Tv3_GetPower(PZEM004Tv3* pzem);

/**
 * @brief อ่านพลังงานสะสม
 * @return พลังงาน (Wh), 0xFFFFFFFF ถ้า error
 */
uint32_t PZEM004Tv3_GetEnergy(PZEM004Tv3* pzem);

/**
 * @brief อ่านความถี่ AC
 * @return ความถี่ (Hz), -1.0 ถ้า error
 */
float PZEM004Tv3_GetFrequency(PZEM004Tv3* pzem);

/**
 * @brief อ่าน Power Factor
 * @return Power Factor (0.00-1.00), -1.0 ถ้า error
 */
float PZEM004Tv3_GetPowerFactor(PZEM004Tv3* pzem);

/**
 * @brief รีเซ็ตตัวนับพลังงาน (Energy counter) เป็น 0
 * @param pzem instance
 * @return PZEM004Tv3_OK หรือ error
 * @note ต้องส่ง password เฉพาะไปยัง device — ดูรายละเอียดใน implementation
 */
PZEM004Tv3_Status PZEM004Tv3_ResetEnergy(PZEM004Tv3* pzem);

/**
 * @brief เปลี่ยน Modbus address ของ device
 * @param pzem     instance
 * @param new_addr address ใหม่ (0x01-0xF7)
 * @return PZEM004Tv3_OK หรือ error
 * @note หลัง set address สำเร็จ ต้องเรียก PZEM004Tv3_Init ใหม่ด้วย address ใหม่
 */
PZEM004Tv3_Status PZEM004Tv3_SetAddress(PZEM004Tv3* pzem, uint8_t new_addr);

#ifdef __cplusplus
}
#endif

#endif /* __PZEM004TV3_H */
