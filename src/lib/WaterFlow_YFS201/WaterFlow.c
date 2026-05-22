/**
 * @file WaterFlow.c
 * @brief YF-S201 Water Flow Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "WaterFlow.h"

/* ========== Private Globals ========== */

static WaterFlow_Instance* g_flow_instances[WATERFLOW_MAX_INSTANCES] = {NULL};
static uint8_t g_flow_count = 0;

/* ========== Private Helpers ========== */

/* ========== ISR Handlers ========== */

/**
 * @brief ISR callback สำหรับ pulse counting
 * @note ฟังก์ชันนี้ถูกเรียกจาก EXTI interrupt
 *       ต้องใช้ __disable_irq/__enable_irq เมื่อ access จาก main loop
 */
static void WaterFlow_PulseISR(void) {
    /* หา instance จาก pin ที่เกิด interrupt */
    /* Note: ใช้ polling approach — อ่าน pin state แล้ว match กับ instance */
    uint8_t i;
    for (i = 0; i < g_flow_count; i++) {
        WaterFlow_Instance* flow = g_flow_instances[i];
        if (flow == NULL || !flow->initialized) continue;

        /* check if this pin triggered (RISING edge) */
        if (digitalRead(flow->pin) == HIGH) {
            flow->pulse_count++;
        }
    }
}

/* ========== Public Functions ========== */

WaterFlow_Status WaterFlow_Init(WaterFlow_Instance* flow, uint8_t pin, float k_factor) {

    if (flow == NULL)     return WATERFLOW_ERROR_PARAM;
    if (k_factor <= 0.0f) return WATERFLOW_ERROR_PARAM;

    /* check max instances */
    if (g_flow_count >= WATERFLOW_MAX_INSTANCES) {
        return WATERFLOW_ERROR_FULL;
    }

    flow->pin          = pin;
    flow->k_factor     = k_factor;
    flow->pulse_count  = 0;
    flow->last_pulse   = 0;
    flow->last_time_ms = 0;
    flow->initialized  = 1;

    /* setup GPIO as input */
    pinMode(pin, PIN_MODE_INPUT_PULLUP);

    /* setup EXTI interrupt on RISING edge */
    attachInterrupt(pin, WaterFlow_PulseISR, RISING);

    /* register instance */
    g_flow_instances[g_flow_count++] = flow;

    return WATERFLOW_OK;
}

uint32_t WaterFlow_GetPulseCount(WaterFlow_Instance* flow) {
    uint32_t count;

    if (flow == NULL || !flow->initialized) return 0;

    __disable_irq();
    count = flow->pulse_count;
    __enable_irq();

    return count;
}

float WaterFlow_GetFlowRate(WaterFlow_Instance* flow) {
    uint32_t current_pulses;
    uint32_t current_time_ms;
    uint32_t delta_pulses;
    uint32_t delta_ms;
    float flow_rate;

    if (flow == NULL || !flow->initialized) return 0.0f;

    current_time_ms = Get_CurrentMs();

    __disable_irq();
    current_pulses = flow->pulse_count;
    __enable_irq();

    /* first call — store baseline */
    if (flow->last_time_ms == 0) {
        flow->last_pulse   = current_pulses;
        flow->last_time_ms = current_time_ms;
        return 0.0f;
    }

    delta_pulses = current_pulses - flow->last_pulse;
    delta_ms     = current_time_ms - flow->last_time_ms;

    /* update baseline for next call */
    flow->last_pulse   = current_pulses;
    flow->last_time_ms = current_time_ms;

    if (delta_ms == 0 || delta_pulses == 0) return 0.0f;

    /* Calculate flow rate: (pulses / k_factor) / (ms / 60000) = L/min */
    flow_rate = ((float)delta_pulses / flow->k_factor) / ((float)delta_ms / 60000.0f);

    return flow_rate;
}

float WaterFlow_GetTotalVolume(WaterFlow_Instance* flow) {
    uint32_t pulses;

    if (flow == NULL || !flow->initialized) return 0.0f;

    __disable_irq();
    pulses = flow->pulse_count;
    __enable_irq();

    if (flow->k_factor <= 0.0f) return 0.0f;

    return (float)pulses / flow->k_factor;
}

void WaterFlow_Reset(WaterFlow_Instance* flow) {
    if (flow == NULL || !flow->initialized) return;

    __disable_irq();
    flow->pulse_count  = 0;
    __enable_irq();

    flow->last_pulse   = 0;
    flow->last_time_ms = 0;
}

void WaterFlow_SetKFactor(WaterFlow_Instance* flow, float k_factor) {
    if (flow == NULL || !flow->initialized) return;
    if (k_factor <= 0.0f) return;
    flow->k_factor = k_factor;
}
