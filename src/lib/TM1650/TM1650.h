/**
 * @file TM1650.h
 * @brief TM1650 I2C 7-Segment Display + Key Scan Driver สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-15
 *
 * @details
 * Library สำหรับควบคุม 4-digit 7-segment display และอ่านปุ่มกด
 * ผ่าน IC Driver TM1650 แบบ I2C
 *
 * **คุณสมบัติ:**
 * - I2C communication (SCL, SDA) — ใช้ฮาร์ดแวร์ I2C
 * - รองรับ 4-digit 7-segment หรือ 8-segment display
 * - ควบคุมความสว่าง 8 ระดับ (0-7)
 * - แสดงตัวเลข, เลขฐาน 16, และ raw segments
 * - Key scan สำหรับปุ่มกด (7x2 matrix = สูงสุด 14 ปุ่ม)
 * - Non-blocking key read พร้อม debounce
 * - Long-press detection
 *
 * **Hardware Requirements:**
 * - TM1650 module (เช่น JY-MCU 4-digit 7-segment + key)
 * - I2C: SCL (PC2), SDA (PC1) — default pins
 * - Pull-up resistors 4.7kΩ ที่ SDA, SCL
 * - VCC: 3.3V-5V, GND
 *
 * **I2C Addresses:**
 * ```
 *   TM1650_MC       : ADDR pin → address
 *   LOW  (GND)      : 0x24 (default)
 *   HIGH (VCC)      : 0x34
 *   SCL             : 0x35
 *   SDA             : 0x36
 *   Floating (NC)   : 0x37
 * ```
 *
 * @note ต้องเรียก I2C_SimpleInit() ก่อนใช้งาน TM1650_Init()
 *
 * @example
 * // Basic display usage
 * #include "SimpleHAL.h"
 * #include "TM1650.h"
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     TM1650_Handle disp;
 *     TM1650_Init(&disp, TM1650_I2C_ADDR_DEFAULT);
 *     TM1650_SetBrightness(&disp, 5);
 *     TM1650_DisplayNumber(&disp, 1234, false);
 *
 *     while(1) {}
 * }
 *
 * @example
 * // Display + Key scan
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     TM1650_Handle disp;
 *     TM1650_Init(&disp, 0x24);
 *     TM1650_DisplayNumber(&disp, 0, true);
 *
 *     while(1) {
 *         char key = TM1650_GetKey(&disp);
 *         if (key != 0) {
 *             TM1650_DisplayDigit(&disp, 0, key - '0', false);
 *         }
 *         Delay_Ms(20);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __TM1650_H
#define __TM1650_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration (override via #define before include) ========== */

/** @brief Default I2C address (ADDR = GND) */
#ifndef TM1650_I2C_ADDR_DEFAULT
#define TM1650_I2C_ADDR_DEFAULT  0x24
#endif

/** @brief I2C address when ADDR = VCC */
#define TM1650_I2C_ADDR_VCC      0x34

/** @brief I2C address when ADDR = SCL */
#define TM1650_I2C_ADDR_SCL      0x35

/** @brief I2C address when ADDR = SDA */
#define TM1650_I2C_ADDR_SDA      0x36

/** @brief I2C address when ADDR = floating (NC) */
#define TM1650_I2C_ADDR_FLOAT    0x37

/** @brief Number of grid positions */
#ifndef TM1650_NUM_DIGITS
#define TM1650_NUM_DIGITS        4
#endif

/** @brief Minimum brightness */
#define TM1650_BRIGHTNESS_MIN    0

/** @brief Maximum brightness */
#define TM1650_BRIGHTNESS_MAX    7

/** @brief Key debounce time (ms) */
#ifndef TM1650_KEY_DEBOUNCE_MS
#define TM1650_KEY_DEBOUNCE_MS   30
#endif

/** @brief Key long-press threshold (ms) */
#ifndef TM1650_KEY_LONG_PRESS_MS
#define TM1650_KEY_LONG_PRESS_MS 800
#endif

/* ========== TM1650 Internal Commands ========== */

#define TM1650_CMD_DATA          0x48  /**< Data command base */
#define TM1650_CMD_DISPLAY       0x48  /**< Display control command (same as data) */

