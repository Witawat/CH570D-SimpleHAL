/**
 * @file StepperMotor.c
 * @brief Stepper Motor Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "StepperMotor.h"

/* ========== Private Variables ========== */

/**
 * @brief ULN2003 Full Step sequence (4 phases × 4 pins)
 *        แต่ละแถว = สถานะ IN1,IN2,IN3,IN4
 */
static const uint8_t _full_step_seq[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

/**
 * @brief ULN2003 Half Step sequence (8 phases × 4 pins)
 */
static const uint8_t _half_step_seq[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

/* ========== Private Functions ========== */

/**
 * @brief คำนวณ step delay จาก RPM
 *
 * @param rpm           ความเร็วรอบต่อนาที (0 → ปรับเป็น 1)
 * @param steps_per_rev จำนวน steps ต่อ 1 รอบ
 *
 * @return delay ระหว่าง step (µs)
 *
 * @note สูตร: delay_us = 60,000,000 / (rpm × steps_per_rev)
 *       60,000,000 µs = 1 นาที
 */
static uint32_t _calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0) rpm = 1;
    /* delay_us = 60,000,000 / (rpm × steps_per_rev) */
    return 60000000UL / (rpm * steps_per_rev);
}

/**
 * @brief เขียน 1 step ให้ ULN2003 ตาม phase ปัจจุบัน
 *
 * @param motor pointer ไปยัง StepperMotor_Instance
 *
 * @note ถ้า step_mode == STEPPER_FULL_STEP → ใช้ _full_step_seq[phase % 4]
 *       ถ้า step_mode == STEPPER_HALF_STEP → ใช้ _half_step_seq[phase % 8]
 *       แต่ละ entry คือ {IN1, IN2, IN3, IN4}
 *       ใช้ digitalWrite() กับ 4 pins พร้อมกัน
 */
static void _uln2003_write_phase(StepperMotor_Instance* motor) {
    if (motor->step_mode == STEPPER_FULL_STEP) {
        uint8_t ph = motor->phase % 4;
        digitalWrite(motor->pin_in1, _full_step_seq[ph][0]);
        digitalWrite(motor->pin_in2, _full_step_seq[ph][1]);
        digitalWrite(motor->pin_in3, _full_step_seq[ph][2]);
        digitalWrite(motor->pin_in4, _full_step_seq[ph][3]);
    } else {
        uint8_t ph = motor->phase % 8;
        digitalWrite(motor->pin_in1, _half_step_seq[ph][0]);
        digitalWrite(motor->pin_in2, _half_step_seq[ph][1]);
        digitalWrite(motor->pin_in3, _half_step_seq[ph][2]);
        digitalWrite(motor->pin_in4, _half_step_seq[ph][3]);
    }
}

/**
 * @brief เดิน 1 step ทิศทาง CW
 *
 * @param motor pointer ไปยัง StepperMotor_Instance
 *
 * @note ULN2003: increment phase → _uln2003_write_phase()
 *       A4988: DIR=HIGH → pulse STEP (HIGH 10 µs → LOW)
 *       อัปเดต motor->position++ และ Delay_Us() ตาม step_delay_us
 */
static void _step_forward(StepperMotor_Instance* motor) {
    if (motor->driver == STEPPER_DRIVER_ULN2003) {
        motor->phase++;
        _uln2003_write_phase(motor);
    } else {
        /* A4988: DIR=HIGH, pulse STEP */
        digitalWrite(motor->pin_dir, HIGH);
        Delay_Us(2);
        digitalWrite(motor->pin_step, HIGH);
        Delay_Us(10);
        digitalWrite(motor->pin_step, LOW);
    }
    motor->position++;
    Delay_Us(motor->step_delay_us);
}

/**
 * @brief เดิน 1 step ทิศทาง CCW
 *
 * @param motor pointer ไปยัง StepperMotor_Instance
 *
 * @note ULN2003: decrement phase (ป้องกัน uint8_t underflow)
 *       ถ้า phase == 0 → กระโดดไป phase สุดท้าย (3 หรือ 7)
 *       A4988: DIR=LOW → pulse STEP (HIGH 10 µs → LOW)
 *       อัปเดต motor->position-- และ Delay_Us() ตาม step_delay_us
 */
