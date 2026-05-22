/**
 * @file RotaryEncoder.h
 * @brief Rotary Encoder KY-040 Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-22
 * 
 * @details
 * Library สำหรับควบคุม Rotary Encoder KY-040 แบบครบวงจร
 * รองรับการหมุน (quadrature decoding) และการกดปุ่ม
 * 
 * **คุณสมบัติ:**
 * - Interrupt-based quadrature decoding
 * - Position counter (unlimited range)
 * - Direction detection (CW/CCW)
 * - Button support (press, long press, double click)
 * - Acceleration mode
 * - Debouncing อัตโนมัติ
 * - Callback system
 * - Min/Max limits
 * 
 * **Hardware:**
 * - KY-040 Rotary Encoder Module
 * - CLK (A) และ DT (B): ต้องเป็น pins ที่รองรับ EXTI
 * - SW (Button): pin ใดก็ได้
 * - VCC: 3.3V-5V
 * - มี pull-up resistor ในตัว
 * 
 * @example
 * // ตัวอย่างการใช้งาน
 * #include "SimpleHAL.h"
 * #include "RotaryEncoder.h"
 * 
 * RotaryEncoder encoder;
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     
 *     // Init encoder (CLK=PC1, DT=PC2, SW=PC3)
 *     Rotary_Init(&encoder, PC1, PC2, PC3);
 *     
 *     while(1) {
 *         int32_t pos = Rotary_GetPosition(&encoder);
 *         printf("Position: %ld\n", pos);
 *         Delay_Ms(100);
 *     }
 * }
 */

#ifndef __ROTARY_ENCODER_H
#define __ROTARY_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"  // IWYU pragma: keep
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief Default debounce time (milliseconds)
 */
#define ROTARY_DEFAULT_DEBOUNCE_MS      5

/**
 * @brief Long press threshold (milliseconds)
 */
#define ROTARY_LONG_PRESS_MS            500

/**
 * @brief Double click threshold (milliseconds)
 */
#define ROTARY_DOUBLE_CLICK_MS          300

/**
 * @brief Acceleration threshold (RPM)
 */
#define ROTARY_ACCEL_THRESHOLD_RPM      60

/* ========== Enumerations ========== */

/**
 * @brief Rotation direction
 */
typedef enum {
    ROTARY_DIR_NONE = 0,    /**< No rotation */
    ROTARY_DIR_CW,          /**< Clockwise */
    ROTARY_DIR_CCW          /**< Counter-clockwise */
} RotaryDirection;

/**
 * @brief Rotary encoder events
 */
typedef enum {
    ROTARY_EVENT_NONE = 0,          /**< No event */
    ROTARY_EVENT_ROTATE_CW,         /**< Rotated clockwise */
    ROTARY_EVENT_ROTATE_CCW,        /**< Rotated counter-clockwise */
    ROTARY_EVENT_BUTTON_PRESS,      /**< Button pressed */
    ROTARY_EVENT_BUTTON_RELEASE,    /**< Button released */
    ROTARY_EVENT_BUTTON_LONG_PRESS, /**< Button long press */
    ROTARY_EVENT_BUTTON_DOUBLE_CLICK /**< Button double click */
} RotaryEvent;

/* ========== Structures ========== */

/**
 * @brief Rotary encoder configuration and state
 */
typedef struct {
    // Pin configuration
    uint8_t pin_clk;        /**< CLK pin (A) */
    uint8_t pin_dt;         /**< DT pin (B) */
    uint8_t pin_sw;         /**< SW pin (Button) */
    
    // Position tracking
    volatile int32_t position;      /**< Current position */
    volatile int32_t last_position; /**< Last position */
    int32_t min_position;           /**< Minimum position (if limited) */
    int32_t max_position;           /**< Maximum position (if limited) */
    bool use_limits;                /**< Enable position limits */
    
    // Direction tracking
    volatile RotaryDirection direction; /**< Last rotation direction */
    volatile uint8_t last_state;        /**< Last encoder state (2-bit) */
    
    // Debouncing
    uint16_t debounce_ms;           /**< Debounce time (ms) */
    volatile uint32_t last_change_time; /**< Last state change time */
    
    // Acceleration
    bool acceleration_enabled;      /**< Enable acceleration mode */
    volatile uint32_t last_rotation_time; /**< Last rotation timestamp */
    volatile uint8_t acceleration_factor;  /**< Current acceleration factor */
    
    // Button state
    volatile bool button_pressed;       /**< Current button state */
    volatile bool button_last_state;    /**< Last button state */
    volatile uint32_t button_press_time;   /**< Button press timestamp */
    volatile uint32_t button_release_time; /**< Button release timestamp */
    volatile uint8_t click_count;          /**< Click counter for double click */
    
    // Callbacks
    void (*on_rotate)(int32_t position, RotaryDirection direction);
    void (*on_button_press)(void);
    void (*on_button_release)(void);
    void (*on_button_long_press)(void);
    void (*on_button_double_click)(void);
    
} RotaryEncoder;

/* ========== Function Prototypes ========== */

