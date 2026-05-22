/**
 * @file OH49E.c
 * @brief OH49E Hall Effect Sensor Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "OH49E.h"

/* ========== Private Helpers ========== */

static float _fabsf(float x) {
    return (x < 0.0f) ? -x : x;
}

/* ========== Public Functions ========== */

OH49E_Status OH49E_Init(OH49E_Instance* hall, ADC_Channel adc_channel,
                         float vcc, float vref) {
    if (hall == NULL)       return OH49E_ERROR_PARAM;
    if (vcc  <= 0.0f)       return OH49E_ERROR_PARAM;
    if (vref <= 0.0f)       return OH49E_ERROR_PARAM;

    hall->adc_channel = adc_channel;
    hall->vcc         = vcc;
    hall->vref        = vref;
    hall->threshold_v = OH49E_DEFAULT_THRESHOLD_V;
    hall->quiescent_v = vcc / 2.0f;
    hall->initialized = 1;
    return OH49E_OK;
}

uint16_t OH49E_ReadRaw(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0;
    return ADC_Read(hall->adc_channel);
}

float OH49E_ReadVoltage(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0.0f;
    uint16_t raw = ADC_Read(hall->adc_channel);
    return ADC_ToVoltage(raw, hall->vref);
}

uint8_t OH49E_IsFieldDetected(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0;
    float v = OH49E_ReadVoltage(hall);
    return (_fabsf(v - hall->quiescent_v) > hall->threshold_v) ? 1 : 0;
}

OH49E_FieldDir OH49E_GetFieldDirection(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return OH49E_FIELD_NONE;
    float v    = OH49E_ReadVoltage(hall);
    float diff = v - hall->quiescent_v;
    if (_fabsf(diff) <= hall->threshold_v) return OH49E_FIELD_NONE;
    return (diff > 0.0f) ? OH49E_FIELD_NORTH : OH49E_FIELD_SOUTH;
}

float OH49E_GetFieldStrength(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0.0f;
    float v    = OH49E_ReadVoltage(hall);
    float diff = _fabsf(v - hall->quiescent_v);
    /* normalize: max possible deviation = quiescent_v (= VCC/2) */
    float max  = hall->quiescent_v;
    if (max <= 0.0f) return 0.0f;
    float strength = diff / max;
    if (strength > 1.0f) strength = 1.0f;
    return strength;
}

void OH49E_SetThreshold(OH49E_Instance* hall, float threshold_v) {
    if (hall == NULL || !hall->initialized) return;
    if (threshold_v < 0.0f) threshold_v = 0.0f;
    hall->threshold_v = threshold_v;
}
