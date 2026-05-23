/**
 * @file ESC.c
 * @brief Electronic Speed Controller (ESC) Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "ESC.h"

/* ========== Private Helpers ========== */

/**
 * @brief Convert throttle percentage to pulse width
 * @param esc      Pointer to ESC_Instance
 * @param throttle Throttle percentage (0–100). Clamped to 100.
 * @return Pulse width in microseconds mapped linearly over esc's pulse range
 * @note Formula: pulse = pulse_min + (range * throttle) / 100. Range is
 *       (pulse_max_us - pulse_min_us).
 */
static uint16_t _throttle_to_pulse(ESC_Instance* esc, uint8_t throttle) {
    uint32_t range;

    if (throttle > 100) throttle = 100;

    range = (uint32_t)(esc->pulse_max_us - esc->pulse_min_us);
    return (uint16_t)(esc->pulse_min_us + (range * throttle) / 100);
}

/**
 * @brief Write PWM pulse to ESC
 * @param esc      Pointer to ESC_Instance
 * @param pulse_us Pulse width in microseconds to output
 * @return None
 * @note Computes duty cycle value by mapping pulse_us to a 16-bit range
 *       (0–65535) over a (1000000 / ESC_PWM_FREQ) µs period. Assumes 50Hz
 *       PWM which gives a 20000µs period.
 */
static void _write_pulse(ESC_Instance* esc, uint16_t pulse_us) {
    uint32_t period = 1000000UL / ESC_PWM_FREQ;  /* period in µs */
    uint16_t duty;

    if (period == 0) return;

    /* duty = (pulse / period) * 65535 */
    duty = (uint16_t)(((uint32_t)pulse_us * 65535) / period);
    PWM_SetDutyCycleRaw(esc->channel, duty);
}

/* ========== Public Functions ========== */

/**
 * @brief Initialize ESC instance and start PWM output
 * @param esc          Pointer to ESC_Instance
 * @param channel      PWM channel for this ESC
 * @param pulse_min_us Minimum pulse width in microseconds (0 to use default ESC_PULSE_MIN_US)
 * @param pulse_max_us Maximum pulse width in microseconds (0 to use default ESC_PULSE_MAX_US)
 * @return None
 * @note Initializes PWM at 50Hz and immediately starts output. Sets the
 *       initial pulse to min (0% throttle) as a safety measure. The ESC is
 *       NOT armed after init — ESC_Arm() must be called before throttle
 *       commands are accepted.
 */
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

/**
 * @brief Arm the ESC by sending the minimum pulse for a timeout period
 * @param esc Pointer to ESC_Instance
 * @return None
 * @note Sends pulse_min_us (0% throttle) for ESC_ARM_TIMEOUT_MS milliseconds.
 *       Most ESCs require a steady min-pulse signal after power-on to arm.
 *       Blocks the calling thread for the duration of the timeout. After
 *       arming, ESC_SetThrottle and related calls are accepted.
 */
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

/**
 * @brief Set ESC throttle as a percentage
 * @param esc      Pointer to ESC_Instance
 * @param throttle Throttle percentage (0–100). Clamped to 100.
 * @return None
 * @note Converts percentage to pulse and writes to PWM. Only accepted if
 *       the ESC is armed. Updates internal throttle and pulse tracking.
 *       No-op if esc is NULL, uninitialized, or not armed.
 */
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

/**
 * @brief Set ESC throttle by direct pulse width
 * @param esc      Pointer to ESC_Instance
 * @param pulse_us Pulse width in microseconds (clamped to min–max range)
 * @return None
 * @note Converts the pulse back to an approximate percentage for tracking.
 *       Only accepted if the ESC is armed. Allows fine-grained control
 *       beyond standard 0–100% mapping.
 */
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

/**
 * @brief Calibrate the ESC throttle range
 * @param esc Pointer to ESC_Instance
 * @return None
 * @note Sends max pulse (5s), then min pulse (3s) to teach the ESC the full
 *       range. Typical procedure: user disconnects battery, calls this function,
 *       then connects battery when prompted by max pulse. Blocks for ~8 seconds.
 *       Sets the ESC as armed after calibration.
 */
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

/**
 * @brief Stop the ESC by setting minimum pulse
 * @param esc Pointer to ESC_Instance
 * @return None
 * @note Sets throttle to 0% by writing pulse_min_us. Does NOT disarm the
 *       ESC — throttle commands will still be accepted after this call.
 */
void ESC_Stop(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return;

    esc->current_pct   = 0;
    esc->current_pulse = esc->pulse_min_us;
    _write_pulse(esc, esc->pulse_min_us);
}

/**
 * @brief Disarm the ESC (stops motor and disarms)
 * @param esc Pointer to ESC_Instance
 * @return None
 * @note Calls ESC_Stop to set min pulse, then clears the armed flag.
 *       Subsequent ESC_SetThrottle calls will be ignored until ESC_Arm
 *       is called again.
 */
void ESC_Disarm(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return;

    ESC_Stop(esc);
    esc->armed = 0;
}

/**
 * @brief Check if the ESC is armed
 * @param esc Pointer to ESC_Instance
 * @return 1 if armed, 0 otherwise
 * @note Returns 0 if esc is NULL or uninitialized.
 */
uint8_t ESC_IsArmed(ESC_Instance* esc) {
    if (esc == NULL || !esc->initialized) return 0;
    return esc->armed;
}
