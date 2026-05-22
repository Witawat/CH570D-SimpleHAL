/**
 * @file SimpleMAX7219.h
 * @brief MAX7219 LED Matrix Driver Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับควบคุม LED Matrix 8x8 ผ่าน IC Driver MAX7219
 * รองรับการแสดงผลตั้งแต่ขั้นพื้นฐานถึงขั้นสูง
 * 
 * **คุณสมบัติ:**
 * - SPI communication (hardware SPI)
 * - รองรับ single และ cascaded displays (1-8 matrices)
 * - Graphics primitives (pixel, line, rect, circle, triangle)
 * - Text rendering (ASCII + Thai characters)
 * - Scrolling text (horizontal/vertical)
 * - Animation และ sprite system
 * - Double buffering support
 * - ควบคุมความสว่าง 16 ระดับ (0-15)
 * 
 * **Hardware Requirements:**
 * - MAX7219 LED Matrix module (8x8)
 * - SPI pins: CLK, MOSI, CS
 * - VCC: 5V, GND
 * 
 * @example
 * // Basic usage
 * #include "MAX7219.h"
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Delay_Init();
 *     
 *     MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);  // CLK, MOSI, CS, 1 matrix
 *     MAX7219_SetIntensity(display, 8);
 *     MAX7219_DrawString(display, "HI", 0, 0);
 *     MAX7219_Update(display);
 *     
 *     while(1) {}
 * }
 * 
 * @author CH57x SimpleHAL Team
 * @copyright MIT License
 */

#ifndef __MAX7219_H
#define __MAX7219_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ==================================================================
 *  Feature Configuration Macros (overrideable)
 * ================================================================== */

#ifndef MAX7219_ENABLE_THAI_FULL
#define MAX7219_ENABLE_THAI_FULL  1  /**< 1=include Thai 8x8 font + UTF-8 */
#endif

#ifndef MAX7219_ENABLE_EFFECTS
#define MAX7219_ENABLE_EFFECTS    1  /**< 1=include wipe/blink/sparkle/rain */
#endif

/* ========== MAX7219 Register Addresses ========== */

#define MAX7219_REG_NOOP        0x00  /**< No operation */
#define MAX7219_REG_DIGIT0      0x01  /**< Digit 0 (row 0) */
#define MAX7219_REG_DIGIT1      0x02  /**< Digit 1 (row 1) */
#define MAX7219_REG_DIGIT2      0x03  /**< Digit 2 (row 2) */
#define MAX7219_REG_DIGIT3      0x04  /**< Digit 3 (row 3) */
#define MAX7219_REG_DIGIT4      0x05  /**< Digit 4 (row 4) */
#define MAX7219_REG_DIGIT5      0x06  /**< Digit 5 (row 5) */
#define MAX7219_REG_DIGIT6      0x07  /**< Digit 6 (row 6) */
#define MAX7219_REG_DIGIT7      0x08  /**< Digit 7 (row 7) */
#define MAX7219_REG_DECODE      0x09  /**< Decode mode */
#define MAX7219_REG_INTENSITY   0x0A  /**< Intensity (0-15) */
#define MAX7219_REG_SCANLIMIT   0x0B  /**< Scan limit (0-7) */
#define MAX7219_REG_SHUTDOWN    0x0C  /**< Shutdown mode */
#define MAX7219_REG_DISPLAYTEST 0x0F  /**< Display test */

/* ========== Configuration Constants ========== */

#define MAX7219_MAX_DEVICES     8     /**< Maximum cascaded devices */
#define MAX7219_MATRIX_SIZE     8     /**< Matrix size (8x8) */
#define MAX7219_INTENSITY_MIN   0     /**< Minimum intensity */
#define MAX7219_INTENSITY_MAX   15    /**< Maximum intensity */

/* ========== Font Structure ========== */

/**
 * @brief Font definition structure
 */
typedef struct {
    uint8_t width;              /**< Character width in pixels */
    uint8_t height;             /**< Character height in pixels */
    uint16_t first_char;        /**< First character code (supports Unicode) */
    uint16_t last_char;         /**< Last character code (supports Unicode) */
    const uint8_t* data;        /**< Pointer to font data */
} MAX7219_Font;

