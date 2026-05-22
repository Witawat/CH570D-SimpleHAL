/**
 * @file TMC5160.h
 * @brief TMC5160 Stepper Driver Library for CH57x
 */

#ifndef __TMC5160_H
#define __TMC5160_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "../../SimpleHAL/SimpleSPI.h"
#include <stdint.h>

#ifndef TMC5160_NEMA17_STEPS_PER_REV
#define TMC5160_NEMA17_STEPS_PER_REV 200U
#endif

typedef struct {
    uint8_t pin_step;
    uint8_t pin_dir;
    uint8_t pin_en;
    uint8_t pin_cs;
    uint8_t has_en;

    uint8_t dir_inverted;

    uint32_t steps_per_rev;
    float linear_mm_per_rev;
    uint32_t speed_rpm;
    uint32_t step_delay_us;

    int32_t position_steps;
    uint8_t is_enabled;
    uint8_t spi_ready;
    uint8_t initialized;
} TMC5160_Instance;

void TMC5160_Init(TMC5160_Instance* drv,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint8_t cs_pin,
                  uint32_t steps_per_rev);

void TMC5160_InitNEMA17(TMC5160_Instance* drv,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin,
                        uint8_t cs_pin);

void TMC5160_SPIBegin(TMC5160_Instance* drv, SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg);

void TMC5160_SetSpeed(TMC5160_Instance* drv, uint32_t rpm);
void TMC5160_SetDirectionInverted(TMC5160_Instance* drv, uint8_t inverted);

void TMC5160_Enable(TMC5160_Instance* drv);
void TMC5160_Disable(TMC5160_Instance* drv);
void TMC5160_Stop(TMC5160_Instance* drv);

void TMC5160_Move(TMC5160_Instance* drv, int32_t steps);
void TMC5160_MoveDegrees(TMC5160_Instance* drv, int32_t degrees);
void TMC5160_MoveRevolutions(TMC5160_Instance* drv, int32_t revolutions);

void TMC5160_SetLinearMmPerRev(TMC5160_Instance* drv, float mm_per_rev);
int32_t TMC5160_MmToSteps(TMC5160_Instance* drv, float distance_mm);
float TMC5160_StepsToMm(TMC5160_Instance* drv, int32_t steps);
void TMC5160_MoveMm(TMC5160_Instance* drv, float distance_mm);

int32_t TMC5160_GetPosition(TMC5160_Instance* drv);
void TMC5160_ResetPosition(TMC5160_Instance* drv);

void TMC5160_WriteReg(TMC5160_Instance* drv, uint8_t reg_addr, uint32_t value);
uint32_t TMC5160_ReadReg(TMC5160_Instance* drv, uint8_t reg_addr);

#ifdef __cplusplus
}
#endif

#endif
