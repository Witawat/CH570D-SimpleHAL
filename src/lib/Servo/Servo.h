/**
 * @file Servo.h
 * @brief Servo Motor Control Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับควบคุม Standard RC Servo Motor ผ่าน PWM signal
 * ใช้ SimplePWM เป็นพื้นฐาน รองรับ servo แบบ 0-180° และ continuous rotation
 *
 * **คุณสมบัติ:**
 * - ควบคุมมุม 0-180° ด้วยความละเอียดสูง
 * - ปรับ pulse width ตรงๆ (WriteMicroseconds)
 * - รองรับ Continuous Rotation Servo
 * - Min/Max pulse calibration สำหรับ servo แต่ละรุ่น
 * - Detach/Attach เพื่อหยุด PWM signal
 * - รองรับสูงสุด 8 servo พร้อมกัน (ขึ้นกับ PWM channels)
 *
 * **หลักการทำงาน:**
 * Servo ควบคุมด้วย PWM ความถี่ 50Hz (period 20ms)
 * - Pulse 1000µs = 0°
 * - Pulse 1500µs = 90° (กลาง)
 * - Pulse 2000µs = 180°
 *
 * **PWM Channels และ Pins (default):**
 * - PWM1_CH1: PD2    PWM1_CH2: PA1
 * - PWM1_CH3: PC3    PWM1_CH4: PC4
 * - PWM2_CH1: PD4    PWM2_CH2: PD3
 * - PWM2_CH3: PC0    PWM2_CH4: PD7
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "Servo.h"
 *
 * Servo_Instance servo;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     // Init servo บน PWM channel PC4 (PWM1_CH4)
 *     Servo_Init(&servo, PWM1_CH4);
 *     Servo_Attach(&servo);
 *
 *     while (1) {
 *         Servo_Write(&servo, 0);    // หมุนไป 0°
 *         Delay_Ms(1000);
 *         Servo_Write(&servo, 90);   // หมุนไป 90°
 *         Delay_Ms(1000);
 *         Servo_Write(&servo, 180);  // หมุนไป 180°
 *         Delay_Ms(1000);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __SERVO_H
#define __SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief ความถี่ PWM สำหรับ servo (Hz)
 * @note Standard servo ใช้ 50Hz (period = 20ms)
 */
#define SERVO_PWM_FREQUENCY     50

/**
 * @brief Pulse width สำหรับ 0° (µs)
 * @note ปรับได้ต่อ instance ด้วย Servo_SetPulseRange()
 */
#define SERVO_PULSE_MIN_US      1000

/**
 * @brief Pulse width สำหรับ 180° (µs)
 * @note ปรับได้ต่อ instance ด้วย Servo_SetPulseRange()
 */
#define SERVO_PULSE_MAX_US      2000

/**
 * @brief มุมสูงสุดของ servo (degrees)
 */
#define SERVO_ANGLE_MAX         180

/**
 * @brief Period ของ PWM (µs) สำหรับ 50Hz
 */
#define SERVO_PERIOD_US         20000

/* ========== Type Definitions ========== */

/**
 * @brief โครงสร้างข้อมูล Servo instance
 *
 * @note ประกาศเป็น global หรือ static ไม่ใช่ local variable
 */
typedef struct {
    PWM_Channel channel;        /**< PWM channel ที่ใช้ */
    uint16_t    pulse_min_us;   /**< Pulse width สำหรับ 0° (µs) */
    uint16_t    pulse_max_us;   /**< Pulse width สำหรับ 180° (µs) */
    uint8_t     current_angle;  /**< มุมปัจจุบัน (degrees) */
    uint16_t    current_pulse;  /**< Pulse width ปัจจุบัน (µs) */
    uint8_t     attached;       /**< 1=PWM กำลังทำงาน, 0=หยุด */
    uint8_t     initialized;    /**< flag บอกว่า init แล้ว */
} Servo_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น Servo instance
 *
 * @param servo    ตัวชี้ไปยัง Servo_Instance
 * @param channel  PWM channel ที่ต้องการใช้ (PWM1_CH1 ถึง PWM2_CH4)
 *
 * @note ต้องเรียก Servo_Attach() เพื่อเริ่ม PWM output
 *
 * @example
 * Servo_Instance servo;
 * Servo_Init(&servo, PWM1_CH4);  // ใช้ pin PC4
 */
void Servo_Init(Servo_Instance* servo, PWM_Channel channel);

/**
 * @brief เริ่ม PWM output (เปิดการทำงาน servo)
 *
 * @param servo  ตัวชี้ไปยัง Servo_Instance
 *
 * @note Servo จะค้างที่ตำแหน่งสุดท้ายหลัง Attach
 *
 * @example
 * Servo_Attach(&servo);
 */
void Servo_Attach(Servo_Instance* servo);

