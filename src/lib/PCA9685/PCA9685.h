/**
 * @file PCA9685.h
 * @brief PCA9685 16-Channel PWM Expander Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับควบคุม PWM 16 channel ด้วย PCA9685 ผ่าน I2C
 * ใช้สำหรับต่อ Servo หลายตัว, LED dimming, Motor driver
 *
 * **วงจร:**
 * ```
 *   CH57x          PCA9685
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VCC  (logic supply)
 *   5V ─────────────> V+   (servo/LED supply, optional)
 *   GND ────────────> GND
 *   GND ────────────> A0-A5 (addr = 0x40)
 *
 *   I2C Address = 0x40 + A5..A0 (binary)
 *   Default (all GND) = 0x40
 *
 *   OE pin: ลาก LOW (enable outputs) หรือต่อ GPIO
 * ```
 * pull-up 4.7kΩ บน SCL/SDA
 *
 * @example
 * PCA9685_Instance pwm;
 * PCA9685_Init(&pwm, PCA9685_ADDR_DEFAULT, 50);  // 50Hz สำหรับ servo
 *
 * // ต่อ Servo: 0°=500µs, 90°=1500µs, 180°=2500µs
 * PCA9685_SetServoAngle(&pwm, 0, 90);   // channel 0, 90 องศา
 *
 * // หรือตั้งค่า duty cycle โดยตรง
 * PCA9685_SetDuty(&pwm, 1, 50.0f);  // channel 1, 50%
 *
 * @author CH57x Library Team
 */

#ifndef __PCA9685_H
#define __PCA9685_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Address ========== */
#define PCA9685_ADDR_DEFAULT  0x40  /**< A0-A5 ทั้งหมด = GND */

/* ========== Registers ========== */
#define PCA9685_REG_MODE1    0x00
#define PCA9685_REG_MODE2    0x01
#define PCA9685_REG_LED0_ON_L  0x06  /**< LED0 ON  low byte  */
#define PCA9685_REG_LED0_ON_H  0x07  /**< LED0 ON  high byte */
#define PCA9685_REG_LED0_OFF_L 0x08  /**< LED0 OFF low byte  */
#define PCA9685_REG_LED0_OFF_H 0x09  /**< LED0 OFF high byte */
#define PCA9685_REG_ALL_ON_L   0xFA  /**< ALL channels ON  */
#define PCA9685_REG_ALL_OFF_L  0xFC  /**< ALL channels OFF */
#define PCA9685_REG_PRE_SCALE  0xFE  /**< Prescaler (ความถี่) */

/* ========== MODE1 bits ========== */
#define PCA9685_MODE1_RESTART  0x80
#define PCA9685_MODE1_SLEEP    0x10
#define PCA9685_MODE1_ALLCALL  0x01

/* ========== Servo defaults ========== */
#ifndef PCA9685_SERVO_MIN_US
#define PCA9685_SERVO_MIN_US  500   /**< Pulse width สำหรับ 0° (µs) */
#endif
#ifndef PCA9685_SERVO_MAX_US
#define PCA9685_SERVO_MAX_US  2500  /**< Pulse width สำหรับ 180° (µs) */
#endif

/* Internal oscillator frequency */
#define PCA9685_OSC_CLOCK  25000000UL  /**< 25 MHz */

/* ========== Type Definitions ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    PCA9685_OK          = 0,
    PCA9685_ERROR_PARAM = 1,
    PCA9685_ERROR_I2C   = 2
} PCA9685_Status;

/**
 * @brief PCA9685 Instance
 */
typedef struct {
    uint8_t  i2c_addr;    /**< I2C address */
    uint16_t frequency;   /**< PWM frequency (Hz) */
    uint32_t period_us;   /**< Period in µs (= 1000000 / freq) */
    uint8_t  initialized;
} PCA9685_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น PCA9685
 * @param pca  ตัวแปร instance
 * @param addr I2C address (PCA9685_ADDR_DEFAULT = 0x40)
 * @param freq PWM frequency Hz (24-1526)
 *             - 50Hz สำหรับ Servo
 *             - 1000Hz สำหรับ LED
 * @return PCA9685_OK หรือ error code
 */
PCA9685_Status PCA9685_Init(PCA9685_Instance* pca, uint8_t addr, uint16_t freq);

/**
 * @brief ตั้ง PWM แบบ ON/OFF count (0-4095 ต่อ period)
 * @param pca     ตัวแปร instance
 * @param channel channel 0-15
 * @param on      count ที่เริ่ม HIGH (0-4095)
 * @param off     count ที่เริ่ม LOW (0-4095)
 *
 * @example
 * // 50% duty cycle: on=0, off=2048
 * PCA9685_SetPWM(&pca, 0, 0, 2048);
 */
PCA9685_Status PCA9685_SetPWM(PCA9685_Instance* pca, uint8_t channel,
                               uint16_t on, uint16_t off);

/**
 * @brief ตั้ง duty cycle (%)
 * @param pca     ตัวแปร instance
 * @param channel channel 0-15
 * @param duty    0.0-100.0 %
 */
PCA9685_Status PCA9685_SetDuty(PCA9685_Instance* pca, uint8_t channel, float duty);

/**
 * @brief ตั้ง pulse width (µs) — สำหรับ Servo
 * @param pca     ตัวแปร instance
 * @param channel channel 0-15
 * @param us      pulse width (µs) เช่น 500-2500
 */
PCA9685_Status PCA9685_SetPulse(PCA9685_Instance* pca, uint8_t channel, uint16_t us);

/**
 * @brief ตั้งมุม Servo (ต้อง Init ด้วย 50Hz)
 * @param pca     ตัวแปร instance
 * @param channel channel 0-15
 * @param angle   มุม 0-180 องศา
 */
PCA9685_Status PCA9685_SetServoAngle(PCA9685_Instance* pca, uint8_t channel, uint8_t angle);

/**
 * @brief ปิด channel (output = 0)
 * @param pca     ตัวแปร instance
 * @param channel channel 0-15 หรือ 255 = ทุก channel
 */
PCA9685_Status PCA9685_Off(PCA9685_Instance* pca, uint8_t channel);

/**
 * @brief เปิด channel เต็ม (output = 1 ตลอด)
 * @param pca     ตัวแปร instance
 * @param channel channel 0-15
 */
PCA9685_Status PCA9685_FullOn(PCA9685_Instance* pca, uint8_t channel);

/**
 * @brief Sleep mode (ประหยัดไฟ)
 */
PCA9685_Status PCA9685_Sleep(PCA9685_Instance* pca);

/**
 * @brief Wake up จาก sleep
 */
PCA9685_Status PCA9685_WakeUp(PCA9685_Instance* pca);

#ifdef __cplusplus
}
#endif

#endif /* __PCA9685_H */
