/**
 * @file OH49E.h
 * @brief OH49E Hall Effect Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับอ่านค่าจาก OH49E Linear Hall Effect Sensor ผ่าน ADC
 *
 * **หลักการทำงาน:**
 * - OH49E เป็น Linear Hall Effect sensor
 * - Output voltage ≈ VCC/2 เมื่อไม่มีสนามแม่เหล็ก (quiescent point)
 * - สนามแม่เหล็ก N-pole → voltage สูงกว่า VCC/2
 * - สนามแม่เหล็ก S-pole → voltage ต่ำกว่า VCC/2
 * - ช่วง output: ~0.5V – (VCC - 0.5V)
 *
 * **Hardware Connection:**
 * ```
 *   OH49E             CH57x
 *   Pin 1 (VCC) ----> 3.3V
 *   Pin 2 (GND) ----> GND
 *   Pin 3 (OUT) ----> ADC pin (PA2, PA1, PC4, PD2-PD7)
 * ```
 *
 * @example
 * OH49E_Instance hall;
 * OH49E_Init(&hall, ADC_CH_PA2, 3.3f, 3.3f);
 *
 * float v = OH49E_ReadVoltage(&hall);
 * if (OH49E_IsFieldDetected(&hall)) {
 *     // มีสนามแม่เหล็ก
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __OH49E_H
#define __OH49E_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Dead-band รอบ quiescent point (V) สำหรับ IsFieldDetected */
#ifndef OH49E_DEFAULT_THRESHOLD_V
#define OH49E_DEFAULT_THRESHOLD_V  0.15f
#endif

/* ========== Type Definitions ========== */

/**
 * @brief OH49E status codes
 */
typedef enum {
    OH49E_OK          = 0,
    OH49E_ERROR_PARAM = 1
} OH49E_Status;

/**
 * @brief ทิศทางของสนามแม่เหล็กที่ตรวจพบ
 */
typedef enum {
    OH49E_FIELD_NONE    = 0,  /**< ไม่มีสนามแม่เหล็ก (quiescent) */
    OH49E_FIELD_NORTH   = 1,  /**< N-pole (voltage > VCC/2) */
    OH49E_FIELD_SOUTH   = 2   /**< S-pole (voltage < VCC/2) */
} OH49E_FieldDir;

/**
 * @brief OH49E instance
 */
typedef struct {
    ADC_Channel adc_channel;  /**< ADC channel ที่ต่อกับ OUT */
    float       vcc;          /**< แรงดัน VCC ของ sensor (V) */
    float       vref;         /**< ADC reference voltage (V) */
    float       threshold_v;  /**< Dead-band รอบ quiescent point (V) */
    float       quiescent_v;  /**< Quiescent voltage = VCC/2 */
    uint8_t     initialized;  /**< Init flag */
} OH49E_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น OH49E sensor
 * @param hall        pointer ไปยัง OH49E_Instance
 * @param adc_channel ADC channel ที่ต่อกับขา OUT ของ sensor
 * @param vcc         แรงดัน VCC ของ sensor (V) — ปกติ 3.3f
 * @param vref        ADC reference voltage (V) — ปกติ 3.3f
 * @return OH49E_OK หรือ OH49E_ERROR_PARAM
 *
 * @example
 * OH49E_Instance hall;
 * OH49E_Init(&hall, ADC_CH_PA2, 3.3f, 3.3f);
 */
OH49E_Status OH49E_Init(OH49E_Instance* hall, ADC_Channel adc_channel,
                         float vcc, float vref);

/**
 * @brief อ่านค่า ADC raw (0–1023)
 * @param hall pointer ไปยัง OH49E_Instance
 * @return ค่า ADC raw
 */
uint16_t OH49E_ReadRaw(OH49E_Instance* hall);

/**
 * @brief อ่านแรงดัน output (V)
 * @param hall pointer ไปยัง OH49E_Instance
 * @return แรงดัน (V)
 */
float OH49E_ReadVoltage(OH49E_Instance* hall);

/**
 * @brief ตรวจสอบว่ามีสนามแม่เหล็กหรือไม่ (เทียบกับ threshold)
 * @param hall pointer ไปยัง OH49E_Instance
 * @return 1 = มีสนาม, 0 = ไม่มีสนาม
 */
uint8_t OH49E_IsFieldDetected(OH49E_Instance* hall);

/**
 * @brief ตรวจทิศทางของสนามแม่เหล็ก
 * @param hall pointer ไปยัง OH49E_Instance
 * @return OH49E_FIELD_NONE / NORTH / SOUTH
 */
OH49E_FieldDir OH49E_GetFieldDirection(OH49E_Instance* hall);

/**
 * @brief อ่านความเข้มสนามแม่เหล็ก normalize (0.0 – 1.0)
 * @details 0.0 = quiescent (ไม่มีสนาม), 1.0 = ความเข้มสูงสุด (ไม่ว่าจะขั้วใด)
 * @param hall pointer ไปยัง OH49E_Instance
 * @return ค่า 0.0f – 1.0f
 */
float OH49E_GetFieldStrength(OH49E_Instance* hall);

/**
 * @brief ตั้ง threshold สำหรับ IsFieldDetected
 * @param hall        pointer ไปยัง OH49E_Instance
 * @param threshold_v dead-band (V) รอบ quiescent point
 */
void OH49E_SetThreshold(OH49E_Instance* hall, float threshold_v);

#ifdef __cplusplus
}
#endif

#endif  /* __OH49E_H */
