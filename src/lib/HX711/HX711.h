/**
 * @file HX711.h
 * @brief HX711 24-bit ADC Load Cell Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านค่าน้ำหนักจาก Load Cell ผ่าน HX711 24-bit ADC
 * HX711 ใช้โปรโตคอล serial แบบ 2 สาย (DOUT + SCK) ที่ไม่ใช่ I2C/SPI
 *
 * **คุณสมบัติ:**
 * - อ่านค่า ADC 24-bit (raw)
 * - Tare (ตั้งศูนย์)
 * - Calibration factor (แปลงค่า raw → กรัม/กิโลกรัม)
 * - เลือก Gain: 128 (CH-A, default), 64 (CH-A), 32 (CH-B)
 * - Power Down / Power Up
 * - ตรวจสอบว่าพร้อมอ่านหรือยัง (DOUT=LOW)
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          HX711
 *   GPIO (any) -----> PD_SCK  (clock)
 *   GPIO (any) <----- DOUT    (data)
 *   3.3V ──────────> VCC
 *   GND  ──────────> GND
 *
 *   Load Cell (4 สาย):
 *   สีแดง (E+)  → E+  (Excitation+)
 *   สีดำ (E-)   → E-  (Excitation-)
 *   สีขาว (A-)  → A-  (Signal-)
 *   สีเขียว(A+) → A+  (Signal+)
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "HX711.h"
 *
 * HX711_Instance scale;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     HX711_Init(&scale, PIN_PD4, PIN_PD3);  // DOUT=PD4, SCK=PD3
 *
 *     // ตั้งศูนย์ (วัตถุอยู่บน scale, กด tare)
 *     HX711_Tare(&scale, 10);
 *
 *     // Calibration: วางน้ำหนัก 500g แล้ว อ่านค่า raw
 *     // HX711_SetCalibration(&scale, raw_500g / 500.0f);
 *
 *     while (1) {
 *         float weight = HX711_GetWeight(&scale);
 *         printf("Weight: %.1f g\r\n", weight);
 *         Delay_Ms(500);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __HX711_H
#define __HX711_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/** @brief จำนวน samples สูงสุดสำหรับ average */
#ifndef HX711_MAX_SAMPLES
#define HX711_MAX_SAMPLES  20
#endif

/** @brief timeout รอ DOUT=LOW (ms) */
#ifndef HX711_READY_TIMEOUT_MS
#define HX711_READY_TIMEOUT_MS  1000
#endif

/* ========== Type Definitions ========== */

/**
 * @brief HX711 Gain / Channel selection
 */
typedef enum {
    HX711_GAIN_128 = 1, /**< Channel A, Gain 128 (default, 25 pulses) */
    HX711_GAIN_32  = 2, /**< Channel B, Gain 32  (26 pulses) */
    HX711_GAIN_64  = 3  /**< Channel A, Gain 64  (27 pulses) */
} HX711_Gain;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    HX711_OK           = 0, /**< สำเร็จ */
    HX711_ERROR_PARAM  = 1, /**< Parameter ผิด (NULL pointer หรือไม่ได้ Init) */
    HX711_ERROR_TIMEOUT = 2  /**< รอ DOUT=LOW นานเกินกำหนด */
} HX711_Status;

/**
 * @brief HX711 Instance
 */
typedef struct {
    GPIO_Pin   pin_dout;           /**< GPIO pin เชื่อมต่อกับ HX711 DOUT */
    GPIO_Pin   pin_sck;            /**< GPIO pin เชื่อมต่อกับ HX711 PD_SCK */
    HX711_Gain gain;               /**< Gain ที่ใช้งาน */
    int32_t    tare_offset;        /**< ค่า offset หลัง tare */
    float      calibration_factor; /**< ค่า calibration: raw_units_per_gram */
    uint8_t    initialized;        /**< flag บอกว่า Init แล้ว */
} HX711_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น HX711
 * @param hx       ตัวแปร instance
 * @param pin_dout GPIO pin เชื่อมต่อกับ DOUT
 * @param pin_sck  GPIO pin เชื่อมต่อกับ PD_SCK
 * @return HX711_OK หรือ error code
 *
 * @note Default gain = 128 (CH-A), tare = 0, calibration = 1.0
 * @example HX711_Init(&scale, PIN_PD4, PIN_PD3);
 */
HX711_Status HX711_Init(HX711_Instance* hx, GPIO_Pin pin_dout, GPIO_Pin pin_sck);

/**
 * @brief ตรวจสอบว่า HX711 พร้อมอ่านหรือยัง
 * @param hx ตัวแปร instance
 * @return 1 = พร้อม (DOUT=LOW), 0 = ยังไม่พร้อม
 *
 * @note ควรเรียกก่อน HX711_Read() ถ้าต้องการ non-blocking
 */
