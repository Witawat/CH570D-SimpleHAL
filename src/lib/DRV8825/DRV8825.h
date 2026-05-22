/**
 * @file DRV8825.h
 * @brief DRV8825 Stepper Driver Library for CH57x
 */

#ifndef __DRV8825_H
#define __DRV8825_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

#ifndef DRV8825_NEMA17_STEPS_PER_REV
#define DRV8825_NEMA17_STEPS_PER_REV 200U
#endif

typedef struct {
    uint8_t pin_step;
    uint8_t pin_dir;
    uint8_t pin_en;
    uint8_t has_en;

    uint8_t dir_inverted;

    uint32_t steps_per_rev;
    float linear_mm_per_rev;
    uint32_t speed_rpm;
    uint32_t step_delay_us;

    int32_t position_steps;
    uint8_t is_enabled;
    uint8_t initialized;
} DRV8825_Instance;

void DRV8825_Init(DRV8825_Instance* drv,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint32_t steps_per_rev);

void DRV8825_InitNEMA17(DRV8825_Instance* drv,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin);

void DRV8825_SetSpeed(DRV8825_Instance* drv, uint32_t rpm);
void DRV8825_SetDirectionInverted(DRV8825_Instance* drv, uint8_t inverted);

void DRV8825_Enable(DRV8825_Instance* drv);
void DRV8825_Disable(DRV8825_Instance* drv);
void DRV8825_Stop(DRV8825_Instance* drv);

void DRV8825_Move(DRV8825_Instance* drv, int32_t steps);
void DRV8825_MoveDegrees(DRV8825_Instance* drv, int32_t degrees);
void DRV8825_MoveRevolutions(DRV8825_Instance* drv, int32_t revolutions);

void DRV8825_SetLinearMmPerRev(DRV8825_Instance* drv, float mm_per_rev);
int32_t DRV8825_MmToSteps(DRV8825_Instance* drv, float distance_mm);
float DRV8825_StepsToMm(DRV8825_Instance* drv, int32_t steps);
void DRV8825_MoveMm(DRV8825_Instance* drv, float distance_mm);

int32_t DRV8825_GetPosition(DRV8825_Instance* drv);
void DRV8825_ResetPosition(DRV8825_Instance* drv);

#ifdef __cplusplus
}
#endif

#endif
