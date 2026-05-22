/**
 * @file ServoCluster.c
 * @brief Multi-Servo Controller with Easing Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "ServoCluster.h"
#include <math.h>  /* for sinf() in sine easing */

/* ========== Private Helpers ========== */

/** @brief PI constant for sine easing */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
 * @brief Ease calculation based on curve type
 * @param t normalized time (0.0 to 1.0)
 * @param easing curve type
 * @return eased value (0.0 to 1.0)
 */
static float _ease_calc(float t, ServoCluster_Easing easing) {
    switch (easing) {
        case EASE_LINEAR:
            return t;

        case EASE_QUAD_IN:
            return t * t;

        case EASE_QUAD_OUT:
            return t * (2.0f - t);

        case EASE_QUAD_IN_OUT:
            if (t < 0.5f)
                return 2.0f * t * t;
            else
                return -1.0f + (4.0f - 2.0f * t) * t;

        case EASE_CUBIC_IN:
            return t * t * t;

        case EASE_CUBIC_OUT: {
            float t1 = t - 1.0f;
            return t1 * t1 * t1 + 1.0f;
        }

        case EASE_CUBIC_IN_OUT:
            if (t < 0.5f)
                return 4.0f * t * t * t;
            else {
                float t1 = t - 1.0f;
                return 0.5f * t1 * t1 * t1 + 1.0f;
            }

        case EASE_SINE_IN:
            return 1.0f - cosf(t * M_PI / 2.0f);

        case EASE_SINE_OUT:
            return sinf(t * M_PI / 2.0f);

        case EASE_SINE_IN_OUT:
            return 0.5f * (1.0f - cosf(M_PI * t));

        default:
            return t;
    }
}

/**
 * @brief Convert angle to pulse width for a servo
 */
static uint16_t _angle_to_pulse(ServoCluster_Servo* s, uint8_t angle) {
    if (angle > 180) angle = 180;
    /* map 0-180 to pulse_min_us - pulse_max_us */
    uint32_t range = (uint32_t)(s->pulse_max_us - s->pulse_min_us);
    return (uint16_t)(s->pulse_min_us + (range * angle) / 180);
}

/**
 * @brief Write pulse to servo via the correct backend
 */
static void _write_servo(ServoCluster_Instance* cluster, ServoCluster_Servo* s, uint16_t pulse_us) {
    if (cluster->backend == SERVO_BACKEND_PCA9685) {
        PCA9685_SetPulse(&cluster->pca9685, s->channel, pulse_us);
    } else {
        /* HW backend: write PWM duty cycle directly */
        uint32_t period = 20000;  /* 50Hz */
        uint16_t duty   = (uint16_t)(((uint32_t)pulse_us * 65535) / period);
        PWM_SetDutyCycleRaw((PWM_Channel)s->channel, duty);
    }
}

/* ========== Public Functions ========== */

void ServoCluster_Init(ServoCluster_Instance* cluster, ServoCluster_Backend backend, uint8_t max_servos) {
    uint8_t i;

    if (cluster == NULL) return;
    if (max_servos > SERVOCLUSTER_MAX_SERVOS) max_servos = SERVOCLUSTER_MAX_SERVOS;

    cluster->backend    = backend;
    cluster->max_servos = max_servos;
    cluster->speed_pct  = 100;

    /* init all servos as inactive */
    for (i = 0; i < SERVOCLUSTER_MAX_SERVOS; i++) {
        cluster->servos[i].active = 0;
        cluster->servos[i].moving = 0;
    }

    /* init PCA9685 if using that backend */
    if (backend == SERVO_BACKEND_PCA9685) {
        PCA9685_Init(&cluster->pca9685, PCA9685_ADDR_DEFAULT, 50);
    }

    /* init hardware PWM at 50Hz if using HW backend */
    if (backend == SERVO_BACKEND_HW) {
        /* PWM is initialized per-channel by Servo lib, so just set up TIM1/TIM2 */
    }

    cluster->initialized = 1;
}

void ServoCluster_AddServo(ServoCluster_Instance* cluster, uint8_t channel, uint16_t pulse_min_us, uint16_t pulse_max_us) {
    uint8_t i;

    if (cluster == NULL || !cluster->initialized) return;

    for (i = 0; i < cluster->max_servos; i++) {
        if (!cluster->servos[i].active) {
            cluster->servos[i].channel      = channel;
            cluster->servos[i].pulse_min_us = pulse_min_us;
            cluster->servos[i].pulse_max_us = pulse_max_us;
            cluster->servos[i].current_angle = 90;
            cluster->servos[i].target_angle  = 90;
            cluster->servos[i].start_angle   = 90;
            cluster->servos[i].easing        = EASE_LINEAR;
            cluster->servos[i].moving        = 0;
            cluster->servos[i].active        = 1;

            /* init servo at center position */
            if (cluster->backend == SERVO_BACKEND_HW) {
                PWM_Init((PWM_Channel)channel, 50);
                PWM_Start((PWM_Channel)channel);
            }
            _write_servo(cluster, &cluster->servos[i], _angle_to_pulse(&cluster->servos[i], 90));
            return;
        }
    }
}

