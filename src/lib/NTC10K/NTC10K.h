/**
 * @file NTC10K.h
 * @brief NTC 10K Thermistor Temperature Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-22
 * 
 * @details
 * Library นี้ใช้สำหรับวัดอุณหภูมิด้วย NTC 10K thermistor
 * รองรับทั้ง Steinhart-Hart equation (แม่นยำ) และ Beta equation (เร็ว)
 * 
 * **คุณสมบัติ:**
 * - รองรับ NTC 10K (B-value 3950, 3435, หรือกำหนดเอง)
 * - Steinhart-Hart equation สำหรับความแม่นยำสูง
 * - Beta equation สำหรับความเร็ว
 * - Calibration system
 * - Averaging และ filtering
 * - Multi-sensor support (สูงสุด 4 sensors)
 * - Temperature monitoring พร้อม callback
 * 
 * **Voltage Divider Circuit:**
 * ```
 * NTC-to-GND Configuration:
 *   VCC (3.3V)
 *      |
 *    [R_series 10K]
 *      |
 *      +----> ADC Pin
 *      |
 *    [NTC 10K]
 *      |
 *     GND
 * 
 * NTC-to-VCC Configuration:
 *   VCC (3.3V)
 *      |
 *    [NTC 10K]
 *      |
 *      +----> ADC Pin
 *      |
 *    [R_series 10K]
 *      |
 *     GND
 * ```
 * 
 * @example
 * #include "NTC10K.h"
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     
 *     // เริ่มต้น NTC sensor บน PD2
 *     NTC_Instance* ntc = NTC_Init(ADC_CH_PD2);
 *     
 *     while(1) {
 *         // อ่านอุณหภูมิ
 *         float temp = NTC_ReadTemperature(ntc);
 *         printf("Temperature: %.2f C\r\n", temp);
 *         
 *         Delay_Ms(1000);
 *     }
 * }
 * 
 * @note ต้องเรียก SystemCoreClockUpdate() ก่อนใช้งาน
 */

#ifndef __NTC10K_H
#define __NTC10K_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ========== Configuration ========== */

#define NTC_MAX_INSTANCES  4  /**< จำนวน NTC sensors สูงสุด */

/* ========== Default Values ========== */

#define NTC_DEFAULT_R_SERIES    10000.0f  /**< Series resistor (Ω) */
#define NTC_DEFAULT_R_NOMINAL   10000.0f  /**< NTC resistance at 25°C (Ω) */
#define NTC_DEFAULT_T_NOMINAL   25.0f     /**< Nominal temperature (°C) */
#define NTC_DEFAULT_B_VALUE     3950.0f   /**< B-value (K) */
#define NTC_DEFAULT_VCC         3.3f      /**< Supply voltage (V) */

/* Steinhart-Hart coefficients for NTC 10K B3950 */
#define NTC_SH_A    0.001129148f
#define NTC_SH_B    0.000234125f
#define NTC_SH_C    0.0000000876741f

/* Temperature limits */
#define NTC_TEMP_MIN    -40.0f   /**< Minimum temperature (°C) */
#define NTC_TEMP_MAX    125.0f   /**< Maximum temperature (°C) */

/* ========== Enumerations ========== */

/**
 * @brief Temperature Calculation Method
 */
typedef enum {
    NTC_METHOD_BETA = 0,        /**< Beta equation (เร็ว, แม่นยำปานกลาง) */
    NTC_METHOD_STEINHART_HART   /**< Steinhart-Hart equation (ช้ากว่า, แม่นยำสูง) */
} NTC_CalculationMethod;

/**
 * @brief Voltage Divider Configuration
 */
typedef enum {
    NTC_DIVIDER_NTC_TO_GND = 0,  /**< NTC ต่อลง GND, R_series ต่อขึ้น VCC */
    NTC_DIVIDER_NTC_TO_VCC       /**< NTC ต่อขึ้น VCC, R_series ต่อลง GND */
} NTC_DividerConfig;

/* ========== Structures ========== */

/**
 * @brief NTC Configuration Structure
 */
typedef struct {
    ADC_Channel adc_channel;           /**< ADC channel */
    float r_series;                    /**< Series resistor (Ω) */
    float r_nominal;                   /**< NTC resistance at T_nominal (Ω) */
    float t_nominal;                   /**< Nominal temperature (°C) */
    float b_value;                     /**< B-value (K) */
    float vcc;                         /**< Supply voltage (V) */
    NTC_CalculationMethod method;      /**< Calculation method */
    NTC_DividerConfig divider_config;  /**< Divider configuration */
    
    // Steinhart-Hart coefficients (ถ้าใช้ method นี้)
    float sh_a;                        /**< Steinhart-Hart coefficient A */
    float sh_b;                        /**< Steinhart-Hart coefficient B */
    float sh_c;                        /**< Steinhart-Hart coefficient C */
} NTC_Config;

/**
 * @brief NTC Instance Structure
 */
