/**
 * @file RainSensor.h
 * @brief YL-83 Rain Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับตรวจจับฝน/น้ำด้วย YL-83 Rain Sensor Module ผ่าน ADC
 *
 * **หลักการทำงาน:**
 * - YL-83 ใช้แผ่น PCB ที่มีขั้วไฟฟ้าเรียงกัน
 * - เมื่อมีหยดน้ำบนแผ่น → ความต้านทานระหว่างขั้วลดลง → ADC อ่านค่าได้สูงขึ้น
 * - แผ่นแห้ง → ADC ต่ำ, แผ่นเปียก → ADC สูง
 * - รองรับการ calibrate เพื่อความแม่นยำ
 *
 * **Hardware Connection:**
 * ```
 *   YL-83              CH57x
 *   VCC  ----------->  3.3V
 *   GND  ----------->  GND
 *   A0   ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
 *   D0   ----------->  (optional) GPIO digital output
 * ```
 *
 * @example
 * RainSensor_Instance rain;
 * RainSensor_Init(&rain, ADC_CH_PA1);
 *
 * float level = RainSensor_ReadLevel(&rain);
 * if (RainSensor_IsRaining(&rain, 0.3f)) {
 *     // ฝนตก!
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __RAINSENSOR_H
#define __RAINSENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default rain detection threshold (0.0-1.0) */
#ifndef RAINSENSOR_DEFAULT_THRESHOLD
#define RAINSENSOR_DEFAULT_THRESHOLD  0.3f
#endif

/* ========== Type Definitions ========== */

/**
 * @brief RainSensor status codes
 */
typedef enum {
    RAINSENSOR_OK          = 0,
    RAINSENSOR_ERROR_PARAM = 1
} RainSensor_Status;

/**
 * @brief RainSensor instance
 */
typedef struct {
    ADC_Channel adc_channel;    /**< ADC channel สำหรับ analog (A0) */
    float       threshold;      /**< Rain detection threshold (0.0-1.0) */
    uint8_t     initialized;    /**< Init flag */
} RainSensor_Instance;

/**
 * @brief Rain intensity levels
 */
typedef enum {
    RAIN_NONE      = 0,  /**< ไม่มีฝน */
    RAIN_LIGHT     = 1,  /**< ฝนปรอยๆ (0-33%) */
    RAIN_MODERATE  = 2,  /**< ฝนปานกลาง (34-66%) */
    RAIN_HEAVY     = 3   /**< ฝนหนัก (67-100%) */
} RainSensor_Intensity;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น YL-83 Rain Sensor
 * @param rain        pointer ไปยัง RainSensor_Instance
 * @param adc_channel ADC channel สำหรับขา A0
 * @return RAINSENSOR_OK หรือ RAINSENSOR_ERROR_PARAM
 *
 * @example
 * RainSensor_Instance rain;
 * RainSensor_Init(&rain, ADC_CH_PA1);
 */
RainSensor_Status RainSensor_Init(RainSensor_Instance* rain, ADC_Channel adc_channel);

/**
 * @brief อ่านค่า ADC แบบ raw
 * @param rain pointer ไปยัง RainSensor_Instance
 * @return ค่า ADC raw (0-4095)
 */
uint16_t RainSensor_ReadRaw(RainSensor_Instance* rain);

/**
 * @brief อ่านระดับน้ำบนแผ่นเซนเซอร์ (0.0 = แห้ง, 1.0 = เปียกโชก)
 * @param rain pointer ไปยัง RainSensor_Instance
 * @return ค่า normalized 0.0 - 1.0
 */
float RainSensor_ReadLevel(RainSensor_Instance* rain);

/**
 * @brief ตรวจสอบว่าฝนตกหรือไม่
 * @param rain      pointer ไปยัง RainSensor_Instance
 * @param threshold ค่า threshold (0.0 = ไวมาก, 1.0 = ต้องเปียกมาก)
 * @return 1 = ฝนตก, 0 = ไม่มีฝน
 *
 * @example
 * if (RainSensor_IsRaining(&rain, 0.3f)) {
 *     // ปิดหน้าต่าง
 * }
 */
uint8_t RainSensor_IsRaining(RainSensor_Instance* rain, float threshold);

/**
 * @brief วัดระดับความแรงของฝน
 * @param rain pointer ไปยัง RainSensor_Instance
 * @return RainSensor_Intensity (NONE / LIGHT / MODERATE / HEAVY)
 */
RainSensor_Intensity RainSensor_GetIntensity(RainSensor_Instance* rain);

/**
 * @brief ตั้งค่า default threshold
 * @param rain      pointer ไปยัง RainSensor_Instance
 * @param threshold ค่า 0.0 (ไวสุด) ถึง 1.0 (ช้าสุด)
 */
void RainSensor_SetThreshold(RainSensor_Instance* rain, float threshold);

#ifdef __cplusplus
}
#endif

#endif /* __RAINSENSOR_H */
