/**
 * @file HCSR04.h
 * @brief HC-SR04 Ultrasonic Distance Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับวัดระยะทางด้วย HC-SR04 Ultrasonic Sensor
 * ใช้หลักการส่ง ultrasonic pulse และวัดเวลาที่ echo กลับมา
 *
 * **คุณสมบัติ:**
 * - วัดระยะทาง 2-400 cm (0.3 ถึง 13 ฟุต)
 * - ความละเอียด 0.3 cm
 * - วัดหน่วย cm และ inch
 * - วัดหลายครั้งแล้ว average เพื่อความเสถียร
 * - Timeout protection ป้องกัน loop ค้าง
 * - รองรับ multi-sensor (หลายตัวพร้อมกัน)
 *
 * **HC-SR04 Specifications:**
 * - ช่วงวัด: 2-400 cm
 * - ความแม่นยำ: ±3 mm
 * - Beam angle: 15°
 * - ไฟเลี้ยง: 5V (แต่ใช้กับ 3.3V ได้ในระยะสั้น)
 * - Current: 15 mA
 *
 * **Hardware Connection:**
 * ```
 *   HC-SR04        CH57x
 *   VCC    ------> 5V (หรือ 3.3V)
 *   TRIG   ------> GPIO Pin (Output)  เช่น PC3
 *   ECHO   ------> GPIO Pin (Input)   เช่น PC4
 *   GND    ------> GND
 *
 *   หมายเหตุ: ถ้าใช้ 5V ต้องมี voltage divider บน ECHO pin:
 *   ECHO --[2kΩ]-- PC4 --[3.3kΩ]-- GND
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "HCSR04.h"
 *
 * HCSR04_Instance sensor;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     HCSR04_Init(&sensor, PC3, PC4);
 *
 *     while (1) {
 *         float dist = HCSR04_MeasureCm(&sensor);
 *         if (dist > 0) {
 *             printf("Distance: %.1f cm\r\n", dist);
 *         } else {
 *             printf("Out of range\r\n");
 *         }
 *         Delay_Ms(100);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __HCSR04_H
#define __HCSR04_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief ระยะทางสูงสุดที่ sensor วัดได้ (cm)
 */
#define HCSR04_MAX_DISTANCE_CM    400.0f

/**
 * @brief ระยะทางน้อยสุดที่ sensor วัดได้ (cm)
 */
#define HCSR04_MIN_DISTANCE_CM    2.0f

/**
 * @brief ค่าที่ return เมื่อเกิด error หรืออยู่นอกช่วงวัด
 */
#define HCSR04_ERROR_VALUE        -1.0f

/**
 * @brief Timeout สำหรับรอ Echo signal (µs)
 * @note 30000µs = 30ms ≈ ระยะทาง ~5m (เกินพอสำหรับ HC-SR04)
 */
#define HCSR04_ECHO_TIMEOUT_US    30000UL

/**
 * @brief ช่วงเวลาน้อยสุดระหว่างการวัด (ms)
 * @note HC-SR04 ต้องการ ≥60ms ระหว่างการวัดแต่ละครั้ง
 */
#define HCSR04_MIN_INTERVAL_MS    60

/* ========== Type Definitions ========== */

/**
 * @brief โครงสร้างข้อมูล HC-SR04 instance
 *
 * @note ประกาศเป็น global หรือ static ไม่ใช่ local variable
 */
typedef struct {
    uint8_t  pin_trig;              /**< TRIG pin - ส่งสัญญาณกระตุ้น */
    uint8_t  pin_echo;              /**< ECHO pin - รับสัญญาณสะท้อน */
    float    last_distance_cm;      /**< ระยะทางล่าสุดที่วัดได้ (cm) */
    uint32_t last_measure_time;     /**< เวลาที่วัดล่าสุด (ms) */
    uint8_t  initialized;           /**< flag บอกว่า init แล้ว */
} HCSR04_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น HC-SR04 sensor
 *
 * @param sensor    ตัวชี้ไปยัง HCSR04_Instance
 * @param pin_trig  GPIO pin ที่ต่อกับ TRIG (Output)
 * @param pin_echo  GPIO pin ที่ต่อกับ ECHO (Input)
 *
 * @example
 * HCSR04_Instance sensor;
 * HCSR04_Init(&sensor, PC3, PC4);
 */
void HCSR04_Init(HCSR04_Instance* sensor, uint8_t pin_trig, uint8_t pin_echo);

/**
 * @brief วัดระยะทางในหน่วยเซนติเมตร
 *
 * @param sensor  ตัวชี้ไปยัง HCSR04_Instance
 * @return ระยะทาง (cm) ในช่วง 2-400 cm
 *         หรือ HCSR04_ERROR_VALUE (-1) ถ้า timeout หรือนอกช่วงวัด
 *
 * @note ใช้เวลาประมาณ 20-30ms ต่อครั้ง
 * @note ต้องรอ ≥60ms ระหว่างการวัดแต่ละครั้ง
 *
 * @example
 * float dist = HCSR04_MeasureCm(&sensor);
 * if (dist != HCSR04_ERROR_VALUE) {
 *     printf("%.1f cm\r\n", dist);
 * }
 */
float HCSR04_MeasureCm(HCSR04_Instance* sensor);

/**
 * @brief วัดระยะทางในหน่วยนิ้ว
 *
 * @param sensor  ตัวชี้ไปยัง HCSR04_Instance
 * @return ระยะทาง (inch) หรือ HCSR04_ERROR_VALUE ถ้า error
 *
 * @example
 * float dist_inch = HCSR04_MeasureInch(&sensor);
 */
float HCSR04_MeasureInch(HCSR04_Instance* sensor);

/**
 * @brief วัดระยะทางหลายครั้งแล้วหาค่าเฉลี่ย (ลด noise)
 *
 * @param sensor   ตัวชี้ไปยัง HCSR04_Instance
 * @param samples  จำนวนครั้งที่วัด (1-10)
 * @return ค่าเฉลี่ย (cm) หรือ HCSR04_ERROR_VALUE ถ้า error ทุกครั้ง
 *
 * @note ใช้เวลา = samples × (เวลาวัด + 60ms)
 *
 * @example
 * // วัด 5 ครั้งแล้วเฉลี่ย
 * float avg = HCSR04_MeasureAvgCm(&sensor, 5);
 */
float HCSR04_MeasureAvgCm(HCSR04_Instance* sensor, uint8_t samples);

/**
 * @brief ดึงระยะทางล่าสุด (ไม่วัดใหม่)
 *
 * @param sensor  ตัวชี้ไปยัง HCSR04_Instance
 * @return ระยะทางล่าสุด (cm) ที่วัดได้
 *
 * @example
 * float last = HCSR04_GetLastDistance(&sensor);
 */
float HCSR04_GetLastDistance(HCSR04_Instance* sensor);

/**
 * @brief ตรวจสอบว่าวัตถุอยู่ใกล้กว่าระยะที่กำหนดหรือไม่
 *
 * @param sensor     ตัวชี้ไปยัง HCSR04_Instance
 * @param threshold  ระยะ threshold (cm)
 * @return true ถ้าวัตถุอยู่ใกล้กว่า threshold
 *
 * @example
 * // ตรวจสอบว่ามีสิ่งกีดขวางในระยะ 30 cm หรือไม่
 * if (HCSR04_IsObjectNear(&sensor, 30.0f)) {
 *     printf("มีสิ่งกีดขวาง!\r\n");
 * }
 */
bool HCSR04_IsObjectNear(HCSR04_Instance* sensor, float threshold);

#ifdef __cplusplus
}
#endif

#endif /* __HCSR04_H */