#define TM1650_DATA_AUTO         0x40  /**< Auto increment address mode */
#define TM1650_DATA_FIXED        0x44  /**< Fixed address mode */

#define TM1650_DISPLAY_ON        0x01  /**< Display ON flag (bit 3) */
#define TM1650_DISPLAY_OFF       0x00  /**< Display OFF flag */
#define TM1650_MODE_7SEG_BIT     0x10  /**< 7-segment mode flag (bit 4) */

#define TM1650_GRID_START        0x68  /**< Start address for grid 1 data */
#define TM1650_GRID1             0x68
#define TM1650_GRID2             0x6A
#define TM1650_GRID3             0x6C
#define TM1650_GRID4             0x6E

/* ========== Segment Bit Definitions ========== */

/**
 * @brief 7-Segment bit positions
 *
 *      A
 *     ---
 *  F |   | B
 *     -G-
 *  E |   | C
 *     ---
 *      D    DP
 */
#define SEG_A   0x01
#define SEG_B   0x02
#define SEG_C   0x04
#define SEG_D   0x08
#define SEG_E   0x10
#define SEG_F   0x20
#define SEG_G   0x40
#define SEG_DP  0x80  /**< Decimal point */

/* ========== Type Definitions ========== */

/**
 * @brief TM1650 status codes
 */
typedef enum {
    TM1650_OK           = 0,  /**< Success */
    TM1650_ERROR_PARAM  = 1,  /**< Invalid parameter */
    TM1650_ERROR_I2C    = 2   /**< I2C communication error */
} TM1650_Status;

/**
 * @brief Display mode
 */
typedef enum {
    TM1650_MODE_8SEG = 0,  /**< 8 segments × 4 grids (default) */
    TM1650_MODE_7SEG = 1   /**< 7 segments × 4 grids (frees KS lines for key scan) */
} TM1650_Mode;

/**
 * @brief Key mapping for standard TM1650 4-digit module
 *
 * Key layout (typical JY-MCU module):
 * ```
 *   S1  S2  S3  S4
 *   S5  S6  S7  S8
 *   S9 S10 S11 S12
 *   S13 S14 S15 S16  (if module has 16 keys)
 * ```
 */
typedef enum {
    TM1650_KEY_NONE     = 0x00,
    TM1650_KEY_S1       = 0x01,
    TM1650_KEY_S2       = 0x02,
    TM1650_KEY_S3       = 0x04,
    TM1650_KEY_S4       = 0x08,
    TM1650_KEY_S5       = 0x10,
    TM1650_KEY_S6       = 0x20,
    TM1650_KEY_S7       = 0x40,
    TM1650_KEY_S8       = 0x80
} TM1650_KeyCode;

/**
 * @brief TM1650 Handle Structure
 */
typedef struct {
    uint8_t  i2c_addr;                         /**< I2C address (7-bit) */
    uint8_t  brightness;                       /**< Current brightness (0-7) */
    bool     display_on;                       /**< Display on/off state */
    uint8_t  mode;                             /**< Display mode (TM1650_MODE_8SEG or TM1650_MODE_7SEG) */
    uint8_t  buffer[TM1650_NUM_DIGITS];        /**< Segment data per grid */
    uint8_t  raw_keys;                         /**< Last raw key byte read */
    uint8_t  last_key_mask;                    /**< Last debounced key mask */
    uint32_t press_start_ms;                   /**< Timestamp when key was first detected */
    uint8_t  is_pressed;                       /**< 1 = key currently held */
    uint8_t  initialized;                      /**< Init flag */
} TM1650_Handle;

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน TM1650
 *
 * @param handle   Pointer to TM1650_Handle
 * @param i2c_addr I2C address ของ chip (0x24 default)
 * @return TM1650_OK หรือ error code
 *
 * @note ต้องเรียก I2C_SimpleInit() ก่อน
 * @note จะ probe device ผ่าน I2C_IsDeviceReady()
 *
 * @example
 * TM1650_Handle disp;
 * if (TM1650_Init(&disp, 0x24) != TM1650_OK) {
 *     // Handle error
 * }
 */
TM1650_Status TM1650_Init(TM1650_Handle* handle, uint8_t i2c_addr);

