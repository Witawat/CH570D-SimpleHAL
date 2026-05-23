/**
 * @file SoundSensor.c
 * @brief KY-038 Sound Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "SoundSensor.h"

/* ========== Private Helpers ========== */

/**
 * @brief Return the maximum of two floats
 * @param a First float value
 * @param b Second float value
 * @return The larger of a and b
 */
static float _fmaxf(float a, float b) {
    return (a > b) ? a : b;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น KY-038 sound sensor
 * @param sound Pointer to SoundSensor_Instance to initialize
 * @param adc_channel ADC channel for microphone output
 * @return SOUNDSENSOR_OK on success, SOUNDSENSOR_ERROR_PARAM if sound is NULL
 * @note  Sets default threshold from SOUNDSENSOR_DEFAULT_THRESHOLD.
 *        Peak tracking starts at 0 and updates on each ReadLevel() call.
 */
SoundSensor_Status SoundSensor_Init(SoundSensor_Instance* sound, ADC_Channel adc_channel) {
    if (sound == NULL) return SOUNDSENSOR_ERROR_PARAM;

    sound->adc_channel = adc_channel;
    sound->threshold   = SOUNDSENSOR_DEFAULT_THRESHOLD;
    sound->peak_value  = 0.0f;
    sound->initialized = 1;
    return SOUNDSENSOR_OK;
}

/**
 * @brief อ่านค่า ADC raw จาก microphone
 * @param sound Pointer to initialized SoundSensor_Instance
 * @return Raw ADC value (0-4095), or 0 if not initialized
 */
uint16_t SoundSensor_ReadRaw(SoundSensor_Instance* sound) {
    if (sound == NULL || !sound->initialized) return 0;
    return ADC_Read(sound->adc_channel);
}

/**
 * @brief อ่านระดับเสียง normalized (0.0 - 1.0)
 * @param sound Pointer to initialized SoundSensor_Instance
 * @return Sound level (0.0 = silence, 1.0 = maximum), or 0.0f if not initialized
 * @note  Normalizes ADC value by dividing by 4095. Updates peak_value tracking.
 */
float SoundSensor_ReadLevel(SoundSensor_Instance* sound) {
    if (sound == NULL || !sound->initialized) return 0.0f;

    uint16_t raw = ADC_Read(sound->adc_channel);
    float level = (float)raw / 4095.0f;

    /* update peak tracking */
    sound->peak_value = _fmaxf(sound->peak_value, level);

    return level;
}

/**
 * @brief ตรวจสอบว่าตรวจพบเสียงดัง (clap) หรือไม่
 * @param sound Pointer to initialized SoundSensor_Instance
 * @param threshold Level threshold (0.0 - 1.0) to trigger detection
 * @return 1 if sound level ≥ threshold, 0 otherwise
 */
uint8_t SoundSensor_IsClapDetected(SoundSensor_Instance* sound, float threshold) {
    if (sound == NULL || !sound->initialized) return 0;

    float level = SoundSensor_ReadLevel(sound);
    return (level >= threshold) ? 1 : 0;
}

/**
 * @brief อ่านค่า peak level สูงสุดตั้งแต่เริ่มต้นหรือ reset
 * @param sound Pointer to initialized SoundSensor_Instance
 * @return Maximum sound level observed so far (0.0 - 1.0)
 */
float SoundSensor_GetPeak(SoundSensor_Instance* sound) {
    if (sound == NULL || !sound->initialized) return 0.0f;
    return sound->peak_value;
}

/**
 * @brief ตั้งค่า threshold สำหรับ clap detection
 * @param sound Pointer to initialized SoundSensor_Instance
 * @param threshold Level threshold (0.0 - 1.0), clamped to valid range
 */
void SoundSensor_SetThreshold(SoundSensor_Instance* sound, float threshold) {
    if (sound == NULL || !sound->initialized) return;
    if (threshold < 0.0f) threshold = 0.0f;
    if (threshold > 1.0f) threshold = 1.0f;
    sound->threshold = threshold;
}
