/**
 * @file RainSensor.c
 * @brief YL-83 Rain Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "RainSensor.h"

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น YL-83 Rain Sensor
 * @param rain        - pointer ไปยัง RainSensor_Instance (NULL → return RAINSENSOR_ERROR_PARAM)
 * @param adc_channel - ADC channel สำหรับขา A0
 * @return RAINSENSOR_OK หรือ RAINSENSOR_ERROR_PARAM
 * @note  threshold เริ่มต้น = RAINSENSOR_DEFAULT_THRESHOLD (0.3f)
 *        ไม่มีการ init ADC hardware — ต้อง init ADC ก่อนแยกต่างหาก
 *        ใช้ได้กับ ADC pin (PA1, PA2, PC4, PD2-PD7)
 */
RainSensor_Status RainSensor_Init(RainSensor_Instance* rain, ADC_Channel adc_channel) {
    if (rain == NULL) return RAINSENSOR_ERROR_PARAM;

    rain->adc_channel = adc_channel;
    rain->threshold   = RAINSENSOR_DEFAULT_THRESHOLD;
    rain->initialized = 1;
    return RAINSENSOR_OK;
}

/**
 * @brief อ่านค่า ADC แบบ raw
 * @param rain - pointer ไปยัง RainSensor_Instance (NULL หรือไม่ initialized → return 0)
 * @return ค่า ADC raw (0-4095)
 * @note  อ่านจาก ADC_Read() โดยตรง 12-bit resolution
 *        YL-83: แผ่นแห้ง → ADC ต่ำ, แผ่นเปียก → ADC สูง
 */
uint16_t RainSensor_ReadRaw(RainSensor_Instance* rain) {
    if (rain == NULL || !rain->initialized) return 0;
    return ADC_Read(rain->adc_channel);
}

/**
 * @brief อ่านระดับน้ำบนแผ่นเซนเซอร์ (0.0 = แห้ง, 1.0 = เปียกโชก)
 * @param rain - pointer ไปยัง RainSensor_Instance (NULL หรือไม่ initialized → return 0.0f)
 * @return ค่า normalized 0.0 - 1.0
 * @note  level = raw / 4095.0
 *        clamp 0.0-1.0 ป้องกันค่าเกินช่วง
 *        ยิ่งเปียกยิ่งค่าเข้าใกล้ 1.0
 */
float RainSensor_ReadLevel(RainSensor_Instance* rain) {
    if (rain == NULL || !rain->initialized) return 0.0f;

    uint16_t raw = ADC_Read(rain->adc_channel);
    float level = (float)raw / 4095.0f;

    if (level < 0.0f) return 0.0f;
    if (level > 1.0f) return 1.0f;
    return level;
}

/**
 * @brief ตรวจสอบว่าฝนตกหรือไม่
 * @param rain      - pointer ไปยัง RainSensor_Instance (NULL หรือไม่ initialized → return 0)
 * @param threshold - ค่า threshold 0.0-1.0 (ยิ่งน้อย = ยิ่งไว)
 * @return 1 = ฝนตก (level >= threshold), 0 = ไม่มีฝน
 * @note  เรียก RainSensor_ReadLevel() ภายใน
 *        ถ้า threshold > 1.0 → จะไม่มีทาง return 1
 */
uint8_t RainSensor_IsRaining(RainSensor_Instance* rain, float threshold) {
    if (rain == NULL || !rain->initialized) return 0;

    float level = RainSensor_ReadLevel(rain);
    return (level >= threshold) ? 1 : 0;
}

/**
 * @brief วัดระดับความแรงของฝน
 * @param rain - pointer ไปยัง RainSensor_Instance (NULL หรือไม่ initialized → return RAIN_NONE)
 * @return RainSensor_Intensity (NONE / LIGHT / MODERATE / HEAVY)
 * @note  level < 0.01 → NONE
 *        level < 0.34 → LIGHT (ฝนปรอยๆ 0-33%)
 *        level < 0.67 → MODERATE (ฝนปานกลาง 34-66%)
 *        level >= 0.67 → HEAVY (ฝนหนัก 67-100%)
 */
RainSensor_Intensity RainSensor_GetIntensity(RainSensor_Instance* rain) {
    if (rain == NULL || !rain->initialized) return RAIN_NONE;

    float level = RainSensor_ReadLevel(rain);

    if (level < 0.01f)       return RAIN_NONE;
    if (level < 0.34f)       return RAIN_LIGHT;
    if (level < 0.67f)       return RAIN_MODERATE;
    return RAIN_HEAVY;
}

/**
 * @brief ตั้งค่า default threshold
 * @param rain      - pointer ไปยัง RainSensor_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param threshold - ค่า 0.0 (ไวสุด) ถึง 1.0 (ช้าสุด)
 * @note  clamp อัตโนมัติ: < 0.0 → 0.0, > 1.0 → 1.0
 *        ค่า threshold นี้ใช้ใน RainSensor_IsRaining() ถ้าไม่ระบุ threshold เอง
 */
void RainSensor_SetThreshold(RainSensor_Instance* rain, float threshold) {
    if (rain == NULL || !rain->initialized) return;
    if (threshold < 0.0f) threshold = 0.0f;
    if (threshold > 1.0f) threshold = 1.0f;
    rain->threshold = threshold;
}
