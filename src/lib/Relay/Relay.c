/**
 * @file Relay.c
 * @brief Relay Control Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "Relay.h"

/* ========== Private Helpers ========== */

/**
 * @brief ตั้งค่า GPIO pin และ state ของ relay ตาม active_level
 *
 * @param relay - ตัวชี้ไปยัง Relay_Instance
 * @param on - 1 = ON, 0 = OFF
 *
 * @note Active High: ON → pin HIGH, OFF → pin LOW
 * @note Active Low:  ON → pin LOW,  OFF → pin HIGH
 * @note อัปเดต relay->state ด้วย
 */
static void _apply(Relay_Instance* relay, uint8_t on) {
    uint8_t pin_state;
    if (relay->active_level == RELAY_ACTIVE_HIGH) {
        pin_state = on ? HIGH : LOW;
    } else {
        pin_state = on ? LOW : HIGH;
    }
    digitalWrite(relay->pin, pin_state);
    relay->state = on;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น relay
 *
 * @param relay - pointer ไปยัง Relay_Instance (NULL → return RELAY_ERROR_PARAM)
 * @param pin - GPIO pin ที่ต่อกับ IN ของ relay module
 * @param level - RELAY_ACTIVE_HIGH หรือ RELAY_ACTIVE_LOW
 * @return Relay_Status - RELAY_OK หรือ RELAY_ERROR_PARAM
 *
 * @note ตั้ง GPIO pin mode เป็น OUTPUT
 * @note ปิด relay ทันทีตอนเริ่มต้น (_apply(relay, 0))
 * @note ต้องเรียกก่อนใช้งานฟังก์ชันอื่น ๆ
 */
Relay_Status Relay_Init(Relay_Instance* relay, uint8_t pin, Relay_ActiveLevel level) {
    if (relay == NULL) return RELAY_ERROR_PARAM;

    relay->pin          = pin;
    relay->active_level = level;
    relay->state        = 0;
    relay->initialized  = 0;

    pinMode(pin, PIN_MODE_OUTPUT);

    /* ปิด relay ตอนเริ่มต้น */
    _apply(relay, 0);

    relay->initialized = 1;
    return RELAY_OK;
}

/**
 * @brief เปิด relay (ON)
 *
 * @param relay - pointer ไปยัง Relay_Instance (NULL หรือ !initialized → return ทันที)
 *
 * @note ใช้ _apply() เพื่อ set output pin ตาม active_level
 * @note active_level == RELAY_ACTIVE_HIGH → pin HIGH = ON
 * @note active_level == RELAY_ACTIVE_LOW  → pin LOW  = ON
 */
void Relay_On(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, 1);
}

/**
 * @brief ปิด relay (OFF)
 *
 * @param relay - pointer ไปยัง Relay_Instance (NULL หรือ !initialized → return ทันที)
 *
 * @note ใช้ _apply() เพื่อ set output pin ตาม active_level
 * @note active_level == RELAY_ACTIVE_HIGH → pin LOW  = OFF
 * @note active_level == RELAY_ACTIVE_LOW  → pin HIGH = OFF
 */
void Relay_Off(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, 0);
}

/**
 * @brief สลับสถานะ relay
 *
 * @param relay - pointer ไปยัง Relay_Instance (NULL หรือ !initialized → return ทันที)
 *
 * @note อ่าน relay->state ปัจจุบัน: ถ้า ON → OFF, ถ้า OFF → ON
 * @note ใช้ _apply() เพื่อ set output pin
 */
void Relay_Toggle(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, relay->state ? 0 : 1);
}

/**
 * @brief ตั้งสถานะ relay โดยตรง
 *
 * @param relay - pointer ไปยัง Relay_Instance (NULL หรือ !initialized → return ทันที)
 * @param state - 1 = ON, 0 = OFF
 *
 * @note state จะถูกแปลงเป็น 0 หรือ 1 ก่อนส่งให้ _apply()
 */
void Relay_Set(Relay_Instance* relay, uint8_t state) {
    if (relay == NULL || !relay->initialized) return;
    _apply(relay, state ? 1 : 0);
}

/**
 * @brief ตรวจสอบว่า relay เปิดอยู่หรือไม่
 *
 * @param relay - pointer ไปยัง Relay_Instance (NULL หรือ !initialized → return 0)
 * @return uint8_t - 1 = ON, 0 = OFF
 *
 * @note อ่านจาก relay->state (ไม่ใช่ pin state จริง)
 */
uint8_t Relay_IsOn(Relay_Instance* relay) {
    if (relay == NULL || !relay->initialized) return 0;
    return relay->state;
}
