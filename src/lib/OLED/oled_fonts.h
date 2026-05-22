/**
 * @file oled_fonts.h
 * @brief OLED Font Library - Font Definitions and Text Rendering
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับจัดการ Font และการแสดงข้อความบน OLED
 * รองรับทั้งภาษาอังกฤษและภาษาไทย
 * 
 * **คุณสมบัติ:**
 * - Built-in fonts หลายขนาด (6x8, 8x16, 12x16)
 * - Thai font support (16x16)
 * - Text alignment (left, center, right)
 * - Text effects (inverse, underline)
 * - Scrolling text
 * - Multi-line text
 * 
 * @example
 * #include "oled_fonts.h"
 * 
 * OLED_SetFont(&oled, &Font_8x16);
 * OLED_DrawString(&oled, 0, 0, "Hello", OLED_COLOR_WHITE);
 * OLED_DrawStringThai(&oled, 0, 20, "สวัสดี", OLED_COLOR_WHITE);
 */

#ifndef __OLED_FONTS_H
#define __OLED_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "oled_i2c.h"
#include <stdint.h>
#include <string.h>

/* ========== Font Structure ========== */

/**
 * @brief โครงสร้างสำหรับ Font
 */
typedef struct {
    uint8_t width;          /**< ความกว้างของตัวอักษร */
    uint8_t height;         /**< ความสูงของตัวอักษร */
    uint8_t first_char;     /**< ตัวอักษรแรก (ASCII) */
    uint8_t last_char;      /**< ตัวอักษรสุดท้าย (ASCII) */
    const uint8_t* data;    /**< ข้อมูล font bitmap */
} OLED_Font;

/**
 * @brief โครงสร้างสำหรับ Thai Font
 */
typedef struct {
    uint16_t unicode;       /**< รหัส Unicode */
    const uint8_t* data;    /**< ข้อมูล bitmap 16x16 */
} OLED_ThaiChar;

/**
 * @brief Text Alignment
 */
typedef enum {
    OLED_ALIGN_LEFT = 0,
    OLED_ALIGN_CENTER = 1,
    OLED_ALIGN_RIGHT = 2
} OLED_TextAlign;

/**
 * @brief โหมดการเรนเดอร์ภาษาไทย
 */
typedef enum {
    OLED_THAI_RENDER_OFF = 0,          /**< ปิดการตีความภาษาไทย */
    OLED_THAI_RENDER_PLACEHOLDER = 1,  /**< วาดแบบ placeholder */
    OLED_THAI_RENDER_COMBINING = 2     /**< วาดแบบ combining marks (phase 1) */
} OLED_ThaiRenderMode;

/**
 * @brief ตั้งค่าการวาดข้อความแบบขยาย
 */
typedef struct {
    uint8_t scale;                     /**< ขยายฟอนต์ 1x, 2x, 3x... */
    uint8_t letter_spacing;            /**< ระยะห่างตัวอักษรเพิ่ม (px) */
    OLED_ThaiRenderMode thai_mode;     /**< โหมดภาษาไทย */
} OLED_TextConfig;

/* ========== Built-in Fonts ========== */

extern const OLED_Font Font_6x8;
extern const OLED_Font Font_8x16;
extern const OLED_Font Font_12x16;

/* ========== Font Selection ========== */

/**
 * @brief ตั้งค่า Font ปัจจุบัน
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param font ตัวชี้ไปยัง font
 * 
 * @example
 * OLED_SetFont(&oled, &Font_8x16);
 */
void OLED_SetFont(OLED_Handle* oled, const OLED_Font* font);

/**
 * @brief ดึง Font ปัจจุบัน
 * @param oled ตัวชี้ไปยัง OLED handle
 * @return ตัวชี้ไปยัง font ปัจจุบัน
 */
const OLED_Font* OLED_GetFont(OLED_Handle* oled);

/* ========== Character Drawing ========== */

/**
 * @brief วาดตัวอักษร ASCII
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param c ตัวอักษร
 * @param color สี
 * @return ความกว้างของตัวอักษร
 * 
 * @example
 * OLED_DrawChar(&oled, 10, 10, 'A', OLED_COLOR_WHITE);
 */
uint8_t OLED_DrawChar(OLED_Handle* oled, uint8_t x, uint8_t y, char c, OLED_Color color);