/**
 * @brief หยุด PWM output (servo จะหยุดจ่ายกระแส)
 *
 * @param servo  ตัวชี้ไปยัง Servo_Instance
 *
 * @note หลัง Detach servo จะไม่ค้างตำแหน่ง (free rotation)
 * @note เหมาะสำหรับประหยัดพลังงาน
 *
 * @example
 * Servo_Detach(&servo);
 */
void Servo_Detach(Servo_Instance* servo);

/**
 * @brief หมุน servo ไปยังมุมที่กำหนด
 *
 * @param servo  ตัวชี้ไปยัง Servo_Instance
 * @param angle  มุมที่ต้องการ (0-180°)
 *
 * @note ถ้า angle เกิน 180 จะถูก clamp เป็น 180
 * @note ต้องเรียก Servo_Attach() ก่อน
 *
 * @example
 * Servo_Write(&servo, 90);   // หมุนไปตรงกลาง
 * Servo_Write(&servo, 0);    // หมุนไปซ้ายสุด
 * Servo_Write(&servo, 180);  // หมุนไปขวาสุด
 */
void Servo_Write(Servo_Instance* servo, uint8_t angle);

/**
 * @brief ตั้ง pulse width โดยตรง (µs)
 *
 * @param servo     ตัวชี้ไปยัง Servo_Instance
 * @param pulse_us  Pulse width ในหน่วย microseconds
 *
 * @note ค่าจะถูก clamp ระหว่าง pulse_min_us และ pulse_max_us
 * @note ใช้สำหรับ calibration หรือ Continuous Rotation Servo
 *
 * @example
 * Servo_WriteMicroseconds(&servo, 1500);  // ตรงกลาง
 * Servo_WriteMicroseconds(&servo, 1000);  // 0°
 * Servo_WriteMicroseconds(&servo, 2000);  // 180°
 */
void Servo_WriteMicroseconds(Servo_Instance* servo, uint16_t pulse_us);

/**
 * @brief อ่านมุมปัจจุบันของ servo
 *
 * @param servo  ตัวชี้ไปยัง Servo_Instance
 * @return มุมปัจจุบัน (0-180°)
 *
 * @example
 * uint8_t angle = Servo_Read(&servo);
 * printf("Angle: %d\r\n", angle);
 */
uint8_t Servo_Read(Servo_Instance* servo);

/**
 * @brief อ่าน pulse width ปัจจุบัน (µs)
 *
 * @param servo  ตัวชี้ไปยัง Servo_Instance
 * @return Pulse width ปัจจุบัน (µs)
 *
 * @example
 * uint16_t us = Servo_ReadMicroseconds(&servo);
 */
uint16_t Servo_ReadMicroseconds(Servo_Instance* servo);

/**
 * @brief ตั้งค่าช่วง pulse width สำหรับ servo รุ่นนี้ (Calibration)
 *
 * @param servo       ตัวชี้ไปยัง Servo_Instance
 * @param min_us      Pulse width สำหรับ 0° (µs) — ปกติ 500-1000
 * @param max_us      Pulse width สำหรับ 180° (µs) — ปกติ 2000-2500
 *
 * @note ต้องเรียกก่อน Servo_Attach() เพื่อให้มีผล
 * @note Servo บางรุ่นมีช่วง 500-2500µs แทน 1000-2000µs
 *
 * @example
 * // Servo ช่วง 500-2500µs
 * Servo_SetPulseRange(&servo, 500, 2500);
 */
void Servo_SetPulseRange(Servo_Instance* servo, uint16_t min_us, uint16_t max_us);

/**
 * @brief หมุน servo ไปยังมุมอย่างช้าๆ (Smooth movement)
 *
 * @param servo      ตัวชี้ไปยัง Servo_Instance
 * @param target     มุมเป้าหมาย (0-180°)
 * @param step_ms    ช่วงเวลาระหว่าง step (ms) — น้อย=เร็ว, มาก=ช้า
 *
 * @note ฟังก์ชันนี้ blocking จนกว่าจะถึง target
 *
 * @example
 * // หมุนไป 180° อย่างช้าๆ ใช้เวลา ~2 วินาที
 * Servo_SweepTo(&servo, 180, 10);
 */
void Servo_SweepTo(Servo_Instance* servo, uint8_t target, uint16_t step_ms);

/**
 * @brief ตรวจสอบว่า servo กำลัง attached (PWM ทำงาน) หรือไม่
 *
 * @param servo  ตัวชี้ไปยัง Servo_Instance
 * @return true ถ้า attached, false ถ้า detached
 *
 * @example
 * if (Servo_IsAttached(&servo)) {
 *     printf("Servo is running\r\n");
 * }
 */
bool Servo_IsAttached(Servo_Instance* servo);

#ifdef __cplusplus
}
#endif

#endif /* __SERVO_H */