typedef struct {
    // Configuration
    NTC_Config config;
    
    // Calibration
    float calibration_offset;          /**< Temperature offset (°C) */
    float calibration_gain;            /**< Temperature gain multiplier */
    
    // Monitoring
    float threshold_high;              /**< High temperature threshold (°C) */
    float threshold_low;               /**< Low temperature threshold (°C) */
    bool threshold_enabled;            /**< Threshold monitoring enabled */
    
    // Callbacks
    void (*on_threshold_high)(float temp);  /**< Callback เมื่อเกิน threshold_high */
    void (*on_threshold_low)(float temp);   /**< Callback เมื่อต่ำกว่า threshold_low */
    
    // State
    float last_temperature;            /**< อุณหภูมิล่าสุด (°C) */
    bool initialized;                  /**< Initialization flag */
} NTC_Instance;

/* ========== Function Prototypes ========== */

/* === Initialization === */

/**
 * @brief เริ่มต้นการใช้งาน NTC sensor แบบง่าย
 * @param adc_channel ADC channel ที่ต่อกับ NTC
 * @return ตัวชี้ไปยัง NTC instance หรือ NULL ถ้าเต็ม
 * 
 * @note ใช้ค่า default: R_series=10K, B=3950, NTC-to-GND, Beta equation
 * 
 * @example
 * NTC_Instance* ntc = NTC_Init(ADC_CH_PD2);
 */
NTC_Instance* NTC_Init(ADC_Channel adc_channel);

/**
 * @brief เริ่มต้น NTC พร้อม configuration
 * @param config ตัวชี้ไปยัง configuration structure
 * @return ตัวชี้ไปยัง NTC instance หรือ NULL ถ้าเต็ม
 * 
 * @example
 * NTC_Config cfg = {
 *     .adc_channel = ADC_CH_PD2,
 *     .r_series = 10000.0f,
 *     .r_nominal = 10000.0f,
 *     .t_nominal = 25.0f,
 *     .b_value = 3950.0f,
 *     .vcc = 3.3f,
 *     .method = NTC_METHOD_STEINHART_HART,
 *     .divider_config = NTC_DIVIDER_NTC_TO_GND,
 *     .sh_a = NTC_SH_A,
 *     .sh_b = NTC_SH_B,
 *     .sh_c = NTC_SH_C
 * };
 * NTC_Instance* ntc = NTC_InitWithConfig(&cfg);
 */
NTC_Instance* NTC_InitWithConfig(NTC_Config* config);

/* === Core Functions === */

/**
 * @brief อ่านอุณหภูมิ (°C)
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @return อุณหภูมิ (°C) หรือ NAN ถ้าผิดพลาด
 * 
 * @example
 * float temp = NTC_ReadTemperature(ntc);
 * if (!isnan(temp)) {
 *     printf("Temperature: %.2f C\r\n", temp);
 * }
 */
float NTC_ReadTemperature(NTC_Instance* ntc);

/**
 * @brief อ่านอุณหภูมิ (°F)
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @return อุณหภูมิ (°F) หรือ NAN ถ้าผิดพลาด
 * 
 * @example
 * float temp_f = NTC_ReadTemperatureF(ntc);
 */
float NTC_ReadTemperatureF(NTC_Instance* ntc);

/**
 * @brief อ่านค่าความต้านทานของ NTC
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @return ความต้านทาน (Ω) หรือ NAN ถ้าผิดพลาด
 * 
 * @example
 * float resistance = NTC_ReadResistance(ntc);
 * printf("NTC Resistance: %.2f ohms\r\n", resistance);
 */
float NTC_ReadResistance(NTC_Instance* ntc);

/**
 * @brief อ่านค่า ADC raw
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @return ค่า ADC (0-1023)
 * 
 * @example
 * uint16_t adc = NTC_ReadADC(ntc);
 */
uint16_t NTC_ReadADC(NTC_Instance* ntc);

/* === Advanced Functions === */

/**
 * @brief อ่านอุณหภูมิแบบ average หลายครั้ง
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param samples จำนวนครั้งที่ต้องการอ่าน
 * @return อุณหภูมิเฉลี่ย (°C) หรือ NAN ถ้าผิดพลาด
 * 
 * @note ใช้สำหรับลด noise
 * 
 * @example
 * float temp = NTC_ReadAverage(ntc, 10);  // อ่าน 10 ครั้งแล้วหาค่าเฉลี่ย
 */
float NTC_ReadAverage(NTC_Instance* ntc, uint8_t samples);

/**
 * @brief ตั้งค่า calibration offset
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param offset ค่า offset (°C)
 * 
 * @note Temperature_calibrated = Temperature_raw + offset
 * 
 * @example
 * NTC_SetCalibration(ntc, -2.5f);  // ลดอุณหภูมิ 2.5°C
 */
void NTC_SetCalibration(NTC_Instance* ntc, float offset);

/**
 * @brief ตั้งค่า calibration แบบ offset และ gain
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param offset ค่า offset (°C)
 * @param gain ค่า gain multiplier
 * 
 * @note Temperature_calibrated = (Temperature_raw * gain) + offset
 * 
 * @example
 * NTC_SetCalibrationAdvanced(ntc, -1.0f, 1.02f);
 */