/**
 * @brief วาดตัวอักษร ASCII พร้อม scale
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param c ตัวอักษร
 * @param color สี
 * @param scale ขนาด 1x, 2x, 3x...
 * @return ความกว้างของตัวอักษร (pixels)
 */
uint8_t OLED_DrawCharScaled(OLED_Handle* oled, uint8_t x, uint8_t y, char c, OLED_Color color, uint8_t scale);

/**
 * @brief วาดตัวอักษร ASCII แบบ inverse
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param c ตัวอักษร
 * @return ความกว้างของตัวอักษร
 * 
 * @example
 * OLED_DrawCharInverse(&oled, 10, 10, 'A');
 */
uint8_t OLED_DrawCharInverse(OLED_Handle* oled, uint8_t x, uint8_t y, char c);

/* ========== String Drawing ========== */

/**
 * @brief วาดข้อความ ASCII
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ
 * @param color สี
 * @return ความกว้างของข้อความ
 * 
 * @example
 * OLED_DrawString(&oled, 0, 0, "Hello World", OLED_COLOR_WHITE);
 */
uint16_t OLED_DrawString(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color);

/**
 * @brief วาดข้อความ ASCII พร้อม scale
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ
 * @param color สี
 * @param scale ขนาด 1x, 2x, 3x...
 * @return ความกว้างของข้อความ
 */
uint16_t OLED_DrawStringScaled(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, uint8_t scale);

/**
 * @brief วาดข้อความ UTF-8 (ไทย+อังกฤษ) ด้วย config
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ UTF-8
 * @param color สี
 * @param config ตั้งค่าการวาด (NULL = ค่าเริ่มต้น)
 * @return ความกว้างของข้อความ
 */
uint16_t OLED_DrawStringUTF8Ex(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, const OLED_TextConfig* config);

/**
 * @brief วาดข้อความแบบมี alignment
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x (สำหรับ LEFT), หรือจุดกึ่งกลาง (CENTER), หรือขอบขวา (RIGHT)
 * @param y ตำแหน่ง y
 * @param str ข้อความ
 * @param color สี
 * @param align การจัดตำแหน่ง
 * 
 * @example
 * OLED_DrawStringAlign(&oled, 64, 0, "Center", OLED_COLOR_WHITE, OLED_ALIGN_CENTER);
 */
void OLED_DrawStringAlign(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, OLED_TextAlign align);

/**
 * @brief วาดข้อความแบบ inverse
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ
 * @return ความกว้างของข้อความ
 * 
 * @example
 * OLED_DrawStringInverse(&oled, 0, 0, "Selected");
 */
uint16_t OLED_DrawStringInverse(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str);

/* ========== Thai Text Support ========== */

/**
 * @brief วาดข้อความภาษาไทย (UTF-8)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ UTF-8
 * @param color สี
 * @return ความกว้างของข้อความ
 * 
 * @note ข้อความต้องเป็น UTF-8 encoding
 * 
 * @example
 * OLED_DrawStringThai(&oled, 0, 0, "สวัสดี", OLED_COLOR_WHITE);
 */
uint16_t OLED_DrawStringThai(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color);

/**
 * @brief วาดข้อความผสมไทย-อังกฤษ
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ UTF-8
 * @param color สี
 * @return ความกว้างของข้อความ
 * 
 * @example
 * OLED_DrawStringMixed(&oled, 0, 0, "Hello สวัสดี", OLED_COLOR_WHITE);
 */
uint16_t OLED_DrawStringMixed(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color);

/* ========== Number Drawing ========== */

/**
 * @brief วาดตัวเลข integer
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param num ตัวเลข
 * @param color สี
 * @return ความกว้างของตัวเลข
 * 
 * @example
 * OLED_DrawInt(&oled, 0, 0, 12345, OLED_COLOR_WHITE);
 */
uint16_t OLED_DrawInt(OLED_Handle* oled, uint8_t x, uint8_t y, int32_t num, OLED_Color color);

/**
 * @brief วาดตัวเลข float
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param num ตัวเลข
 * @param decimals จำนวนทศนิยม
 * @param color สี
 * @return ความกว้างของตัวเลข
 * 
 * @example
 * OLED_DrawFloat(&oled, 0, 0, 3.14159, 2, OLED_COLOR_WHITE);
 */