void ServoCluster_MoveTo(ServoCluster_Instance* cluster, uint8_t servo_id, uint8_t angle, uint16_t duration_ms, ServoCluster_Easing easing) {
    if (cluster == NULL || !cluster->initialized) return;
    if (servo_id >= cluster->max_servos) return;
    if (!cluster->servos[servo_id].active) return;
    if (angle > 180) angle = 180;

    /* apply speed modifier to duration */
    if (cluster->speed_pct != 100 && cluster->speed_pct > 0) {
        duration_ms = (uint16_t)((uint32_t)duration_ms * 100 / cluster->speed_pct);
    }

    cluster->servos[servo_id].start_angle  = cluster->servos[servo_id].current_angle;
    cluster->servos[servo_id].target_angle = angle;
    cluster->servos[servo_id].duration_ms  = duration_ms;
    cluster->servos[servo_id].easing       = easing;
    cluster->servos[servo_id].start_time_ms = Get_CurrentMs();
    cluster->servos[servo_id].moving       = 1;
}

void ServoCluster_MoveAll(ServoCluster_Instance* cluster, uint8_t angle, uint16_t duration_ms, ServoCluster_Easing easing) {
    uint8_t i;
    if (cluster == NULL || !cluster->initialized) return;

    for (i = 0; i < cluster->max_servos; i++) {
        if (cluster->servos[i].active) {
            ServoCluster_MoveTo(cluster, i, angle, duration_ms, easing);
        }
    }
}

void ServoCluster_SetEasing(ServoCluster_Instance* cluster, uint8_t servo_id, ServoCluster_Easing easing) {
    if (cluster == NULL || !cluster->initialized) return;
    if (servo_id >= cluster->max_servos) return;
    if (!cluster->servos[servo_id].active) return;
    cluster->servos[servo_id].easing = easing;
}

void ServoCluster_SetSpeed(ServoCluster_Instance* cluster, uint8_t speed_pct) {
    if (cluster == NULL || !cluster->initialized) return;
    if (speed_pct < 1) speed_pct = 1;
    cluster->speed_pct = speed_pct;
}

void ServoCluster_Update(ServoCluster_Instance* cluster) {
    uint8_t i;
    uint32_t now;
    uint32_t elapsed;
    float t;       /* normalized time 0.0-1.0 */
    float eased;
    uint8_t new_angle;
    uint16_t pulse;

    if (cluster == NULL || !cluster->initialized) return;

    now = Get_CurrentMs();

    for (i = 0; i < cluster->max_servos; i++) {
        if (!cluster->servos[i].active) continue;
        if (!cluster->servos[i].moving) continue;

        elapsed = now - cluster->servos[i].start_time_ms;

        if (elapsed >= cluster->servos[i].duration_ms) {
            /* movement complete */
            cluster->servos[i].current_angle = cluster->servos[i].target_angle;
            cluster->servos[i].moving = 0;
            pulse = _angle_to_pulse(&cluster->servos[i], cluster->servos[i].current_angle);
            _write_servo(cluster, &cluster->servos[i], pulse);
        } else {
            /* calculate eased angle */
            t = (float)elapsed / (float)cluster->servos[i].duration_ms;
            if (t > 1.0f) t = 1.0f;

            eased = _ease_calc(t, cluster->servos[i].easing);

            float angle_f = (float)cluster->servos[i].start_angle
                          + (float)((int)cluster->servos[i].target_angle - (int)cluster->servos[i].start_angle) * eased;

            new_angle = (uint8_t)(angle_f + 0.5f);  /* round */
            if (new_angle > 180) new_angle = 180;

            cluster->servos[i].current_angle = new_angle;
            pulse = _angle_to_pulse(&cluster->servos[i], new_angle);
            _write_servo(cluster, &cluster->servos[i], pulse);
        }
    }
}

uint8_t ServoCluster_IsMoving(ServoCluster_Instance* cluster, uint8_t servo_id) {
    if (cluster == NULL || !cluster->initialized) return 0;
    if (servo_id >= cluster->max_servos) return 0;
    if (!cluster->servos[servo_id].active) return 0;
    return cluster->servos[servo_id].moving;
}

uint8_t ServoCluster_IsAllDone(ServoCluster_Instance* cluster) {
    uint8_t i;
    if (cluster == NULL || !cluster->initialized) return 1;

    for (i = 0; i < cluster->max_servos; i++) {
        if (cluster->servos[i].active && cluster->servos[i].moving) {
            return 0;
        }
    }
    return 1;
}

void ServoCluster_Stop(ServoCluster_Instance* cluster, uint8_t servo_id) {
    if (cluster == NULL || !cluster->initialized) return;
    if (servo_id >= cluster->max_servos) return;
    cluster->servos[servo_id].moving = 0;
}

void ServoCluster_StopAll(ServoCluster_Instance* cluster) {
    uint8_t i;
    if (cluster == NULL || !cluster->initialized) return;

    for (i = 0; i < cluster->max_servos; i++) {
        cluster->servos[i].moving = 0;
    }
}