static void _step_backward(StepperMotor_Instance* motor) {
    if (motor->driver == STEPPER_DRIVER_ULN2003) {
        /* หลีกเลี่ยง underflow ของ uint8_t */
        if (motor->phase == 0)
            motor->phase = (motor->step_mode == STEPPER_FULL_STEP) ? 3 : 7;
        else
            motor->phase--;
        _uln2003_write_phase(motor);
    } else {
        /* A4988: DIR=LOW, pulse STEP */
        digitalWrite(motor->pin_dir, LOW);
        Delay_Us(2);
        digitalWrite(motor->pin_step, HIGH);
        Delay_Us(10);
        digitalWrite(motor->pin_step, LOW);
    }
    motor->position--;
    Delay_Us(motor->step_delay_us);
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น Stepper Motor แบบ ULN2003 (28BYJ-48)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL → return ทันที)
 * @param in1   GPIO pin สำหรับ IN1
 * @param in2   GPIO pin สำหรับ IN2
 * @param in3   GPIO pin สำหรับ IN3
 * @param in4   GPIO pin สำหรับ IN4
 * @param mode  โหมด step: STEPPER_FULL_STEP หรือ STEPPER_HALF_STEP
 *
 * @note ตั้งค่า default: steps_per_rev = 2048 (28BYJ-48), speed = 10 RPM
 *       ตั้ง GPIO ทั้ง 4 pin เป็น OUTPUT → เรียก StepperMotor_Stop() เพื่อ clear coils
 *       phase = 0, position = 0, is_enabled = 1
 */
void StepperMotor_InitULN2003(StepperMotor_Instance* motor,
                               uint8_t in1, uint8_t in2,
                               uint8_t in3, uint8_t in4,
                               StepperMotor_StepMode mode) {
    if (motor == NULL) return;

    motor->driver       = STEPPER_DRIVER_ULN2003;
    motor->step_mode    = mode;
    motor->pin_in1      = in1;
    motor->pin_in2      = in2;
    motor->pin_in3      = in3;
    motor->pin_in4      = in4;
    motor->pin_step     = 0;
    motor->pin_dir      = 0;
    motor->pin_en       = 0;
    motor->has_en       = 0;
    motor->steps_per_rev = STEPPER_28BYJ48_STEPS_PER_REV;
    motor->linear_mm_per_rev = 0.0f;
    motor->speed_rpm    = 10;
    motor->phase        = 0;
    motor->position     = 0;
    motor->is_enabled   = 1;
    motor->initialized  = 1;

    motor->step_delay_us = _calc_step_delay_us(motor->speed_rpm, motor->steps_per_rev);

    /* ตั้ง GPIO เป็น output */
    pinMode(in1, PIN_MODE_OUTPUT);
    pinMode(in2, PIN_MODE_OUTPUT);
    pinMode(in3, PIN_MODE_OUTPUT);
    pinMode(in4, PIN_MODE_OUTPUT);

    /* Clear all coils */
    StepperMotor_Stop(motor);
}

/**
 * @brief เริ่มต้น Stepper Motor แบบ A4988/DRV8825
 *
 * @param motor         pointer ไปยัง StepperMotor_Instance (NULL → return ทันที)
 * @param step_pin      GPIO pin สำหรับ STEP
 * @param dir_pin       GPIO pin สำหรับ DIR
 * @param en_pin        GPIO pin สำหรับ EN (0 = ไม่ใช้)
 * @param steps_per_rev จำนวน steps ต่อ 1 รอบ (0 → ใช้ค่า default 200)
 *
 * @note ตั้ง GPIO step_pin/dir_pin เป็น OUTPUT (LOW)
 *       ถ้า en_pin != 0 → ตั้งเป็น OUTPUT, HIGH = disable (A4988)
 *       initial speed = 60 RPM, position = 0, is_enabled = 0
 *       phase = 0 (ไม่ใช้กับ A4988 แต่เก็บไว้)
 */
