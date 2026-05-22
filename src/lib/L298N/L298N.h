/**
 * @file L298N.h
 * @brief L298N DC Motor Driver Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับควบคุม DC Motor ผ่าน L298N Motor Driver
 * ใช้ GPIO 2 ขา (IN1/IN2) สำหรับทิศทาง และ PWM 1 channel สำหรับความเร็ว
 *
 * **Hardware Connection:**
 * ```
 *   CH57x              L298N
 *   IN1 pin  -----------> IN1
 *   IN2 pin  -----------> IN2
 *   PWM pin  -----------> EN (Enable)
 *   GND      -----------> GND
 *
 *   L298N                Motor
 *   OUT1 + OUT2 --------> Motor A
 *   12V  ----------------> +12V
 *   GND  ----------------> GND
 * ```
 *
 * @note PWM_Channel ต้องตรงกับ pin ที่ต่อกับ EN:
 *   PA1→PWM1_CH2, PC0→PWM2_CH3, PC3→PWM1_CH3, PC4→PWM1_CH4
 *   PD2→PWM1_CH1, PD3→PWM2_CH2, PD4→PWM2_CH1, PD7→PWM2_CH4
 *
 * @example
 * L298N_Instance motor;
 * L298N_Init(&motor, PC1, PC2, PWM2_CH3);  // IN1=PC1, IN2=PC2, EN=PC0→PWM2_CH3
 * L298N_Run(&motor, L298N_FORWARD, 75);     // หมุนไปข้างหน้า 75%
 * Delay_Ms(2000);
 * L298N_Stop(&motor);
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __L298N_H
#define __L298N_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Type Definitions ========== */

/**
 * @brief L298N status codes
 */
typedef enum {
    L298N_OK          = 0,
    L298N_ERROR_PARAM = 1
} L298N_Status;

/**
 * @brief ทิศทางการหมุน
 */
typedef enum {
    L298N_FORWARD  = 0,  /**< หมุนไปข้างหน้า (IN1=H, IN2=L) */
    L298N_BACKWARD = 1,  /**< หมุนไปข้างหลัง (IN1=L, IN2=H) */
    L298N_STOP     = 2,  /**< หยุด — Coast (IN1=L, IN2=L) */
    L298N_BRAKE    = 3   /**< เบรก — Short brake (IN1=H, IN2=H) */
} L298N_Direction;

/**
 * @brief L298N motor instance
 */
typedef struct {
    uint8_t       pin_in1;     /**< GPIO pin สำหรับ IN1 */
    uint8_t       pin_in2;     /**< GPIO pin สำหรับ IN2 */
    PWM_Channel   pwm_channel; /**< PWM channel สำหรับ EN */
    uint8_t       speed;       /**< ความเร็วปัจจุบัน (0–100%) */
    L298N_Direction direction; /**< ทิศทางปัจจุบัน */
    uint8_t       initialized; /**< Init flag */
} L298N_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น L298N motor driver
 * @param motor       pointer ไปยัง L298N_Instance
 * @param pin_in1     GPIO pin สำหรับ IN1
 * @param pin_in2     GPIO pin สำหรับ IN2
 * @param pwm_channel PWM channel สำหรับ EN (Enable/Speed)
 * @return L298N_OK หรือ L298N_ERROR_PARAM
 *
 * @example
 * L298N_Instance m;
 * L298N_Init(&m, PC1, PC2, PWM2_CH3);
 */
L298N_Status L298N_Init(L298N_Instance* motor, uint8_t pin_in1, uint8_t pin_in2,
                         PWM_Channel pwm_channel);

/**
 * @brief หมุน motor ด้วยทิศทางและความเร็วที่กำหนด
 * @param motor  pointer ไปยัง L298N_Instance
 * @param dir    ทิศทาง (L298N_FORWARD / L298N_BACKWARD)
 * @param speed  ความเร็ว 0–100 (%)
 *
 * @example
 * L298N_Run(&motor, L298N_FORWARD, 80);
 */
void L298N_Run(L298N_Instance* motor, L298N_Direction dir, uint8_t speed);

/**
 * @brief หยุด motor แบบ Coast (ไม่เบรก)
 * @param motor pointer ไปยัง L298N_Instance
 */
void L298N_Stop(L298N_Instance* motor);

/**
 * @brief เบรก motor (Short brake)
 * @param motor pointer ไปยัง L298N_Instance
 */
void L298N_Brake(L298N_Instance* motor);

/**
 * @brief ตั้งความเร็ว (คงทิศทางเดิม)
 * @param motor  pointer ไปยัง L298N_Instance
 * @param speed  ความเร็ว 0–100 (%)
 */
void L298N_SetSpeed(L298N_Instance* motor, uint8_t speed);

/**
 * @brief เปลี่ยนทิศทาง (คงความเร็วเดิม)
 * @param motor pointer ไปยัง L298N_Instance
 * @param dir   ทิศทางใหม่
 */
void L298N_SetDirection(L298N_Instance* motor, L298N_Direction dir);

#ifdef __cplusplus
}
#endif

#endif  /* __L298N_H */
