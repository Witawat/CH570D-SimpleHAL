/**
 * @file StepperMotor.h
 * @brief Stepper Motor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับควบคุม Stepper Motor รองรับ 2 ประเภท driver:
 *
 * **1. ULN2003 (4-wire, สำหรับ 28BYJ-48)**
 * - ใช้ 4 GPIO pins ส่ง step sequence
 * - รองรับ Full Step และ Half Step
 * - ราคาถูก, แรงบิดดี, ใช้ กับ 28BYJ-48 โดยตรง
 *
 * **2. A4988 / DRV8825 / TB6600 (STEP+DIR)**
 * - ใช้ 2-3 GPIO pins: STEP, DIR, EN (optional)
 * - Microstepping รองรับ (ขึ้นอยู่กับ driver)
 * - เหมาะกับ NEMA17, NEMA23 และ motor ขนาดใหญ่
 *
 * **Hardware Connection (ULN2003 + 28BYJ-48):**
 * ```
 *   CH57x         ULN2003        28BYJ-48
 *   IN1 (PC0) -----> IN1 --------> BLUE   (coil A)
 *   IN2 (PC1) -----> IN2 --------> PINK   (coil B)
 *   IN3 (PC2) -----> IN3 --------> YELLOW (coil C)
 *   IN4 (PC3) -----> IN4 --------> ORANGE (coil D)
 *   GND       -----> GND
 *   5V        -----> VCC (motor power, ไม่ใช่ 3.3V!)
 * ```
 *
 * **Hardware Connection (A4988):**
 * ```
 *   CH57x         A4988
 *   STEP (PC0) ----> STEP
 *   DIR  (PC1) ----> DIR
 *   EN   (PC2) ----> EN   (LOW=enable, optional)
 *   GND        ----> GND
 *   3.3V       ----> VDD (logic)
 *   12V        ----> VMOT (motor power)
 * ```
 *
 * @example
 * // ตัวอย่าง ULN2003 + 28BYJ-48
 * #include "SimpleHAL.h"
 * #include "StepperMotor.h"
 *
 * StepperMotor_Instance motor;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     // Init ULN2003: IN1=PC0, IN2=PC1, IN3=PC2, IN4=PC3
 *     StepperMotor_InitULN2003(&motor, PC0, PC1, PC2, PC3, STEPPER_HALF_STEP);
 *     StepperMotor_SetSpeed(&motor, 10);   // 10 RPM
 *
 *     StepperMotor_Move(&motor, 2048);     // 1 รอบ (2048 half-steps)
 *     StepperMotor_MoveDegrees(&motor, 90); // หมุน 90°
 *
 *     while (1) {}
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __STEPPERMOTOR_H
#define __STEPPERMOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/** @brief จำนวน steps ต่อรอบของ 28BYJ-48 (half-step mode) */
#ifndef STEPPER_28BYJ48_STEPS_PER_REV
#define STEPPER_28BYJ48_STEPS_PER_REV   2048
#endif

/** @brief จำนวน steps ต่อรอบของ NEMA17 (1.8°/step = 200 steps) */
#ifndef STEPPER_NEMA17_STEPS_PER_REV
#define STEPPER_NEMA17_STEPS_PER_REV    200
#endif

/** @brief ความเร็วต่ำสุด (RPM) */
#define STEPPER_SPEED_MIN_RPM           1

/** @brief ความเร็วสูงสุดสำหรับ ULN2003 (RPM) */
#define STEPPER_ULN2003_SPEED_MAX_RPM   15

/** @brief ความเร็วสูงสุดสำหรับ A4988 (RPM) */
#define STEPPER_A4988_SPEED_MAX_RPM     300

/* ========== Type Definitions ========== */

/**
 * @brief ประเภท Driver
 */
typedef enum {
    STEPPER_DRIVER_ULN2003 = 0, /**< ULN2003 (4-wire, 28BYJ-48) */
    STEPPER_DRIVER_A4988   = 1  /**< A4988/DRV8825/TB6600 (STEP+DIR+EN) */
} StepperMotor_Driver;

/**
 * @brief โหมด Step ของ ULN2003
 */
typedef enum {
    STEPPER_FULL_STEP = 0, /**< Full Step: แรงบิดสูงสุด, ความละเอียดน้อย */
    STEPPER_HALF_STEP = 1  /**< Half Step: ความละเอียด 2x, แรงบิดน้อยกว่าเล็กน้อย */
} StepperMotor_StepMode;

/**
 * @brief ทิศทางการหมุน
 */
typedef enum {
    STEPPER_CW  = 0, /**< Clockwise (ตามเข็มนาฬิกา) */
    STEPPER_CCW = 1  /**< Counter-Clockwise (ทวนเข็มนาฬิกา) */
} StepperMotor_Direction;