void StepperMotor_InitA4988(StepperMotor_Instance* motor,
                             uint8_t step_pin, uint8_t dir_pin, uint8_t en_pin,
                             uint32_t steps_per_rev) {
    if (motor == NULL) return;

    motor->driver        = STEPPER_DRIVER_A4988;
    motor->step_mode     = STEPPER_FULL_STEP;
    motor->pin_step      = step_pin;
    motor->pin_dir       = dir_pin;
    motor->pin_en        = en_pin;
    motor->has_en        = (en_pin != 0) ? 1 : 0;
    motor->pin_in1       = 0;
    motor->pin_in2       = 0;
    motor->pin_in3       = 0;
    motor->pin_in4       = 0;
    motor->steps_per_rev = (steps_per_rev > 0) ? steps_per_rev : STEPPER_NEMA17_STEPS_PER_REV;
    motor->linear_mm_per_rev = 0.0f;
    motor->speed_rpm     = 60;
    motor->phase         = 0;
    motor->position      = 0;
    motor->is_enabled    = 0;
    motor->initialized   = 1;

    motor->step_delay_us = _calc_step_delay_us(motor->speed_rpm, motor->steps_per_rev);

    /* ตั้ง GPIO */
    pinMode(step_pin, PIN_MODE_OUTPUT);
    pinMode(dir_pin,  PIN_MODE_OUTPUT);
    digitalWrite(step_pin, LOW);
    digitalWrite(dir_pin,  LOW);

    if (motor->has_en) {
        pinMode(en_pin, PIN_MODE_OUTPUT);
        digitalWrite(en_pin, HIGH); /* HIGH = disable สำหรับ A4988 */
    }
}

/**
 * @brief เริ่มต้นมอเตอร์ NEMA17 ผ่าน A4988 (ค่ามาตรฐาน 200 steps/rev)
 *
 * @param motor    pointer ไปยัง StepperMotor_Instance (NULL → return ทันที)
 * @param step_pin GPIO pin สำหรับ STEP
 * @param dir_pin  GPIO pin สำหรับ DIR
 * @param en_pin   GPIO pin สำหรับ EN (0 = ไม่ใช้)
 *
 * @note convenience wrapper รอบ StepperMotor_InitA4988() โดยส่ง steps_per_rev = 200
 *       ดูรายละเอียดเพิ่มเติมที่ StepperMotor_InitA4988()
 */
void StepperMotor_InitA4988NEMA17(StepperMotor_Instance* motor,
                                  uint8_t step_pin, uint8_t dir_pin, uint8_t en_pin) {
    StepperMotor_InitA4988(motor, step_pin, dir_pin, en_pin, STEPPER_NEMA17_STEPS_PER_REV);
}

/**
 * @brief ตั้งค่าความเร็ว (RPM)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param rpm   ความเร็วหน่วย RPM (ถ้า < STEPPER_SPEED_MIN_RPM → clamp)
 *
 * @note คำนวณ step_delay_us = 60,000,000 / (rpm × steps_per_rev)
 *       ULN2003: แนะนำ 1-15 RPM / A4988: 1-300 RPM
 *       ยิ่ง RPM สูง → step_delay_us ยิ่งน้อย
 */
void StepperMotor_SetSpeed(StepperMotor_Instance* motor, uint32_t rpm) {
    if (motor == NULL || !motor->initialized) return;
    if (rpm < STEPPER_SPEED_MIN_RPM) rpm = STEPPER_SPEED_MIN_RPM;
    motor->speed_rpm     = rpm;
    motor->step_delay_us = _calc_step_delay_us(rpm, motor->steps_per_rev);
}

/**
 * @brief หมุน motor ตามจำนวน steps (blocking)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param steps จำนวน steps (บวก = CW, ลบ = CCW, 0 = return ทันที)
 *
 * @note blocking — วน loop เรียก _step_forward() หรือ _step_backward()
 *       แต่ละ step มี Delay_Us() ตาม step_delay_us
 *       หลังหมุนเสร็จ ULN2003 จะปิด coils อัตโนมัติ (StepperMotor_Stop) เพื่อลด heat
 *       A4988: coil ยังมีแรง (ต้องเรียก Disable() แยกถ้าต้องการ)
 */
void StepperMotor_Move(StepperMotor_Instance* motor, int32_t steps) {
    if (motor == NULL || !motor->initialized) return;

    if (steps > 0) {
        for (int32_t i = 0; i < steps; i++) {
            _step_forward(motor);
        }
    } else {
        int32_t abs_steps = -steps;
        for (int32_t i = 0; i < abs_steps; i++) {
            _step_backward(motor);
        }
    }

    /* ปิด coils หลังหมุนเสร็จ (ลด heat) */
    if (motor->driver == STEPPER_DRIVER_ULN2003) {
        StepperMotor_Stop(motor);
    }
}

