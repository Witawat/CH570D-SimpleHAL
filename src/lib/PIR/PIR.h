/**
 * @file SimplePIR.h
 * @brief Simple PIR Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library นี้ควบคุม PIR Motion Sensors (NS312, RCWL-0516)
 * พร้อมเทคนิคขั้นต้นถึงขั้นสูง
 * 
 * **คุณสมบัติ:**
 * - รองรับ NS312 และ RCWL-0516
 * - Debounce ป้องกันสัญญาณกระเด้ง
 * - LED interference prevention (blanking window)
 * - Continuous motion detection
 * - State machine สำหรับการตรวจจับที่ซับซ้อน
 * - Callback system สำหรับ events
 * - Advanced filtering techniques
 * 
 * **PIR Sensors:**
 * - NS312: Digital output, 3.3V-5V, detection range ~7m
 * - RCWL-0516: Microwave radar, 3.3V-5V, detection range ~5-7m
 * 
 * @example
 * #include "PIR.h"
 * 
 * void on_motion_detected(void) {
 *     printf("Motion detected!\r\n");
 * }
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     
 *     // เริ่มต้น PIR บน PC4
 *     PIR_Init(PC4);
 *     PIR_SetDebounce(150);
 *     PIR_SetCallback(on_motion_detected, NULL);
 *     
 *     while(1) {
 *         PIR_Update();
 *     }
 * }
 * 
 * @note ต้องเรียก Timer_Init() ก่อนใช้งาน library นี้
 */

#ifndef __PIR_H
#define __PIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

#define PIR_MAX_INSTANCES  4  /**< จำนวน PIR sensors สูงสุด */

/* ========== Enumerations ========== */

/**
 * @brief PIR Detection States
 */
typedef enum {
    PIR_STATE_IDLE = 0,           /**< ไม่มีการเคลื่อนไหว */
    PIR_STATE_MOTION_DETECTED,    /**< ตรวจพบการเคลื่อนไหว (ครั้งแรก) */
    PIR_STATE_MOTION_ACTIVE,      /**< กำลังมีการเคลื่อนไหว */
    PIR_STATE_MOTION_TIMEOUT,     /**< รอ timeout หลังหยุดเคลื่อนไหว */
    PIR_STATE_MOTION_END          /**< การเคลื่อนไหวสิ้นสุด */
} PIR_State;

/**
 * @brief PIR Filter Levels
 */
typedef enum {
    PIR_FILTER_NONE = 0,     /**< ไม่มีการกรอง */
    PIR_FILTER_LOW,          /**< กรองน้อย (2 samples) */
    PIR_FILTER_MEDIUM,       /**< กรองปานกลาง (4 samples) */
    PIR_FILTER_HIGH          /**< กรองมาก (8 samples) */
} PIR_FilterLevel;

/**
 * @brief PIR Detection Modes
 */
typedef enum {
    PIR_MODE_SINGLE = 0,     /**< ตรวจจับครั้งเดียว */
    PIR_MODE_CONTINUOUS      /**< ตรวจจับต่อเนื่อง */
} PIR_Mode;

/* ========== Structures ========== */

/**
 * @brief PIR Configuration Structure
 */
typedef struct {
    uint8_t pin;                    /**< GPIO pin (PC0-PC7, PD2-PD7) */
    uint16_t debounce_ms;           /**< Debounce time (ms) */
    uint16_t blanking_window_ms;    /**< LED blanking window (ms) */
    uint16_t timeout_ms;            /**< Continuous detection timeout (ms) */
    PIR_FilterLevel filter_level;   /**< Filter level */
    PIR_Mode mode;                  /**< Detection mode */
    bool led_protection_enabled;    /**< LED interference protection */
} PIR_Config;

/**
 * @brief PIR Instance Structure
 */
typedef struct {
    // Configuration
    PIR_Config config;
    
    // State tracking
    PIR_State state;
    bool current_value;
    bool last_stable_value;
    
    // Timing
    uint32_t last_change_time;
    uint32_t motion_start_time;
    uint32_t motion_end_time;
    uint32_t last_blanking_time;
    
    // Filtering
    uint8_t filter_buffer[8];
    uint8_t filter_index;
    uint8_t filter_count;
    
    // Callbacks
    void (*on_motion_start)(void);
    void (*on_motion_end)(void);
    void (*on_motion_active)(void);
    
    // Flags
    bool initialized;
    bool in_blanking_window;
} PIR_Instance;

/* ========== Function Prototypes ========== */

/* === Initialization === */

/**
 * @brief เริ่มต้นการใช้งาน PIR sensor
 * @param pin GPIO pin (PC0-PC7, PD2-PD7)
 * @return ตัวชี้ไปยัง PIR instance หรือ NULL ถ้าเต็ม
 * 
 * @note ต้องเรียก Timer_Init() ก่อนใช้งาน
 * 
 * @example
 * PIR_Instance* pir = PIR_Init(PC4);
 */
