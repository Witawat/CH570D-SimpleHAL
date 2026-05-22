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

void L298N_Run(L298N_Instance* motor, L298N_Direction dir, uint8_t speed) {
    if (motor == NULL || !motor->initialized) return;
    if (speed > 100) speed = 100;

    motor->speed = speed;
    _set_direction_pins(motor, dir);
    PWM_SetDutyCycle(motor->pwm_channel, speed);
}

void L298N_Stop(L298N_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    _set_direction_pins(motor, L298N_STOP);
    PWM_SetDutyCycle(motor->pwm_channel, 0);
    motor->speed = 0;
}

void L298N_Brake(L298N_Instance* motor) {
    if (motor == NULL || !motor->initialized) return;
    _set_direction_pins(motor, L298N_BRAKE);
    PWM_SetDutyCycle(motor->pwm_channel, 100);
    motor->speed = 0;
}

void L298N_SetSpeed(L298N_Instance* motor, uint8_t speed) {
    if (motor == NULL || !motor->initialized) return;
    if (speed > 100) speed = 100;
    motor->speed = speed;
    PWM_SetDutyCycle(motor->pwm_channel, speed);
}

void L298N_SetDirection(L298N_Instance* motor, L298N_Direction dir) {
    if (motor == NULL || !motor->initialized) return;
    _set_direction_pins(motor, dir);
}