/**
 * @brief หมุน motor ตามมุม (degrees, blocking)
 *
 * @param motor   pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param degrees มุมที่ต้องการหมุน (บวก = CW, ลบ = CCW)
 *
 * @note แปลง: steps = degrees × steps_per_rev / 360
 *       delegate ไปที่ StepperMotor_Move()
 *       ค่าอาจมีการ truncation ถ้า steps_per_rev หาร 360 ไม่ลงตัว
 */
void StepperMotor_MoveDegrees(StepperMotor_Instance* motor, int32_t degrees) {
    if (motor == NULL || !motor->initialized) return;
    /* steps = degrees × steps_per_rev / 360 */
    int32_t steps = (int32_t)((int64_t)degrees * (int32_t)motor->steps_per_rev / 360);
    StepperMotor_Move(motor, steps);
}

/**
 * @brief หมุน motor จำนวนรอบที่กำหนด (blocking)
 *
 * @param motor       pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param revolutions จำนวนรอบ (บวก = CW, ลบ = CCW)
 *
 * @note แปลง: steps = revolutions × steps_per_rev
 *       delegate ไปที่ StepperMotor_Move()
 */
void StepperMotor_MoveRevolutions(StepperMotor_Instance* motor, int32_t revolutions) {
    if (motor == NULL || !motor->initialized) return;
    int32_t steps = revolutions * (int32_t)motor->steps_per_rev;
    StepperMotor_Move(motor, steps);
}

/**
 * @brief ตั้งค่าระยะเชิงเส้นที่มอเตอร์เคลื่อนที่ต่อ 1 รอบ
 *
 * @param motor      pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param mm_per_rev ระยะทางต่อ 1 รอบ (mm) เช่น lead screw 8.0 mm/rev
 *
 * @note ต้องตั้งค่านี้ก่อนใช้ MoveMm/MmToSteps/StepsToMm
 *       ถ้า mm_per_rev <= 0 → ตั้งเป็น 0.0 (disabled)
 */
void StepperMotor_SetLinearMmPerRev(StepperMotor_Instance* motor, float mm_per_rev) {
    if (motor == NULL || !motor->initialized) return;
    if (mm_per_rev <= 0.0f) {
        motor->linear_mm_per_rev = 0.0f;
        return;
    }
    motor->linear_mm_per_rev = mm_per_rev;
}

/**
 * @brief แปลงระยะทาง (mm) เป็นจำนวน steps
 *
 * @param motor       pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return 0)
 * @param distance_mm ระยะทาง (mm), บวก = CW, ลบ = CCW
 *
 * @return จำนวน steps ที่คำนวณได้ (ปัดเศษ), 0 ถ้า linear_mm_per_rev ยังไม่ถูกตั้ง
 *
 * @note สูตร: steps = distance_mm × steps_per_rev / linear_mm_per_rev
 *       ปัดเศษ: +0.5 สำหรับบวก, -0.5 สำหรับลบ
 *       ต้องเรียก SetLinearMmPerRev() ก่อน
 */
int32_t StepperMotor_MmToSteps(StepperMotor_Instance* motor, float distance_mm) {
    if (motor == NULL || !motor->initialized) return 0;
    if (motor->linear_mm_per_rev <= 0.0f) return 0;

    float steps_f = (distance_mm * (float)motor->steps_per_rev) / motor->linear_mm_per_rev;
    if (steps_f >= 0.0f) {
        return (int32_t)(steps_f + 0.5f);
    }
    return (int32_t)(steps_f - 0.5f);
}

/**
 * @brief แปลงจำนวน steps เป็นระยะทาง (mm)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return 0.0f)
 * @param steps จำนวน steps
 *
 * @return ระยะทาง (mm), 0.0f ถ้า linear_mm_per_rev ยังไม่ถูกตั้ง
 *
 * @note สูตร: mm = steps × linear_mm_per_rev / steps_per_rev
 *       ต้องเรียก SetLinearMmPerRev() ก่อน
 */
