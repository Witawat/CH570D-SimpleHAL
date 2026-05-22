/**
 * @file BH1750.h
 * @brief BH1750 Ambient Light Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านค่าความสว่าง (Lux) จาก BH1750FVI ผ่าน I2C
 * ความละเอียดสูงสุด 0.5 lux, ช่วงวัด 1-65535 lux
 *
 * **วงจร:**
 * ```
 *   CH57x          BH1750
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VCC
 *   GND ────────────> GND
 *   GND ────────────> ADDR  (I2C addr = 0x23)
 *   (VCC ──────────> ADDR   I2C addr = 0x5C)
 * ```
 * pull-up 4.7kΩ บน SCL/SDA
 *
 * @example
 * BH1750_Instance light;
 * BH1750_Init(&light, BH1750_ADDR_LOW);
 * float lux = BH1750_ReadLux(&light);
 * printf("Light: %.1f lux\r\n", lux);
 *
 * @author CH57x Library Team
 */

#ifndef __BH1750_H
#define __BH1750_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Addresses ========== */
#define BH1750_ADDR_LOW   0x23  /**< ADDR pin = GND */
#define BH1750_ADDR_HIGH  0x5C  /**< ADDR pin = VCC */

/* ========== Commands ========== */
#define BH1750_CMD_POWER_DOWN  0x00
#define BH1750_CMD_POWER_ON    0x01
#define BH1750_CMD_RESET       0x07

/* ========== Type Definitions ========== */

/**
 * @brief โหมดการวัด
 */
typedef enum {
    BH1750_CONT_H_RES   = 0x10, /**< Continuous High-Res   1.0 lux, 120ms */
    BH1750_CONT_H_RES2  = 0x11, /**< Continuous High-Res2  0.5 lux, 120ms */
    BH1750_CONT_L_RES   = 0x13, /**< Continuous Low-Res    4.0 lux,  16ms */
    BH1750_ONE_H_RES    = 0x20, /**< One-time High-Res     1.0 lux, 120ms */
    BH1750_ONE_H_RES2   = 0x21, /**< One-time High-Res2    0.5 lux, 120ms */
    BH1750_ONE_L_RES    = 0x23  /**< One-time Low-Res      4.0 lux,  16ms */
} BH1750_Mode;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    BH1750_OK          = 0,
    BH1750_ERROR_PARAM = 1,
    BH1750_ERROR_I2C   = 2
} BH1750_Status;

/**
 * @brief BH1750 Instance
 */
typedef struct {
    uint8_t      i2c_addr;   /**< I2C address (0x23 หรือ 0x5C) */
    BH1750_Mode  mode;       /**< โหมดการวัดปัจจุบัน */
    uint8_t      initialized;
} BH1750_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น BH1750
 * @param bh   ตัวแปร instance
 * @param addr BH1750_ADDR_LOW (0x23) หรือ BH1750_ADDR_HIGH (0x5C)
 * @return BH1750_OK หรือ error code
 * @note ต้องเรียก I2C_SimpleInit() ก่อน
 */
BH1750_Status BH1750_Init(BH1750_Instance* bh, uint8_t addr);

/**
 * @brief ตั้งโหมดการวัด (default: BH1750_CONT_H_RES)
 * @param bh   ตัวแปร instance
 * @param mode โหมดการวัด
 */
BH1750_Status BH1750_SetMode(BH1750_Instance* bh, BH1750_Mode mode);

/**
 * @brief อ่านค่าความสว่าง
 * @param bh ตัวแปร instance
 * @return ค่า Lux (0.0 = error)
 *
 * @note โหมด Continuous: อ่านได้ทันที
 * @note โหมด One-time: ส่ง command + รอ 180ms อัตโนมัติ
 */
float BH1750_ReadLux(BH1750_Instance* bh);

/**
 * @brief อ่านค่า raw (ก่อนแปลงเป็น lux)
 * @param bh  ตัวแปร instance
 * @param raw ตัวแปรรับค่า raw (0-65535)
 */
BH1750_Status BH1750_ReadRaw(BH1750_Instance* bh, uint16_t* raw);

/**
 * @brief Power Down (ประหยัดไฟ ~0.01µA)
 */
BH1750_Status BH1750_PowerDown(BH1750_Instance* bh);

/**
 * @brief Power Up + ตั้งโหมดเดิม
 */
BH1750_Status BH1750_PowerUp(BH1750_Instance* bh);

#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H */
