/**
 * @file SoilMoisture.c
 * @brief YL-69 Soil Moisture Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "SoilMoisture.h"

/* ========== Private Helpers ========== */

static uint8_t _constrain_u8(int32_t val, uint8_t min, uint8_t max) {
    if (val < (int32_t)min) return min;
    if (val > (int32_t)max) return max;
    return (uint8_t)val;
}

/* ========== Public Functions ========== */

SoilMoisture_Status SoilMoisture_Init(SoilMoisture_Instance* soil, ADC_Channel adc_channel) {
    if (soil == NULL) return SOILMOISTURE_ERROR_PARAM;

    soil->adc_channel = adc_channel;
    soil->dry_value   = 4095;  /* default: max dry */
    soil->wet_value   = 0;     /* default: min wet */
    soil->initialized = 1;
    return SOILMOISTURE_OK;
}

void SoilMoisture_Calibrate(SoilMoisture_Instance* soil, uint16_t dry_value, uint16_t wet_value) {
    if (soil == NULL || !soil->initialized) return;

    soil->dry_value = dry_value;
    soil->wet_value = wet_value;
}

uint16_t SoilMoisture_ReadRaw(SoilMoisture_Instance* soil) {
    if (soil == NULL || !soil->initialized) return 0;
    return ADC_Read(soil->adc_channel);
}

uint8_t SoilMoisture_Read(SoilMoisture_Instance* soil) {
    if (soil == NULL || !soil->initialized) return 0;

    uint16_t raw = ADC_Read(soil->adc_channel);
    uint16_t dry = soil->dry_value;
    uint16_t wet = soil->wet_value;

    /* avoid division by zero */
    if (dry == wet) return 50;

    if (raw >= dry) return 0;   /* แห้งสุด */
    if (raw <= wet) return 100; /* เปียกสุด */

    /* map: dry → 0%, wet → 100% */
    int32_t percent = (int32_t)(dry - raw) * 100 / (int32_t)(dry - wet);
    return _constrain_u8(percent, 0, 100);
}

uint8_t SoilMoisture_IsDry(SoilMoisture_Instance* soil, uint8_t threshold) {
    if (soil == NULL || !soil->initialized) return 0;

    uint8_t moisture = SoilMoisture_Read(soil);
    return (moisture < threshold) ? 1 : 0;
}