/* ========== Animation Structure ========== */

/**
 * @brief Animation state structure
 */
typedef struct {
    const uint8_t** frames;     /**< Pointer to frame array */
    uint8_t num_frames;         /**< Number of frames */
    uint8_t current_frame;      /**< Current frame index */
    uint16_t frame_delay;       /**< Delay between frames (ms) */
    uint32_t last_update;       /**< Last update timestamp */
    bool loop;                  /**< Loop animation */
    bool active;                /**< Animation active flag */
} MAX7219_Animation;

/* ========== Scrolling Structure ========== */

/**
 * @brief Scrolling text state structure
 */
typedef struct {
    const char* text;           /**< Text to scroll */
    int16_t offset;             /**< Current scroll offset */
    uint16_t scroll_delay;      /**< Delay between scroll steps (ms) */
    uint32_t last_update;       /**< Last update timestamp */
    bool active;                /**< Scrolling active flag */
    bool vertical;              /**< Vertical scrolling */
    const MAX7219_Font* font;   /**< Font to use */
} MAX7219_Scroll;

/* ========== Display Handle Structure ========== */

/**
 * @brief MAX7219 Display Handle
 */
typedef struct {
    uint8_t clk_pin;            /**< SPI CLK pin */
    uint8_t mosi_pin;           /**< SPI MOSI pin */
    uint8_t cs_pin;             /**< SPI CS pin */
    uint8_t num_devices;        /**< Number of cascaded devices */
    uint8_t intensity;          /**< Current intensity (0-15) */
    bool display_on;            /**< Display on/off state */
    
    // Display buffers (8 bytes per device)
    uint8_t buffer[MAX7219_MAX_DEVICES][MAX7219_MATRIX_SIZE];
    
    // Current font
    const MAX7219_Font* font;
    
    // Animation state
    MAX7219_Animation animation;
    
    // Scrolling state
    MAX7219_Scroll scroll;
    
} MAX7219_Handle;

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน MAX7219
 * 
 * @param clk_pin หมายเลข GPIO pin สำหรับ SPI CLK
 * @param mosi_pin หมายเลข GPIO pin สำหรับ SPI MOSI
 * @param cs_pin หมายเลข GPIO pin สำหรับ SPI CS
 * @param num_devices จำนวน cascaded devices (1-8)
 * @return Pointer to MAX7219_Handle
 * 
 * @note ต้องเรียก SystemCoreClockUpdate() และ Delay_Init() ก่อนใช้ฟังก์ชันนี้
 * 
 * @example
 * MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);
 */
MAX7219_Handle* MAX7219_Init(uint8_t clk_pin, uint8_t mosi_pin, uint8_t cs_pin, uint8_t num_devices);

/**
 * @brief ตั้งค่าความสว่างของ display
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param intensity ระดับความสว่าง (0-15, 0=มืดที่สุด, 15=สว่างที่สุด)
 * 
 * @example
 * MAX7219_SetIntensity(display, 8);
 */
void MAX7219_SetIntensity(MAX7219_Handle* handle, uint8_t intensity);

/**
 * @brief เปิด/ปิด display
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param on true=เปิด, false=ปิด
 * 
 * @example
 * MAX7219_DisplayControl(display, true);   // เปิด display
 * MAX7219_DisplayControl(display, false);  // ปิด display
 */
void MAX7219_DisplayControl(MAX7219_Handle* handle, bool on);

/**
 * @brief ล้างหน้าจอทั้งหมด
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param update true=อัพเดท hardware ทันที, false=อัพเดท buffer อย่างเดียว
 * 
 * @example
 * MAX7219_Clear(display, true);
 */
void MAX7219_Clear(MAX7219_Handle* handle, bool update);

/**
 * @brief อัพเดท display buffer ไปยัง hardware
 * 
 * @param handle Pointer to MAX7219_Handle
 * 
 * @note เรียกฟังก์ชันนี้หลังจากวาดกราฟิกเสร็จเพื่ออัพเดทหน้าจอ
 * 
 * @example
 * MAX7219_SetPixel(display, 0, 0, true);
 * MAX7219_Update(display);
 */
