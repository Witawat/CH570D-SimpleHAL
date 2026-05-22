/**
 * @file INA219.h
 * @brief INA219 Current/Voltage/Power Monitor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับวัดแรงดัน กระแส และกำลังไฟฟ้าด้วย INA219 ผ่าน I2C
 * รองรับการวัด:
 * - Bus Voltage: 0-26V (±32V full range)
 * - Shunt Voltage: ±40mV (±320mV full range)
 * - Current: คำนวณจาก Shunt Voltage ÷ R_shunt
 * - Power: Bus Voltage × Current
 *
 * **วงจร:**
 * ```
 *   Power Supply (+)─────────── IN+  ──[R_shunt]── IN- ──── Load (+)
 *                                │                   │
 *                              INA219               Load (-)
 *                                │
 *   CH57x          INA219
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VCC
 *   GND ────────────> GND
 *   GND ────────────> A0, A1  (addr = 0x40)
 *
 * I2C Address:
 *   A1=GND, A0=GND → 0x40 (default)
 *   A1=GND, A0=VCC → 0x41
 *   A1=VCC, A0=GND → 0x44
 *   A1=VCC, A0=VCC → 0x45
 * ```
 *
 * @example
 * INA219_Instance ina;
 * INA219_Init(&ina, INA219_ADDR_0, 0.1f, 3.2f);  // 100mΩ shunt, 3.2A max
 * float v = INA219_GetBusVoltage(&ina);
 * float i = INA219_GetCurrent(&ina);
 * printf("%.3f V, %.3f A, %.3f W\r\n", v, i, v*i);
 *
 * @author CH57x Library Team
 */

#ifndef __INA219_H
#define __INA219_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Addresses ========== */
#define INA219_ADDR_0  0x40  /**< A1=GND, A0=GND */
#define INA219_ADDR_1  0x41  /**< A1=GND, A0=VCC */
#define INA219_ADDR_2  0x44  /**< A1=VCC, A0=GND */
#define INA219_ADDR_3  0x45  /**< A1=VCC, A0=VCC */

/* ========== Registers ========== */
#define INA219_REG_CONFIG       0x00
#define INA219_REG_SHUNTVOLTAGE 0x01
#define INA219_REG_BUSVOLTAGE   0x02
#define INA219_REG_POWER        0x03
#define INA219_REG_CURRENT      0x04
#define INA219_REG_CALIBRATION  0x05

/* ========== CONFIG bits ========== */
#define INA219_BRNG_32V   (1 << 13)  /**< Bus range 32V (default) */
#define INA219_BRNG_16V   (0 << 13)  /**< Bus range 16V */
#define INA219_PGA_40MV   (0 << 11)  /**< Shunt ±40mV  Gain=1 */
#define INA219_PGA_80MV   (1 << 11)  /**< Shunt ±80mV  Gain=2 */
#define INA219_PGA_160MV  (2 << 11)  /**< Shunt ±160mV Gain=4 */
#define INA219_PGA_320MV  (3 << 11)  /**< Shunt ±320mV Gain=8 */
#define INA219_MODE_CONT  0x07       /**< Shunt+Bus, Continuous (default) */

/* ========== Type Definitions ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    INA219_OK          = 0,
    INA219_ERROR_PARAM = 1,
    INA219_ERROR_I2C   = 2
} INA219_Status;

/**
 * @brief INA219 Instance
 */
typedef struct {
    uint8_t  i2c_addr;         /**< I2C address */
    float    current_lsb;      /**< LSB ของ current register (A/bit) */
    float    r_shunt;          /**< ค่า shunt resistor (Ω) */
    uint16_t calibration;      /**< ค่า calibration register */
    uint8_t  initialized;
} INA219_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น INA219 พร้อม calibrate
 * @param ina       ตัวแปร instance
 * @param addr      I2C address (INA219_ADDR_0 ถึง INA219_ADDR_3)
 * @param r_shunt   ค่า shunt resistor (Ω) เช่น 0.1 = 100mΩ
 * @param max_amps  กระแสสูงสุดที่คาดว่าจะวัด (A) เช่น 3.2
 * @return INA219_OK หรือ error code
 * @note ต้องเรียก I2C_SimpleInit() ก่อน
 *
 * @example
 * // Shunt 100mΩ, max current 3.2A
 * INA219_Init(&ina, INA219_ADDR_0, 0.1f, 3.2f);
 */
INA219_Status INA219_Init(INA219_Instance* ina, uint8_t addr,
                           float r_shunt, float max_amps);

/**
 * @brief อ่านแรงดัน Bus (V)
 * @return แรงดัน (V) หรือ -1.0 ถ้า error
 * @note Bus voltage = แรงดันที่ load, ช่วง 0-26V
 */
float INA219_GetBusVoltage(INA219_Instance* ina);

/**
 * @brief อ่านแรงดัน Shunt (mV)
 * @return แรงดัน shunt (mV) หรือ -999.0 ถ้า error
 */
float INA219_GetShuntVoltage(INA219_Instance* ina);

/**
 * @brief อ่านกระแส (A)
 * @return กระแส (A) หรือ -999.0 ถ้า error
 * @note ต้อง Init ด้วย r_shunt และ max_amps ที่ถูกต้อง
 */
float INA219_GetCurrent(INA219_Instance* ina);

/**
 * @brief อ่านกำลังไฟ (W)
 * @return กำลังไฟ (W) หรือ -1.0 ถ้า error
 */
float INA219_GetPower(INA219_Instance* ina);

/**
 * @brief อ่านค่าทั้งหมดพร้อมกัน
 * @param ina     ตัวแปร instance
 * @param voltage แรงดัน bus (V)
 * @param current กระแส (A)
 * @param power   กำลังไฟ (W)
 */
INA219_Status INA219_GetAll(INA219_Instance* ina,
                             float* voltage, float* current, float* power);

/**
 * @brief Power Down
 */
INA219_Status INA219_PowerDown(INA219_Instance* ina);

/**
 * @brief Power Up (resume continuous mode)
 */
INA219_Status INA219_PowerUp(INA219_Instance* ina);

#ifdef __cplusplus
}
#endif

#endif /* __INA219_H */
