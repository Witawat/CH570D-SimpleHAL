/**
 * @file MCP4725.h
 * @brief MCP4725 12-bit DAC Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับส่งสัญญาณ Analog Output ด้วย MCP4725 I2C DAC
 * ความละเอียด 12-bit (0-4095) → 0V ถึง VCC
 * มี EEPROM สำหรับจำค่าล่าสุดแม้ไฟดับ
 *
 * **วงจร:**
 * ```
 *   CH57x          MCP4725
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VDD (reference voltage)
 *   GND ────────────> GND
 *   GND ────────────> A0   (addr = 0x60)
 *   (VDD ──────────> A0    addr = 0x61)
 *                    VOUT ──────> Signal out
 *                    VOUT ──[10kΩ]──> next stage (optional load)
 * ```
 *
 * @example
 * MCP4725_Instance dac;
 * MCP4725_Init(&dac, MCP4725_ADDR_A0_GND);
 *
 * // ส่งแรงดัน 1.65V (ครึ่งหนึ่งของ 3.3V)
 * MCP4725_SetVoltage(&dac, 1.65f, 3.3f);
 *
 * // หรือส่งค่า raw
 * MCP4725_SetRaw(&dac, 2048);
 *
 * @author CH57x Library Team
 */

#ifndef __MCP4725_H
#define __MCP4725_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Addresses ========== */
#define MCP4725_ADDR_A0_GND  0x60  /**< A0 = GND (default) */
#define MCP4725_ADDR_A0_VCC  0x61  /**< A0 = VCC */

/* ========== Type Definitions ========== */

/**
 * @brief Power-down mode
 */
typedef enum {
    MCP4725_PD_NORMAL  = 0, /**< Normal operation */
    MCP4725_PD_1K      = 1, /**< Power-down, VOUT → 1kΩ to GND */
    MCP4725_PD_100K    = 2, /**< Power-down, VOUT → 100kΩ to GND */
    MCP4725_PD_500K    = 3  /**< Power-down, VOUT → 500kΩ to GND */
} MCP4725_PowerDown;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    MCP4725_OK          = 0,
    MCP4725_ERROR_PARAM = 1,
    MCP4725_ERROR_I2C   = 2
} MCP4725_Status;

/**
 * @brief MCP4725 Instance
 */
typedef struct {
    uint8_t  i2c_addr;   /**< I2C address */
    uint16_t value;      /**< ค่าปัจจุบัน (0-4095) */
    uint8_t  initialized;
} MCP4725_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น MCP4725
 * @param dac  ตัวแปร instance
 * @param addr MCP4725_ADDR_A0_GND หรือ MCP4725_ADDR_A0_VCC
 * @return MCP4725_OK หรือ error code
 */
MCP4725_Status MCP4725_Init(MCP4725_Instance* dac, uint8_t addr);

/**
 * @brief ตั้งค่า DAC แบบ raw (0-4095)
 * @param dac   ตัวแปร instance
 * @param value ค่า 12-bit (0-4095)
 * @return MCP4725_OK หรือ error code
 *
 * @note ใช้ Fast Write mode (2 bytes) — ไม่บันทึก EEPROM
 */
MCP4725_Status MCP4725_SetRaw(MCP4725_Instance* dac, uint16_t value);

/**
 * @brief ตั้งค่าแรงดัน output
 * @param dac     ตัวแปร instance
 * @param voltage แรงดันที่ต้องการ (V)
 * @param vref    แรงดัน reference = VDD ของ module (V) เช่น 3.3
 * @return MCP4725_OK หรือ error code
 *
 * @example MCP4725_SetVoltage(&dac, 1.65f, 3.3f);  // 1.65V
 */
MCP4725_Status MCP4725_SetVoltage(MCP4725_Instance* dac, float voltage, float vref);

/**
 * @brief ตั้งค่า DAC และบันทึกลง EEPROM
 * @param dac   ตัวแปร instance
 * @param value ค่า 12-bit (0-4095)
 * @return MCP4725_OK หรือ error code
 *
 * @note เขียน EEPROM ช้ากว่า (~25ms) แต่จำค่าแม้ไฟดับ
 * @warning EEPROM เขียนได้ประมาณ 1 ล้านครั้ง
 */
MCP4725_Status MCP4725_SetRawEEPROM(MCP4725_Instance* dac, uint16_t value);

/**
 * @brief อ่านค่าปัจจุบันจาก DAC
 * @param dac   ตัวแปร instance
 * @param value ตัวแปรรับค่า (0-4095)
 */
MCP4725_Status MCP4725_GetRaw(MCP4725_Instance* dac, uint16_t* value);

/**
 * @brief ตั้ง Power-Down mode
 * @param dac ตัวแปร instance
 * @param pd  MCP4725_PD_NORMAL / PD_1K / PD_100K / PD_500K
 */
MCP4725_Status MCP4725_SetPowerDown(MCP4725_Instance* dac, MCP4725_PowerDown pd);

#ifdef __cplusplus
}
#endif

#endif /* __MCP4725_H */
