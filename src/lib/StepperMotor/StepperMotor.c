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
 */
static uint32_t _calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0) rpm = 1;
    /* delay_us = 60,000,000 / (rpm × steps_per_rev) */
    return 60000000UL / (rpm * steps_per_rev);
}

/**
 * @brief เขียน 1 step ให้ ULN2003 ตาม phase ปัจจุบัน
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

void StepperMotor_InitA4988NEMA17(StepperMotor_Instance* motor,
                                  uint8_t step_pin, uint8_t dir_pin, uint8_t en_pin) {
    StepperMotor_InitA4988(motor, step_pin, dir_pin, en_pin, STEPPER_NEMA17_STEPS_PER_REV);
}

void StepperMotor_SetSpeed(StepperMotor_Instance* motor, uint32_t rpm) {
    if (motor == NULL || !motor->initialized) return;
    if (rpm < STEPPER_SPEED_MIN_RPM) rpm = STEPPER_SPEED_MIN_RPM;
    motor->speed_rpm     = rpm;
    motor->step_delay_us = _calc_step_delay_us(rpm, motor->steps_per_rev);
}

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

void StepperMotor_MoveDegrees(StepperMotor_Instance* motor, int32_t degrees) {
    if (motor == NULL || !motor->initialized) return;
    /* steps = degrees × steps_per_rev / 360 */
    int32_t steps = (int32_t)((int64_t)degrees * (int32_t)motor->steps_per_rev / 360);
    StepperMotor_Move(motor, steps);
}

void StepperMotor_MoveRevolutions(StepperMotor_Instance* motor, int32_t revolutions) {
    if (motor == NULL || !motor->initialized) return;
    int32_t steps = revolutions * (int32_t)motor->steps_per_rev;
    StepperMotor_Move(motor, steps);
}

void StepperMotor_SetLinearMmPerRev(StepperMotor_Instance* motor, float mm_per_rev) {
    if (motor == NULL || !motor->initialized) return;
    if (mm_per_rev <= 0.0f) {
        motor->linear_mm_per_rev = 0.0f;
        return;
    }
    motor->linear_mm_per_rev = mm_per_rev;
}

int32_t StepperMotor_MmToSteps(StepperMotor_Instance* motor, float distance_mm) {
    if (motor == NULL || !motor->initialized) return 0;
    if (motor->linear_mm_per_rev <= 0.0f) return 0;

    float steps_f = (distance_mm * (float)motor->steps_per_rev) / motor->linear_mm_per_rev;
    if (steps_f >= 0.0f) {
        return (int32_t)(steps_f + 0.5f);
    }
    return (int32_t)(steps_f - 0.5f);
}

float StepperMotor_StepsToMm(StepperMotor_Instance* motor, int32_t steps) {
    if (motor == NULL || !motor->initialized) return 0.0f;
    if (motor->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * motor->linear_mm_per_rev) / (float)motor->steps_per_rev;
}

void StepperMotor_MoveMm(StepperMotor_Instance* motor, float distance_mm) {
    if (motor == NULL || !motor->initialized) return;
    int32_t steps = StepperMotor_MmToSteps(motor, distance_mm);
    StepperMotor_Move(motor, steps);
}

void StepperMotor_Enable(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    if (motor->driver == STEPPER_DRIVER_A4988 && motor->has_en) {
        digitalWrite(motor->pin_en, LOW); /* LOW = enable สำหรับ A4988 */
    }
    motor->is_enabled = 1;
}

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

void StepperMotor_ResetPosition(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    motor->position = 0;
}

int32_t StepperMotor_GetPosition(StepperMotor_Instance* motor) {
    if (motor == NULL || !motor->initialized) return 0;
    return motor->position;
}