/* === Initialization === */

/**
 * @brief เริ่มต้น Rotary Encoder
 * @param encoder Pointer to RotaryEncoder structure
 * @param pin_clk CLK pin (A) - ต้องรองรับ EXTI
 * @param pin_dt DT pin (B) - ต้องรองรับ EXTI
 * @param pin_sw SW pin (Button) - pin ใดก็ได้
 * 
 * @note Pins จะถูกตั้งเป็น INPUT_PULLUP อัตโนมัติ
 * @note Interrupts จะถูกเปิดใช้งานสำหรับ CLK และ DT
 * 
 * @example
 * RotaryEncoder encoder;
 * Rotary_Init(&encoder, PC1, PC2, PC3);
 */
void Rotary_Init(RotaryEncoder* encoder, uint8_t pin_clk, uint8_t pin_dt, uint8_t pin_sw);

/**
 * @brief รีเซ็ต encoder state
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @note รีเซ็ตตำแหน่งเป็น 0 และล้าง state ทั้งหมด
 * 
 * @example
 * Rotary_Reset(&encoder);
 */
void Rotary_Reset(RotaryEncoder* encoder);

/* === Position Control === */

/**
 * @brief อ่านตำแหน่งปัจจุบัน
 * @param encoder Pointer to RotaryEncoder structure
 * @return ตำแหน่งปัจจุบัน (signed integer)
 * 
 * @example
 * int32_t pos = Rotary_GetPosition(&encoder);
 * printf("Position: %ld\n", pos);
 */
int32_t Rotary_GetPosition(RotaryEncoder* encoder);

/**
 * @brief ตั้งค่าตำแหน่ง
 * @param encoder Pointer to RotaryEncoder structure
 * @param position ตำแหน่งที่ต้องการ
 * 
 * @example
 * Rotary_SetPosition(&encoder, 0);  // Reset to 0
 * Rotary_SetPosition(&encoder, 50); // Set to 50
 */
void Rotary_SetPosition(RotaryEncoder* encoder, int32_t position);

/**
 * @brief อ่านทิศทางการหมุนล่าสุด
 * @param encoder Pointer to RotaryEncoder structure
 * @return ROTARY_DIR_CW, ROTARY_DIR_CCW, หรือ ROTARY_DIR_NONE
 * 
 * @example
 * RotaryDirection dir = Rotary_GetDirection(&encoder);
 * if(dir == ROTARY_DIR_CW) {
 *     printf("Clockwise\n");
 * }
 */
RotaryDirection Rotary_GetDirection(RotaryEncoder* encoder);

/**
 * @brief ตรวจสอบว่ามีการหมุนหรือไม่
 * @param encoder Pointer to RotaryEncoder structure
 * @return true ถ้าตำแหน่งเปลี่ยนจากครั้งล่าสุด
 * 
 * @example
 * if(Rotary_HasChanged(&encoder)) {
 *     int32_t pos = Rotary_GetPosition(&encoder);
 *     printf("New position: %ld\n", pos);
 * }
 */
bool Rotary_HasChanged(RotaryEncoder* encoder);

/**
 * @brief ตั้งค่าขอบเขตตำแหน่ง (min/max)
 * @param encoder Pointer to RotaryEncoder structure
 * @param min ตำแหน่งต่ำสุด
 * @param max ตำแหน่งสูงสุด
 * 
 * @note ตำแหน่งจะถูกจำกัดอยู่ในช่วง [min, max]
 * 
 * @example
 * // จำกัดตำแหน่งระหว่าง 0-100
 * Rotary_SetLimits(&encoder, 0, 100);
 */
void Rotary_SetLimits(RotaryEncoder* encoder, int32_t min, int32_t max);

/**
 * @brief ยกเลิกการจำกัดตำแหน่ง
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @example
 * Rotary_ClearLimits(&encoder);  // ไม่จำกัดตำแหน่ง
 */
void Rotary_ClearLimits(RotaryEncoder* encoder);

/* === Button Control === */

/**
 * @brief ตรวจสอบว่าปุ่มถูกกดอยู่หรือไม่
 * @param encoder Pointer to RotaryEncoder structure
 * @return true ถ้าปุ่มถูกกด
 * 
 * @example
 * if(Rotary_IsButtonPressed(&encoder)) {
 *     printf("Button pressed!\n");
 * }
 */
bool Rotary_IsButtonPressed(RotaryEncoder* encoder);

/**
 * @brief รอจนกว่าปุ่มจะถูกกด (blocking)
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @example
 * printf("Press button...\n");
 * Rotary_WaitForButton(&encoder);
 * printf("Button pressed!\n");
 */
void Rotary_WaitForButton(RotaryEncoder* encoder);

/**
 * @brief อัปเดต button state (เรียกใน main loop)
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @note ต้องเรียกฟังก์ชันนี้ใน main loop เพื่อตรวจจับ button events
 * @note ควรเรียกบ่อยๆ (ทุก 10-50ms) สำหรับ debouncing ที่ดี
 * 
 * @example
 * while(1) {
 *     Rotary_UpdateButton(&encoder);
 *     Delay_Ms(10);
 * }
 */
