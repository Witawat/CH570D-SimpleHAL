/**
 * @file ServoCluster.h
 * @brief Multi-Servo Controller with Easing Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับควบคุม servo หลายตัวพร้อมกันแบบ non-blocking
 * รองรับ easing curves หลากหลาย และ dual backend (ฮาร์ดแวร์ PWM / PCA9685 I2C)
 *
 * **คุณสมบัติ:**
 * - ควบคุม servo สูงสุด 8 ตัว (ฮาร์ดแวร์ PWM) หรือ 16 ตัว (PCA9685)
 * - Non-blocking movement พร้อม easing curves
 * - Dual backend: เลือกใช้ Servo.h หรือ PCA9685 ได้
 * - Parallel movement: สั่งขยับหลาย servo พร้อมกัน
 * - Global speed control: ปรับความเร็วรวมทุก servo
 * - Easing: 10 รูปแบบ (LINEAR, QUAD, CUBIC, SINE)
 *
 * **Easing Curves:**
 * - LINEAR
 * - QUAD_IN / QUAD_OUT / QUAD_IN_OUT
 * - CUBIC_IN / CUBIC_OUT / CUBIC_IN_OUT
 * - SINE_IN / SINE_OUT / SINE_IN_OUT
 *
 * **Backend:**
 * - SERVO_BACKEND_HW: ใช้ hardware PWM (Servo.h) — 8 channels
 * - SERVO_BACKEND_PCA9685: ใช้ I2C PWM expander — 16 channels
 *
 * @example
 * #include "ServoCluster.h"
 *
 * ServoCluster_Instance cluster;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     // ใช้ PCA9685 backend สำหรับ 4 servos
 *     ServoCluster_Init(&cluster, SERVO_BACKEND_PCA9685, 4);
 *     ServoCluster_AddServo(&cluster, 0, 500, 2500);  // channel 0
 *     ServoCluster_AddServo(&cluster, 1, 500, 2500);  // channel 1
 *     ServoCluster_AddServo(&cluster, 2, 1000, 2000); // channel 2
 *     ServoCluster_AddServo(&cluster, 3, 1000, 2000); // channel 3
 *
 *     while (1) {
 *         ServoCluster_MoveTo(&cluster, 0, 90, 1000, EASE_QUAD_IN_OUT);
 *         ServoCluster_MoveTo(&cluster, 1, 45, 1500, EASE_SINE_OUT);
 *
 *         // non-blocking update
 *         ServoCluster_Update(&cluster);
 *
 *         if (ServoCluster_IsAllDone(&cluster)) {
 *             // movements complete
 *         }
 *         Delay_Ms(10);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __SERVOCLUSTER_H
#define __SERVOCLUSTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "../Servo/Servo.h"
#include "../PCA9685/PCA9685.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Maximum servos in a cluster */
#ifndef SERVOCLUSTER_MAX_SERVOS
#define SERVOCLUSTER_MAX_SERVOS  16
#endif

/** @brief Default movement duration (ms) */
#ifndef SERVOCLUSTER_DEFAULT_DURATION_MS
#define SERVOCLUSTER_DEFAULT_DURATION_MS  500
#endif

/* ========== Type Definitions ========== */

/**
 * @brief Backend type
 */
typedef enum {
    SERVO_BACKEND_HW      = 0,  /**< Hardware PWM (Servo.h) */
    SERVO_BACKEND_PCA9685 = 1   /**< I2C PCA9685 expander */
} ServoCluster_Backend;

/**
 * @brief Easing curve types
 */
typedef enum {
    EASE_LINEAR       = 0,
    EASE_QUAD_IN      = 1,
    EASE_QUAD_OUT     = 2,
    EASE_QUAD_IN_OUT  = 3,
    EASE_CUBIC_IN     = 4,
    EASE_CUBIC_OUT    = 5,
    EASE_CUBIC_IN_OUT = 6,
    EASE_SINE_IN      = 7,
    EASE_SINE_OUT     = 8,
    EASE_SINE_IN_OUT  = 9
} ServoCluster_Easing;

/**
 * @brief Single servo in the cluster
 */
typedef struct {
    uint8_t  channel;        /**< PWM/PCA9685 channel */
    uint16_t pulse_min_us;   /**< Pulse for 0 degrees */
    uint16_t pulse_max_us;   /**< Pulse for 180 degrees */
    uint8_t  current_angle;  /**< Current angle (0-180) */
    uint8_t  target_angle;   /**< Target angle (0-180) */
    uint8_t  start_angle;    /**< Start angle when move began */
    uint32_t start_time_ms;  /**< Millis when move started */
    uint16_t duration_ms;    /**< Movement duration */
    ServoCluster_Easing easing; /**< Easing curve */
    uint8_t  moving;         /**< 1 = currently moving */
    uint8_t  active;         /**< 1 = servo added to cluster */
} ServoCluster_Servo;

/**
 * @brief ServoCluster instance
 */