void MAX7219_Update(MAX7219_Handle* handle);

/**
 * @brief เปิด/ปิด display test mode
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param test true=เปิด test mode (ไฟติดทั้งหมด), false=ปิด
 * 
 * @example
 * MAX7219_DisplayTest(display, true);   // ทดสอบ LED
 * Delay_Ms(1000);
 * MAX7219_DisplayTest(display, false);  // กลับสู่โหมดปกติ
 */
void MAX7219_DisplayTest(MAX7219_Handle* handle, bool test);

/* ========== Graphics Primitives ========== */

/**
 * @brief ตั้งค่า/ล้าง pixel ที่ตำแหน่งที่กำหนด
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X (0-7 สำหรับ 1 matrix, 0-63 สำหรับ 8 matrices)
 * @param y พิกัด Y (0-7)
 * @param on true=เปิด pixel, false=ปิด pixel
 * 
 * @note ต้องเรียก MAX7219_Update() เพื่ออัพเดทหน้าจอ
 * 
 * @example
 * MAX7219_SetPixel(display, 3, 4, true);  // เปิด pixel ที่ (3,4)
 * MAX7219_Update(display);
 */
void MAX7219_SetPixel(MAX7219_Handle* handle, int16_t x, int16_t y, bool on);

/**
 * @brief อ่านสถานะของ pixel
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X
 * @param y พิกัด Y
 * @return true=pixel เปิด, false=pixel ปิด
 * 
 * @example
 * bool pixel_state = MAX7219_GetPixel(display, 3, 4);
 */
bool MAX7219_GetPixel(MAX7219_Handle* handle, int16_t x, int16_t y);

/**
 * @brief วาดเส้นตรง (Bresenham's algorithm)
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x0 พิกัด X เริ่มต้น
 * @param y0 พิกัด Y เริ่มต้น
 * @param x1 พิกัด X สิ้นสุด
 * @param y1 พิกัด Y สิ้นสุด
 * @param on true=เปิด pixels, false=ปิด pixels
 * 
 * @example
 * MAX7219_DrawLine(display, 0, 0, 7, 7, true);  // เส้นทแยงมุม
 * MAX7219_Update(display);
 */
void MAX7219_DrawLine(MAX7219_Handle* handle, int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on);

/**
 * @brief วาดสี่เหลี่ยม
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X มุมบนซ้าย
 * @param y พิกัด Y มุมบนซ้าย
 * @param w ความกว้าง
 * @param h ความสูง
 * @param filled true=เติมสี, false=เส้นขอบอย่างเดียว
 * @param on true=เปิด pixels, false=ปิด pixels
 * 
 * @example
 * MAX7219_DrawRect(display, 1, 1, 6, 6, false, true);  // กรอบสี่เหลี่ยม
 * MAX7219_Update(display);
 */
void MAX7219_DrawRect(MAX7219_Handle* handle, int16_t x, int16_t y, int16_t w, int16_t h, bool filled, bool on);

/**
 * @brief วาดวงกลม (Bresenham's circle algorithm)
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x0 พิกัด X ศูนย์กลาง
 * @param y0 พิกัด Y ศูนย์กลาง
 * @param r รัศมี
 * @param filled true=เติมสี, false=เส้นขอบอย่างเดียว
 * @param on true=เปิด pixels, false=ปิด pixels
 * 
 * @example
 * MAX7219_DrawCircle(display, 3, 3, 3, false, true);  // วงกลม
 * MAX7219_Update(display);
 */
void MAX7219_DrawCircle(MAX7219_Handle* handle, int16_t x0, int16_t y0, int16_t r, bool filled, bool on);

/**
 * @brief วาดสามเหลี่ยม
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x0 พิกัด X จุดที่ 1
 * @param y0 พิกัด Y จุดที่ 1
 * @param x1 พิกัด X จุดที่ 2
 * @param y1 พิกัด Y จุดที่ 2
 * @param x2 พิกัด X จุดที่ 3
 * @param y2 พิกัด Y จุดที่ 3
 * @param filled true=เติมสี, false=เส้นขอบอย่างเดียว
 * @param on true=เปิด pixels, false=ปิด pixels
 * 
 * @example
 * MAX7219_DrawTriangle(display, 3, 0, 0, 7, 6, 7, false, true);
 * MAX7219_Update(display);
 */
