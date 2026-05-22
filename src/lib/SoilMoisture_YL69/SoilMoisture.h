/**
 * @file SoilMoisture.h
 * @brief YL-69 Soil Moisture Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับอ่านค่าความชื้นในดินจาก YL-69 Soil Moisture Sensor ผ่าน ADC
 *
 * **หลักการทำงาน:**
 * - YL-69 วัดความชื้นในดินโดยใช้หลักการความต้านทานระหว่างขั้ว 2 ขั้ว
 * - ดินแห้ง → ความต้านทานสูง → ADC อ่านค่าได้ต่ำ → % ความชื้นต่ำ
 * - ดินเปียก → ความต้านทานต่ำ → ADC อ่านค่าได้สูง → % ความชื้นสูง
 * - รองรับการ calibrate เพื่อปรับช่วงการอ่านให้แม่นยำ
 *
 * **Hardware Connection:**
 * ```
 *   YL-69              CH57x
 *   VCC  ----------->  3.3V
 *   GND  ----------->  GND
 *   AOUT ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
 *   DOUT ----------->  (optional) GPIO digital output
 * ```
 *
 * @example
 * SoilMoisture_Instance soil;
 * SoilMoisture_Init(&soil, ADC_CH_PA1);
 * SoilMoisture_Calibrate(&soil, 3200, 1800);  // dry=3200, wet=1800
 *
 * uint8_t moisture = SoilMoisture_Read(&soil);
 * if (SoilMoisture_IsDry(&soil, 30)) {
 *     // ดินแห้งเกิน 30% threshold
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __SOILMOISTURE_H
#define __SOILMOISTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default dry threshold (0-100%) */
#ifndef SOILMOISTURE_DEFAULT_DRY_THRESHOLD
#define SOILMOISTURE_DEFAULT_DRY_THRESHOLD  30
#endif

/* ========== Type Definitions ========== */

/**
 * @brief SoilMoisture status codes
 */
typedef enum {
    SOILMOISTURE_OK          = 0,
    SOILMOISTURE_ERROR_PARAM = 1
} SoilMoisture_Status;

/**
 * @brief SoilMoisture instance
 */
typedef struct {
    ADC_Channel adc_channel;    /**< ADC channel */
    uint16_t    dry_value;      /**< ADC raw value เมื่อดินแห้งสนิท */
    uint16_t    wet_value;      /**< ADC raw value เมื่อดินเปียกน้ำ */
    uint8_t     initialized;    /**< Init flag */
} SoilMoisture_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น YL-69 Soil Moisture Sensor
 * @param soil       pointer ไปยัง SoilMoisture_Instance
 * @param adc_channel ADC channel ที่ต่อกับขา AOUT
 * @return SOILMOISTURE_OK หรือ SOILMOISTURE_ERROR_PARAM
 *
 * @note หลัง init ควรเรียก SoilMoisture_Calibrate() เพื่อตั้งค่า dry/wet
 *
 * @example
 * SoilMoisture_Instance soil;
 * SoilMoisture_Init(&soil, ADC_CH_PA1);
 */
SoilMoisture_Status SoilMoisture_Init(SoilMoisture_Instance* soil, ADC_Channel adc_channel);

/**
 * @brief Calibrate ช่วงการวัดความชื้น
 * @param soil     pointer ไปยัง SoilMoisture_Instance
 * @param dry_value ADC raw value เมื่อดินแห้งสนิท (ในอากาศ)
 * @param wet_value ADC raw value เมื่อเสียบในน้ำสะอาด
 *
 * @note dry_value > wet_value เสมอ (YL-69 อ่านค่าสูงเมื่อแห้ง)
 *
 * @example
 * // ปัก probe ในอากาศ → อ่าน raw = 3200
 * // จุ่ม probe ในน้ำ     → อ่าน raw = 1800
 * SoilMoisture_Calibrate(&soil, 3200, 1800);
 */
void SoilMoisture_Calibrate(SoilMoisture_Instance* soil, uint16_t dry_value, uint16_t wet_value);

/**
 * @brief อ่านค่า ADC แบบ raw
 * @param soil pointer ไปยัง SoilMoisture_Instance
 * @return ค่า ADC raw (0-4095)
 */
uint16_t SoilMoisture_ReadRaw(SoilMoisture_Instance* soil);

/**
 * @brief อ่านค่าความชื้นเป็นเปอร์เซ็นต์ (0-100%)
 * @param soil pointer ไปยัง SoilMoisture_Instance
 * @return ค่า 0-100% (0 = แห้ง, 100 = เปียก)
 *
 * @note ต้องเรียก SoilMoisture_Calibrate() ก่อน มิฉะนั้นค่าจะไม่แม่นยำ
 */
uint8_t SoilMoisture_Read(SoilMoisture_Instance* soil);

/**
 * @brief ตรวจสอบว่าดินแห้งเกิน threshold หรือไม่
 * @param soil      pointer ไปยัง SoilMoisture_Instance
 * @param threshold ค่าความชื้นขั้นต่ำที่ต้องการ (0-100%)
 * @return 1 = แห้งเกิน threshold, 0 = ยังชื้นอยู่
 *
 * @example
 * if (SoilMoisture_IsDry(&soil, 30)) {
 *     // รดน้ำ
 * }
 */
uint8_t SoilMoisture_IsDry(SoilMoisture_Instance* soil, uint8_t threshold);

#ifdef __cplusplus
}
#endif

#endif /* __SOILMOISTURE_H */