typedef struct {
    ServoCluster_Backend backend;   /**< Backend type */
    uint8_t              max_servos; /**< Number of servos */
    uint8_t              speed_pct;  /**< Global speed (1-200%, default 100) */

    /* backend-specific */
    PCA9685_Instance     pca9685;    /**< PCA9685 instance (if PCA9685 backend) */

    ServoCluster_Servo   servos[SERVOCLUSTER_MAX_SERVOS]; /**< Servo array */

    uint8_t initialized;  /**< Init flag */
} ServoCluster_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น ServoCluster
 * @param cluster       pointer ไปยัง ServoCluster_Instance
 * @param backend       SERVO_BACKEND_HW หรือ SERVO_BACKEND_PCA9685
 * @param max_servos    จำนวน servo สูงสุด (ไม่เกิน SERVOCLUSTER_MAX_SERVOS)
 *
 * @note สำหรับ PCA9685 backend ต้องเรียก I2C_SimpleInit() ก่อน
 *
 * @example
 * ServoCluster_Instance cluster;
 * ServoCluster_Init(&cluster, SERVO_BACKEND_PCA9685, 8);
 */
void ServoCluster_Init(ServoCluster_Instance* cluster, ServoCluster_Backend backend, uint8_t max_servos);

/**
 * @brief เพิ่ม servo เข้า cluster
 * @param cluster       pointer ไปยัง ServoCluster_Instance
 * @param channel       PWM channel (hw) หรือ PCA9685 channel (0-15)
 * @param pulse_min_us  pulse width สำหรับ 0 องศา (µs)
 * @param pulse_max_us  pulse width สำหรับ 180 องศา (µs)
 *
 * @example
 * ServoCluster_AddServo(&cluster, 0, 500, 2500);
 * ServoCluster_AddServo(&cluster, 1, 1000, 2000);
 */
void ServoCluster_AddServo(ServoCluster_Instance* cluster, uint8_t channel, uint16_t pulse_min_us, uint16_t pulse_max_us);

/**
 * @brief สั่งขยับ servo ไปที่มุมเป้าหมาย (non-blocking)
 * @param cluster     pointer ไปยัง ServoCluster_Instance
 * @param servo_id    ลำดับ servo (0-based ตาม AddServo)
 * @param angle       มุมเป้าหมาย (0-180)
 * @param duration_ms ระยะเวลาในการขยับ (ms)
 * @param easing      easing curve
 *
 * @example
 * ServoCluster_MoveTo(&cluster, 0, 90, 1000, EASE_QUAD_IN_OUT);
 */
void ServoCluster_MoveTo(ServoCluster_Instance* cluster, uint8_t servo_id, uint8_t angle, uint16_t duration_ms, ServoCluster_Easing easing);

/**
 * @brief สั่งขยับทุก servo พร้อมกันไปที่มุมเดียวกัน
 * @param cluster     pointer ไปยัง ServoCluster_Instance
 * @param angle       มุมเป้าหมาย (0-180) สำหรับทุก servo
 * @param duration_ms ระยะเวลา (ms)
 * @param easing      easing curve
 *
 * @example
 * ServoCluster_MoveAll(&cluster, 0, 500, EASE_SINE_OUT);
 */
void ServoCluster_MoveAll(ServoCluster_Instance* cluster, uint8_t angle, uint16_t duration_ms, ServoCluster_Easing easing);

/**
 * @brief ตั้งค่า easing สำหรับ servo เฉพาะตัว
 * @param cluster  pointer ไปยัง ServoCluster_Instance
 * @param servo_id ลำดับ servo
 * @param easing   easing curve
 */
void ServoCluster_SetEasing(ServoCluster_Instance* cluster, uint8_t servo_id, ServoCluster_Easing easing);

/**
 * @brief ตั้งค่าความเร็วรวม (%)
 * @param cluster   pointer ไปยัง ServoCluster_Instance
 * @param speed_pct 1-200 (100 = ปกติ, 50 = ครึ่งเร็ว, 200 = เร็วกว่า 2x)
 */
void ServoCluster_SetSpeed(ServoCluster_Instance* cluster, uint8_t speed_pct);

/**
 * @brief อัปเดต servo ทุกตัว (เรียกใน main loop)
 * @param cluster pointer ไปยัง ServoCluster_Instance
 *
 * @note ต้องเรียกบ่อยๆ (~10ms interval) เพื่อให้ animation ลื่น
 *
 * @example
 * while(1) {
 *     ServoCluster_Update(&cluster);
 *     Delay_Ms(10);
 * }
 */
void ServoCluster_Update(ServoCluster_Instance* cluster);

/**
 * @brief เช็คว่า servo กำลังขยับอยู่หรือไม่
 * @param cluster  pointer ไปยัง ServoCluster_Instance
 * @param servo_id ลำดับ servo
 * @return 1 = กำลังขยับ, 0 = หยุดแล้ว
 */
uint8_t ServoCluster_IsMoving(ServoCluster_Instance* cluster, uint8_t servo_id);

/**
 * @brief เช็คว่าทุก servo หยุดหมดแล้วหรือยัง
 * @param cluster pointer ไปยัง ServoCluster_Instance
 * @return 1 = ทุกตัวหยุด, 0 = ยังมีขยับอยู่
 */
uint8_t ServoCluster_IsAllDone(ServoCluster_Instance* cluster);

/**
 * @brief หยุด servo เฉพาะตัว
 * @param cluster  pointer ไปยัง ServoCluster_Instance
 * @param servo_id ลำดับ servo
 */
void ServoCluster_Stop(ServoCluster_Instance* cluster, uint8_t servo_id);

/**
 * @brief หยุดทุก servo
 * @param cluster pointer ไปยัง ServoCluster_Instance
 */
void ServoCluster_StopAll(ServoCluster_Instance* cluster);

#ifdef __cplusplus
}
#endif

#endif /* __SERVOCLUSTER_H */
