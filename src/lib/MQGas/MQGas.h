/**
 * @file MQGas.h
 * @brief MQ Gas Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านค่าก๊าซจาก MQ Series Gas Sensors ผ่าน ADC
 * รองรับ MQ2, MQ3, MQ4, MQ5, MQ6, MQ7, MQ9, MQ135 และ sensor ทั่วไป
 *
 * **หลักการทำงาน:**
 * - MQ sensor เป็น Chemo-resistive sensor
 * - ค่าความต้านทาน Rs เปลี่ยนแปลงตามความเข้มข้นของก๊าซ
 * - ต้อง Calibrate ค่า Ro (ความต้านทานใน clean air) ก่อนใช้
 * - ค่า PPM คำนวณจากกราฟใน datasheet: PPM = A × (Rs/Ro)^B
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          MQ Sensor
 *   ADC pin (PA2) <-- AOUT (Analog output)
 *   GND ------------> GND
 *   5V  ------------> VCC  (⚠️ MQ ต้องการ 5V สำหรับ heater)
 *
 *   ตัวแบ่งแรงดัน (ถ้า MCU 3.3V):
 *   AOUT ──[10kΩ]── ADC pin
 *              │
 *             [10kΩ]
 *              │
 *             GND
 *   → แรงดัน 5V ÷ 2 = 2.5V (ปลอดภัยสำหรับ 3.3V MCU)
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "MQGas.h"
 *
 * MQGas_Instance mq2;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     ADC_SimpleInit();
 *
 *     MQGas_Init(&mq2, ADC_CH_PA2, MQ_TYPE_MQ2, 5.0f, 3.3f);
 *
 *     // Calibrate ใน clean air (รอ warmup 2-5 นาทีก่อน)
 *     MQGas_Calibrate(&mq2, 50);
 *
 *     while (1) {
 *         float ppm = MQGas_GetPPM(&mq2);
 *         printf("Gas: %.1f PPM\r\n", ppm);
 *         Delay_Ms(500);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __MQGAS_H
#define __MQGAS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/** @brief ค่า load resistance บน module (kΩ) */
#ifndef MQGAS_RL_KOHM
#define MQGAS_RL_KOHM   10.0f
#endif

/** @brief จำนวน samples สูงสุดสำหรับ calibration */
#ifndef MQGAS_MAX_CALIB_SAMPLES
#define MQGAS_MAX_CALIB_SAMPLES  100
#endif

/* ========== Type Definitions ========== */

/**
 * @brief ประเภท MQ Sensor พร้อม default curve parameters
 */
typedef enum {
    MQ_TYPE_GENERIC = 0, /**< Generic sensor — ต้องตั้ง A, B เอง */
    MQ_TYPE_MQ2     = 1, /**< MQ2 — LPG, Propane, Hydrogen (A=574.25, B=-2.222) */
    MQ_TYPE_MQ3     = 2, /**< MQ3 — Alcohol/Ethanol (A=0.3934, B=-1.504) */
    MQ_TYPE_MQ4     = 3, /**< MQ4 — Methane/CNG (A=1012.7, B=-2.786) */
    MQ_TYPE_MQ5     = 4, /**< MQ5 — LPG, Natural Gas (A=1163.8, B=-3.874) */
    MQ_TYPE_MQ6     = 5, /**< MQ6 — LPG, Butane (A=2127.2, B=-2.526) */
    MQ_TYPE_MQ7     = 6, /**< MQ7 — Carbon Monoxide CO (A=99.042, B=-1.518) */
    MQ_TYPE_MQ9     = 7, /**< MQ9 — CO + Flammable Gas (A=1000.5, B=-2.186) */
    MQ_TYPE_MQ135   = 8  /**< MQ135 — Air Quality (A=110.47, B=-2.862) */
} MQGas_Type;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    MQGAS_OK              = 0,
    MQGAS_ERROR_PARAM     = 1,
    MQGAS_ERROR_NOT_CALIB = 2  /**< ยังไม่ได้ calibrate */
} MQGas_Status;

/**
 * @brief MQGas Instance
 */
