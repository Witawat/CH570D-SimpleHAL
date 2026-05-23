/**
 * @file ServoTester.c
 * @brief Servo Calibration & Diagnostic Tool Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "ServoTester.h"

/* ========== Private Helpers ========== */

/**
 * @brief Write pulse to servo via PWM
 * @param tester  Pointer to ServoTester_Instance
 * @param pulse_us Pulse width in microseconds to output
 * @return None
 * @note Computes the PWM duty value by mapping pulse_us to a 16-bit range
 *       (0–65535) over a 20000µs period (50Hz). Updates tester->current_pulse.
 *       Period is calculated as 1000000 / SERVOTESTER_FREQ.
 */
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

/**
 * @brief Initialize the servo tester
 * @param tester  Pointer to ServoTester_Instance
 * @param channel PWM channel to use for output
 * @return None
 * @note Initializes PWM at SERVOTESTER_FREQ (50Hz), starts output immediately,
 *       and sets the servo to center position (1500µs). The tester is ready
 *       for sweep or calibration operations after this call.
 */
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

/**
 * @brief Sweep servo between two pulse widths
 * @param tester   Pointer to ServoTester_Instance
 * @param start_us Starting pulse width in microseconds
 * @param end_us   Ending pulse width in microseconds
 * @param step_us  Step size in microseconds (defaults to 10 if 0)
 * @param delay_ms Delay in milliseconds at each step
 * @return None
 * @note Sweeps from start_us to end_us in steps of step_us, pausing delay_ms
 *       at each step. Automatically handles forward and reverse direction.
 *       Blocks until the sweep is complete.
 */
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

/**
 * @brief Find and go to the center pulse between start and end
 * @param tester   Pointer to ServoTester_Instance
 * @param start_us Lower bound in microseconds
 * @param end_us   Upper bound in microseconds
 * @return Center pulse width in microseconds
 * @note Computes (start_us + end_us) / 2 and moves the servo to that position
 *       for visual confirmation. Returns 1500µs as fallback if tester invalid.
 */
uint16_t ServoTester_FindCenter(ServoTester_Instance* tester, uint16_t start_us, uint16_t end_us) {
    if (tester == NULL || !tester->initialized) return 1500;

    /* center = midpoint */
    uint16_t center = (uint16_t)(((uint32_t)start_us + (uint32_t)end_us) / 2);

    /* move to center for visual confirmation */
    _write_pulse(tester, center);

    return center;
}

/**
 * @brief Identify usable pulse range by visual inspection
 * @param tester  Pointer to ServoTester_Instance
 * @param start_us Candidate minimum pulse in microseconds
 * @param end_us   Candidate maximum pulse in microseconds
 * @param min_us   Output pointer for minimum pulse (can be NULL)
 * @param max_us   Output pointer for maximum pulse (can be NULL)
 * @return None
 * @note Writes the candidate range to the output pointers, then moves the
 *       servo to min, max, and center positions sequentially with 1s delays
 *       for visual verification. The caller should observe the servo and
 *       adjust if needed.
 */
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

/**
 * @brief Set servo to a specific pulse width immediately
 * @param tester   Pointer to ServoTester_Instance
 * @param pulse_us Target pulse width in microseconds
 * @return None
 * @note Directly writes the given pulse to the PWM output without any
 *       sweeping or ramping. Updates tester->current_pulse.
 */
void ServoTester_SetPulse(ServoTester_Instance* tester, uint16_t pulse_us) {
    if (tester == NULL || !tester->initialized) return;
    _write_pulse(tester, pulse_us);
}

/**
 * @brief Get the current pulse width
 * @param tester Pointer to ServoTester_Instance
 * @return Current pulse width in microseconds, or 0 if tester is invalid
 * @note Returns the last written pulse value from the instance state.
 */
uint16_t ServoTester_GetCurrentPulse(ServoTester_Instance* tester) {
    if (tester == NULL || !tester->initialized) return 0;
    return tester->current_pulse;
}