uint16_t OLED_DrawFloat(OLED_Handle* oled, uint8_t x, uint8_t y, float num, uint8_t decimals, OLED_Color color);

/* ========== Text Measurement ========== */

/**
 * @brief วัดความกว้างของข้อความ
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param str ข้อความ
 * @return ความกว้าง (pixels)
 * 
 * @example
 * uint16_t width = OLED_GetStringWidth(&oled, "Hello");
 */
uint16_t OLED_GetStringWidth(OLED_Handle* oled, const char* str);

/**
 * @brief วัดความกว้างของข้อความ ASCII พร้อม scale
 */
uint16_t OLED_GetStringWidthScaled(OLED_Handle* oled, const char* str, uint8_t scale);

/**
 * @brief วัดความกว้างข้อความ UTF-8 ตาม config
 */
uint16_t OLED_MeasureStringUTF8(OLED_Handle* oled, const char* str, const OLED_TextConfig* config);

/**
 * @brief วัดความสูงของ font ปัจจุบัน
 * @param oled ตัวชี้ไปยัง OLED handle
 * @return ความสูง (pixels)
 * 
 * @example
 * uint8_t height = OLED_GetFontHeight(&oled);
 */
uint8_t OLED_GetFontHeight(OLED_Handle* oled);

/**
 * @brief ตั้งค่า scale ข้อความเริ่มต้น (global)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param scale ค่า 1..4 (ค่าที่เกินจะถูก clamp)
 */
void OLED_SetTextScale(OLED_Handle* oled, uint8_t scale);

/**
 * @brief อ่านค่า scale ข้อความเริ่มต้น (global)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @return scale ปัจจุบัน
 */
uint8_t OLED_GetTextScale(OLED_Handle* oled);

/* ========== Advanced Text Features ========== */

/**
 * @brief วาดข้อความแบบหลายบรรทัด
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param str ข้อความ (ใช้ \\n เพื่อขึ้นบรรทัดใหม่)
 * @param color สี
 * @param line_spacing ระยะห่างระหว่างบรรทัด
 * 
 * @example
 * OLED_DrawMultiLine(&oled, 0, 0, "Line 1\\nLine 2\\nLine 3", OLED_COLOR_WHITE, 2);
 */
void OLED_DrawMultiLine(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, uint8_t line_spacing);

/**
 * @brief วาดข้อความแบบ scrolling
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param w ความกว้างของพื้นที่แสดง
 * @param str ข้อความ
 * @param color สี
 * @param offset ระยะ scroll (pixels)
 * 
 * @example
 * static uint8_t scroll_offset = 0;
 * OLED_DrawScrollText(&oled, 0, 0, 128, "Long scrolling text...", OLED_COLOR_WHITE, scroll_offset++);
 */
void OLED_DrawScrollText(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, const char* str, OLED_Color color, uint16_t offset);

/**
 * @brief วาดข้อความในกรอบ
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param w ความกว้างกรอบ
 * @param h ความสูงกรอบ
 * @param str ข้อความ
 * @param color สี
 * @param align การจัดตำแหน่ง
 * 
 * @example
 * OLED_DrawTextBox(&oled, 10, 10, 100, 20, "Centered", OLED_COLOR_WHITE, OLED_ALIGN_CENTER);
 */
void OLED_DrawTextBox(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, const char* str, OLED_Color color, OLED_TextAlign align);

/* ========== UTF-8 Helper Functions ========== */

/**
 * @brief แปลง UTF-8 เป็น Unicode
 * @param utf8 ตัวชี้ไปยัง UTF-8 string
 * @param unicode ตัวชี้สำหรับเก็บ Unicode
 * @return จำนวน bytes ที่ใช้
 */
uint8_t OLED_UTF8ToUnicode(const char* utf8, uint16_t* unicode);

/**
 * @brief ตรวจสอบว่าเป็นตัวอักษรไทยหรือไม่
 * @param unicode รหัส Unicode
 * @return 1 = ไทย, 0 = ไม่ใช่
 */
uint8_t OLED_IsThaiChar(uint16_t unicode);

#ifdef __cplusplus
}
#endif

#endif  // __OLED_FONTS_H
