/**
 * @file SimpleTM1637.h
 * @brief TM1637 7-Segment Display Driver Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับควบคุม 7-segment display ผ่าน IC Driver TM1637
 * รองรับการแสดงผลตั้งแต่ขั้นพื้นฐานถึงขั้นสูง
 * 
 * **คุณสมบัติ:**
 * - Two-wire communication (CLK, DIO)
 * - รองรับ 4-digit และ 6-digit displays
 * - ควบคุมความสว่าง 8 ระดับ (0-7)
 * - แสดงตัวเลข, ตัวอักษร, และสัญลักษณ์
 * - Scrolling text และ animations
 * - Non-blocking display updates
 * 
 * **Hardware Requirements:**
 * - TM1637 4-digit or 6-digit 7-segment module
 * - 2 GPIO pins (CLK, DIO)
 * - VCC: 3.3V-5V, GND
 * 
 * @example
 * // Basic usage
 * #include "TM1637.h"
 * 
 * int main(void) {
 *     Timer_Init();
 *     TM1637_Init(PC0, PC1, 4);  // CLK=PC0, DIO=PC1, 4 digits
 *     TM1637_SetBrightness(5);
 *     TM1637_DisplayNumber(1234);
 *     while(1) {}
 * }
 */

#ifndef __TM1637_H
#define __TM1637_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== TM1637 Commands ========== */

#define TM1637_CMD_DATA         0x40  /**< Data command */
#define TM1637_CMD_DISPLAY      0x80  /**< Display control command */
#define TM1637_CMD_ADDRESS      0xC0  /**< Address command */

#define TM1637_BRIGHTNESS_MIN   0     /**< Minimum brightness */
#define TM1637_BRIGHTNESS_MAX   7     /**< Maximum brightness */

#define TM1637_MAX_DIGITS       6     /**< Maximum supported digits */

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

/* ========== TM1637 Handle Structure ========== */

/**
 * @brief TM1637 Display Handle
 */
typedef struct {
    uint8_t clk_pin;           /**< CLK pin number */
    uint8_t dio_pin;           /**< DIO pin number */
    uint8_t num_digits;        /**< Number of digits (4 or 6) */
    uint8_t brightness;        /**< Current brightness (0-7) */
    bool display_on;           /**< Display on/off state */
    uint8_t buffer[TM1637_MAX_DIGITS];  /**< Display buffer */
} TM1637_Handle;

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน TM1637
 * 
 * @param clk_pin หมายเลข GPIO pin สำหรับ CLK
 * @param dio_pin หมายเลข GPIO pin สำหรับ DIO
 * @param num_digits จำนวน digits ของ display (4 หรือ 6)
 * @return Pointer to TM1637_Handle
 * 
 * @note ต้องเรียก Timer_Init() ก่อนใช้ฟังก์ชันนี้
 * 
 * @example
 * TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
 */
TM1637_Handle* TM1637_Init(uint8_t clk_pin, uint8_t dio_pin, uint8_t num_digits);

/**
 * @brief ตั้งค่าความสว่างของ display
 * 
 * @param handle Pointer to TM1637_Handle
 * @param brightness ระดับความสว่าง (0-7, 0=มืดที่สุด, 7=สว่างที่สุด)
 * 
 * @example
 * TM1637_SetBrightness(display, 5);
 */
void TM1637_SetBrightness(TM1637_Handle* handle, uint8_t brightness);

/**
 * @brief เปิด/ปิด display
 * 
 * @param handle Pointer to TM1637_Handle
 * @param on true=เปิด, false=ปิด
 * 
 * @example
 * TM1637_DisplayControl(display, true);   // เปิด display
 * TM1637_DisplayControl(display, false);  // ปิด display
 */
void TM1637_DisplayControl(TM1637_Handle* handle, bool on);

/**
 * @brief ล้างหน้าจอทั้งหมด
 * 
 * @param handle Pointer to TM1637_Handle
 * 
 * @example
 * TM1637_Clear(display);
 */
void TM1637_Clear(TM1637_Handle* handle);

/* ========== Basic Display Functions ========== */

/**
 * @brief แสดงตัวเลขจำนวนเต็ม
 * 
 * @param handle Pointer to TM1637_Handle
 * @param number ตัวเลขที่ต้องการแสดง (-999 ถึง 9999 สำหรับ 4-digit)
 * @param leading_zero true=แสดง 0 นำหน้า, false=ไม่แสดง
 * 
 * @example
 * TM1637_DisplayNumber(display, 42, false);    // แสดง "  42"
 * TM1637_DisplayNumber(display, 42, true);     // แสดง "0042"
 */
void TM1637_DisplayNumber(TM1637_Handle* handle, int16_t number, bool leading_zero);