typedef struct {
    ADC_Channel adc_ch;       /**< ADC channel ที่ต่อกับ AOUT */
    MQGas_Type  type;         /**< ประเภท sensor */
    float       vcc;          /**< ไฟเลี้ยง sensor (V) — ปกติ 5.0 */
    float       vref;         /**< ADC reference voltage (V) — ปกติ 3.3 */
    float       rl;           /**< Load resistance (kΩ) */
    float       ro;           /**< Clean air resistance (kΩ) — จาก calibration */
    float       curve_a;      /**< Curve coefficient A (PPM = A × (Rs/Ro)^B) */
    float       curve_b;      /**< Curve coefficient B */
    float       threshold;    /**< ค่า threshold สำหรับ alarm (PPM) */
    uint8_t     is_calibrated;/**< 1 = calibrate แล้ว */
    uint8_t     initialized;  /**< flag บอกว่า Init แล้ว */
} MQGas_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น MQ Gas Sensor
 * @param mq     ตัวแปร instance
 * @param ch     ADC channel ที่ต่อกับ AOUT ของ sensor
 * @param type   ประเภท sensor (MQ_TYPE_MQ2 ฯลฯ)
 * @param vcc    ไฟเลี้ยง sensor (V) — ปกติ 5.0
 * @param vref   ADC reference voltage (V) — ปกติ 3.3
 * @return MQGAS_OK หรือ error code
 *
 * @note ต้องเรียก ADC_SimpleInit() ก่อน
 * @example MQGas_Init(&mq2, ADC_CH_PA2, MQ_TYPE_MQ2, 5.0f, 3.3f);
 */
MQGas_Status MQGas_Init(MQGas_Instance* mq, ADC_Channel ch,
                          MQGas_Type type, float vcc, float vref);

/**
 * @brief ตั้งค่า curve parameters สำหรับ MQ_TYPE_GENERIC
 * @param mq ตัวแปร instance
 * @param a  Coefficient A ใน PPM = A × (Rs/Ro)^B
 * @param b  Coefficient B (ค่าลบ)
 */
void MQGas_SetCurve(MQGas_Instance* mq, float a, float b);

/**
 * @brief Calibrate ค่า Ro ใน clean air
 * @param mq      ตัวแปร instance
 * @param samples จำนวน samples สำหรับเฉลี่ย (1-100)
 * @return MQGAS_OK หรือ error code
 *
 * @note **ต้อง warmup sensor 2-5 นาที ก่อน calibrate**
 * @note ต้องอยู่ใน clean air (ไม่มีก๊าซผิดปกติ) ระหว่าง calibrate
 * @example
 * printf("Warming up...\r\n");
 * Delay_Ms(120000);  // 2 minutes
 * MQGas_Calibrate(&mq, 50);
 */
MQGas_Status MQGas_Calibrate(MQGas_Instance* mq, uint8_t samples);

/**
 * @brief ตั้ง Ro โดยตรง (ถ้าทราบค่าจาก datasheet หรือ calibration ก่อนหน้า)
 * @param mq ตัวแปร instance
 * @param ro Ro (kΩ)
 */
void MQGas_SetRo(MQGas_Instance* mq, float ro);

/**
 * @brief อ่านค่า ADC raw
 * @param mq ตัวแปร instance
 * @return ค่า ADC (0-1023)
 */
uint16_t MQGas_ReadRaw(MQGas_Instance* mq);

/**
 * @brief อ่านค่าแรงดัน AOUT
 * @param mq ตัวแปร instance
 * @return แรงดัน (V)
 */
float MQGas_ReadVoltage(MQGas_Instance* mq);

/**
 * @brief คำนวณ Rs จาก ADC
 * @param mq ตัวแปร instance
 * @return Rs (kΩ)
 */
float MQGas_GetRs(MQGas_Instance* mq);

/**
 * @brief คำนวณ PPM ของก๊าซที่ sensor ตรวจจับ
 * @param mq ตัวแปร instance
 * @return ค่า PPM หรือ -1.0f ถ้ายังไม่ calibrate
 *
 * @example
 * float ppm = MQGas_GetPPM(&mq2);
 * printf("Gas: %.1f PPM\r\n", ppm);
 */
float MQGas_GetPPM(MQGas_Instance* mq);

/**
 * @brief ตั้ง threshold alarm (PPM)
 * @param mq        ตัวแปร instance
 * @param threshold ค่า PPM ที่ถือว่าอันตราย
 */
void MQGas_SetThreshold(MQGas_Instance* mq, float threshold);

/**
 * @brief ตรวจสอบว่าค่า PPM เกิน threshold หรือเปล่า
 * @param mq ตัวแปร instance
 * @return 1 = เกิน threshold (อันตราย), 0 = ปกติ
 */
uint8_t MQGas_IsAlarm(MQGas_Instance* mq);

#ifdef __cplusplus
}
#endif

#endif /* __MQGAS_H */
