/**
 * @file RainSensor.c
 * @brief YL-83 Rain Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "RainSensor.h"

/* ========== Public Functions ========== */

RainSensor_Status RainSensor_Init(RainSensor_Instance* rain, ADC_Channel adc_channel) {
    if (rain == NULL) return RAINSENSOR_ERROR_PARAM;

    rain->adc_channel = adc_channel;
    rain->threshold   = RAINSENSOR_DEFAULT_THRESHOLD;
    rain->initialized = 1;
    return RAINSENSOR_OK;
}

uint16_t RainSensor_ReadRaw(RainSensor_Instance* rain) {
    if (rain == NULL || !rain->initialized) return 0;
    return ADC_Read(rain->adc_channel);
}

float RainSensor_ReadLevel(RainSensor_Instance* rain) {
    if (rain == NULL || !rain->initialized) return 0.0f;

    uint16_t raw = ADC_Read(rain->adc_channel);
    float level = (float)raw / 4095.0f;

    if (level < 0.0f) return 0.0f;
    if (level > 1.0f) return 1.0f;
    return level;
}

uint8_t RainSensor_IsRaining(RainSensor_Instance* rain, float threshold) {
    if (rain == NULL || !rain->initialized) return 0;

    float level = RainSensor_ReadLevel(rain);
    return (level >= threshold) ? 1 : 0;
}

RainSensor_Intensity RainSensor_GetIntensity(RainSensor_Instance* rain) {
    if (rain == NULL || !rain->initialized) return RAIN_NONE;

    float level = RainSensor_ReadLevel(rain);

    if (level < 0.01f)       return RAIN_NONE;
    if (level < 0.34f)       return RAIN_LIGHT;
    if (level < 0.67f)       return RAIN_MODERATE;
    return RAIN_HEAVY;
}

void RainSensor_SetThreshold(RainSensor_Instance* rain, float threshold) {
    if (rain == NULL || !rain->initialized) return;
    if (threshold < 0.0f) threshold = 0.0f;
    if (threshold > 1.0f) threshold = 1.0f;
    rain->threshold = threshold;
}