/**
 * @brief Stepper Motor Instance
 */
typedef struct {
    /* Driver type */
    StepperMotor_Driver  driver;      /**< ประเภท driver */
    StepperMotor_StepMode step_mode;  /**< Full/Half step (ULN2003 เท่านั้น) */

    /* ULN2003 pins (IN1-IN4) */
    uint8_t pin_in1;  /**< IN1 pin (ULN2003) */
    uint8_t pin_in2;  /**< IN2 pin (ULN2003) */
    uint8_t pin_in3;  /**< IN3 pin (ULN2003) */
    uint8_t pin_in4;  /**< IN4 pin (ULN2003) */

    /* A4988 pins */
    uint8_t pin_step; /**< STEP pin (A4988) */
    uint8_t pin_dir;  /**< DIR pin (A4988) */
    uint8_t pin_en;   /**< EN pin (A4988, 0=ไม่ใช้) */
    uint8_t has_en;   /**< 1=มี EN pin, 0=ไม่มี */

    /* Configuration */
    uint32_t steps_per_rev; /**< Steps ต่อ 1 รอบ */
    float    linear_mm_per_rev; /**< ระยะเชิงเส้นต่อ 1 รอบ (mm), 0 = ไม่ได้ตั้ง */
    uint32_t speed_rpm;     /**< ความเร็ว (RPM) */
    uint32_t step_delay_us; /**< Delay ระหว่าง step (µs, คำนวณจาก speed_rpm) */

    /* State */
    int32_t  position;      /**< ตำแหน่งปัจจุบัน (steps, นับจาก home) */
    uint8_t  phase;         /**< Phase ปัจจุบันของ ULN2003 (0-7) */
    uint8_t  is_enabled;    /**< สถานะ enable (A4988) */

    uint8_t  initialized;   /**< flag บอกว่า Init แล้ว */
} StepperMotor_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น Stepper Motor แบบ ULN2003 (28BYJ-48)
 * @param motor ตัวแปร instance
 * @param in1 GPIO pin สำหรับ IN1
 * @param in2 GPIO pin สำหรับ IN2
 * @param in3 GPIO pin สำหรับ IN3
 * @param in4 GPIO pin สำหรับ IN4
 * @param mode โหมด step: STEPPER_FULL_STEP หรือ STEPPER_HALF_STEP
 *
 * @example
 * StepperMotor_InitULN2003(&motor, PC0, PC1, PC2, PC3, STEPPER_HALF_STEP);
 */
void StepperMotor_InitULN2003(StepperMotor_Instance* motor,
                               uint8_t in1, uint8_t in2,
                               uint8_t in3, uint8_t in4,
                               StepperMotor_StepMode mode);

/**
 * @brief เริ่มต้น Stepper Motor แบบ A4988/DRV8825
 * @param motor ตัวแปร instance
 * @param step_pin GPIO pin สำหรับ STEP
 * @param dir_pin  GPIO pin สำหรับ DIR
 * @param en_pin   GPIO pin สำหรับ EN (ใส่ 0 ถ้าไม่ต้องการใช้)
 * @param steps_per_rev จำนวน steps ต่อ 1 รอบ (เช่น 200 สำหรับ NEMA17)
 *
 * @example
 * StepperMotor_InitA4988(&motor, PC0, PC1, PC2, 200);
 * StepperMotor_InitA4988(&motor, PC0, PC1, 0,   200); // ไม่มี EN pin
 */
void StepperMotor_InitA4988(StepperMotor_Instance* motor,
                             uint8_t step_pin, uint8_t dir_pin, uint8_t en_pin,
                             uint32_t steps_per_rev);

/**
 * @brief เริ่มต้นมอเตอร์ NEMA17 ผ่าน A4988 (ค่ามาตรฐาน 200 steps/rev)
 * @param motor ตัวแปร instance
 * @param step_pin GPIO pin สำหรับ STEP
 * @param dir_pin  GPIO pin สำหรับ DIR
 * @param en_pin   GPIO pin สำหรับ EN (ใส่ 0 ถ้าไม่ต้องการใช้)
 *
 * @example
 * StepperMotor_InitA4988NEMA17(&motor, PC0, PC1, PC2);
 */
void StepperMotor_InitA4988NEMA17(StepperMotor_Instance* motor,
                                  uint8_t step_pin, uint8_t dir_pin, uint8_t en_pin);

/**
 * @brief ตั้งค่าความเร็ว (RPM)
 * @param motor ตัวแปร instance
 * @param rpm ความเร็วหน่วย RPM
 * @note ULN2003: แนะนำ 1-15 RPM / A4988: 1-300 RPM
 *
 * @example
 * StepperMotor_SetSpeed(&motor, 10); // 10 RPM
 */
