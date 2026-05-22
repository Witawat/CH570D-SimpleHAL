/**
 * @file SoundSensor.h
 * @brief KY-038 Sound Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับอ่านค่าเสียงจาก KY-038 Sound Sensor Module ผ่าน ADC
 *
 * **หลักการทำงาน:**
 * - KY-038 มี condenser microphone + LM393 comparator
 * - Analog output (A0): แรงดันแปรผันตามความดังของเสียง
 * - Digital output (D0): HIGH เมื่อเสียงเกิน threshold (ปรับได้ด้วย potentiometer)
 * - Library นี้ใช้ ADC วัดระดับเสียงแบบ analog เพื่อความละเอียด
 *
 * **Hardware Connection:**
 * ```
 *   KY-038             CH57x
 *   VCC  ----------->  3.3V
 *   GND  ----------->  GND
 *   A0   ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
 *   D0   ----------->  (optional) GPIO digital output
 * ```
 *
 * @example
 * SoundSensor_Instance sound;
 * SoundSensor_Init(&sound, ADC_CH_PA1);
 *
 * float level = SoundSensor_ReadLevel(&sound);
 * if (SoundSensor_IsClapDetected(&sound, 0.5f)) {
 *     // ตรวจจับเสียงปรบมือ
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __SOUNDSENSOR_H
#define __SOUNDSENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default clap detection threshold (0.0-1.0) */
#ifndef SOUNDSENSOR_DEFAULT_THRESHOLD
#define SOUNDSENSOR_DEFAULT_THRESHOLD  0.5f
#endif

/** @brief Number of samples for peak detection */
#ifndef SOUNDSENSOR_PEAK_SAMPLES
#define SOUNDSENSOR_PEAK_SAMPLES  10
#endif

/* ========== Type Definitions ========== */

/**
 * @brief SoundSensor status codes
 */
typedef enum {
    SOUNDSENSOR_OK          = 0,
    SOUNDSENSOR_ERROR_PARAM = 1
} SoundSensor_Status;

/**
 * @brief SoundSensor instance
 */
typedef struct {
    ADC_Channel adc_channel;    /**< ADC channel สำหรับ analog (A0) */
    float       threshold;      /**< Detection threshold (0.0-1.0) */
    float       peak_value;     /**< Peak value since last reset */
    uint8_t     initialized;    /**< Init flag */
} SoundSensor_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น KY-038 Sound Sensor
 * @param sound       pointer ไปยัง SoundSensor_Instance
 * @param adc_channel ADC channel สำหรับขา A0
 * @return SOUNDSENSOR_OK หรือ SOUNDSENSOR_ERROR_PARAM
 *
 * @example
 * SoundSensor_Instance sound;
 * SoundSensor_Init(&sound, ADC_CH_PA1);
 */
SoundSensor_Status SoundSensor_Init(SoundSensor_Instance* sound, ADC_Channel adc_channel);

/**
 * @brief อ่านค่า ADC แบบ raw
 * @param sound pointer ไปยัง SoundSensor_Instance
 * @return ค่า ADC raw (0-4095)
 */
uint16_t SoundSensor_ReadRaw(SoundSensor_Instance* sound);

/**
 * @brief อ่านระดับเสียง (0.0 = เงียบ, 1.0 = ดังสุด)
 * @param sound pointer ไปยัง SoundSensor_Instance
 * @return ค่า normalized 0.0 - 1.0
 */
float SoundSensor_ReadLevel(SoundSensor_Instance* sound);

/**
 * @brief ตรวจจับเสียงดังเกิน threshold (เช่น เสียงปรบมือ)
 * @param sound     pointer ไปยัง SoundSensor_Instance
 * @param threshold ค่า threshold 0.0 (ไวสุด) ถึง 1.0 (ช้าสุด)
 * @return 1 = ตรวจพบเสียงดัง, 0 = ไม่พบ
 *
 * @example
 * if (SoundSensor_IsClapDetected(&sound, 0.5f)) {
 *     // toggle LED
 * }
 */
uint8_t SoundSensor_IsClapDetected(SoundSensor_Instance* sound, float threshold);

/**
 * @brief อ่านค่า peak ของเสียงตั้งแต่上次 reset
 * @param sound pointer ไปยัง SoundSensor_Instance
 * @return ค่า peak (0.0-1.0)
 *
 * @note peak จะ reset เมื่อเรียก SoundSensor_Init() ใหม่
 *       ใช้สำหรับวัดเสียงดังที่สุดในช่วงเวลาหนึ่ง
 */
float SoundSensor_GetPeak(SoundSensor_Instance* sound);

/**
 * @brief ตั้งค่า default threshold
 * @param sound     pointer ไปยัง SoundSensor_Instance
 * @param threshold ค่า 0.0 (ไวสุด) ถึง 1.0 (ช้าสุด)
 */
void SoundSensor_SetThreshold(SoundSensor_Instance* sound, float threshold);

#ifdef __cplusplus
}
#endif

#endif /* __SOUNDSENSOR_H */