/**
 * @brief แสดงตัวเลขทศนิยม
 * 
 * @param handle Pointer to TM1637_Handle
 * @param number ตัวเลขทศนิยม
 * @param decimal_places จำนวนตำแหน่งทศนิยม (0-3)
 * 
 * @example
 * TM1637_DisplayFloat(display, 12.34, 2);  // แสดง "12.34"
 * TM1637_DisplayFloat(display, 98.6, 1);   // แสดง "98.6"
 */
void TM1637_DisplayFloat(TM1637_Handle* handle, float number, uint8_t decimal_places);

/**
 * @brief แสดงตัวเลขฐาน 16 (Hexadecimal)
 * 
 * @param handle Pointer to TM1637_Handle
 * @param number ตัวเลข hex (0x0000-0xFFFF สำหรับ 4-digit)
 * @param leading_zero true=แสดง 0 นำหน้า, false=ไม่แสดง
 * 
 * @example
 * TM1637_DisplayHex(display, 0xABCD, true);  // แสดง "ABCD"
 */
void TM1637_DisplayHex(TM1637_Handle* handle, uint16_t number, bool leading_zero);

/**
 * @brief แสดงตัวเลขที่ตำแหน่งเดียว
 * 
 * @param handle Pointer to TM1637_Handle
 * @param position ตำแหน่ง (0 = ซ้ายสุด)
 * @param digit ตัวเลข (0-9)
 * @param show_dp true=แสดงจุดทศนิยม, false=ไม่แสดง
 * 
 * @example
 * TM1637_DisplayDigit(display, 0, 1, false);  // แสดง "1" ที่ตำแหน่งแรก
 * TM1637_DisplayDigit(display, 2, 5, true);   // แสดง "5." ที่ตำแหน่งที่ 3
 */
void TM1637_DisplayDigit(TM1637_Handle* handle, uint8_t position, uint8_t digit, bool show_dp);

/**
 * @brief ควบคุม segment แต่ละตัวโดยตรง
 * 
 * @param handle Pointer to TM1637_Handle
 * @param position ตำแหน่ง (0 = ซ้ายสุด)
 * @param segments ค่า segment (bit 0-7 = A-G,DP)
 * 
 * @example
 * // แสดง "E" ที่ตำแหน่งแรก
 * TM1637_DisplayRaw(display, 0, SEG_A | SEG_D | SEG_E | SEG_F | SEG_G);
 */
void TM1637_DisplayRaw(TM1637_Handle* handle, uint8_t position, uint8_t segments);

/* ========== Character Display Functions ========== */

/**
 * @brief แสดงตัวอักษรหรือสัญลักษณ์
 * 
 * @param handle Pointer to TM1637_Handle
 * @param position ตำแหน่ง (0 = ซ้ายสุด)
 * @param ch ตัวอักษร (A-Z, a-z, 0-9, และสัญลักษณ์บางตัว)
 * @param show_dp true=แสดงจุดทศนิยม, false=ไม่แสดง
 * 
 * @note รองรับ: 0-9, A-F, H, L, P, U, -, _, space
 * 
 * @example
 * TM1637_DisplayChar(display, 0, 'H', false);
 * TM1637_DisplayChar(display, 1, 'E', false);
 * TM1637_DisplayChar(display, 2, 'L', false);
 * TM1637_DisplayChar(display, 3, 'P', false);  // แสดง "HELP"
 */
void TM1637_DisplayChar(TM1637_Handle* handle, uint8_t position, char ch, bool show_dp);

/**
 * @brief แสดงข้อความ (ไม่เลื่อน)
 * 
 * @param handle Pointer to TM1637_Handle
 * @param text ข้อความที่ต้องการแสดง
 * @param start_pos ตำแหน่งเริ่มต้น
 * 
 * @note ข้อความที่ยาวเกินจะถูกตัดทิ้ง
 * 
 * @example
 * TM1637_DisplayString(display, "COOL", 0);  // แสดง "COOL"
 */
void TM1637_DisplayString(TM1637_Handle* handle, const char* text, uint8_t start_pos);

/* ========== Advanced Display Functions ========== */

/**
 * @brief ตั้งค่าการกะพริบ
 * 
 * @param handle Pointer to TM1637_Handle
 * @param enable true=เปิดการกะพริบ, false=ปิด
 * @param blink_rate อัตราการกะพริบ (ms)
 * 
 * @note ต้องเรียก TM1637_UpdateBlink() ใน main loop
 * 
 * @example
 * TM1637_SetBlink(display, true, 500);  // กะพริบทุก 500ms
 */
void TM1637_SetBlink(TM1637_Handle* handle, bool enable, uint16_t blink_rate);

/**
 * @brief อัพเดทสถานะการกะพริบ (เรียกใน main loop)
 * 
 * @param handle Pointer to TM1637_Handle
 * 
 * @example
 * while(1) {
 *     TM1637_UpdateBlink(display);
 * }
 */
void TM1637_UpdateBlink(TM1637_Handle* handle);