void MAX7219_DrawTriangle(MAX7219_Handle* handle, int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                          int16_t x2, int16_t y2, bool filled, bool on);

/**
 * @brief แสดง bitmap 8x8
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X มุมบนซ้าย
 * @param y พิกัด Y มุมบนซ้าย
 * @param bitmap Pointer to bitmap data (8 bytes)
 * @param width ความกว้าง bitmap
 * @param height ความสูง bitmap
 * 
 * @note Bitmap format: แต่ละ byte เป็น 1 row, bit 0 = pixel ซ้ายสุด
 * 
 * @example
 * const uint8_t heart[] = {0x00, 0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18};
 * MAX7219_DrawBitmap(display, 0, 0, heart, 8, 8);
 * MAX7219_Update(display);
 */
void MAX7219_DrawBitmap(MAX7219_Handle* handle, int16_t x, int16_t y, const uint8_t* bitmap, 
                        uint8_t width, uint8_t height);

/* ========== Text Functions ========== */

/**
 * @brief ตั้งค่า font ที่ใช้
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param font Pointer to MAX7219_Font
 * 
 * @example
 * MAX7219_SetFont(display, &font_5x7);
 */
void MAX7219_SetFont(MAX7219_Handle* handle, const MAX7219_Font* font);

/**
 * @brief วาดตัวอักษร 1 ตัว
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X มุมบนซ้าย
 * @param y พิกัด Y มุมบนซ้าย
 * @param ch ตัวอักษร
 * @return ความกว้างของตัวอักษร (pixels)
 * 
 * @note ต้องเรียก MAX7219_Update() เพื่ออัพเดทหน้าจอ
 * 
 * @example
 * MAX7219_DrawChar(display, 0, 0, 'A');
 * MAX7219_Update(display);
 */
uint8_t MAX7219_DrawChar(MAX7219_Handle* handle, int16_t x, int16_t y, char ch);

/**
 * @brief วาดข้อความ
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X มุมบนซ้าย
 * @param y พิกัด Y มุมบนซ้าย
 * @param text ข้อความ
 * @return ความกว้างของข้อความ (pixels)
 * 
 * @note ต้องเรียก MAX7219_Update() เพื่ออัพเดทหน้าจอ
 * 
 * @example
 * MAX7219_DrawString(display, 0, 0, "HELLO");
 * MAX7219_Update(display);
 */
uint16_t MAX7219_DrawString(MAX7219_Handle* handle, int16_t x, int16_t y, const char* text);

/**
 * @brief คำนวณความกว้างของข้อความ
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param text ข้อความ
 * @return ความกว้าง (pixels)
 * 
 * @example
 * uint16_t width = MAX7219_GetStringWidth(display, "HELLO");
 */
uint16_t MAX7219_GetStringWidth(MAX7219_Handle* handle, const char* text);

/* ========== Scrolling Functions ========== */

/**
 * @brief เริ่มการเลื่อนข้อความแบบ horizontal
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param text ข้อความที่ต้องการเลื่อน
 * @param scroll_delay ความเร็วในการเลื่อน (ms)
 * 
 * @note ต้องเรียก MAX7219_UpdateScroll() ใน main loop
 * 
 * @example
 * MAX7219_StartScrollText(display, "HELLO WORLD", 100);
 * while(1) {
 *     MAX7219_UpdateScroll(display);
 * }
 */
void MAX7219_StartScrollText(MAX7219_Handle* handle, const char* text, uint16_t scroll_delay);

/**
 * @brief อัพเดทการเลื่อนข้อความ (เรียกใน main loop)
 * 
 * @param handle Pointer to MAX7219_Handle
 * @return true=ยังเลื่อนอยู่, false=เลื่อนเสร็จแล้ว
 * 
 * @example
 * while(1) {
 *     if(!MAX7219_UpdateScroll(display)) {
 *         // Scrolling finished
 *         break;
 *     }
 * }
 */
