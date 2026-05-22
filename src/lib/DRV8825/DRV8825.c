/**
 * @file DRV8825.c
 * @brief DRV8825 Stepper Driver Library Implementation
 */

#include "DRV8825.h"

static uint32_t _drv8825_calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0U) rpm = 1U;
    if (steps_per_rev == 0U) steps_per_rev = DRV8825_NEMA17_STEPS_PER_REV;
    return 60000000UL / (rpm * steps_per_rev);
}

static void _drv8825_set_dir(DRV8825_Instance* drv, uint8_t is_cw) {
    uint8_t dir_level = is_cw ? HIGH : LOW;
    if (drv->dir_inverted) {
        dir_level = (dir_level == HIGH) ? LOW : HIGH;
    }
    digitalWrite(drv->pin_dir, dir_level);
}

static void _drv8825_pulse_step(DRV8825_Instance* drv) {
    digitalWrite(drv->pin_step, HIGH);
    Delay_Us(2);
    digitalWrite(drv->pin_step, LOW);
    Delay_Us(drv->step_delay_us);
}

void DRV8825_Init(DRV8825_Instance* drv,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint32_t steps_per_rev) {
    if (drv == NULL) return;

    drv->pin_step = step_pin;
    drv->pin_dir = dir_pin;
    drv->pin_en = en_pin;
    drv->has_en = (en_pin != 0U) ? 1U : 0U;

    drv->dir_inverted = 0U;

    drv->steps_per_rev = (steps_per_rev > 0U) ? steps_per_rev : DRV8825_NEMA17_STEPS_PER_REV;
    drv->linear_mm_per_rev = 0.0f;
    drv->speed_rpm = 60U;
    drv->step_delay_us = _drv8825_calc_step_delay_us(drv->speed_rpm, drv->steps_per_rev);

    drv->position_steps = 0;
    drv->is_enabled = 0U;
    drv->initialized = 1U;

    pinMode(step_pin, PIN_MODE_OUTPUT);
    pinMode(dir_pin, PIN_MODE_OUTPUT);
    digitalWrite(step_pin, LOW);
    digitalWrite(dir_pin, LOW);

    if (drv->has_en) {
        pinMode(en_pin, PIN_MODE_OUTPUT);
        digitalWrite(en_pin, HIGH);  // HIGH = disable on DRV8825 modules
    }
}

void DRV8825_InitNEMA17(DRV8825_Instance* drv,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin) {
    DRV8825_Init(drv, step_pin, dir_pin, en_pin, DRV8825_NEMA17_STEPS_PER_REV);
}

void DRV8825_SetSpeed(DRV8825_Instance* drv, uint32_t rpm) {
    if (drv == NULL || !drv->initialized) return;
    if (rpm < 1U) rpm = 1U;
    drv->speed_rpm = rpm;
    drv->step_delay_us = _drv8825_calc_step_delay_us(rpm, drv->steps_per_rev);
}

void DRV8825_SetDirectionInverted(DRV8825_Instance* drv, uint8_t inverted) {
    if (drv == NULL || !drv->initialized) return;
    drv->dir_inverted = inverted ? 1U : 0U;
}

void DRV8825_Enable(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, LOW);
    }
    drv->is_enabled = 1U;
}

void DRV8825_Disable(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, HIGH);
    }
    drv->is_enabled = 0U;
}

void DRV8825_Stop(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    digitalWrite(drv->pin_step, LOW);
}

void DRV8825_Move(DRV8825_Instance* drv, int32_t steps) {
    int32_t n;

    if (drv == NULL || !drv->initialized) return;
    if (steps == 0) return;

    if (steps > 0) {
        _drv8825_set_dir(drv, 1U);
        n = steps;
        while (n-- > 0) {
            _drv8825_pulse_step(drv);
            drv->position_steps++;
        }
    } else {
        _drv8825_set_dir(drv, 0U);
        n = -steps;
        while (n-- > 0) {
            _drv8825_pulse_step(drv);
            drv->position_steps--;
        }
    }
}

void DRV8825_MoveDegrees(DRV8825_Instance* drv, int32_t degrees) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)degrees * (int64_t)drv->steps_per_rev / 360LL);
    DRV8825_Move(drv, steps);
}

void DRV8825_MoveRevolutions(DRV8825_Instance* drv, int32_t revolutions) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)revolutions * (int64_t)drv->steps_per_rev);
    DRV8825_Move(drv, steps);
}

void DRV8825_SetLinearMmPerRev(DRV8825_Instance* drv, float mm_per_rev) {
    if (drv == NULL || !drv->initialized) return;
    if (mm_per_rev <= 0.0f) {
        drv->linear_mm_per_rev = 0.0f;
        return;
    }
    drv->linear_mm_per_rev = mm_per_rev;
}

int32_t DRV8825_MmToSteps(DRV8825_Instance* drv, float distance_mm) {
    float steps_f;
    if (drv == NULL || !drv->initialized) return 0;
    if (drv->linear_mm_per_rev <= 0.0f) return 0;

    steps_f = (distance_mm * (float)drv->steps_per_rev) / drv->linear_mm_per_rev;
    if (steps_f >= 0.0f) return (int32_t)(steps_f + 0.5f);
    return (int32_t)(steps_f - 0.5f);
}

float DRV8825_StepsToMm(DRV8825_Instance* drv, int32_t steps) {
    if (drv == NULL || !drv->initialized) return 0.0f;
    if (drv->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * drv->linear_mm_per_rev) / (float)drv->steps_per_rev;
}

void DRV8825_MoveMm(DRV8825_Instance* drv, float distance_mm) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = DRV8825_MmToSteps(drv, distance_mm);
    DRV8825_Move(drv, steps);
}

int32_t DRV8825_GetPosition(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return 0;
    return drv->position_steps;
}

void DRV8825_ResetPosition(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    drv->position_steps = 0;
}
