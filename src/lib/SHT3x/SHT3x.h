/**
 * @file SHT3x.h
 * @brief SHT30/SHT31/SHT35 Temp+Humidity Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านอุณหภูมิและความชื้นจาก SHT3x series
 * รองรับ SHT30 (±0.3°C), SHT31 (±0.2°C), SHT35 (±0.1°C)
 * ใช้ CRC-8 verify ความถูกต้องของข้อมูล
 *
 * **วงจร:**
 * ```
 *   CH57x          SHT3x
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VDD
 *   GND ────────────> GND
 *   GND ────────────> ADDR  (I2C addr = 0x44)
 *   (VDD ──────────> ADDR   I2C addr = 0x45)
 *   (ไม่ต้องต่อ ALERT ถ้าไม่ใช้ interrupt)
 * ```
 * pull-up 4.7kΩ บน SCL/SDA
 *
 * @example
 * SHT3x_Instance sht;
 * SHT3x_Init(&sht, SHT3X_ADDR_LOW);
 * float temp, hum;
 * SHT3x_Read(&sht, &temp, &hum);
 * printf("Temp: %.2f C, Hum: %.1f %%\r\n", temp, hum);
 *
 * @author CH57x Library Team
 */

#ifndef __SHT3X_H
#define __SHT3X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Addresses ========== */
#define SHT3X_ADDR_LOW   0x44  /**< ADDR pin = GND (default) */
#define SHT3X_ADDR_HIGH  0x45  /**< ADDR pin = VDD */

/* ========== Type Definitions ========== */

/**
 * @brief Repeatability (ความแม่นยำ vs เวลา)
 */
typedef enum {
    SHT3X_REP_HIGH   = 0, /**< High repeatability   ~15ms */
    SHT3X_REP_MEDIUM = 1, /**< Medium repeatability ~6ms  */
    SHT3X_REP_LOW    = 2  /**< Low repeatability    ~4ms  */
} SHT3x_Repeatability;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    SHT3X_OK           = 0,
    SHT3X_ERROR_PARAM  = 1,
    SHT3X_ERROR_I2C    = 2,
    SHT3X_ERROR_CRC    = 3  /**< CRC ไม่ตรง — ข้อมูลอาจผิด */
} SHT3x_Status;

/**
 * @brief SHT3x Instance
 */
typedef struct {
    uint8_t           i2c_addr;     /**< I2C address */
    SHT3x_Repeatability repeatability; /**< ระดับความแม่นยำ */
    uint8_t           initialized;
} SHT3x_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น SHT3x
 * @param sht  ตัวแปร instance
 * @param addr SHT3X_ADDR_LOW (0x44) หรือ SHT3X_ADDR_HIGH (0x45)
 * @return SHT3X_OK หรือ error code
 * @note ต้องเรียก I2C_SimpleInit() ก่อน
 */
SHT3x_Status SHT3x_Init(SHT3x_Instance* sht, uint8_t addr);

/**
 * @brief อ่านอุณหภูมิและความชื้น (Single Shot mode)
 * @param sht  ตัวแปร instance
 * @param temp ตัวแปรรับอุณหภูมิ (°C)
 * @param hum  ตัวแปรรับความชื้น (%RH)
 * @return SHT3X_OK, SHT3X_ERROR_I2C หรือ SHT3X_ERROR_CRC
 *
 * @example
 * float t, h;
 * if (SHT3x_Read(&sht, &t, &h) == SHT3X_OK)
 *     printf("%.2f C, %.1f %%\r\n", t, h);
 */
SHT3x_Status SHT3x_Read(SHT3x_Instance* sht, float* temp, float* hum);

/**
 * @brief ตั้ง Repeatability
 * @param sht  ตัวแปร instance
 * @param rep  SHT3X_REP_HIGH / MEDIUM / LOW
 */
void SHT3x_SetRepeatability(SHT3x_Instance* sht, SHT3x_Repeatability rep);

/**
 * @brief Soft Reset
 */
SHT3x_Status SHT3x_Reset(SHT3x_Instance* sht);

/**
 * @brief อ่าน Status Register
 * @param sht    ตัวแปร instance
 * @param status ตัวแปรรับค่า status (16-bit)
 */
SHT3x_Status SHT3x_GetStatus(SHT3x_Instance* sht, uint16_t* status);

#ifdef __cplusplus
}
#endif

#endif /* __SHT3X_H */
