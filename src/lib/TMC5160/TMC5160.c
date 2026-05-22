/**
 * @file TMC5160.c
 * @brief TMC5160 Stepper Driver Library Implementation
 */

#include "TMC5160.h"

#define TMC5160_WRITE_BIT 0x80U

static uint32_t _tmc5160_calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0U) rpm = 1U;
    if (steps_per_rev == 0U) steps_per_rev = TMC5160_NEMA17_STEPS_PER_REV;
    return 60000000UL / (rpm * steps_per_rev);
}

static void _tmc5160_set_dir(TMC5160_Instance* drv, uint8_t is_cw) {
    uint8_t dir_level = is_cw ? HIGH : LOW;
    if (drv->dir_inverted) {
        dir_level = (dir_level == HIGH) ? LOW : HIGH;
    }
    digitalWrite(drv->pin_dir, dir_level);
}

static void _tmc5160_pulse_step(TMC5160_Instance* drv) {
    digitalWrite(drv->pin_step, HIGH);
    Delay_Us(2);
    digitalWrite(drv->pin_step, LOW);
    Delay_Us(drv->step_delay_us);
}

static void _tmc5160_cs_low(TMC5160_Instance* drv) {
    digitalWrite(drv->pin_cs, LOW);
}

static void _tmc5160_cs_high(TMC5160_Instance* drv) {
    digitalWrite(drv->pin_cs, HIGH);
}

void TMC5160_Init(TMC5160_Instance* drv,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint8_t cs_pin,
                  uint32_t steps_per_rev) {
    if (drv == NULL) return;

    drv->pin_step = step_pin;
    drv->pin_dir = dir_pin;
    drv->pin_en = en_pin;
    drv->pin_cs = cs_pin;
    drv->has_en = (en_pin != 0U) ? 1U : 0U;

    drv->dir_inverted = 0U;

    drv->steps_per_rev = (steps_per_rev > 0U) ? steps_per_rev : TMC5160_NEMA17_STEPS_PER_REV;
    drv->linear_mm_per_rev = 0.0f;
    drv->speed_rpm = 60U;
    drv->step_delay_us = _tmc5160_calc_step_delay_us(drv->speed_rpm, drv->steps_per_rev);

    drv->position_steps = 0;
    drv->is_enabled = 0U;
    drv->spi_ready = 0U;
    drv->initialized = 1U;

    pinMode(step_pin, PIN_MODE_OUTPUT);
    pinMode(dir_pin, PIN_MODE_OUTPUT);
    pinMode(cs_pin, PIN_MODE_OUTPUT);

    digitalWrite(step_pin, LOW);
    digitalWrite(dir_pin, LOW);
    _tmc5160_cs_high(drv);

    if (drv->has_en) {
        pinMode(en_pin, PIN_MODE_OUTPUT);
        digitalWrite(en_pin, HIGH);  // HIGH = disable
    }
}

void TMC5160_InitNEMA17(TMC5160_Instance* drv,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin,
                        uint8_t cs_pin) {
    TMC5160_Init(drv, step_pin, dir_pin, en_pin, cs_pin, TMC5160_NEMA17_STEPS_PER_REV);
}

void TMC5160_SPIBegin(TMC5160_Instance* drv, SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg) {
    if (drv == NULL || !drv->initialized) return;
    SPI_SimpleInit(mode, speed, pin_cfg);
    drv->spi_ready = 1U;
}

void TMC5160_SetSpeed(TMC5160_Instance* drv, uint32_t rpm) {
    if (drv == NULL || !drv->initialized) return;
    if (rpm < 1U) rpm = 1U;
    drv->speed_rpm = rpm;
    drv->step_delay_us = _tmc5160_calc_step_delay_us(rpm, drv->steps_per_rev);
}

void TMC5160_SetDirectionInverted(TMC5160_Instance* drv, uint8_t inverted) {
    if (drv == NULL || !drv->initialized) return;
    drv->dir_inverted = inverted ? 1U : 0U;
}

void TMC5160_Enable(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, LOW);
    }
    drv->is_enabled = 1U;
}

void TMC5160_Disable(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, HIGH);
    }
    drv->is_enabled = 0U;
}

void TMC5160_Stop(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    digitalWrite(drv->pin_step, LOW);
}