/**
 * @brief เลื่อนข้อความจากขวาไปซ้าย
 * 
 * @param handle Pointer to TM1637_Handle
 * @param text ข้อความที่ต้องการเลื่อน
 * @param scroll_delay ความเร็วในการเลื่อน (ms)
 * 
 * @note ฟังก์ชันนี้เป็นแบบ blocking
 * 
 * @example
 * TM1637_ScrollText(display, "HELLO WORLD", 300);
 */
void TM1637_ScrollText(TM1637_Handle* handle, const char* text, uint16_t scroll_delay);

/**
 * @brief เริ่มต้นการเลื่อนข้อความแบบ non-blocking
 * 
 * @param handle Pointer to TM1637_Handle
 * @param text ข้อความที่ต้องการเลื่อน
 * @param scroll_delay ความเร็วในการเลื่อน (ms)
 * 
 * @note ต้องเรียก TM1637_UpdateScroll() ใน main loop
 * 
 * @example
 * TM1637_StartScroll(display, "HELLO", 300);
 * while(1) {
 *     TM1637_UpdateScroll(display);
 * }
 */
void TM1637_StartScroll(TM1637_Handle* handle, const char* text, uint16_t scroll_delay);

/**
 * @brief อัพเดทการเลื่อนข้อความ (เรียกใน main loop)
 * 
 * @param handle Pointer to TM1637_Handle
 * @return true=ยังเลื่อนอยู่, false=เลื่อนเสร็จแล้ว
 */
bool TM1637_UpdateScroll(TM1637_Handle* handle);

/**
 * @brief หยุดการเลื่อนข้อความ
 * 
 * @param handle Pointer to TM1637_Handle
 */
void TM1637_StopScroll(TM1637_Handle* handle);

/**
 * @brief แสดง animation แบบกำหนดเอง
 * 
 * @param handle Pointer to TM1637_Handle
 * @param frames อาร์เรย์ของ frames (แต่ละ frame เป็น array ของ segments)
 * @param num_frames จำนวน frames
 * @param frame_delay ความเร็วของ animation (ms)
 * @param repeat จำนวนรอบที่ต้องการเล่น (0=เล่นไม่จำกัด)
 * 
 * @note ฟังก์ชันนี้เป็นแบบ blocking
 * 
 * @example
 * uint8_t loading_frames[4][4] = {
 *     {SEG_A, 0, 0, 0},
 *     {0, SEG_A, 0, 0},
 *     {0, 0, SEG_A, 0},
 *     {0, 0, 0, SEG_A}
 * };
 * TM1637_PlayAnimation(display, loading_frames, 4, 100, 3);
 */
void TM1637_PlayAnimation(TM1637_Handle* handle, const uint8_t frames[][TM1637_MAX_DIGITS], 
                          uint8_t num_frames, uint16_t frame_delay, uint8_t repeat);

/**
 * @brief ตั้งค่าเครื่องหมาย colon (สำหรับนาฬิกา)
 * 
 * @param handle Pointer to TM1637_Handle
 * @param show true=แสดง colon, false=ซ่อน
 * 
 * @note ใช้ได้กับ display ที่มี colon เท่านั้น
 * 
 * @example
 * TM1637_SetColon(display, true);   // แสดง "12:34"
 * TM1637_SetColon(display, false);  // แสดง "1234"
 */
void TM1637_SetColon(TM1637_Handle* handle, bool show);

/**
 * @brief แสดงเวลาในรูปแบบ HH:MM
 * 
 * @param handle Pointer to TM1637_Handle
 * @param hours ชั่วโมง (0-23)
 * @param minutes นาที (0-59)
 * @param show_colon true=แสดง colon, false=ซ่อน
 * 
 * @example
 * TM1637_DisplayTime(display, 12, 34, true);  // แสดง "12:34"
 */
void TM1637_DisplayTime(TM1637_Handle* handle, uint8_t hours, uint8_t minutes, bool show_colon);

/* ========== Utility Functions ========== */

/**
 * @brief อัพเดท display buffer ไปยัง hardware
 * 
 * @param handle Pointer to TM1637_Handle
 * 
 * @note ฟังก์ชันนี้ถูกเรียกอัตโนมัติโดยฟังก์ชันแสดงผลอื่นๆ
 */
void TM1637_Update(TM1637_Handle* handle);

/**
 * @brief แปลงตัวเลขเป็น segment pattern
 * 
 * @param digit ตัวเลข (0-9)
 * @return Segment pattern
 */
uint8_t TM1637_DigitToSegment(uint8_t digit);

/**
 * @brief แปลงตัวอักษรเป็น segment pattern
 * 
 * @param ch ตัวอักษร
 * @return Segment pattern (0 ถ้าไม่รองรับ)
 */
uint8_t TM1637_CharToSegment(char ch);

#ifdef __cplusplus
}
#endif

#endif  // __TM1637_H