PIR_Instance* PIR_Init(uint8_t pin);

/**
 * @brief เริ่มต้น PIR พร้อม configuration
 * @param config ตัวชี้ไปยัง configuration structure
 * @return ตัวชี้ไปยัง PIR instance หรือ NULL ถ้าเต็ม
 * 
 * @example
 * PIR_Config cfg = {
 *     .pin = PC4,
 *     .debounce_ms = 150,
 *     .blanking_window_ms = 200,
 *     .timeout_ms = 5000,
 *     .filter_level = PIR_FILTER_MEDIUM,
 *     .mode = PIR_MODE_CONTINUOUS,
 *     .led_protection_enabled = true
 * };
 * PIR_Instance* pir = PIR_InitWithConfig(&cfg);
 */
PIR_Instance* PIR_InitWithConfig(PIR_Config* config);

/* === Configuration === */

/**
 * @brief ตั้งค่า debounce time
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param ms เวลา debounce (milliseconds)
 * 
 * @note ค่า default = 150ms
 * 
 * @example
 * PIR_SetDebounce(pir, 200);  // 200ms debounce
 */
void PIR_SetDebounce(PIR_Instance* pir, uint16_t ms);

/**
 * @brief ตั้งค่า blanking window สำหรับป้องกันการรบกวนจาก LED
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param ms เวลา blanking window (milliseconds)
 * 
 * @note ค่า default = 200ms
 * @note ใช้ร่วมกับ PIR_TriggerBlankingWindow()
 * 
 * @example
 * PIR_SetBlankingWindow(pir, 300);  // 300ms blanking
 */
void PIR_SetBlankingWindow(PIR_Instance* pir, uint16_t ms);

/**
 * @brief ตั้งค่า timeout สำหรับ continuous detection
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param ms เวลา timeout (milliseconds)
 * 
 * @note ค่า default = 5000ms (5 วินาที)
 * 
 * @example
 * PIR_SetTimeout(pir, 10000);  // 10 วินาที
 */
void PIR_SetTimeout(PIR_Instance* pir, uint16_t ms);

/**
 * @brief ตั้งค่า filter level
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param level ระดับการกรอง
 * 
 * @example
 * PIR_SetFilterLevel(pir, PIR_FILTER_HIGH);
 */
void PIR_SetFilterLevel(PIR_Instance* pir, PIR_FilterLevel level);

/**
 * @brief ตั้งค่า detection mode
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param mode โหมดการตรวจจับ
 * 
 * @example
 * PIR_SetMode(pir, PIR_MODE_CONTINUOUS);
 */
void PIR_SetMode(PIR_Instance* pir, PIR_Mode mode);

/**
 * @brief เปิด/ปิดการป้องกันการรบกวนจาก LED
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param enabled true = เปิดใช้งาน, false = ปิดใช้งาน
 * 
 * @example
 * PIR_EnableLEDProtection(pir, true);
 */
void PIR_EnableLEDProtection(PIR_Instance* pir, bool enabled);

/* === Callbacks === */

/**
 * @brief ตั้งค่า callback functions
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param on_start ฟังก์ชันที่เรียกเมื่อเริ่มตรวจพบการเคลื่อนไหว
 * @param on_end ฟังก์ชันที่เรียกเมื่อการเคลื่อนไหวสิ้นสุด
 * 
 * @example
 * void motion_start(void) { printf("Motion started\r\n"); }
 * void motion_end(void) { printf("Motion ended\r\n"); }
 * PIR_SetCallback(pir, motion_start, motion_end);
 */
void PIR_SetCallback(PIR_Instance* pir, void (*on_start)(void), void (*on_end)(void));

/**
 * @brief ตั้งค่า callback สำหรับ motion active (เรียกซ้ำขณะมีการเคลื่อนไหว)
 * @param pir ตัวชี้ไปยัง PIR instance
 * @param on_active ฟังก์ชันที่เรียกขณะมีการเคลื่อนไหว
 * 
 * @example
 * void motion_active(void) { printf("Motion active\r\n"); }
 * PIR_SetActiveCallback(pir, motion_active);
 */
void PIR_SetActiveCallback(PIR_Instance* pir, void (*on_active)(void));

/* === Core Functions === */

/**
 * @brief อัพเดทสถานะของ PIR (เรียกใน main loop)
 * @param pir ตัวชี้ไปยัง PIR instance
 * 
 * @note ต้องเรียกฟังก์ชันนี้ใน main loop เพื่อให้ library ทำงาน
 * 
 * @example
 * while(1) {
 *     PIR_Update(pir);
 *     // other code
 * }
 */