void Rotary_UpdateButton(RotaryEncoder* encoder);

/* === Advanced Settings === */

/**
 * @brief ตั้งค่าเวลา debounce
 * @param encoder Pointer to RotaryEncoder structure
 * @param debounce_ms เวลา debounce (milliseconds)
 * 
 * @note ค่าเริ่มต้น: 5ms
 * @note เพิ่มค่าถ้ามีปัญหา bouncing, ลดค่าถ้าต้องการ response เร็วขึ้น
 * 
 * @example
 * Rotary_SetDebounceTime(&encoder, 10);  // 10ms debounce
 */
void Rotary_SetDebounceTime(RotaryEncoder* encoder, uint16_t debounce_ms);

/**
 * @brief เปิด/ปิด acceleration mode
 * @param encoder Pointer to RotaryEncoder structure
 * @param enabled true = เปิด, false = ปิด
 * 
 * @note Acceleration mode: หมุนเร็ว = ค่าเปลี่ยนเร็วขึ้น
 * @note เหมาะสำหรับการปรับค่าในช่วงกว้าง (เช่น 0-1000)
 * 
 * @example
 * Rotary_SetAcceleration(&encoder, true);  // เปิด acceleration
 */
void Rotary_SetAcceleration(RotaryEncoder* encoder, bool enabled);

/* === Callback Functions === */

/**
 * @brief ตั้งค่า callback เมื่อหมุน encoder
 * @param encoder Pointer to RotaryEncoder structure
 * @param callback ฟังก์ชันที่จะถูกเรียก (position, direction)
 * 
 * @example
 * void on_rotate(int32_t pos, RotaryDirection dir) {
 *     printf("Position: %ld, Dir: %d\n", pos, dir);
 * }
 * Rotary_OnRotate(&encoder, on_rotate);
 */
void Rotary_OnRotate(RotaryEncoder* encoder, void (*callback)(int32_t, RotaryDirection));

/**
 * @brief ตั้งค่า callback เมื่อกดปุ่ม
 * @param encoder Pointer to RotaryEncoder structure
 * @param callback ฟังก์ชันที่จะถูกเรียก
 * 
 * @example
 * void on_press(void) {
 *     printf("Button pressed!\n");
 * }
 * Rotary_OnButtonPress(&encoder, on_press);
 */
void Rotary_OnButtonPress(RotaryEncoder* encoder, void (*callback)(void));

/**
 * @brief ตั้งค่า callback เมื่อปล่อยปุ่ม
 * @param encoder Pointer to RotaryEncoder structure
 * @param callback ฟังก์ชันที่จะถูกเรียก
 * 
 * @example
 * void on_release(void) {
 *     printf("Button released!\n");
 * }
 * Rotary_OnButtonRelease(&encoder, on_release);
 */
void Rotary_OnButtonRelease(RotaryEncoder* encoder, void (*callback)(void));

/**
 * @brief ตั้งค่า callback เมื่อกดปุ่มค้าง
 * @param encoder Pointer to RotaryEncoder structure
 * @param callback ฟังก์ชันที่จะถูกเรียก
 * 
 * @note Long press = กดค้างเกิน 500ms
 * 
 * @example
 * void on_long_press(void) {
 *     printf("Long press detected!\n");
 * }
 * Rotary_OnButtonLongPress(&encoder, on_long_press);
 */
void Rotary_OnButtonLongPress(RotaryEncoder* encoder, void (*callback)(void));

/**
 * @brief ตั้งค่า callback เมื่อดับเบิลคลิก
 * @param encoder Pointer to RotaryEncoder structure
 * @param callback ฟังก์ชันที่จะถูกเรียก
 * 
 * @note Double click = กด 2 ครั้งภายใน 300ms
 * 
 * @example
 * void on_double_click(void) {
 *     printf("Double click!\n");
 * }
 * Rotary_OnButtonDoubleClick(&encoder, on_double_click);
 */
void Rotary_OnButtonDoubleClick(RotaryEncoder* encoder, void (*callback)(void));

/* === Utility Functions === */

/**
 * @brief อัปเดต encoder state (internal use)
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @note ฟังก์ชันนี้ถูกเรียกจาก ISR อัตโนมัติ
 * @note ไม่ต้องเรียกด้วยตัวเอง
 */
void Rotary_Update(RotaryEncoder* encoder);

/**
 * @brief Interrupt handler สำหรับ CLK pin (internal use)
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @note ฟังก์ชันนี้ถูกเรียกจาก EXTI ISR
 */
void Rotary_CLK_ISR(RotaryEncoder* encoder);

/**
 * @brief Interrupt handler สำหรับ DT pin (internal use)
 * @param encoder Pointer to RotaryEncoder structure
 * 
 * @note ฟังก์ชันนี้ถูกเรียกจาก EXTI ISR
 */
void Rotary_DT_ISR(RotaryEncoder* encoder);

#ifdef __cplusplus
}
#endif

#endif  // __ROTARY_ENCODER_H