bool MAX7219_UpdateScroll(MAX7219_Handle* handle);

/**
 * @brief หยุดการเลื่อนข้อความ
 * 
 * @param handle Pointer to MAX7219_Handle
 * 
 * @example
 * MAX7219_StopScroll(display);
 */
void MAX7219_StopScroll(MAX7219_Handle* handle);

/* ========== Animation Functions ========== */

/**
 * @brief เริ่ม animation
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param frames Pointer to array of frame pointers
 * @param num_frames จำนวน frames
 * @param frame_delay ความเร็วของ animation (ms)
 * @param loop true=เล่นวนซ้ำ, false=เล่นครั้งเดียว
 * 
 * @note ต้องเรียก MAX7219_UpdateAnimation() ใน main loop
 * 
 * @example
 * const uint8_t* frames[] = {frame1, frame2, frame3};
 * MAX7219_StartAnimation(display, frames, 3, 100, true);
 */
void MAX7219_StartAnimation(MAX7219_Handle* handle, const uint8_t** frames, uint8_t num_frames, 
                           uint16_t frame_delay, bool loop);

/**
 * @brief อัพเดท animation (เรียกใน main loop)
 * 
 * @param handle Pointer to MAX7219_Handle
 * @return true=animation ยังเล่นอยู่, false=เล่นเสร็จแล้ว
 * 
 * @example
 * while(1) {
 *     MAX7219_UpdateAnimation(display);
 * }
 */
bool MAX7219_UpdateAnimation(MAX7219_Handle* handle);

/**
 * @brief หยุด animation
 * 
 * @param handle Pointer to MAX7219_Handle
 * 
 * @example
 * MAX7219_StopAnimation(display);
 */
void MAX7219_StopAnimation(MAX7219_Handle* handle);

/* ========== Sprite Functions ========== */

/**
 * @brief วาด sprite พร้อม transparency
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param sprite Pointer to sprite data
 * @param mask Pointer to transparency mask (NULL = ไม่มี transparency)
 * @param width ความกว้าง sprite
 * @param height ความสูง sprite
 * 
 * @note Mask: bit 1 = แสดง pixel, bit 0 = ข้าม pixel
 * 
 * @example
 * const uint8_t sprite[] = {0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C};
 * const uint8_t mask[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
 * MAX7219_DrawSprite(display, 0, 0, sprite, mask, 8, 8);
 * MAX7219_Update(display);
 */
void MAX7219_DrawSprite(MAX7219_Handle* handle, int16_t x, int16_t y, const uint8_t* sprite, 
                       const uint8_t* mask, uint8_t width, uint8_t height);

/* ========== Utility Functions ========== */

/**
 * @brief Fade in effect
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param duration ระยะเวลา fade (ms)
 * 
 * @example
 * MAX7219_FadeIn(display, 1000);  // Fade in 1 วินาที
 */
void MAX7219_FadeIn(MAX7219_Handle* handle, uint16_t duration);

/**
 * @brief Fade out effect
 * 
 * @param handle Pointer to MAX7219_Handle
 * @param duration ระยะเวลา fade (ms)
 * 
 * @example
 * MAX7219_FadeOut(display, 1000);  // Fade out 1 วินาที
 */
void MAX7219_FadeOut(MAX7219_Handle* handle, uint16_t duration);

/**
 * @brief Invert display (สลับ on/off ทุก pixel)
 * 
 * @param handle Pointer to MAX7219_Handle
 * 
 * @example
 * MAX7219_Invert(display);
 * MAX7219_Update(display);
 */
void MAX7219_Invert(MAX7219_Handle* handle);

/* ==================================================================
 *  Thai Text Functions (UTF-8) — ต้อง include max7219_fonts_thai.h
 * ================================================================== */

/**
 * @brief วาดตัวอักษรไทย 1 ตัว (UTF-8 3 bytes)
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param thai_char ตัวอักษรไทย (UTF-8, 3 bytes)
 * @param intensity ความสว่าง (0-15, 0=ดับ)
 * @return ความกว้าง (pixels) หรือ 0 ถ้าไม่พบฟอนต์
 *
 * @example
 * MAX7219_DrawCharThai(display, 0, 0, "ก", 8);
 * MAX7219_Update(display);
 */
