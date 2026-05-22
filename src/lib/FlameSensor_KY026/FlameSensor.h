/**
 * @file FlameSensor.h
 * @brief KY-026 Flame Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับตรวจจับเปลวไฟด้วย KY-026 Flame Sensor ผ่าน ADC + GPIO
 *
 * **หลักการทำงาน:**
 * - KY-026 ตรวจจับแสง infrared จากเปลวไฟในช่วง 760nm-1100nm
 * - Analog output (A0): ค่า ADC สูงขึ้นเมื่อมีเปลวไฟใกล้/แรงขึ้น
 * - Digital output (D0): HIGH เมื่อมีเปลวไฟเกิน threshold (ปรับได้ด้วย potentiometer บนโมดูล)
 * - ระยะตรวจจับ: ~20cm - 100cm ขึ้นกับความแรงของเปลวไฟ
 *
 * **Hardware Connection:**
 * ```
 *   KY-026             CH57x
 *   VCC  ----------->  3.3V
 *   GND  ----------->  GND
 *   A0   ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
 *   D0   ----------->  GPIO (optional) digital output
 * ```
 *
 * @example
 * FlameSensor_Instance flame;
 * FlameSensor_Init(&flame, ADC_CH_PA1, PD0);
 *
 * float intensity = FlameSensor_ReadIntensity(&flame);
 * if (FlameSensor_IsFlameDetected(&flame)) {
 *     // ไฟไหม้!
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __FLAMESENSOR_H
#define __FLAMESENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default ADC threshold for software flame detection (0.0-1.0) */
#ifndef FLAMESENSOR_DEFAULT_THRESHOLD
#define FLAMESENSOR_DEFAULT_THRESHOLD  0.3f
#endif

/* ========== Type Definitions ========== */

/**
 * @brief FlameSensor status codes
 */
typedef enum {
    FLAMESENSOR_OK          = 0,
    FLAMESENSOR_ERROR_PARAM = 1
} FlameSensor_Status;

/**
 * @brief FlameSensor instance
 */
typedef struct {
    ADC_Channel adc_channel;    /**< ADC channel สำหรับ analog (A0) */
    uint8_t     digital_pin;    /**< GPIO pin สำหรับ digital (D0), 0xFF = not used */
    float       threshold;      /**< Software threshold (0.0 = ต่ำสุด, 1.0 = สูงสุด) */
    uint8_t     initialized;    /**< Init flag */
} FlameSensor_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น KY-026 Flame Sensor
 * @param flame       pointer ไปยัง FlameSensor_Instance
 * @param adc_channel ADC channel สำหรับขา A0
 * @param digital_pin GPIO pin สำหรับขา D0 (ใช้ 0xFF ถ้าไม่ต่อ)
 * @return FLAMESENSOR_OK หรือ FLAMESENSOR_ERROR_PARAM
 *
 * @example
 * FlameSensor_Instance flame;
 * FlameSensor_Init(&flame, ADC_CH_PA1, PD0);
 */
FlameSensor_Status FlameSensor_Init(FlameSensor_Instance* flame, ADC_Channel adc_channel, uint8_t digital_pin);

/**
 * @brief อ่านค่า ADC แบบ raw
 * @param flame pointer ไปยัง FlameSensor_Instance
 * @return ค่า ADC raw (0-4095)
 */
uint16_t FlameSensor_ReadRaw(FlameSensor_Instance* flame);

/**
 * @brief อ่านความเข้มของเปลวไฟ (0.0 = ไม่มีไฟ, 1.0 = ไฟแรงสุด)
 * @param flame pointer ไปยัง FlameSensor_Instance
 * @return ค่า normalized 0.0 - 1.0
 */
float FlameSensor_ReadIntensity(FlameSensor_Instance* flame);

/**
 * @brief ตรวจสอบว่ามีเปลวไฟหรือไม่ (ใช้ digital pin + software threshold)
 * @param flame pointer ไปยัง FlameSensor_Instance
 * @return 1 = มีเปลวไฟ, 0 = ไม่มี
 *
 * @details ถ้าต่อ digital pin (D0) จะอ่านจาก hardware threshold
 *          ถ้าไม่ต่อ จะใช้ ADC เทียบกับ software threshold
 */
uint8_t FlameSensor_IsFlameDetected(FlameSensor_Instance* flame);

/**
 * @brief ตั้งค่า software threshold สำหรับตรวจจับเปลวไฟ
 * @param flame     pointer ไปยัง FlameSensor_Instance
 * @param threshold ค่า 0.0 (ไวสุด) ถึง 1.0 (ช้าสุด)
 *
 * @note ใช้เฉพาะกรณีที่ไม่ต่อ digital pin (D0)
 */
void FlameSensor_SetThreshold(FlameSensor_Instance* flame, float threshold);

#ifdef __cplusplus
}
#endif

#endif /* __FLAMESENSOR_H */
