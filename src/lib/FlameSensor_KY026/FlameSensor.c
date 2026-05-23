/**
 * @file FlameSensor.c
 * @brief KY-026 Flame Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "FlameSensor.h"

/* ========== Private Helpers ========== */

/**
 * @brief Constrain a float value within [min, max]
 * @param val  Input value to constrain
 * @param min  Lower bound
 * @param max  Upper bound
 * @return Clamped value within the inclusive range
 */
static float _fconstrain(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น KY-026 flame sensor
 * @param flame Pointer to FlameSensor_Instance to initialize
 * @param adc_channel ADC channel for analog flame intensity reading
 * @param digital_pin Digital pin for comparator output (0xFF to disable)
 * @return FLAMESENSOR_OK on success, FLAMESENSOR_ERROR_PARAM if flame is NULL
 * @note  Configures ADC for analog reading and optional digital input pin.
 *        Default threshold is FLAMESENSOR_DEFAULT_THRESHOLD (normalized 0.0 - 1.0).
 */
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

/**
 * @brief อ่านค่า RAW จาก ADC
 * @param flame Pointer to initialized FlameSensor_Instance
 * @return Raw ADC value (0-4095)
 */
uint16_t FlameSensor_ReadRaw(FlameSensor_Instance* flame) {
    if (flame == NULL || !flame->initialized) return 0;
    return ADC_Read(flame->adc_channel);
}

/**
 * @brief อ่านความเข้มแสง flame (0.0 - 1.0)
 * @param flame Pointer to initialized FlameSensor_Instance
 * @return Normalized flame intensity (0.0 = no flame, 1.0 = maximum)
 * @note  ADC reading normalized by dividing by 4095. The IR signal increases with flame proximity.
 */
float FlameSensor_ReadIntensity(FlameSensor_Instance* flame) {
    if (flame == NULL || !flame->initialized) return 0.0f;

    uint16_t raw = ADC_Read(flame->adc_channel);
    /* Normalize to 0.0 - 1.0 (signal goes UP with flame) */
    float intensity = (float)raw / 4095.0f;
    return _fconstrain(intensity, 0.0f, 1.0f);
}

/**
 * @brief ตรวจสอบว่าตรวจพบ flame หรือไม่
 * @param flame Pointer to initialized FlameSensor_Instance
 * @return 1 if flame detected, 0 otherwise
 * @note  Uses digital pin comparator if connected (non-0xFF), otherwise falls back to
 *        ADC comparison against the software threshold.
 */
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

/**
 * @brief ตั้งค่า threshold สำหรับ software detection
 * @param flame Pointer to initialized FlameSensor_Instance
 * @param threshold Threshold value (0.0 - 1.0), constrained automatically
 */
void FlameSensor_SetThreshold(FlameSensor_Instance* flame, float threshold) {
    if (flame == NULL || !flame->initialized) return;
    flame->threshold = _fconstrain(threshold, 0.0f, 1.0f);
}
