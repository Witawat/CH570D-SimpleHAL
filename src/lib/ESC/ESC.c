/**
 * @file ESC.c
 * @brief Electronic Speed Controller (ESC) Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "ESC.h"

/* ========== Private Helpers ========== */

/** @brief Convert throttle percentage to pulse width */
static uint16_t _throttle_to_pulse(ESC_Instance* esc, uint8_t throttle) {
    uint32_t range;

    if (throttle > 100) throttle = 100;

    range = (uint32_t)(esc->pulse_max_us - esc->pulse_min_us);
    return (uint16_t)(esc->pulse_min_us + (range * throttle) / 100);
}

/** @brief Write PWM pulse to ESC */
static void _write_pulse(ESC_Instance* esc, uint16_t pulse_us) {
    uint32_t period = 1000000UL / ESC_PWM_FREQ;  /* period in µs */
    uint16_t duty;

    if (period == 0) return;

    /* duty = (pulse / period) * 65535 */
    duty = (uint16_t)(((uint32_t)pulse_us * 65535) / period);
    PWM_SetDutyCycleRaw(esc->channel, duty);
}

/* ========== Public Functions ========== */

void ESC_Init(ESC_Instance* esc, PWM_Channel channel, uint16_t pulse_min_us, uint16_t pulse_max_us) {
    if (esc == NULL) return;

    esc->channel      = channel;
    esc->pulse_min_us = (pulse_min_us > 0) ? pulse_min_us : ESC_PULSE_MIN_US;
    esc->pulse_max_us = (pulse_max_us > 0) ? pulse_max_us : ESC_PULSE_MAX_US;
    esc->current_pct  = 0;
    esc->current_pulse = esc->pulse_min_us;
    esc->armed        = 0;
    esc->initialized  = 1;

    /* init PWM at 50Hz */
    PWM_Init(channel, ESC_PWM_FREQ);
    PWM_Start(channel);

    /* set to 0% (safety) */
    _write_pulse(esc, esc->pulse_min_us);
}

void ESC_Arm(ESC_Instance* esc) {
    uint32_t start;
    if (esc == NULL || !esc->initialized) return;

    /* send min pulse (0% throttle) for arming */
    _write_pulse(esc, esc->pulse_min_us);

    /* wait for ESC to initialize */
    start = Get_CurrentMs();
    while ((Get_CurrentMs() - start) < ESC_ARM_TIMEOUT_MS) {
        /* just wait — ESC needs time to detect min pulse */
    }

    esc->armed = 1;
}

void ESC_SetThrottle(ESC_Instance* esc, uint8_t throttle) {
    uint16_t pulse;

    if (esc == NULL || !esc->initialized) return;
    if (!esc->armed) return;

    if (throttle > 100) throttle = 100;

    pulse = _throttle_to_pulse(esc, throttle);
    esc->current_pct   = throttle;
    esc->current_pulse = pulse;

    _write_pulse(esc, pulse);
}

void ESC_SetThrottleMicroseconds(ESC_Instance* esc, uint16_t pulse_us) {
    uint8_t throttle;

    if (esc == NULL || !esc->initialized) return;
    if (!esc->armed) return;

    if (pulse_us < esc->pulse_min_us) pulse_us = esc->pulse_min_us;
    if (pulse_us > esc->pulse_max_us) pulse_us = esc->pulse_max_us;

    /* calculate percentage for tracking */
    {
        uint32_t range = (uint32_t)(esc->pulse_max_us - esc->pulse_min_us);
        if (range > 0) {
            throttle = (uint8_t)(((uint32_t)(pulse_us - esc->pulse_min_us) * 100) / range);
        } else {
            throttle = 0;
        }
    }

    esc->current_pct   = throttle;
    esc->current_pulse = pulse_us;

    _write_pulse(esc, pulse_us);
}

void ESC_Calibrate(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return;

    /* Step 1: Send max throttle — user connects battery */
    _write_pulse(esc, esc->pulse_max_us);
    Delay_Ms(5000);  /* wait for user to connect battery & hear beep */

    /* Step 2: Send min throttle — ESC learns min */
    _write_pulse(esc, esc->pulse_min_us);
    Delay_Ms(3000);  /* wait for confirmation beep */

    /* Calibration done — ESC is at idle */
    esc->current_pct   = 0;
    esc->current_pulse = esc->pulse_min_us;
    esc->armed         = 1;  /* assume armed after calibration */
}

void ESC_Stop(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return;

    esc->current_pct   = 0;
    esc->current_pulse = esc->pulse_min_us;
    _write_pulse(esc, esc->pulse_min_us);
}

void ESC_Disarm(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return;

    ESC_Stop(esc);
    esc->armed = 0;
}

uint8_t ESC_IsArmed(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return 0;
    return esc->armed;
}
