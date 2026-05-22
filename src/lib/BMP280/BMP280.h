/**
 * @file BMP280.h
 * @brief BMP280 Temperature & Pressure Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านอุณหภูมิและความดันอากาศจาก BMP280 ผ่าน I2C
 * BMP280 เป็น sensor ความแม่นยำสูงจาก Bosch Sensortec
 * สามารถคำนวณความสูงโดยประมาณจากความดันอากาศได้
 *
 * **คุณสมบัติ:**
 * - อุณหภูมิ: -40 ถึง +85°C, ±1°C accuracy, 0.01°C resolution
 * - ความดัน: 300 ถึง 1100 hPa, ±1 hPa accuracy
 * - ความสูงโดยประมาณ (จาก sea-level pressure)
 * - Oversampling และ IIR filter ปรับได้
 * - 2 โหมด: Forced (อ่านครั้งเดียวแล้วกลับ sleep) / Normal (continuous)
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          BMP280
 *   PC2 (SCL) ------> SCK  + [4.7kΩ → 3.3V]
 *   PC1 (SDA) <-----> SDI  + [4.7kΩ → 3.3V]
 *   3.3V -----------> VDD, VDDIO
 *   GND ------------> GND
 *   GND ------------> SDO  (address = 0x76)
 *   // หรือ VCC → SDO เพื่อใช้ address 0x77
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "BMP280.h"
 *
 * BMP280_Instance bmp;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     BMP280_Init(&bmp, BMP280_ADDR_LOW);
 *
 *     while (1) {
 *         float temp, press, alt;
 *         BMP280_Read(&bmp, &temp, &press);
 *         alt = BMP280_GetAltitude(&bmp, press, 1013.25f);
 *         printf("T=%.2fC P=%.2fhPa Alt=%.1fm\r\n", temp, press, alt);
 *         Delay_Ms(1000);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __BMP280_H
#define __BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== I2C Addresses ========== */
#define BMP280_ADDR_LOW   0x76  /**< SDO = GND (default) */
#define BMP280_ADDR_HIGH  0x77  /**< SDO = VCC */

/* ========== Registers ========== */
#define BMP280_REG_CALIB_START  0x88
#define BMP280_REG_CHIP_ID      0xD0
#define BMP280_REG_RESET        0xE0
#define BMP280_REG_STATUS       0xF3
#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_TEMP_MSB     0xFA

#define BMP280_CHIP_ID          0x60  /**< Expected CHIP_ID value */
#define BMP280_RESET_VALUE      0xB6  /**< Soft reset value */

/* ========== Type Definitions ========== */

/**
 * @brief Oversampling settings
 */
typedef enum {
    BMP280_OS_SKIP = 0, /**< Skip (output 0x80000) */
    BMP280_OS_X1   = 1, /**< Oversampling x1 */
    BMP280_OS_X2   = 2, /**< Oversampling x2 */
    BMP280_OS_X4   = 3, /**< Oversampling x4 */
    BMP280_OS_X8   = 4, /**< Oversampling x8 */
    BMP280_OS_X16  = 5  /**< Oversampling x16 */
} BMP280_Oversampling;

/**
 * @brief IIR Filter coefficient
 */
typedef enum {
    BMP280_FILTER_OFF = 0, /**< ปิด filter */
    BMP280_FILTER_X2  = 1, /**< Coefficient 2 */
    BMP280_FILTER_X4  = 2, /**< Coefficient 4 */
    BMP280_FILTER_X8  = 3, /**< Coefficient 8 */
    BMP280_FILTER_X16 = 4  /**< Coefficient 16 */
} BMP280_Filter;

/**
 * @brief Power mode
 */
typedef enum {
    BMP280_MODE_SLEEP  = 0x00, /**< Sleep mode — ประหยัดไฟ */
    BMP280_MODE_FORCED = 0x01, /**< Forced mode — อ่านครั้งเดียวแล้วกลับ sleep */
    BMP280_MODE_NORMAL = 0x03  /**< Normal mode — continuous measurement */
} BMP280_Mode;

/**
 * @brief Standby time (Normal mode เท่านั้น)
 */