/**
 * @brief ตั้งค่าความสว่างของ display
 *
 * @param handle     Pointer to TM1650_Handle
 * @param brightness ระดับความสว่าง (0-7, 0=มืดที่สุด, 7=สว่างที่สุด)
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_SetBrightness(&disp, 5);
 */
TM1650_Status TM1650_SetBrightness(TM1650_Handle* handle, uint8_t brightness);

/**
 * @brief เปิด display
 *
 * @param handle Pointer to TM1650_Handle
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_DisplayOn(&disp);
 */
TM1650_Status TM1650_DisplayOn(TM1650_Handle* handle);

/**
 * @brief ปิด display
 *
 * @param handle Pointer to TM1650_Handle
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_DisplayOff(&disp);
 */
TM1650_Status TM1650_DisplayOff(TM1650_Handle* handle);

/**
 * @brief ล้างหน้าจอทั้งหมด (ทุก digit = ว่าง)
 *
 * @param handle Pointer to TM1650_Handle
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_Clear(&disp);
 */
TM1650_Status TM1650_Clear(TM1650_Handle* handle);

/**
 * @brief ตั้งค่า display mode (8-segment หรือ 7-segment)
 *
 * @param handle Pointer to TM1650_Handle
 * @param mode   TM1650_MODE_8SEG หรือ TM1650_MODE_7SEG
 * @return TM1650_OK หรือ error code
 *
 * @note 7-segment mode จะปล่อย KS1-KS7 สำหรับ key scan
 * @note การเปลี่ยน mode จะล้าง buffer ทั้งหมด
 *
 * @example
 * TM1650_SetMode(&disp, TM1650_MODE_7SEG);
 */
TM1650_Status TM1650_SetMode(TM1650_Handle* handle, TM1650_Mode mode);

/* ========== Display Data Functions ========== */

/**
 * @brief แสดงตัวเลขจำนวนเต็ม
 *
 * @param handle       Pointer to TM1650_Handle
 * @param number       ตัวเลข (-999 ถึง 9999 สำหรับ 4-digit)
 * @param leading_zero true=แสดง 0 นำหน้า, false=ไม่แสดง
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_DisplayNumber(&disp, 42, false);    // แสดง "  42"
 * TM1650_DisplayNumber(&disp, 42, true);     // แสดง "0042"
 * TM1650_DisplayNumber(&disp, -5, false);    // แสดง "  -5"
 */
TM1650_Status TM1650_DisplayNumber(TM1650_Handle* handle, int16_t number, bool leading_zero);

/**
 * @brief แสดงตัวเลขฐาน 16 (Hexadecimal)
 *
 * @param handle       Pointer to TM1650_Handle
 * @param number       ตัวเลข hex (0x0000-0xFFFF สำหรับ 4-digit)
 * @param leading_zero true=แสดง 0 นำหน้า, false=ไม่แสดง
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_DisplayHex(&disp, 0xABCD, true);  // แสดง "ABCD"
 * TM1650_DisplayHex(&disp, 0x00FF, false); // แสดง "  FF"
 */
TM1650_Status TM1650_DisplayHex(TM1650_Handle* handle, uint16_t number, bool leading_zero);

/**
 * @brief แสดงตัวเลข 1 หลักที่ตำแหน่งที่กำหนด
 *
 * @param handle   Pointer to TM1650_Handle
 * @param position ตำแหน่ง (0 = ซ้ายสุด, 3 = ขวาสุด)
 * @param digit    ตัวเลข (0-9)
 * @param show_dp  true=แสดงจุดทศนิยม, false=ไม่แสดง
 * @return TM1650_OK หรือ error code
 *
 * @example
 * TM1650_DisplayDigit(&disp, 0, 1, false);  // แสดง "1" ที่ตำแหน่งแรก
 * TM1650_DisplayDigit(&disp, 2, 5, true);   // แสดง "5." ที่ตำแหน่งที่ 3
 */
TM1650_Status TM1650_DisplayDigit(TM1650_Handle* handle, uint8_t position, uint8_t digit, bool show_dp);