void NTC_SetCalibrationAdvanced(NTC_Instance* ntc, float offset, float gain);

/**
 * @brief ตั้งค่า B-value
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param b_value B-value (K)
 * 
 * @example
 * NTC_SetBValue(ntc, 3435.0f);  // สำหรับ NTC B3435
 */
void NTC_SetBValue(NTC_Instance* ntc, float b_value);

/**
 * @brief ตั้งค่า Steinhart-Hart coefficients
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param a Coefficient A
 * @param b Coefficient B
 * @param c Coefficient C
 * 
 * @example
 * NTC_SetCoefficients(ntc, 0.001129148f, 0.000234125f, 0.0000000876741f);
 */
void NTC_SetCoefficients(NTC_Instance* ntc, float a, float b, float c);

/**
 * @brief เปลี่ยนวิธีการคำนวณ
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param method วิธีการคำนวณ
 * 
 * @example
 * NTC_SetMethod(ntc, NTC_METHOD_STEINHART_HART);
 */
void NTC_SetMethod(NTC_Instance* ntc, NTC_CalculationMethod method);

/* === Monitoring Functions === */

/**
 * @brief ตั้งค่าขีดจำกัดอุณหภูมิ
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param low อุณหภูมิต่ำสุด (°C)
 * @param high อุณหภูมิสูงสุด (°C)
 * 
 * @example
 * NTC_SetThreshold(ntc, 20.0f, 30.0f);  // เตือนถ้าต่ำกว่า 20°C หรือสูงกว่า 30°C
 */
void NTC_SetThreshold(NTC_Instance* ntc, float low, float high);

/**
 * @brief ตั้งค่า callback functions
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param on_high ฟังก์ชันที่เรียกเมื่ออุณหภูมิเกิน threshold_high
 * @param on_low ฟังก์ชันที่เรียกเมื่ออุณหภูมิต่ำกว่า threshold_low
 * 
 * @example
 * void temp_high(float t) { printf("High: %.2f\r\n", t); }
 * void temp_low(float t) { printf("Low: %.2f\r\n", t); }
 * NTC_SetCallback(ntc, temp_high, temp_low);
 */
void NTC_SetCallback(NTC_Instance* ntc, void (*on_high)(float), void (*on_low)(float));

/**
 * @brief เปิด/ปิดการตรวจสอบขีดจำกัด
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param enabled true = เปิด, false = ปิด
 * 
 * @example
 * NTC_EnableThreshold(ntc, true);
 */
void NTC_EnableThreshold(NTC_Instance* ntc, bool enabled);

/**
 * @brief อัพเดทสถานะของ NTC (เรียกใน main loop)
 * @param ntc ตัวชี้ไปยัง NTC instance
 * 
 * @note ต้องเรียกฟังก์ชันนี้ใน main loop เพื่อให้ threshold monitoring ทำงาน
 * 
 * @example
 * while(1) {
 *     NTC_Update(ntc);
 *     // other code
 * }
 */
void NTC_Update(NTC_Instance* ntc);

/* === Utility Functions === */

/**
 * @brief แปลง °C เป็น °F
 * @param celsius อุณหภูมิ (°C)
 * @return อุณหภูมิ (°F)
 * 
 * @example
 * float f = NTC_CelsiusToFahrenheit(25.0f);  // 77.0°F
 */
float NTC_CelsiusToFahrenheit(float celsius);

/**
 * @brief แปลง °F เป็น °C
 * @param fahrenheit อุณหภูมิ (°F)
 * @return อุณหภูมิ (°C)
 * 
 * @example
 * float c = NTC_FahrenheitToCelsius(77.0f);  // 25.0°C
 */
float NTC_FahrenheitToCelsius(float fahrenheit);

/**
 * @brief ตรวจสอบว่าค่าอุณหภูมิที่อ่านได้ถูกต้อง
 * @param ntc ตัวชี้ไปยัง NTC instance
 * @param temperature อุณหภูมิที่ต้องการตรวจสอบ
 * @return true = ถูกต้อง, false = ผิดปกติ
 * 
 * @example
 * float temp = NTC_ReadTemperature(ntc);
 * if (NTC_IsValid(ntc, temp)) {
 *     // ใช้ค่าได้
 * }
 */
bool NTC_IsValid(NTC_Instance* ntc, float temperature);

/**
 * @brief อัพเดท NTC instances ทั้งหมด
 * 
 * @note เรียกฟังก์ชันนี้ใน main loop แทน NTC_Update() แต่ละตัว
 * 
 * @example
 * while(1) {
 *     NTC_UpdateAll();
 * }
 */
void NTC_UpdateAll(void);

/**
 * @brief หา NTC instance จาก ADC channel
 * @param adc_channel ADC channel
 * @return ตัวชี้ไปยัง NTC instance หรือ NULL ถ้าไม่พบ
 * 
 * @example
 * NTC_Instance* ntc = NTC_GetInstanceByChannel(ADC_CH_PD2);
 */
NTC_Instance* NTC_GetInstanceByChannel(ADC_Channel adc_channel);

#ifdef __cplusplus
}
#endif

#endif  // __NTC10K_H
