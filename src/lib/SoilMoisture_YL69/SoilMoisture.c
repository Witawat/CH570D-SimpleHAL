/**
 * @file SoilMoisture.c
 * @brief YL-69 Soil Moisture Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "SoilMoisture.h"

/* ========== Private Helpers ========== */

/**
 * @brief Constrain an int32 value to uint8 range [min, max]
 * @param val Input value to constrain
 * @param min Lower bound (uint8)
 * @param max Upper bound (uint8)
 * @return Constrained value within [min, max]
 */
static uint8_t _constrain_u8(int32_t val, uint8_t min, uint8_t max) {
    if (val < (int32_t)min) return min;
    if (val > (int32_t)max) return max;
    return (uint8_t)val;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น YL-69 soil moisture sensor
 * @param soil Pointer to SoilMoisture_Instance to initialize
 * @param adc_channel ADC channel for analog moisture reading
 * @return SOILMOISTURE_OK on success, SOILMOISTURE_ERROR_PARAM if soil is NULL
 * @note  Default dry = 4095 (max ADC), default wet = 0 (min ADC).
 *        Calibrate with SoilMoisture_Calibrate() for accurate readings.
 */
SoilMoisture_Status SoilMoisture_Init(SoilMoisture_Instance* soil, ADC_Channel adc_channel) {
    if (soil == NULL) return SOILMOISTURE_ERROR_PARAM;

    soil->adc_channel = adc_channel;
    soil->dry_value   = 4095;  /* default: max dry */
    soil->wet_value   = 0;     /* default: min wet */
    soil->initialized = 1;
    return SOILMOISTURE_OK;
}

/**
 * @brief ตั้งค่า calibration values (dry/wet ADC thresholds)
 * @param soil Pointer to initialized SoilMoisture_Instance
 * @param dry_value ADC reading in dry air (typically high)
 * @param wet_value ADC reading fully submerged (typically low)
 * @note  Dry value should be measured in air, wet value in water.
 *        These values define the mapping range for SoilMoisture_Read().
 */
void SoilMoisture_Calibrate(SoilMoisture_Instance* soil, uint16_t dry_value, uint16_t wet_value) {
    if (soil == NULL || !soil->initialized) return;

    soil->dry_value = dry_value;
    soil->wet_value = wet_value;
}

/**
 * @brief อ่านค่า ADC raw จาก sensor
 * @param soil Pointer to initialized SoilMoisture_Instance
 * @return Raw ADC value (0-4095), or 0 if not initialized
 */
uint16_t SoilMoisture_ReadRaw(SoilMoisture_Instance* soil) {
    if (soil == NULL || !soil->initialized) return 0;
    return ADC_Read(soil->adc_channel);
}

/**
 * @brief อ่านค่าความชื้นเป็นเปอร์เซ็นต์ (0% = dry, 100% = wet)
 * @param soil Pointer to initialized SoilMoisture_Instance
 * @return Moisture percentage (0-100), or 50 if dry == wet (uncalibrated)
 * @note  Maps ADC reading between dry_value (0%) and wet_value (100%).
 *        Uses linear interpolation: (dry - raw) × 100 / (dry - wet).
 */
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

/**
 * @brief ตรวจสอบว่าดินแห้งหรือไม่
 * @param soil Pointer to initialized SoilMoisture_Instance
 * @param threshold Moisture threshold below which soil is considered dry (0-100)
 * @return 1 if moisture < threshold, 0 otherwise
 */
uint8_t SoilMoisture_IsDry(SoilMoisture_Instance* soil, uint8_t threshold) {
    if (soil == NULL || !soil->initialized) return 0;

    uint8_t moisture = SoilMoisture_Read(soil);
    return (moisture < threshold) ? 1 : 0;
}