uint8_t HX711_IsReady(HX711_Instance* hx);

/**
 * @brief อ่านค่า ADC 24-bit แบบ raw
 * @param hx     ตัวแปร instance
 * @param result pointer สำหรับรับค่า (signed 24-bit, sign-extended เป็น int32_t)
 * @return HX711_OK หรือ error code
 *
 * @note รอให้ DOUT=LOW ก่อน (blocking จนถึง HX711_READY_TIMEOUT_MS)
 * @note ใช้ __disable_irq() ระหว่างอ่านเพื่อ timing ที่แม่นยำ
 */
HX711_Status HX711_Read(HX711_Instance* hx, int32_t* result);

/**
 * @brief ตั้งศูนย์ (Tare) — อ่านค่าเฉลี่ยหลายครั้งและเก็บเป็น offset
 * @param hx      ตัวแปร instance
 * @param samples จำนวน samples สำหรับเฉลี่ย (1-HX711_MAX_SAMPLES)
 * @return HX711_OK หรือ error code
 *
 * @note อย่าวางสิ่งของบน scale ระหว่าง tare
 * @example HX711_Tare(&scale, 10);  // เฉลี่ย 10 ครั้ง
 */
HX711_Status HX711_Tare(HX711_Instance* hx, uint8_t samples);

/**
 * @brief ตั้งค่า calibration factor
 * @param hx     ตัวแปร instance
 * @param factor จำนวน raw units ต่อ 1 กรัม
 *
 * @details วิธีหา calibration factor:
 *   1. เรียก HX711_Tare() ก่อน
 *   2. วางน้ำหนักที่รู้ค่า (เช่น 500g)
 *   3. อ่านค่า raw: HX711_Read(&hx, &raw)
 *   4. factor = raw / 500.0f
 *
 * @example
 * int32_t raw;
 * HX711_Read(&scale, &raw);
 * HX711_SetCalibration(&scale, (float)raw / 500.0f);  // น้ำหนัก 500g
 */
void HX711_SetCalibration(HX711_Instance* hx, float factor);

/**
 * @brief อ่านน้ำหนักในหน่วย gram
 * @param hx     ตัวแปร instance
 * @param weight pointer สำหรับรับน้ำหนัก (gram)
 * @return HX711_OK หรือ error code
 *
 * @note ค่าจะถูกหัก tare_offset และหาร calibration_factor
 * @example
 * float w;
 * HX711_GetWeight(&scale, &w);
 * printf("%.1f g\r\n", w);
 */
HX711_Status HX711_GetWeight(HX711_Instance* hx, float* weight);

/**
 * @brief อ่านน้ำหนักเฉลี่ยหลายครั้ง
 * @param hx      ตัวแปร instance
 * @param samples จำนวน samples (1-HX711_MAX_SAMPLES)
 * @param weight  pointer สำหรับรับน้ำหนักเฉลี่ย (gram)
 * @return HX711_OK หรือ error code
 */
HX711_Status HX711_GetWeightAvg(HX711_Instance* hx, uint8_t samples, float* weight);

/**
 * @brief เปลี่ยน Gain
 * @param hx   ตัวแปร instance
 * @param gain HX711_GAIN_128, HX711_GAIN_64, หรือ HX711_GAIN_32
 * @return HX711_OK หรือ error code
 *
 * @note Gain ใหม่จะมีผลในการอ่านครั้งถัดไป (ต้องอ่าน 1 ครั้งเพื่อ apply)
 */
HX711_Status HX711_SetGain(HX711_Instance* hx, HX711_Gain gain);

/**
 * @brief Power Down HX711 (ประหยัดไฟ)
 * @param hx ตัวแปร instance
 * @return HX711_OK หรือ error code
 *
 * @note ทำโดยตั้ง SCK=HIGH นานกว่า 60µs
 */
HX711_Status HX711_PowerDown(HX711_Instance* hx);

/**
 * @brief Power Up HX711
 * @param hx ตัวแปร instance
 * @return HX711_OK หรือ error code
 *
 * @note ทำโดยตั้ง SCK=LOW (chip จะ reset และเริ่มใหม่)
 */
HX711_Status HX711_PowerUp(HX711_Instance* hx);

/**
 * @brief แปลง HX711_Status เป็น string
 * @param st สถานะ
 * @return เช่น "OK", "TIMEOUT"
 */
const char* HX711_StatusStr(HX711_Status st);

#ifdef __cplusplus
}
#endif

#endif /* __HX711_H */
