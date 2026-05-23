/**
 * @file OH49E.c
 * @brief OH49E Hall Effect Sensor Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "OH49E.h"

/* ========== Private Helpers ========== */

/**
 * @brief Absolute value for float
 * @param x Input value
 * @return Absolute value of x
 */
static float _fabsf(float x) {
    return (x < 0.0f) ? -x : x;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น OH49E Hall effect sensor
 * @param hall Pointer to OH49E_Instance to initialize
 * @param adc_channel ADC channel for sensor output
 * @param vcc Sensor supply voltage (V)
 * @param vref ADC reference voltage (V)
 * @return OH49E_OK on success, OH49E_ERROR_PARAM on invalid parameter
 * @note  Sets quiescent voltage to VCC/2. Default threshold from OH49E_DEFAULT_THRESHOLD_V.
 *        Output voltage swings around VCC/2 proportional to magnetic field.
 */
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

/**
 * @brief อ่านค่า ADC raw จาก Hall sensor
 * @param hall Pointer to initialized OH49E_Instance
 * @return Raw ADC value (0-4095), or 0 if not initialized
 */
uint16_t OH49E_ReadRaw(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0;
    return ADC_Read(hall->adc_channel);
}

/**
 * @brief อ่านแรงดัน output ของ Hall sensor
 * @param hall Pointer to initialized OH49E_Instance
 * @return Output voltage (V), or 0.0f if not initialized
 */
float OH49E_ReadVoltage(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0.0f;
    uint16_t raw = ADC_Read(hall->adc_channel);
    return ADC_ToVoltage(raw, hall->vref);
}

/**
 * @brief ตรวจสอบว่าตรวจพบสนามแม่เหล็กหรือไม่
 * @param hall Pointer to initialized OH49E_Instance
 * @return 1 if magnetic field detected (|Vout - Vquiescent| > threshold), 0 otherwise
 * @note  Compares deviation from quiescent voltage against the configured threshold.
 */
uint8_t OH49E_IsFieldDetected(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return 0;
    float v = OH49E_ReadVoltage(hall);
    return (_fabsf(v - hall->quiescent_v) > hall->threshold_v) ? 1 : 0;
}

/**
 * @brief ระบุทิศทางของสนามแม่เหล็ก (N/S)
 * @param hall Pointer to initialized OH49E_Instance
 * @return OH49E_FIELD_NORTH if Vout > quiescent, OH49E_FIELD_SOUTH if Vout < quiescent,
 *         OH49E_FIELD_NONE if within threshold
 * @note  North pole produces positive voltage deviation, South pole produces negative deviation.
 */
OH49E_FieldDir OH49E_GetFieldDirection(OH49E_Instance* hall) {
    if (hall == NULL || !hall->initialized) return OH49E_FIELD_NONE;
    float v    = OH49E_ReadVoltage(hall);
    float diff = v - hall->quiescent_v;
    if (_fabsf(diff) <= hall->threshold_v) return OH49E_FIELD_NONE;
    return (diff > 0.0f) ? OH49E_FIELD_NORTH : OH49E_FIELD_SOUTH;
}

/**
 * @brief อ่านความแรงของสนามแม่เหล็ก (normalized 0.0 - 1.0)
 * @param hall Pointer to initialized OH49E_Instance
 * @return Normalized field strength (0.0 = no field, 1.0 = maximum measurable)
 * @note  Strength = |Vout - Vquiescent| / Vquiescent, clamped to [0.0, 1.0].
 *        Maximum deviation is when Vout = 0 or Vout = VCC.
 */
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

/**
 * @brief ตั้งค่า threshold สำหรับการตรวจสอบสนามแม่เหล็ก
 * @param hall Pointer to initialized OH49E_Instance
 * @param threshold_v Voltage deviation threshold (V), clamped to ≥ 0
 */
void OH49E_SetThreshold(OH49E_Instance* hall, float threshold_v) {
    if (hall == NULL || !hall->initialized) return;
    if (threshold_v < 0.0f) threshold_v = 0.0f;
    hall->threshold_v = threshold_v;
}
