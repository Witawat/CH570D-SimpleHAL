/**
 * @file ServoTester.c
 * @brief Servo Calibration & Diagnostic Tool Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "ServoTester.h"

/* ========== Private Helpers ========== */

/** @brief Write pulse to servo via PWM */
static void _write_pulse(ServoTester_Instance* tester, uint16_t pulse_us) {
    uint32_t period = 1000000UL / SERVOTESTER_FREQ;  /* 20000µs for 50Hz */
    uint16_t duty;

    if (period == 0) return;

    /* duty = (pulse / period) * 65535 */
    duty = (uint16_t)(((uint32_t)pulse_us * 65535) / period);
    PWM_SetDutyCycleRaw(tester->channel, duty);
    tester->current_pulse = pulse_us;
}

/* ========== Public Functions ========== */

void ServoTester_Init(ServoTester_Instance* tester, PWM_Channel channel) {
    if (tester == NULL) return;

    tester->channel       = channel;
    tester->current_pulse = 1500;  /* default center */
    tester->initialized   = 1;

    /* init PWM at 50Hz */
    PWM_Init(channel, SERVOTESTER_FREQ);
    PWM_Start(channel);

    /* set to center */
    _write_pulse(tester, 1500);
}

void ServoTester_Sweep(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us, uint16_t step_us, uint16_t delay_ms) {
    uint16_t pulse;
    int16_t direction;

    if (tester == NULL || !tester->initialized) return;
    if (step_us == 0) step_us = 10;

    direction = (end_us >= start_us) ? (int16_t)step_us : -(int16_t)step_us;

    for (pulse = start_us;
         (direction > 0) ? (pulse <= end_us) : (pulse >= end_us);
         pulse = (uint16_t)((int32_t)pulse + direction)) {

        _write_pulse(tester, pulse);
        Delay_Ms(delay_ms);
    }
}

uint16_t ServoTester_FindCenter(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us) {
    if (tester == NULL || !tester->initialized) return 1500;

    /* center = midpoint */
    uint16_t center = (uint16_t)(((uint32_t)start_us + (uint32_t)end_us) / 2);

    /* move to center for visual confirmation */
    _write_pulse(tester, center);

    return center;
}

void ServoTester_FindPulseRange(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us, uint16_t* min_us, uint16_t* max_us) {
    if (tester == NULL || !tester->initialized) return;

    /* simplified approach: use start/end as min/max */
    /* user should verify visually during sweep */

    if (min_us != NULL) *min_us = start_us;
    if (max_us != NULL) *max_us = end_us;

    /* move to min, then max for visual check */
    _write_pulse(tester, start_us);
    Delay_Ms(1000);
    _write_pulse(tester, end_us);
    Delay_Ms(1000);
    _write_pulse(tester, (uint16_t)(((uint32_t)start_us + (uint32_t)end_us) / 2));
}

void ServoTester_SetPulse(ServoTester_Instance* tester, uint16_t pulse_us) {
    if (tester == NULL || !tester->initialized) return;
    _write_pulse(tester, pulse_us);
}

uint16_t ServoTester_GetCurrentPulse(ServoTester_Instance* tester) {
    if (tester == NULL || !tester->initialized) return 0;
    return tester->current_pulse;
}