void PIR_Update(PIR_Instance* pir);

/**
 * @brief อ่านค่า PIR ปัจจุบัน (หลังผ่าน debounce และ filter)
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return true = ตรวจพบการเคลื่อนไหว, false = ไม่พบ
 * 
 * @example
 * if(PIR_Read(pir)) {
 *     printf("Motion detected!\r\n");
 * }
 */
bool PIR_Read(PIR_Instance* pir);

/**
 * @brief อ่านค่า PIR แบบ raw (ไม่ผ่าน debounce/filter)
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return true = HIGH, false = LOW
 * 
 * @example
 * bool raw = PIR_ReadRaw(pir);
 */
bool PIR_ReadRaw(PIR_Instance* pir);

/* === State Query === */

/**
 * @brief อ่านสถานะปัจจุบัน
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return สถานะปัจจุบัน
 * 
 * @example
 * PIR_State state = PIR_GetState(pir);
 * if(state == PIR_STATE_MOTION_ACTIVE) {
 *     // มีการเคลื่อนไหว
 * }
 */
PIR_State PIR_GetState(PIR_Instance* pir);

/**
 * @brief ตรวจสอบว่ามีการเคลื่อนไหวหรือไม่
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return true = มีการเคลื่อนไหว, false = ไม่มี
 * 
 * @example
 * if(PIR_IsMotionDetected(pir)) {
 *     // มีการเคลื่อนไหว
 * }
 */
bool PIR_IsMotionDetected(PIR_Instance* pir);

/**
 * @brief อ่านระยะเวลาที่ตรวจพบการเคลื่อนไหว
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return ระยะเวลา (milliseconds)
 * 
 * @example
 * uint32_t duration = PIR_GetMotionDuration(pir);
 * printf("Motion duration: %lu ms\r\n", duration);
 */
uint32_t PIR_GetMotionDuration(PIR_Instance* pir);

/**
 * @brief อ่านเวลาที่ผ่านไปนับจากการเคลื่อนไหวครั้งล่าสุด
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return เวลาที่ผ่านไป (milliseconds)
 * 
 * @example
 * uint32_t elapsed = PIR_GetTimeSinceLastMotion(pir);
 */
uint32_t PIR_GetTimeSinceLastMotion(PIR_Instance* pir);

/* === LED Interference Prevention === */

/**
 * @brief เรียกใช้ blanking window (เมื่อ LED เปิด/ปิด)
 * @param pir ตัวชี้ไปยัง PIR instance
 * 
 * @note เรียกฟังก์ชันนี้ทันทีหลังจาก LED เปิด/ปิด
 * @note PIR จะไม่อ่านค่าในช่วง blanking window
 * 
 * @example
 * digitalWrite(LED_PIN, HIGH);  // เปิด LED
 * PIR_TriggerBlankingWindow(pir);  // เริ่ม blanking
 */
void PIR_TriggerBlankingWindow(PIR_Instance* pir);

/**
 * @brief ตรวจสอบว่าอยู่ใน blanking window หรือไม่
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return true = อยู่ใน blanking window, false = ไม่อยู่
 * 
 * @example
 * if(PIR_IsInBlankingWindow(pir)) {
 *     // PIR กำลังถูก blank
 * }
 */
bool PIR_IsInBlankingWindow(PIR_Instance* pir);

/* === Advanced Functions === */

/**
 * @brief รีเซ็ตสถานะของ PIR
 * @param pir ตัวชี้ไปยัง PIR instance
 * 
 * @example
 * PIR_Reset(pir);
 */
void PIR_Reset(PIR_Instance* pir);

/**
 * @brief อ่านค่าจาก filter buffer
 * @param pir ตัวชี้ไปยัง PIR instance
 * @return ค่าที่กรองแล้ว (0-255)
 * 
 * @note ใช้สำหรับ debugging
 * 
 * @example
 * uint8_t filtered = PIR_GetFilteredValue(pir);
 */
uint8_t PIR_GetFilteredValue(PIR_Instance* pir);

/* === Utility Functions === */

/**
 * @brief อัพเดท PIR instances ทั้งหมด
 * 
 * @note เรียกฟังก์ชันนี้ใน main loop แทน PIR_Update() แต่ละตัว
 * 
 * @example
 * while(1) {
 *     PIR_UpdateAll();
 * }
 */
void PIR_UpdateAll(void);

/**
 * @brief หา PIR instance จาก pin
 * @param pin GPIO pin
 * @return ตัวชี้ไปยัง PIR instance หรือ NULL ถ้าไม่พบ
 * 
 * @example
 * PIR_Instance* pir = PIR_GetInstanceByPin(PC4);
 */
PIR_Instance* PIR_GetInstanceByPin(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif  // __PIR_H