void TMC5160_Move(TMC5160_Instance* drv, int32_t steps) {
    int32_t n;

    if (drv == NULL || !drv->initialized) return;
    if (steps == 0) return;

    if (steps > 0) {
        _tmc5160_set_dir(drv, 1U);
        n = steps;
        while (n-- > 0) {
            _tmc5160_pulse_step(drv);
            drv->position_steps++;
        }
    } else {
        _tmc5160_set_dir(drv, 0U);
        n = -steps;
        while (n-- > 0) {
            _tmc5160_pulse_step(drv);
            drv->position_steps--;
        }
    }
}

void TMC5160_MoveDegrees(TMC5160_Instance* drv, int32_t degrees) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)degrees * (int64_t)drv->steps_per_rev / 360LL);
    TMC5160_Move(drv, steps);
}

void TMC5160_MoveRevolutions(TMC5160_Instance* drv, int32_t revolutions) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)revolutions * (int64_t)drv->steps_per_rev);
    TMC5160_Move(drv, steps);
}

void TMC5160_SetLinearMmPerRev(TMC5160_Instance* drv, float mm_per_rev) {
    if (drv == NULL || !drv->initialized) return;
    if (mm_per_rev <= 0.0f) {
        drv->linear_mm_per_rev = 0.0f;
        return;
    }
    drv->linear_mm_per_rev = mm_per_rev;
}

int32_t TMC5160_MmToSteps(TMC5160_Instance* drv, float distance_mm) {
    float steps_f;
    if (drv == NULL || !drv->initialized) return 0;
    if (drv->linear_mm_per_rev <= 0.0f) return 0;

    steps_f = (distance_mm * (float)drv->steps_per_rev) / drv->linear_mm_per_rev;
    if (steps_f >= 0.0f) return (int32_t)(steps_f + 0.5f);
    return (int32_t)(steps_f - 0.5f);
}

float TMC5160_StepsToMm(TMC5160_Instance* drv, int32_t steps) {
    if (drv == NULL || !drv->initialized) return 0.0f;
    if (drv->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * drv->linear_mm_per_rev) / (float)drv->steps_per_rev;
}

void TMC5160_MoveMm(TMC5160_Instance* drv, float distance_mm) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = TMC5160_MmToSteps(drv, distance_mm);
    TMC5160_Move(drv, steps);
}

int32_t TMC5160_GetPosition(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return 0;
    return drv->position_steps;
}

void TMC5160_ResetPosition(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    drv->position_steps = 0;
}

void TMC5160_WriteReg(TMC5160_Instance* drv, uint8_t reg_addr, uint32_t value) {
    if (drv == NULL || !drv->initialized) return;
    if (!drv->spi_ready) return;

    _tmc5160_cs_low(drv);
    SPI_Transfer((uint8_t)(reg_addr | TMC5160_WRITE_BIT));
    SPI_Transfer((uint8_t)((value >> 24) & 0xFFU));
    SPI_Transfer((uint8_t)((value >> 16) & 0xFFU));
    SPI_Transfer((uint8_t)((value >> 8) & 0xFFU));
    SPI_Transfer((uint8_t)(value & 0xFFU));
    _tmc5160_cs_high(drv);
}

uint32_t TMC5160_ReadReg(TMC5160_Instance* drv, uint8_t reg_addr) {
    uint32_t value;

    if (drv == NULL || !drv->initialized) return 0U;
    if (!drv->spi_ready) return 0U;

    // Dummy read phase (pipeline)
    _tmc5160_cs_low(drv);
    SPI_Transfer((uint8_t)(reg_addr & 0x7FU));
    SPI_Transfer(0x00U);
    SPI_Transfer(0x00U);
    SPI_Transfer(0x00U);
    SPI_Transfer(0x00U);
    _tmc5160_cs_high(drv);

    // Actual read phase
    _tmc5160_cs_low(drv);
    SPI_Transfer((uint8_t)(reg_addr & 0x7FU));
    value  = ((uint32_t)SPI_Transfer(0x00U) << 24);
    value |= ((uint32_t)SPI_Transfer(0x00U) << 16);
    value |= ((uint32_t)SPI_Transfer(0x00U) << 8);
    value |= ((uint32_t)SPI_Transfer(0x00U));
    _tmc5160_cs_high(drv);

    return value;
}
