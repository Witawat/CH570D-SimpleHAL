/**
 * @file TMC220x.h
 * @brief TMC2208/TMC2209 Stepper Driver Library for CH57x
 */

#ifndef __TMC220X_H
#define __TMC220X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

#ifndef TMC220X_NEMA17_STEPS_PER_REV
#define TMC220X_NEMA17_STEPS_PER_REV 200U
#endif

typedef enum {
    TMC220X_MODEL_2208 = 0,
    TMC220X_MODEL_2209 = 1
} TMC220x_Model;

typedef struct {
    TMC220x_Model model;

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
} TMC220x_Instance;

void TMC220x_Init(TMC220x_Instance* drv,
                  TMC220x_Model model,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint32_t steps_per_rev);

void TMC220x_InitNEMA17(TMC220x_Instance* drv,
                        TMC220x_Model model,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin);

void TMC220x_SetSpeed(TMC220x_Instance* drv, uint32_t rpm);
void TMC220x_SetDirectionInverted(TMC220x_Instance* drv, uint8_t inverted);

void TMC220x_Enable(TMC220x_Instance* drv);
void TMC220x_Disable(TMC220x_Instance* drv);
void TMC220x_Stop(TMC220x_Instance* drv);

void TMC220x_Move(TMC220x_Instance* drv, int32_t steps);
void TMC220x_MoveDegrees(TMC220x_Instance* drv, int32_t degrees);
void TMC220x_MoveRevolutions(TMC220x_Instance* drv, int32_t revolutions);

void TMC220x_SetLinearMmPerRev(TMC220x_Instance* drv, float mm_per_rev);
int32_t TMC220x_MmToSteps(TMC220x_Instance* drv, float distance_mm);
float TMC220x_StepsToMm(TMC220x_Instance* drv, int32_t steps);
void TMC220x_MoveMm(TMC220x_Instance* drv, float distance_mm);

int32_t TMC220x_GetPosition(TMC220x_Instance* drv);
void TMC220x_ResetPosition(TMC220x_Instance* drv);

#ifdef __cplusplus
}
#endif

#endif