void StepperMotor_SetSpeed(StepperMotor_Instance* motor, uint32_t rpm);

/**
 * @brief หมุน motor ตามจำนวน steps (blocking)
 * @param motor ตัวแปร instance
 * @param steps จำนวน steps (บวก=CW, ลบ=CCW)
 * @note ฟังก์ชันนี้ block จนกว่าจะหมุนครบ
 *
 * @example
 * StepperMotor_Move(&motor,  2048);  // หมุน CW 1 รอบ (28BYJ-48 half-step)
 * StepperMotor_Move(&motor, -1024);  // หมุน CCW ครึ่งรอบ
 */
void StepperMotor_Move(StepperMotor_Instance* motor, int32_t steps);

/**
 * @brief หมุน motor ตามมุม (degrees, blocking)
 * @param motor ตัวแปร instance
 * @param degrees มุมที่ต้องการหมุน (บวก=CW, ลบ=CCW)
 *
 * @example
 * StepperMotor_MoveDegrees(&motor, 90);   // หมุน CW 90°
 * StepperMotor_MoveDegrees(&motor, -180); // หมุน CCW 180°
 */
void StepperMotor_MoveDegrees(StepperMotor_Instance* motor, int32_t degrees);

/**
 * @brief หมุน motor จำนวนรอบที่กำหนด (blocking)
 * @param motor ตัวแปร instance
 * @param revolutions จำนวนรอบ (บวก=CW, ลบ=CCW)
 *
 * @example
 * StepperMotor_MoveRevolutions(&motor, 3);   // หมุน CW 3 รอบ
 * StepperMotor_MoveRevolutions(&motor, -1);  // หมุน CCW 1 รอบ
 */
void StepperMotor_MoveRevolutions(StepperMotor_Instance* motor, int32_t revolutions);

/**
 * @brief ตั้งค่าระยะเชิงเส้นที่มอเตอร์เคลื่อนที่ต่อ 1 รอบ
 * @param motor ตัวแปร instance
 * @param mm_per_rev ระยะทางต่อ 1 รอบ (mm) เช่น lead screw 8.0mm/rev
 *
 * @note ต้องตั้งค่านี้ก่อนใช้ MoveMm/MmToSteps/StepsToMm
 */
void StepperMotor_SetLinearMmPerRev(StepperMotor_Instance* motor, float mm_per_rev);

/**
 * @brief แปลงระยะทาง (mm) เป็นจำนวน steps
 * @param motor ตัวแปร instance
 * @param distance_mm ระยะทาง (mm), บวก=CW, ลบ=CCW
 * @return จำนวน steps ที่คำนวณได้
 */
int32_t StepperMotor_MmToSteps(StepperMotor_Instance* motor, float distance_mm);

/**
 * @brief แปลงจำนวน steps เป็นระยะทาง (mm)
 * @param motor ตัวแปร instance
 * @param steps จำนวน steps
 * @return ระยะทาง (mm)
 */
float StepperMotor_StepsToMm(StepperMotor_Instance* motor, int32_t steps);

/**
 * @brief หมุนตามระยะทางเชิงเส้น (mm, blocking)
 * @param motor ตัวแปร instance
 * @param distance_mm ระยะทางที่ต้องการ (mm), บวก=CW, ลบ=CCW
 */
void StepperMotor_MoveMm(StepperMotor_Instance* motor, float distance_mm);

/**
 * @brief Enable motor (เปิด coil, A4988 ดึง EN ลง LOW)
 * @param motor ตัวแปร instance
 * @note ULN2003: ไม่มีผล / A4988: จำเป็นต้อง enable ก่อน move
 */
void StepperMotor_Enable(StepperMotor_Instance* motor);

/**
 * @brief Disable motor (ปิด coil ลดความร้อน)
 * @param motor ตัวแปร instance
 * @note เรียกหลังหมุนเสร็จเพื่อลด current draw
 */
void StepperMotor_Disable(StepperMotor_Instance* motor);

/**
 * @brief ล้าง coils ทั้งหมด (ULN2003: ปิด pins ทั้ง 4)
 * @param motor ตัวแปร instance
 */
void StepperMotor_Stop(StepperMotor_Instance* motor);

/**
 * @brief รีเซ็ตตำแหน่งเป็น 0 (home)
 * @param motor ตัวแปร instance
 */
void StepperMotor_ResetPosition(StepperMotor_Instance* motor);

/**
 * @brief ดูตำแหน่งปัจจุบัน (steps จาก home)
 * @param motor ตัวแปร instance
 * @return ตำแหน่งปัจจุบัน (steps)
 */
int32_t StepperMotor_GetPosition(StepperMotor_Instance* motor);

#ifdef __cplusplus
}
#endif

#endif /* __STEPPERMOTOR_H */