float StepperMotor_StepsToMm(StepperMotor_Instance* motor, int32_t steps) {
    if (motor == NULL || !motor->initialized) return 0.0f;
    if (motor->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * motor->linear_mm_per_rev) / (float)motor->steps_per_rev;
}

/**
 * @brief หมุนตามระยะทางเชิงเส้น (mm, blocking)
 *
 * @param motor       pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param distance_mm ระยะทางที่ต้องการ (mm), บวก = CW, ลบ = CCW
 *
 * @note แปลง mm → steps ด้วย StepperMotor_MmToSteps() → delegate ไปที่ StepperMotor_Move()
 *       ต้องเรียก SetLinearMmPerRev() ก่อน
 */
void StepperMotor_MoveMm(StepperMotor_Instance* motor, float distance_mm) {
    if (motor == NULL || !motor->initialized) return;
    int32_t steps = StepperMotor_MmToSteps(motor, distance_mm);
    StepperMotor_Move(motor, steps);
}

/**
 * @brief Enable motor (เปิด coil)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note A4988: ถ้า has_en → ดึง EN pin ลง LOW (LOW = enable)
 *       ULN2003: ไม่มี effect (coil ถูกควบคุมผ่าน step sequence อยู่แล้ว)
 *       ตั้ง is_enabled = 1
 */
void StepperMotor_Enable(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    if (motor->driver == STEPPER_DRIVER_A4988 && motor->has_en) {
        digitalWrite(motor->pin_en, LOW); /* LOW = enable สำหรับ A4988 */
    }
    motor->is_enabled = 1;
}

/**
 * @brief Disable motor (ปิด coil ลดความร้อน)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note A4988: ถ้า has_en → ดึง EN pin ขึ้น HIGH (HIGH = disable)
 *       ULN2003: เรียก StepperMotor_Stop() เพื่อ clear coils ทั้ง 4
 *       ตั้ง is_enabled = 0
 *       ควรเรียกหลังหมุนเสร็จเพื่อลด current draw
 */
void StepperMotor_Disable(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    if (motor->driver == STEPPER_DRIVER_A4988 && motor->has_en) {
        digitalWrite(motor->pin_en, HIGH); /* HIGH = disable */
    }
    if (motor->driver == STEPPER_DRIVER_ULN2003) {
        StepperMotor_Stop(motor);
    }
    motor->is_enabled = 0;
}

/**
 * @brief หยุด motor (ล้าง coils / STEP)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ULN2003: set IN1-IN4 ทั้งหมดเป็น LOW (clear coils, motor คลายตัว)
 *       A4988: set STEP pin เป็น LOW เท่านั้น (coil ยังมีแรง)
 *       ใช้เพื่อหยุด motor ลงจอด ไม่ได้ disable coil (ใช้ Disable() สำหรับ coil off)
 */
void StepperMotor_Stop(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    if (motor->driver == STEPPER_DRIVER_ULN2003) {
        digitalWrite(motor->pin_in1, LOW);
        digitalWrite(motor->pin_in2, LOW);
        digitalWrite(motor->pin_in3, LOW);
        digitalWrite(motor->pin_in4, LOW);
    } else {
        digitalWrite(motor->pin_step, LOW);
    }
}

/**
 * @brief รีเซ็ตตำแหน่งเป็น 0 (home)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ตั้ง motor->position = 0
 *       ไม่ได้ทำให้ motor หมุน — แค่รีเซ็ต counter
 *       ใช้หลังจากทำ homing sequence ด้วยตัวเอง
 */
void StepperMotor_ResetPosition(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    motor->position = 0;
}

/**
 * @brief ดูตำแหน่งปัจจุบัน (steps จาก home)
 *
 * @param motor pointer ไปยัง StepperMotor_Instance (NULL หรือไม่ initialized → return 0)
 *
 * @return ตำแหน่งปัจจุบัน (steps) — ค่าสะสมตั้งแต่ init หรือ reset ครั้งล่าสุด
 *
 * @note ตำแหน่งถูกอัปเดตใน _step_forward()/_step_backward() (+/- 1 ต่อ step)
 *       ไม่ใช่ absolute encoder — เป็น virtual position counter
 */
int32_t StepperMotor_GetPosition(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return 0;
    return motor->position;
}
