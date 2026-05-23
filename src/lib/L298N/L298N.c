/**
 * @file L298N.c
 * @brief L298N DC Motor Driver Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "L298N.h"

/* ========== Private Helpers ========== */

static void _set_direction_pins(L298N_Instance* motor, L298N_Direction dir) {
    switch (dir) {
        case L298N_FORWARD:
            digitalWrite(motor->pin_in1, HIGH);
            digitalWrite(motor->pin_in2, LOW);
            break;
        case L298N_BACKWARD:
            digitalWrite(motor->pin_in1, LOW);
            digitalWrite(motor->pin_in2, HIGH);
            break;
        case L298N_STOP:
            digitalWrite(motor->pin_in1, LOW);
            digitalWrite(motor->pin_in2, LOW);
            break;
        case L298N_BRAKE:
            digitalWrite(motor->pin_in1, HIGH);
            digitalWrite(motor->pin_in2, HIGH);
            break;
    }
    motor->direction = dir;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น L298N motor driver
 *
 * @param motor       pointer ไปยัง L298N_Instance (NULL → return L298N_ERROR_PARAM)
 * @param pin_in1     GPIO pin สำหรับ IN1
 * @param pin_in2     GPIO pin สำหรับ IN2
 * @param pwm_channel PWM channel สำหรับ EN (Enable/Speed)
 *
 * @return L298N_OK ถ้าสำเร็จ, L298N_ERROR_PARAM ถ้า motor == NULL
 *
 * @note ตั้ง IN1/IN2 เป็น OUTPUT (LOW), ตั้ง PWM ที่ 1 kHz duty 0 แล้ว start
 *       ใช้ PWM_Init() + PWM_SetDutyCycle() + PWM_Start()
 *       ต้องเรียก Timer_Init() ก่อน (ใน SimpleHAL)
 */
L298N_Status L298N_Init(L298N_Instance* motor, uint8_t pin_in1, uint8_t pin_in2,
                         PWM_Channel pwm_channel) {
    if (motor == NULL) return L298N_ERROR_PARAM;

    motor->pin_in1     = pin_in1;
    motor->pin_in2     = pin_in2;
    motor->pwm_channel = pwm_channel;
    motor->speed       = 0;
    motor->direction   = L298N_STOP;
    motor->initialized = 0;

    /* ตั้ง direction pins เป็น output */
    pinMode(pin_in1, PIN_MODE_OUTPUT);
    pinMode(pin_in2, PIN_MODE_OUTPUT);
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, LOW);

    /* ตั้ง PWM สำหรับ Enable pin ที่ 1kHz */
    PWM_Init(pwm_channel, 1000);
    PWM_SetDutyCycle(pwm_channel, 0);
    PWM_Start(pwm_channel);

    motor->initialized = 1;
    return L298N_OK;
}

/**
 * @brief หมุน motor ด้วยทิศทางและความเร็วที่กำหนด
 *
 * @param motor pointer ไปยัง L298N_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param dir   ทิศทาง (L298N_FORWARD / L298N_BACKWARD)
 * @param speed ความเร็ว 0–100 (%) ถ้า > 100 จะถูก clamp
 *
 * @note เรียก _set_direction_pins() เพื่อตั้ง IN1/IN2 ตามทิศทาง
 *       แล้ว PWM_SetDutyCycle() กับ PWM channel
 *       ค่า speed 0 = หยุด (แต่ยังมี current ถ้าไม่ใช้ Brake/Stop)
 */
void L298N_Run(L298N_Instance* motor, L298N_Direction dir, uint8_t speed) {
    if (motor == NULL || !motor->initialized) return;
    if (speed > 100) speed = 100;

    motor->speed = speed;
    _set_direction_pins(motor, dir);
    PWM_SetDutyCycle(motor->pwm_channel, speed);
}

/**
 * @brief หยุด motor แบบ Coast (ไม่เบรก)
 *
 * @param motor pointer ไปยัง L298N_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ตั้ง IN1=LOW, IN2=LOW แล้ว PWM duty = 0
 *       motor จะค่อย ๆ หยุดตาม inertia (coast)
 *       ต่างจาก Brake ที่ short ทั้ง 2 ขาเข้าด้วยกัน
 */
void L298N_Stop(L298N_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    _set_direction_pins(motor, L298N_STOP);
    PWM_SetDutyCycle(motor->pwm_channel, 0);
    motor->speed = 0;
}

/**
 * @brief เบรก motor (Short brake)
 *
 * @param motor pointer ไปยัง L298N_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ตั้ง IN1=HIGH, IN2=HIGH แล้ว PWM duty = 100
 *       H-bridge จะ short ขั้ว motor ทำให้หยุดทันที
 *       กินกระแสสูง ควรใช้สั้น ๆ เท่านั้น
 */
void L298N_Brake(L298N_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    _set_direction_pins(motor, L298N_BRAKE);
    PWM_SetDutyCycle(motor->pwm_channel, 100);
    motor->speed = 0;
}

/**
 * @brief ตั้งความเร็ว (คงทิศทางเดิม)
 *
 * @param motor pointer ไปยัง L298N_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param speed ความเร็ว 0–100 (%) ถ้า > 100 จะถูก clamp
 *
 * @note เปลี่ยนเฉพาะ PWM duty cycle ไม่เปลี่ยน IN1/IN2
 *       เหมาะสำหรับปรับ speed แบบ real-time โดยไม่กระทบทิศทาง
 */
void L298N_SetSpeed(L298N_Instance* motor, uint8_t speed) {
    if (motor == NULL || !motor->initialized) return;
    if (speed > 100) speed = 100;
    motor->speed = speed;
    PWM_SetDutyCycle(motor->pwm_channel, speed);
}

/**
 * @brief เปลี่ยนทิศทาง (คงความเร็วเดิม)
 *
 * @param motor pointer ไปยัง L298N_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param dir   ทิศทางใหม่ (L298N_FORWARD / L298N_BACKWARD / L298N_STOP / L298N_BRAKE)
 *
 * @note เปลี่ยนเฉพาะ IN1/IN2 ผ่าน _set_direction_pins()
 *       ไม่เปลี่ยน PWM duty cycle ดังนั้นความเร็วยังเท่าเดิม
 */
void L298N_SetDirection(L298N_Instance* motor, L298N_Direction dir) {
    if (motor == NULL || !motor->initialized) return;
    _set_direction_pins(motor, dir);
}
