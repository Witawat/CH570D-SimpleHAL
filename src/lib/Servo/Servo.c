/**
 * @file Servo.c
 * @brief Servo Motor Control Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "Servo.h"

/* ========== Private Functions ========== */

/**
 * @brief แปลง pulse width (µs) เป็น raw PWM duty value
 *
 * สูตร:
 *   duty_raw = pulse_us * PWM_GetPeriod(channel) / SERVO_PERIOD_US
 *
 * ตัวอย่าง (50Hz, Period register = 20000):
 *   1000µs → duty_raw = 1000 * 20000 / 20000 = 1000  (0°)
 *   1500µs → duty_raw = 1500 * 20000 / 20000 = 1500  (90°)
 *   2000µs → duty_raw = 2000 * 20000 / 20000 = 2000  (180°)
 *
 * @param servo     ตัวชี้ไปยัง Servo_Instance
 * @param pulse_us  Pulse width (µs)
 */
static void Servo_ApplyPulse(Servo_Instance* servo, uint16_t pulse_us) {
    /* ดึงค่า period register จาก SimplePWM */
    uint16_t period = PWM_GetPeriod(servo->channel);

    /* duty_raw = pulse_us * period / SERVO_PERIOD_US */
    uint32_t duty_raw = ((uint32_t)pulse_us * (uint32_t)period) / (uint32_t)SERVO_PERIOD_US;

    /* Clamp ไม่ให้เกิน period */
    if (duty_raw > period) duty_raw = period;

    PWM_SetDutyCycleRaw(servo->channel, (uint16_t)duty_raw);
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น Servo instance (ยังไม่เปิด PWM)
 */
void Servo_Init(Servo_Instance* servo, PWM_Channel channel) {
    if (servo == NULL) return;

    servo->channel       = channel;
    servo->pulse_min_us  = SERVO_PULSE_MIN_US;
    servo->pulse_max_us  = SERVO_PULSE_MAX_US;
    servo->current_angle = 90;                  /* เริ่มที่ตรงกลาง */
    servo->current_pulse = 1500;                /* 90° = 1500µs */
    servo->attached      = 0;
    servo->initialized   = 1;
}

/**
 * @brief เริ่ม PWM output สำหรับ servo
 */
void Servo_Attach(Servo_Instance* servo) {
    if (servo == NULL || !servo->initialized) return;

    /* ตั้งค่า PWM ที่ 50Hz */
    PWM_Init(servo->channel, SERVO_PWM_FREQUENCY);

    /* Apply pulse width ปัจจุบัน */
    Servo_ApplyPulse(servo, servo->current_pulse);

    /* เริ่ม output */
    PWM_Start(servo->channel);
    servo->attached = 1;
}

/**
 * @brief หยุด PWM output
 */
void Servo_Detach(Servo_Instance* servo) {
    if (servo == NULL || !servo->initialized) return;

    PWM_Stop(servo->channel);
    servo->attached = 0;
}

/**
 * @brief หมุน servo ไปยังมุมที่กำหนด (0-180°)
 */
void Servo_Write(Servo_Instance* servo, uint8_t angle) {
    if (servo == NULL || !servo->initialized) return;

    /* Clamp มุมให้อยู่ใน 0-180 */
    if (angle > SERVO_ANGLE_MAX) angle = SERVO_ANGLE_MAX;

    /* แปลงมุม → pulse width
     * pulse = pulse_min + (angle * (pulse_max - pulse_min)) / 180 */
    uint32_t range_us  = (uint32_t)(servo->pulse_max_us - servo->pulse_min_us);
    uint16_t pulse_us  = (uint16_t)(servo->pulse_min_us + (range_us * angle) / SERVO_ANGLE_MAX);

    servo->current_angle = angle;
    servo->current_pulse = pulse_us;

    /* ถ้า attached ให้ apply pulse ทันที */
    if (servo->attached) {
        Servo_ApplyPulse(servo, pulse_us);
    }
}

/**
 * @brief ตั้ง pulse width โดยตรง (µs)
 */
void Servo_WriteMicroseconds(Servo_Instance* servo, uint16_t pulse_us) {
    if (servo == NULL || !servo->initialized) return;

    /* Clamp ให้อยู่ในช่วง min-max */
    if (pulse_us < servo->pulse_min_us) pulse_us = servo->pulse_min_us;
    if (pulse_us > servo->pulse_max_us) pulse_us = servo->pulse_max_us;

    /* แปลง pulse กลับเป็นมุมโดยประมาณ */
    uint32_t range_us = (uint32_t)(servo->pulse_max_us - servo->pulse_min_us);
    servo->current_angle = (uint8_t)(((uint32_t)(pulse_us - servo->pulse_min_us) * SERVO_ANGLE_MAX) / range_us);
    servo->current_pulse = pulse_us;

    if (servo->attached) {
        Servo_ApplyPulse(servo, pulse_us);
    }
}

/**
 * @brief อ่านมุมปัจจุบัน
 */
uint8_t Servo_Read(Servo_Instance* servo) {
    if (servo == NULL) return 0;
    return servo->current_angle;
}

/**
 * @brief อ่าน pulse width ปัจจุบัน (µs)
 */
uint16_t Servo_ReadMicroseconds(Servo_Instance* servo) {
    if (servo == NULL) return 0;
    return servo->current_pulse;
}

/**
 * @brief ตั้งค่าช่วง pulse สำหรับ servo รุ่นนี้
 */
void Servo_SetPulseRange(Servo_Instance* servo, uint16_t min_us, uint16_t max_us) {
    if (servo == NULL) return;
    if (min_us >= max_us) return;  /* ป้องกัน range ผิดพลาด */

    servo->pulse_min_us = min_us;
    servo->pulse_max_us = max_us;
}

/**
 * @brief หมุน servo ไปยังมุมอย่างช้าๆ (Smooth movement)
 */
void Servo_SweepTo(Servo_Instance* servo, uint8_t target, uint16_t step_ms) {
    if (servo == NULL || !servo->initialized) return;
    if (!servo->attached) Servo_Attach(servo);
    if (target > SERVO_ANGLE_MAX) target = SERVO_ANGLE_MAX;

    uint8_t current = servo->current_angle;

    if (current == target) return;

    if (current < target) {
        /* หมุนเพิ่มมุม */
        for (uint8_t a = current + 1; a <= target; a++) {
            Servo_Write(servo, a);
            Delay_Ms(step_ms);
        }
    } else {
        /* หมุนลดมุม */
        for (int16_t a = (int16_t)current - 1; a >= (int16_t)target; a--) {
            Servo_Write(servo, (uint8_t)a);
            Delay_Ms(step_ms);
        }
    }
}

/**
 * @brief ตรวจสอบว่า servo attached หรือไม่
 */
bool Servo_IsAttached(Servo_Instance* servo) {
    if (servo == NULL || !servo->initialized) return false;
    return servo->attached == 1;
}
