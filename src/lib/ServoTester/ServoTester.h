/**
 * @file ServoTester.h
 * @brief Servo Calibration & Diagnostic Tool สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับทดสอบและ calibrate servo motor
 * ใช้วัดหาค่า pulse width ที่เหมาะสมสำหรับ servo แต่ละตัว
 *
 * **คุณสมบัติ:**
 * - Auto-sweep: กวาด pulse อัตโนมัติเพื่อดูการเคลื่อนที่
 * - Find center: หาตำแหน่งกลาง (neutral) โดย auto
 * - Find pulse range: หาค่า min/max pulse ที่ servo ขยับได้
 * - Manual pulse: ส่ง pulse เองเพื่อทดสอบ
 *
 * **Use Cases:**
 * - ทดสอบ servo ใหม่ก่อนใช้งาน
 * - Calibrate servo สำหรับหุ่นยนต์
 * - หาค่า pulse range สำหรับ servo ยี่ห้อที่ไม่รู้ spec
 *
 * @example
 * ServoTester_Instance tester;
 * ServoTester_Init(&tester, PWM1_CH1);
 *
 * // Auto-sweep 500-2500µs step 100µs
 * ServoTester_Sweep(&tester, 500, 2500, 100, 200);
 *
 * // หา center
 * uint16_t center = ServoTester_FindCenter(&tester, 1000, 2000);
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __SERVOTESTER_H
#define __SERVOTESTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "../Servo/Servo.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default servo PWM frequency */
#ifndef SERVOTESTER_FREQ
#define SERVOTESTER_FREQ  50
#endif

/* ========== Type Definitions ========== */

/**
 * @brief ServoTester instance
 */
typedef struct {
    PWM_Channel channel;        /**< PWM channel */
    uint16_t    current_pulse;  /**< Current pulse (µs) */
    uint8_t     initialized;    /**< Init flag */
} ServoTester_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น Servo Tester
 * @param tester  pointer ไปยัง ServoTester_Instance
 * @param channel PWM channel
 *
 * @example
 * ServoTester_Instance tester;
 * ServoTester_Init(&tester, PWM1_CH1);
 */
void ServoTester_Init(ServoTester_Instance* tester, PWM_Channel channel);

/**
 * @brief Auto-sweep: กวาด pulse จาก start ไป end ทีละ step
 * @param tester     pointer ไปยัง ServoTester_Instance
 * @param start_us   pulse เริ่มต้น (µs)
 * @param end_us     pulse สิ้นสุด (µs)
 * @param step_us    ระยะห่างแต่ละ step (µs)
 * @param delay_ms   หน่วงเวลาแต่ละ step (ms)
 *
 * @note ฟังก์ชันนี้เป็น blocking — ใช้สำหรับ testing เท่านั้น
 *
 * @example
 * // Sweep 500-2500µs ทีละ 100µs, หน่วง 200ms แต่ละ step
 * ServoTester_Sweep(&tester, 500, 2500, 100, 200);
 */
void ServoTester_Sweep(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us, uint16_t step_us, uint16_t delay_ms);

/**
 * @brief หาตำแหน่งกลาง (neutral) ของ servo
 * @param tester   pointer ไปยัง ServoTester_Instance
 * @param start_us pulse ต่ำสุดที่ servo ขยับ (µs)
 * @param end_us   pulse สูงสุดที่ servo ขยับ (µs)
 * @return pulse ที่ตำแหน่งกลาง (µs)
 *
 * @note คำนวณจาก (start + end) / 2 — ใช้ manual sweep หา start/end ก่อน
 *
 * @example
 * // หา center ระหว่าง 1000-2000µs
 * uint16_t center = ServoTester_FindCenter(&tester, 1000, 2000);
 * // center = 1500µs
 */
uint16_t ServoTester_FindCenter(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us);

/**
 * @brief หาช่วง pulse ที่ servo ขยับได้ (min/max)
 * @param tester   pointer ไปยัง ServoTester_Instance
 * @param start_us pulse เริ่มค้นหา (µs) — ควรเป็นค่าที่ servo หยุด
 * @param end_us   pulse สิ้นสุดค้นหา (µs)
 * @param min_us   [out] pointer เก็บค่า min pulse
 * @param max_us   [out] pointer เก็บค่า max pulse
 *
 * @note ใช้วิธี binary search หาจุดที่ servo เริ่มขยับ
 *       ฟังก์ชันนี้ให้ค่าประมาณ — ควร verify ด้วยสายตา
 *
 * @example
 * uint16_t min_pulse, max_pulse;
 * ServoTester_FindPulseRange(&tester, 500, 2500, &min_pulse, &max_pulse);
 * printf("Min: %d, Max: %d\n", min_pulse, max_pulse);
 */
void ServoTester_FindPulseRange(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us, uint16_t* min_us, uint16_t* max_us);

/**
 * @brief ส่ง pulse โดยตรง (manual control)
 * @param tester   pointer ไปยัง ServoTester_Instance
 * @param pulse_us pulse width (µs)
 *
 * @example
 * ServoTester_SetPulse(&tester, 1500);  // center
 */
void ServoTester_SetPulse(ServoTester_Instance* tester, uint16_t pulse_us);

/**
 * @brief อ่านค่า pulse ปัจจุบัน
 * @param tester pointer ไปยัง ServoTester_Instance
 * @return pulse ปัจจุบัน (µs)
 */
uint16_t ServoTester_GetCurrentPulse(ServoTester_Instance* tester);

#ifdef __cplusplus
}
#endif

#endif /* __SERVOTESTER_H */
