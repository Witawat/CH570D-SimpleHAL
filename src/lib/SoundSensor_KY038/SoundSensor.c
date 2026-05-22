/**
 * @file SoundSensor.c
 * @brief KY-038 Sound Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "SoundSensor.h"

/* ========== Private Helpers ========== */

static float _fmaxf(float a, float b) {
    return (a > b) ? a : b;
}

/* ========== Public Functions ========== */

SoundSensor_Status SoundSensor_Init(SoundSensor_Instance* sound, ADC_Channel adc_channel) {
    if (sound == NULL) return SOUNDSENSOR_ERROR_PARAM;

    sound->adc_channel = adc_channel;
    sound->threshold   = SOUNDSENSOR_DEFAULT_THRESHOLD;
    sound->peak_value  = 0.0f;
    sound->initialized = 1;
    return SOUNDSENSOR_OK;
}

uint16_t SoundSensor_ReadRaw(SoundSensor_Instance* sound) {
    if (sound == NULL || !sound->initialized) return 0;
    return ADC_Read(sound->adc_channel);
}

float SoundSensor_ReadLevel(SoundSensor_Instance* sound) {
    if (sound == NULL || !sound->initialized) return 0.0f;

    uint16_t raw = ADC_Read(sound->adc_channel);
    float level = (float)raw / 4095.0f;

    /* update peak tracking */
    sound->peak_value = _fmaxf(sound->peak_value, level);

    return level;
}

uint8_t SoundSensor_IsClapDetected(SoundSensor_Instance* sound, float threshold) {
    if (sound == NULL || !sound->initialized) return 0;

    float level = SoundSensor_ReadLevel(sound);
    return (level >= threshold) ? 1 : 0;
}

float SoundSensor_GetPeak(SoundSensor_Instance* sound) {
    if (sound == NULL || !sound->initialized) return 0.0f;
    return sound->peak_value;
}

void SoundSensor_SetThreshold(SoundSensor_Instance* sound, float threshold) {
    if (sound == NULL || !sound->initialized) return;
    if (threshold < 0.0f) threshold = 0.0f;
    if (threshold > 1.0f) threshold = 1.0f;
    sound->threshold = threshold;
}