typedef enum {
    BMP280_STANDBY_0_5MS  = 0, /**< 0.5ms */
    BMP280_STANDBY_62_5MS = 1, /**< 62.5ms */
    BMP280_STANDBY_125MS  = 2, /**< 125ms */
    BMP280_STANDBY_250MS  = 3, /**< 250ms */
    BMP280_STANDBY_500MS  = 4, /**< 500ms */
    BMP280_STANDBY_1000MS = 5, /**< 1000ms */
    BMP280_STANDBY_2000MS = 6, /**< 2000ms */
    BMP280_STANDBY_4000MS = 7  /**< 4000ms */
} BMP280_Standby;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    BMP280_OK            = 0, /**< สำเร็จ */
    BMP280_ERROR_I2C     = 1, /**< I2C error */
    BMP280_ERROR_PARAM   = 2, /**< Parameter ผิด */
    BMP280_ERROR_CHIPID  = 3  /**< CHIP_ID ผิด (ไม่ใช่ BMP280) */
} BMP280_Status;

/**
 * @brief Calibration data (อ่านจาก OTP ของ BMP280)
 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5;
    int16_t  dig_P6, dig_P7, dig_P8, dig_P9;
} BMP280_Calib;

/**
 * @brief BMP280 Instance
 */
typedef struct {
    uint8_t      i2c_addr;   /**< I2C address */
    BMP280_Calib calib;      /**< Calibration data */
    int32_t      t_fine;     /**< Internal fine temperature (ใช้ใน pressure calc) */
    uint8_t      initialized;/**< flag บอกว่า Init แล้ว */
} BMP280_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น BMP280
 * @param bmp      ตัวแปร instance
 * @param i2c_addr I2C address (BMP280_ADDR_LOW หรือ BMP280_ADDR_HIGH)
 * @return BMP280_OK หรือ error code
 *
 * @note ตั้งค่า default: Normal mode, Temp x2, Press x16, Filter x4, Standby 125ms
 * @example BMP280_Init(&bmp, BMP280_ADDR_LOW);
 */
BMP280_Status BMP280_Init(BMP280_Instance* bmp, uint8_t i2c_addr);

/**
 * @brief ตั้งค่า mode และ oversampling
 * @param bmp      ตัวแปร instance
 * @param mode     BMP280_MODE_SLEEP / FORCED / NORMAL
 * @param os_temp  Oversampling สำหรับอุณหภูมิ
 * @param os_press Oversampling สำหรับความดัน
 * @return BMP280_OK หรือ error code
 */
BMP280_Status BMP280_SetMode(BMP280_Instance* bmp, BMP280_Mode mode,
                              BMP280_Oversampling os_temp, BMP280_Oversampling os_press);

/**
 * @brief ตั้ง IIR Filter
 * @param bmp    ตัวแปร instance
 * @param filter BMP280_FILTER_OFF ถึง BMP280_FILTER_X16
 * @return BMP280_OK หรือ error code
 */
BMP280_Status BMP280_SetFilter(BMP280_Instance* bmp, BMP280_Filter filter);

/**
 * @brief อ่านอุณหภูมิและความดัน
 * @param bmp   ตัวแปร instance
 * @param temp  pointer รับอุณหภูมิ (°C) — NULL ถ้าไม่ต้องการ
 * @param press pointer รับความดัน (hPa) — NULL ถ้าไม่ต้องการ
 * @return BMP280_OK หรือ error code
 *
 * @note ต้องอ่านอุณหภูมิก่อนเสมอ (เพราะ t_fine ใช้ใน pressure compensation)
 * @example
 * float temp, press;
 * BMP280_Read(&bmp, &temp, &press);
 * printf("T=%.2fC P=%.2fhPa\r\n", temp, press);
 */
BMP280_Status BMP280_Read(BMP280_Instance* bmp, float* temp, float* press);

/**
 * @brief คำนวณความสูงโดยประมาณจากความดัน
 * @param bmp         ตัวแปร instance (ไม่ใช้ค่า, แค่ตรวจ initialized)
 * @param pressure    ความดัน ณ จุดนั้น (hPa) จาก BMP280_Read()
 * @param sea_level_pa ความดันที่ระดับน้ำทะเล (hPa) — ปกติ 1013.25
 * @return ความสูง (เมตร)
 *
 * @note สูตร: alt = 44330 × (1 - (P/P0)^(1/5.255))
 * @example
 * float alt = BMP280_GetAltitude(&bmp, press, 1013.25f);
 * printf("Alt: %.1f m\r\n", alt);
 */
float BMP280_GetAltitude(BMP280_Instance* bmp, float pressure, float sea_level_pa);

/**
 * @brief Soft reset BMP280
 * @param bmp ตัวแปร instance
 * @return BMP280_OK หรือ error code
 */
BMP280_Status BMP280_Reset(BMP280_Instance* bmp);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */
