/**
 * @file WaterFlow.h
 * @brief YF-S201 Water Flow Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับวัดอัตราการไหลของน้ำด้วย YF-S201 Hall Effect Flow Sensor
 * ใช้ GPIO interrupt นับ pulse จาก hall sensor
 *
 * **หลักการทำงาน:**
 * - YF-S201 มี hall effect sensor + ใบพัดภายใน
 * - เมื่อน้ำไหลผ่าน → ใบพัดหมุน → hall sensor สร้าง pulse
 * - K-factor: ~450 pulses / ลิตร (ขึ้นกับแรงดันน้ำ)
 * - Library ใช้ GPIO interrupt (EXTI) นับจำนวน pulse
 *
 * **Hardware Connection:**
 * ```
 *   YF-S201            CH57x
 *   Red   (VCC) ---->  3.3V - 5V
 *   Black (GND) ---->  GND
 *   Yellow(SIG) ---->  GPIO pin (ต้องเป็น EXTI pin)
 * ```
 *
 * **K-Factor (Pulses per Liter):**
 * - YF-S201: ~450 (default)
 * - YF-B1:   ~660
 * - YF-B3:   ~834
 * - YF-B5:   ~450
 * - ปรับค่าได้ด้วย WaterFlow_SetKFactor()
 *
 * @example
 * WaterFlow_Instance flow;
 * WaterFlow_Init(&flow, PC0, 450.0f);
 *
 * while(1) {
 *     float lpm = WaterFlow_GetFlowRate(&flow);
 *     float liters = WaterFlow_GetTotalVolume(&flow);
 *     printf("Flow: %.2f L/min, Total: %.2f L\n", lpm, liters);
 *     Delay_Ms(1000);
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __WATERFLOW_H
#define __WATERFLOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default K-factor (pulses per liter) for YF-S201 */
#ifndef WATERFLOW_DEFAULT_KFACTOR
#define WATERFLOW_DEFAULT_KFACTOR  450.0f
#endif

/** @brief Maximum flow sensors supported */
#ifndef WATERFLOW_MAX_INSTANCES
#define WATERFLOW_MAX_INSTANCES  4
#endif

/* ========== Type Definitions ========== */

/**
 * @brief WaterFlow status codes
 */
typedef enum {
    WATERFLOW_OK          = 0,
    WATERFLOW_ERROR_PARAM = 1,
    WATERFLOW_ERROR_FULL  = 2
} WaterFlow_Status;

/**
 * @brief Callback เมื่อมีการเปลี่ยนแปลงของ pulse count
 * @param instance pointer ไปยัง WaterFlow_Instance
 */
typedef void (*WaterFlow_Callback)(void* instance);

/**
 * @brief WaterFlow instance
 */
typedef struct {
    uint8_t     pin;            /**< GPIO pin (ต้องเป็น EXTI pin) */
    float       k_factor;       /**< K-factor (pulses per liter) */
    volatile uint32_t pulse_count;  /**< จำนวน pulse ทั้งหมด (interrupt-safe) */
    uint32_t    last_pulse;     /**< pulse count ครั้งก่อน (ใช้คำนวณ flow rate) */
    uint32_t    last_time_ms;   /**< timestamp ครั้งล่าสุดที่อ่าน flow rate */
    uint8_t     initialized;    /**< Init flag */
} WaterFlow_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น YF-S201 Water Flow Sensor
 * @param flow     pointer ไปยัง WaterFlow_Instance
 * @param pin      GPIO pin ที่ต่อกับขา SIG (ต้องเป็น EXTI pin)
 * @param k_factor K-factor (pulses per liter) — ใช้ WATERFLOW_DEFAULT_KFACTOR สำหรับ YF-S201
 * @return WATERFLOW_OK, WATERFLOW_ERROR_PARAM, หรือ WATERFLOW_ERROR_FULL
 *
 * @note ต้องเป็น pin ที่รองรับ EXTI interrupt (PC0, PC1, PC2, PD0, PD1, ฯลฯ)
 *       ดู datasheet CH57x สำหรับ EXTI pin list
 *
 * @example
 * WaterFlow_Instance flow;
 * WaterFlow_Init(&flow, PC0, 450.0f);
 */
WaterFlow_Status WaterFlow_Init(WaterFlow_Instance* flow, uint8_t pin, float k_factor);

/**
 * @brief อ่านจำนวน pulse ที่นับได้ทั้งหมด
 * @param flow pointer ไปยัง WaterFlow_Instance
 * @return จำนวน pulse ทั้งหมด (uint32)
 *
 * @note thread-safe — สามารถเรียกจาก main loop ได้แม้ ISR กำลังอัปเดต
 */
uint32_t WaterFlow_GetPulseCount(WaterFlow_Instance* flow);

/**
 * @brief คำนวณอัตราการไหล (L/min)
 * @param flow pointer ไปยัง WaterFlow_Instance
 * @return อัตราการไหล หน่วย L/min
 *
 * @details ใช้ค่า delta pulse ระหว่างครั้งนี้กับครั้งก่อนหารด้วยเวลาที่ผ่านไป
 *          ครั้งแรกที่เรียกจะคืนค่า 0 (ต้องรอ pulse สะสมก่อน)
 *
 * @example
 * float lpm = WaterFlow_GetFlowRate(&flow);
 */
float WaterFlow_GetFlowRate(WaterFlow_Instance* flow);

/**
 * @brief คำนวณปริมาณน้ำทั้งหมดที่ไหลผ่าน (L)
 * @param flow pointer ไปยัง WaterFlow_Instance
 * @return ปริมาณน้ำทั้งหมด หน่วย ลิตร
 */
float WaterFlow_GetTotalVolume(WaterFlow_Instance* flow);

/**
 * @brief รีเซ็ตค่า pulse count กลับเป็น 0
 * @param flow pointer ไปยัง WaterFlow_Instance
 */
void WaterFlow_Reset(WaterFlow_Instance* flow);

/**
 * @brief ตั้งค่า K-factor ใหม่ (กรณีใช้ flow sensor รุ่นอื่น)
 * @param flow     pointer ไปยัง WaterFlow_Instance
 * @param k_factor K-factor (pulses per liter)
 *
 * @example
 * WaterFlow_SetKFactor(&flow, 660.0f);  // YF-B1
 */
void WaterFlow_SetKFactor(WaterFlow_Instance* flow, float k_factor);

#ifdef __cplusplus
}
#endif

#endif /* __WATERFLOW_H */