uint8_t MAX7219_DrawCharThai(MAX7219_Handle* handle, int16_t x, int16_t y,
                             const char* thai_char, uint8_t intensity);

/**
 * @brief วาดข้อความภาษาไทย (UTF-8, รองรับผสม ASCII)
 * @param handle Pointer to MAX7219_Handle
 * @param x พิกัด X เริ่มต้น
 * @param y พิกัด Y
 * @param text ข้อความ UTF-8
 * @param intensity ความสว่าง (0-15)
 * @return ความกว้างรวม (pixels)
 *
 * @example
 * MAX7219_DrawStringThai(display, 0, 0, "สวัสดี", 8);
 * MAX7219_Update(display);
 */
uint16_t MAX7219_DrawStringThai(MAX7219_Handle* handle, int16_t x, int16_t y,
                                const char* text, uint8_t intensity);

/* ==================================================================
 *  Effects (Wipe, Blink, Sparkle, Marquee)
 * ================================================================== */

/**
 * @brief Wipe effect — ค่อยๆ เติมจากซ้ายไปขวา
 * @param handle Pointer to MAX7219_Handle
 * @param delay_ms หน่วงเวลาต่อคอลัมน์ (ms)
 *
 * @example
 * MAX7219_WipeLeft(display, 30);
 */
void MAX7219_WipeLeft(MAX7219_Handle* handle, uint16_t delay_ms);

/**
 * @brief Wipe effect — ค่อยๆ เติมจากขวาไปซ้าย
 */
void MAX7219_WipeRight(MAX7219_Handle* handle, uint16_t delay_ms);

/**
 * @brief Wipe effect — ค่อยๆ เติมจากบนลงล่าง
 */
void MAX7219_WipeUp(MAX7219_Handle* handle, uint16_t delay_ms);

/**
 * @brief Wipe effect — ค่อยๆ เติมจากล่างขึ้นบน
 */
void MAX7219_WipeDown(MAX7219_Handle* handle, uint16_t delay_ms);

/**
 * @brief Blink effect — กระพริบ display
 * @param handle Pointer to MAX7219_Handle
 * @param count จำนวนครั้งที่กระพริบ
 * @param on_ms ระยะเวลาเปิด (ms)
 * @param off_ms ระยะเวลาปิด (ms)
 *
 * @example
 * MAX7219_Blink(display, 3, 200, 200);
 */
void MAX7219_Blink(MAX7219_Handle* handle, uint16_t count,
                   uint16_t on_ms, uint16_t off_ms);

/**
 * @brief Sparkle effect — เปิด/ปิด pixels แบบสุ่ม
 * @param handle Pointer to MAX7219_Handle
 * @param duration_ms ระยะเวลาทั้งหมด (ms)
 * @param density ความหนาแน่น (0-100%, 10 = ประมาณ 10% ของ pixels)
 *
 * @example
 * MAX7219_Sparkle(display, 2000, 10);
 */
void MAX7219_Sparkle(MAX7219_Handle* handle, uint16_t duration_ms,
                     uint8_t density);

/**
 * @brief Marquee border — แสงวิ่งรอบขอบจอ
 * @param handle Pointer to MAX7219_Handle
 * @param speed_ms ความเร็ว (ms ต่อ step)
 * @param rounds จำนวนรอบ (0 = ไม่มีที่สิ้นสุด)
 *
 * @example
 * MAX7219_MarqueeBorder(display, 50, 3);
 */
void MAX7219_MarqueeBorder(MAX7219_Handle* handle, uint16_t speed_ms,
                           uint8_t rounds);

/**
 * @brief Rain effect — ฝนตกจากบนลงล่าง
 * @param handle Pointer to MAX7219_Handle
 * @param duration_ms ระยะเวลาทั้งหมด (ms)
 * @param speed_ms ความเร็ว (ms ต่อ drop step)
 *
 * @example
 * MAX7219_RainEffect(display, 3000, 100);
 */
void MAX7219_RainEffect(MAX7219_Handle* handle, uint16_t duration_ms,
                        uint16_t speed_ms);