/**
 * @brief ควบคุม segment โดยตรง (สำหรับแสดงตัวอักษรพิเศษ)
 *
 * @param handle   Pointer to TM1650_Handle
 * @param position ตำแหน่ง (0 = ซ้ายสุด)
 * @param segments ค่า segment (bit 0-7 = A-G, DP)
 * @return TM1650_OK หรือ error code
 *
 * @example
 * // แสดง "E" ที่ตำแหน่งแรก
 * TM1650_DisplayRaw(&disp, 0, SEG_A | SEG_D | SEG_E | SEG_F | SEG_G);
 */
TM1650_Status TM1650_DisplayRaw(TM1650_Handle* handle, uint8_t position, uint8_t segments);

/**
 * @brief ส่ง buffer ไปยัง display (เรียกอัตโนมัติโดย Display functions)
 *
 * @param handle Pointer to TM1650_Handle
 * @return TM1650_OK หรือ error code
 *
 * @note ใช้เมื่อต้องการควบคุม buffer โดยตรงแล้วจึงส่งทีเดียว
 *
 * @example
 * disp.buffer[0] = SEG_A | SEG_B | SEG_C;  // แสดง "7"
 * disp.buffer[1] = 0x00;                    // ว่าง
 * TM1650_Update(&disp);
 */
TM1650_Status TM1650_Update(TM1650_Handle* handle);

/* ========== Key Scan Functions ========== */

/**
 * @brief อ่านปุ่มกด (non-blocking, มี debounce)
 *
 * @param handle Pointer to TM1650_Handle
 * @return ค่า key mask (TM1650_KEY_S1-S8) หรือ 0 ถ้าไม่มีปุ่มกด
 *
 * @note คืนค่าปุ่มเพียงครั้งเดียวต่อการกด 1 ครั้ง (ไม่ repeat)
 * @note รองรับการกดหลายปุ่มพร้อมกัน (multi-key)
 *
 * @example
 * uint8_t keys = TM1650_GetKeys(&disp);
 * if (keys & TM1650_KEY_S1) {
 *     // S1 ถูกกด
 * }
 * if (keys & TM1650_KEY_S2) {
 *     // S2 ถูกกด
 * }
 */
uint8_t TM1650_GetKeys(TM1650_Handle* handle);

/**
 * @brief อ่านค่าปุ่มแบบ raw โดยตรง (ไม่มี debounce)
 *
 * @param handle Pointer to TM1650_Handle
 * @return Raw key byte จาก TM1650
 *
 * @note ใช้เมื่อต้องการจัดการ debounce เอง
 * @note bit 0 = S1, bit 1 = S2, ..., bit 7 = S8
 *
 * @example
 * uint8_t raw = TM1650_GetRawKeys(&disp);
 */
uint8_t TM1650_GetRawKeys(TM1650_Handle* handle);

/**
 * @brief ตรวจสอบว่ามีปุ่มถูกกดอยู่หรือไม่ (instant, no debounce)
 *
 * @param handle Pointer to TM1650_Handle
 * @return true ถ้ามีปุ่มใดๆ ถูกกด, false ถ้าไม่มี
 *
 * @example
 * if (TM1650_IsKeyPressed(&disp)) {
 *     // Some key is held down
 * }
 */
bool TM1650_IsKeyPressed(TM1650_Handle* handle);

/**
 * @brief รอจนกว่ามีปุ่มกด (blocking)
 *
 * @param handle Pointer to TM1650_Handle
 * @return ค่า key mask ของปุ่มที่กด
 *
 * @note จะรอจนกว่าปล่อยปุ่มก่อนจึงคืนค่า
 *
 * @example
 * uint8_t key = TM1650_WaitKey(&disp);
 */
uint8_t TM1650_WaitKey(TM1650_Handle* handle);

/* ========== Utility Functions ========== */

/**
 * @brief แปลงตัวเลขเป็น segment pattern
 *
 * @param digit ตัวเลข (0-9)
 * @return Segment pattern หรือ 0 ถ้าไม่ถูกต้อง
 *
 * @example
 * uint8_t seg = TM1650_DigitToSegment(5);  // returns 0x6D
 */
uint8_t TM1650_DigitToSegment(uint8_t digit);

#ifdef __cplusplus
}
#endif

#endif  /* __TM1650_H */
