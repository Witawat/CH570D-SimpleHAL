/**
 * @file FlameSensor.c
 * @brief KY-026 Flame Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "FlameSensor.h"

/* ========== Private Helpers ========== */

static float _fconstrain(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* ========== Public Functions ========== */

FlameSensor_Status FlameSensor_Init(FlameSensor_Instance* flame, ADC_Channel adc_channel, uint8_t digital_pin) {
    if (flame == NULL) return FLAMESENSOR_ERROR_PARAM;

    flame->adc_channel = adc_channel;
    flame->digital_pin = digital_pin;
    flame->threshold   = FLAMESENSOR_DEFAULT_THRESHOLD;
    flame->initialized = 1;

    /* setup digital pin as input if used */
    if (digital_pin != 0xFF) {
        pinMode(digital_pin, PIN_MODE_INPUT);
    }

    return FLAMESENSOR_OK;
}

uint16_t FlameSensor_ReadRaw(FlameSensor_Instance* flame) {
    if (flame == NULL || !flame->initialized) return 0;
    return ADC_Read(flame->adc_channel);
}

float FlameSensor_ReadIntensity(FlameSensor_Instance* flame) {
    if (flame == NULL || !flame->initialized) return 0.0f;

    uint16_t raw = ADC_Read(flame->adc_channel);
    /* Normalize to 0.0 - 1.0 (signal goes UP with flame) */
    float intensity = (float)raw / 4095.0f;
    return _fconstrain(intensity, 0.0f, 1.0f);
}

uint8_t FlameSensor_IsFlameDetected(FlameSensor_Instance* flame) {
    if (flame == NULL || !flame->initialized) return 0;

    /* if digital pin is connected, use hardware threshold */
    if (flame->digital_pin != 0xFF) {
        return (digitalRead(flame->digital_pin) == HIGH) ? 1 : 0;
    }

    /* fallback: use ADC + software threshold */
    float intensity = FlameSensor_ReadIntensity(flame);
    return (intensity >= flame->threshold) ? 1 : 0;
}

void FlameSensor_SetThreshold(FlameSensor_Instance* flame, float threshold) {
    if (flame == NULL || !flame->initialized) return;
    flame->threshold = _fconstrain(threshold, 0.0f, 1.0f);
}
