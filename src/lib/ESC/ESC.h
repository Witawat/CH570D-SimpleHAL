/**
 * @file ESC.h
 * @brief Electronic Speed Controller (ESC) Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับควบคุม Brushless DC Motor (BLDC) ผ่าน ESC (Electronic Speed Controller)
 * ใช้ PWM 50Hz มาตรฐาน (1000-2000µs) เหมือน Servo
 *
 * **หลักการทำงาน:**
 * - ESC รับสัญญาณ PWM 50Hz (period 20ms)
 * - 1000µs = มอเตอร์หยุด (0% throttle)
 * - 1500µs = มอเตอร์หมุนครึ่งกำลัง (50% throttle)
 * - 2000µs = มอเตอร์หมุนเต็มกำลัง (100% throttle)
 * - Arming sequence: ส่ง 1000µs ค้างไว้ ~2 วินาที → ESC พร้อมทำงาน
 * - Calibration: ส่ง 2000µs → รอ → ส่ง 1000µs (standard ESC calibration)
 *
 * **⚠️ ความปลอดภัย:**
 * - ต้อง Arm ก่อนใช้งานทุกครั้ง
 * - ถอด propeller ออกก่อน calibrate
 * - ESC บางรุ่นมี反向保护 (reverse protection)
 *
 * **Hardware Connection:**
 * ```
 *   ESC                CH57x
 *   Signal (White) -->  PWM pin (PD2, PA1, PC3, PC4, PD4, PD3, PC0, PD7)
 *   GND (Black) ----->  GND
 *   Power (Red) ----->  Battery (LiPo 2S-6S)
 * ```
 *
 * @example
 * ESC_Instance esc;
 * ESC_Init(&esc, PWM1_CH1, 1000, 2000);
 * ESC_Arm(&esc);
 * Delay_Ms(3000);  // รอ ESC arming
 *
 * ESC_SetThrottle(&esc, 50);  // 50% throttle
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __ESC_H
#define __ESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief Default pulse for 0% throttle (µs) */
#ifndef ESC_PULSE_MIN_US
#define ESC_PULSE_MIN_US  1000
#endif

/** @brief Default pulse for 100% throttle (µs) */
#ifndef ESC_PULSE_MAX_US
#define ESC_PULSE_MAX_US  2000
#endif

/** @brief ESC PWM frequency */
#ifndef ESC_PWM_FREQ
#define ESC_PWM_FREQ      50
#endif

/** @brief Arming timeout (ms) */
#ifndef ESC_ARM_TIMEOUT_MS
#define ESC_ARM_TIMEOUT_MS  3000
#endif

/** @brief Max ESC instances */
#ifndef ESC_MAX_INSTANCES
#define ESC_MAX_INSTANCES  4
#endif

/* ========== Type Definitions ========== */

/**
 * @brief ESC status codes
 */
typedef enum {
    ESC_OK          = 0,
    ESC_ERROR_PARAM = 1,
    ESC_NOT_ARMED   = 2
} ESC_Status;

/**
 * @brief ESC instance
 */
typedef struct {
    PWM_Channel channel;        /**< PWM channel */
    uint16_t    pulse_min_us;   /**< Pulse for 0% throttle */
    uint16_t    pulse_max_us;   /**< Pulse for 100% throttle */
    uint8_t     current_pct;    /**< Current throttle (0-100) */
    uint16_t    current_pulse;  /**< Current pulse (µs) */
    uint8_t     armed;          /**< 1 = armed, 0 = disarmed */
    uint8_t     initialized;    /**< Init flag */
} ESC_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น ESC instance
 * @param esc          pointer ไปยัง ESC_Instance
 * @param channel      PWM channel
 * @param pulse_min_us pulse สำหรับ 0% throttle (µs) — default ESC_PULSE_MIN_US
 * @param pulse_max_us pulse สำหรับ 100% throttle (µs) — default ESC_PULSE_MAX_US
 *
 * @example
 * ESC_Instance esc;
 * ESC_Init(&esc, PWM1_CH1, 1000, 2000);
 */
void ESC_Init(ESC_Instance* esc, PWM_Channel channel, uint16_t pulse_min_us, uint16_t pulse_max_us);

/**
 * @brief Arm ESC (ส่ง arming sequence)
 * @param esc pointer ไปยัง ESC_Instance
 *
 * @details ส่ง 0% throttle (pulse_min_us) ค้างไว้ ~3 วินาที
 *          หลังจากนั้น ESC จะส่งเสียง beep ยืนยัน
 *
 * @note ต้องเรียกก่อน SetThrottle ทุกครั้ง
 *
 * @example
 * ESC_Arm(&esc);
 * Delay_Ms(3000);  // รอให้ arming เสร็จ
 */
void ESC_Arm(ESC_Instance* esc);

/**
 * @brief ตั้งค่า throttle (0-100%)
 * @param esc      pointer ไปยัง ESC_Instance
 * @param throttle 0 = หยุด, 50 = ครึ่ง, 100 = เต็มกำลัง
 *
 * @note ต้อง Arm ก่อน
 *
 * @example
 * ESC_SetThrottle(&esc, 50);  // 50% throttle
 */
void ESC_SetThrottle(ESC_Instance* esc, uint8_t throttle);

/**
 * @brief ตั้งค่า throttle ด้วย pulse width โดยตรง
 * @param esc      pointer ไปยัง ESC_Instance
 * @param pulse_us pulse width (µs) — ปกติ 1000-2000
 *
 * @example
 * ESC_SetThrottleMicroseconds(&esc, 1500);  // กลาง
 */
void ESC_SetThrottleMicroseconds(ESC_Instance* esc, uint16_t pulse_us);

/**
 * @brief Calibrate ESC (auto-calibration sequence)
 * @param esc pointer ไปยัง ESC_Instance
 *
 * @details Sequence:
 *          1. ส่ง max throttle → รอ → เสียบแบต → ESC beep
 *          2. ส่ง min throttle → รอ → ESC beep ยืนยัน
 *
 * @warning ถอด propeller ออกก่อน calibrate!
 *
 * @example
 * ESC_Calibrate(&esc);
 * // ทำตามขั้นตอน: รอ beep → ลด throttle → รอ beep
 */
void ESC_Calibrate(ESC_Instance* esc);

/**
 * @brief หยุดมอเตอร์ (set throttle = 0%)
 * @param esc pointer ไปยัง ESC_Instance
 */
void ESC_Stop(ESC_Instance* esc);

/**
 * @brief Disarm ESC (กลับสู่สถานะปลอดภัย)
 * @param esc pointer ไปยัง ESC_Instance
 */
void ESC_Disarm(ESC_Instance* esc);

/**
 * @brief เช็คว่า ESC armed หรือยัง
 * @param esc pointer ไปยัง ESC_Instance
 * @return 1 = armed, 0 = not armed
 */
uint8_t ESC_IsArmed(ESC_Instance* esc);

#ifdef __cplusplus
}
#endif

#endif /* __ESC_H */