/**
 * @brief Running light — จุดวิ่งรอบขอบจอ
 * @param handle Pointer to MAX7219_Handle
 * @param speed_ms ความเร็ว (ms ต่อ step)
 *
 * @example
 * MAX7219_RunningLight(display, 100);
 */
void MAX7219_RunningLight(MAX7219_Handle* handle, uint16_t speed_ms);

/* ==================================================================
 *  Vertical Scrolling
 * ================================================================== */

/**
 * @brief เริ่มการเลื่อนข้อความแบบแนวตั้ง (ล่างขึ้นบน)
 * @param handle Pointer to MAX7219_Handle
 * @param text ข้อความ
 * @param scroll_delay ความเร็ว (ms ต่อ step)
 *
 * @note ใช้ MAX7219_UpdateScroll() เช่นเดียวกับ horizontal scroll
 *
 * @example
 * MAX7219_StartScrollTextVertical(display, "Hello", 100);
 * while(1) { MAX7219_UpdateScroll(display); }
 */
void MAX7219_StartScrollTextVertical(MAX7219_Handle* handle, const char* text,
                                     uint16_t scroll_delay);

/* ==================================================================
 *  Buffer Utilities
 * ================================================================== */

/**
 * @brief เลื่อน buffer ไปทางซ้าย
 * @param handle Pointer to MAX7219_Handle
 * @param pixels จำนวน pixels ที่จะเลื่อน
 *
 * @example
 * MAX7219_ShiftLeft(display, 1);
 * MAX7219_Update(display);
 */
void MAX7219_ShiftLeft(MAX7219_Handle* handle, uint8_t pixels);

/**
 * @brief เลื่อน buffer ไปทางขวา
 */
void MAX7219_ShiftRight(MAX7219_Handle* handle, uint8_t pixels);

/**
 * @brief เลื่อน buffer ขึ้นด้านบน
 */
void MAX7219_ShiftUp(MAX7219_Handle* handle, uint8_t pixels);

/**
 * @brief เลื่อน buffer ลงด้านล่าง
 */
void MAX7219_ShiftDown(MAX7219_Handle* handle, uint8_t pixels);

/**
 * @brief Auto-scroll buffer ซ้าย (non-blocking, ใช้ Get_CurrentMs)
 * @param handle Pointer to MAX7219_Handle
 * @param speed_ms ความเร็ว (ms ต่อ step)
 *
 * @note เรียกใน main loop — ใช้ timer internal
 *
 * @example
 * while(1) { MAX7219_ScrollBufferLeft(display, 100); }
 */
void MAX7219_ScrollBufferLeft(MAX7219_Handle* handle, uint16_t speed_ms);

/**
 * @brief Auto-scroll buffer ขวา (non-blocking)
 */
void MAX7219_ScrollBufferRight(MAX7219_Handle* handle, uint16_t speed_ms);

/**
 * @brief Progress bar — วาดแถบแสดงความคืบหน้า
 * @param handle Pointer to MAX7219_Handle
 * @param percent เปอร์เซ็นต์ (0-100)
 * @param vertical true=แนวตั้ง, false=แนวนอน
 *
 * @example
 * MAX7219_ProgressBar(display, 75, false);  // 75% แนวนอน
 * MAX7219_Update(display);
 */
void MAX7219_ProgressBar(MAX7219_Handle* handle, uint8_t percent,
                         bool vertical);

/* ========== External Font Declarations ========== */

extern const MAX7219_Font font_5x7;      /**< 5x7 ASCII font */
extern const MAX7219_Font font_8x8;      /**< 8x8 ASCII font */
extern const MAX7219_Font font_thai_5x7; /**< 5x7 Thai font */

#if (MAX7219_ENABLE_THAI_FULL)
extern const MAX7219_Font font_thai_8x8;      /**< 8x8 Thai consonant font */
extern const MAX7219_Font font_thai_numbers_8x8; /**< 8x8 Thai digit font */
#endif /* MAX7219_ENABLE_THAI_FULL */

#ifdef __cplusplus
}
#endif

#endif  // __MAX7219_H
