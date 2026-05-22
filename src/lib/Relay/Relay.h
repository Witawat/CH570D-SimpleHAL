/**
 * @file Relay.h
 * @brief Relay Control Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับควบคุม Relay module รองรับทั้ง Active High และ Active Low
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          Relay Module
 *   GPIO pin -------> IN
 *   3.3V  ----------> VCC  (หรือ 5V ขึ้นอยู่กับ module)
 *   GND   ----------> GND
 * ```
 *
 * @example
 * Relay_Instance relay;
 * Relay_Init(&relay, PC0, RELAY_ACTIVE_LOW);
 * Relay_On(&relay);
 * Delay_Ms(1000);
 * Relay_Off(&relay);
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __RELAY_H
#define __RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Type Definitions ========== */

/**
 * @brief Relay active level
 */
typedef enum {
    RELAY_ACTIVE_HIGH = 0,  /**< Active High — เปิดด้วย HIGH (relay โดยตรง) */
    RELAY_ACTIVE_LOW  = 1   /**< Active Low  — เปิดด้วย LOW  (module ส่วนใหญ่) */
} Relay_ActiveLevel;

/**
 * @brief Relay status codes
 */
typedef enum {
    RELAY_OK          = 0,
    RELAY_ERROR_PARAM = 1
} Relay_Status;

/**
 * @brief Relay instance
 */
typedef struct {
    uint8_t            pin;          /**< GPIO pin */
    Relay_ActiveLevel  active_level; /**< Active High / Low */
    uint8_t            state;        /**< 1 = ON, 0 = OFF */
    uint8_t            initialized;  /**< Init flag */
} Relay_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น relay
 * @param relay   pointer ไปยัง Relay_Instance
 * @param pin     GPIO pin ที่ต่อกับ IN ของ relay module
 * @param level   RELAY_ACTIVE_HIGH หรือ RELAY_ACTIVE_LOW
 * @return RELAY_OK หรือ RELAY_ERROR_PARAM
 *
 * @example
 * Relay_Instance r;
 * Relay_Init(&r, PC0, RELAY_ACTIVE_LOW);
 */
Relay_Status Relay_Init(Relay_Instance* relay, uint8_t pin, Relay_ActiveLevel level);

/**
 * @brief เปิด relay (ON)
 * @param relay pointer ไปยัง Relay_Instance
 */
void Relay_On(Relay_Instance* relay);

/**
 * @brief ปิด relay (OFF)
 * @param relay pointer ไปยัง Relay_Instance
 */
void Relay_Off(Relay_Instance* relay);

/**
 * @brief สลับสถานะ relay
 * @param relay pointer ไปยัง Relay_Instance
 */
void Relay_Toggle(Relay_Instance* relay);

/**
 * @brief ตั้งสถานะ relay โดยตรง
 * @param relay  pointer ไปยัง Relay_Instance
 * @param state  1 = ON, 0 = OFF
 */
void Relay_Set(Relay_Instance* relay, uint8_t state);

/**
 * @brief ตรวจสอบว่า relay เปิดอยู่หรือไม่
 * @param relay pointer ไปยัง Relay_Instance
 * @return 1 = ON, 0 = OFF
 */
uint8_t Relay_IsOn(Relay_Instance* relay);

#ifdef __cplusplus
}
#endif

#endif  /* __RELAY_H */
