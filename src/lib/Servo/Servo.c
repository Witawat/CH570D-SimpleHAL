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
 * @param servo     Pointer to Servo_Instance
 * @param pulse_us  Pulse width in microseconds (µs)
 * @return None
 * @note Uses PWM_GetPeriod to retrieve the timer period register, then computes
 *       duty_raw = (pulse_us * period) / SERVO_PERIOD_US. Result is clamped to
 *       the timer period to prevent overflow. Assumes 50Hz PWM (20ms period).
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
 * @param servo  Pointer to Servo_Instance to initialize
 * @param channel PWM channel to associate with this servo
 * @return None
 * @note Initializes internal state with default pulse range (SERVO_PULSE_MIN_US
 *       to SERVO_PULSE_MAX_US), sets initial angle to 90° (center, 1500µs),
 *       and marks the instance as initialized but not yet attached. The PWM
 *       output is not started until Servo_Attach() is called.
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
 * @param servo Pointer to Servo_Instance
 * @return None
 * @note Configures the associated PWM channel to 50Hz (SERVO_PWM_FREQUENCY),
 *       applies the current pulse width stored in the instance, and starts the
 *       PWM output. Sets the attached flag on success. No-op if servo is NULL
 *       or not initialized.
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
 * @param servo Pointer to Servo_Instance
 * @return None
 * @note Stops the PWM output on the servo's channel and clears the attached
 *       flag. The servo's current angle and pulse values are preserved so
 *       Servo_Attach() can resume from the same position. No-op if servo is
 *       NULL or not initialized.
 */
void Servo_Detach(Servo_Instance* servo) {
    if (servo == NULL || !servo->initialized) return;

    PWM_Stop(servo->channel);
    servo->attached = 0;
}

/**
 * @brief หมุน servo ไปยังมุมที่กำหนด (0-180°)
 * @param servo Pointer to Servo_Instance
 * @param angle Target angle in degrees (0–180). Clamped internally.
 * @return None
 * @note Maps angle to pulse width linearly: pulse = pulse_min + (angle *
 *       (pulse_max - pulse_min)) / 180. Updates internal angle and pulse
 *       tracking. If attached, applies the new pulse immediately via
 *       Servo_ApplyPulse. No-op if servo is NULL or not initialized.
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
 * @param servo    Pointer to Servo_Instance
 * @param pulse_us Pulse width in microseconds, clamped to servo's min–max range
 * @return None
 * @note Converts the given pulse width back to an approximate angle for internal
 *       tracking. If attached, applies the new pulse immediately. Useful for
 *       fine-grained control beyond standard 0–180° mapping (e.g., 500–2500µs).
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
 * @param servo Pointer to Servo_Instance
 * @return Current servo angle in degrees (0–180). Returns 0 if servo is NULL.
 * @note Returns the last written or calculated angle, not a physical measurement.
 *       Accuracy depends on the servo's pulse-to-angle mapping.
 */
uint8_t Servo_Read(Servo_Instance* servo) {
    if (servo == NULL) return 0;
    return servo->current_angle;
}

/**
 * @brief อ่าน pulse width ปัจจุบัน (µs)
 * @param servo Pointer to Servo_Instance
 * @return Current pulse width in microseconds. Returns 0 if servo is NULL.
 * @note Returns the last pulse value stored in the instance. Does not read
 *       back from the PWM hardware.
 */
uint16_t Servo_ReadMicroseconds(Servo_Instance* servo) {
    if (servo == NULL) return 0;
    return servo->current_pulse;
}

/**
 * @brief ตั้งค่าช่วง pulse สำหรับ servo รุ่นนี้
 * @param servo  Pointer to Servo_Instance
 * @param min_us Minimum pulse width in microseconds
 * @param max_us Maximum pulse width in microseconds (must be > min_us)
 * @return None
 * @note Updates the pulse range mapping used by Servo_Write and
 *       Servo_WriteMicroseconds. Does not re-apply the current pulse. If
 *       min_us >= max_us the call is ignored. Call after Servo_Init but
 *       before or after Attach as needed.
 */
void Servo_SetPulseRange(Servo_Instance* servo, uint16_t min_us, uint16_t max_us) {
    if (servo == NULL) return;
    if (min_us >= max_us) return;  /* ป้องกัน range ผิดพลาด */

    servo->pulse_min_us = min_us;
    servo->pulse_max_us = max_us;
}

/**
 * @brief หมุน servo ไปยังมุมอย่างช้าๆ (Smooth movement)
 * @param servo   Pointer to Servo_Instance
 * @param target  Target angle in degrees (0–180). Clamped internally.
 * @param step_ms Delay in milliseconds between each 1° increment/decrement
 * @return None
 * @note Increments or decrements the angle by 1° at a time with a delay
 *       between steps. Automatically calls Servo_Attach if not already
 *       attached. Blocks the calling thread for the duration of the sweep.
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
 * @param servo Pointer to Servo_Instance
 * @return true if servo is attached (PWM output active), false otherwise
 * @note Also returns false if servo is NULL or not initialized.
 */
bool Servo_IsAttached(Servo_Instance* servo) {
    if (servo == NULL || !servo->initialized) return false;
    return servo->attached == 1;
}
