/**
 * @file WS2812Matrix.h
 * @brief WS2812 8x8 LED Matrix Library สำหรับ CH57x series
 * @version 1.0
 * @date 2026-05-03
 *
 * @details
 * Library สำหรับควบคุม WS2812 RGB LEDs ที่ต่อเป็น Matrix 8x8
 * ใช้ NeoPixel เป็น low-level driver + Instance struct pattern
 * รองรับ 2 wiring patterns: Zigzag (ซิกแซก) และ Snake (งูเลื้อย)
 *
 * **คุณสมบัติ:**
 * - ควบคุม Matrix 8x8 (ขยายได้ถึง 32x32)
 * - รูปแบบการต่อสาย: Zigzag และ Snake
 * - ระบบพิกัด (x, y) ที่ใช้งานง่าย
 * - ฟังก์ชันวาดรูปพื้นฐาน (เส้น, สี่เหลี่ยม, วงกลม)
 * - ใช้ SimpleGPIO pins (PC4, PD2, ...)
 * - Instance struct pattern (แบบ P10/PIR/HCSR04)
 * - Brightness control (0-255)
 *
 * **WS2812 Protocol ใช้ร่วมกับ:**
 * - WS2812B, WS2813, WS2815, SK6812 — timing protocol เดียวกัน
 *
 * **Hardware Connection:**
 * ```
 *   WS2812 8x8 Matrix       CH57x
 *   DIN  (Data In)  ------> GPIO Pin (PC4)
 *   VCC              ------> 5V
 *   GND              ------> GND
 *
 *   หมายเหตุ: บาง matrix มี DOUT (Data Out) — ใช้ cascade ต่อพ่วงแผงถัดไป
 *   ต้องใช้ Level Shifter ถ้า MCU 3.3V (CH57x) และ Matrix 5V
 * ```
 *
 * **Wiring Patterns:**
 *
 * Zigzag (ซิกแซกเริ่มจากซ้าย):
 * ```
 * Row 0:  0→ 1→ 2→ 3→ 4→ 5→ 6→ 7
 * Row 1: 15←14←13←12←11←10← 9← 8
 * Row 2: 16→17→18→19→20→21→22→23
 * Row 3: 31←30←29←28←27←26←25←24
 * ```
 *
 * Snake (งูเลื้อยต่อเนื่อง):
 * ```
 * Row 0:  0→ 1→ 2→ 3→ 4→ 5→ 6→ 7
 * Row 1:  8→ 9→10→11→12→13→14→15
 * Row 2: 16→17→18→19→20→21→22→23
 * Row 3: 24→25→26→27→28→29→30→31
 * ```
 *
 * @example
 * #include "WS2812Matrix.h"
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     WS2812M_Instance matrix;
 *     WS2812M_Init(&matrix, PC4, 8, 8, WIRING_ZIGZAG);
 *
 *     WS2812M_Clear(&matrix);
 *     WS2812M_SetPixel(&matrix, 3, 3, 255, 0, 0);  // จุดกลางสีแดง
 *     WS2812M_Show(&matrix);
 *
 *     while(1);
 * }
 *
 * @note ต้องเรียก SystemCoreClockUpdate() และ Timer_Init() ก่อน
 * @note NeoPixel ใช้ static buffer → รองรับ 1 instance เท่านั้น
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __WS2812MATRIX_H
#define __WS2812MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "../NeoPixel/NeoPixel.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* ==================================================================
 *  Feature Configuration Macros (overrideable)
 * ================================================================== */

#ifndef WS2812M_ENABLE_THAI
#define WS2812M_ENABLE_THAI     1   /**< 1=include Thai fonts, 0=ASCII only */
#endif

#ifndef WS2812M_ENABLE_EFFECTS
#define WS2812M_ENABLE_EFFECTS  1   /**< 1=include fade/wipe effects */
#endif

#ifndef WS2812M_ENABLE_SPRITES
#define WS2812M_ENABLE_SPRITES  1   /**< 1=include sprite/bitmap functions */
#endif

/* ========== Configuration (overrideable) ========== */

#ifndef WS2812M_MAX_WIDTH
#define WS2812M_MAX_WIDTH   32  /**< ความกว้างสูงสุด (px) */
#endif

#ifndef WS2812M_MAX_HEIGHT
#define WS2812M_MAX_HEIGHT  32  /**< ความสูงสูงสุด (px) */
#endif

#ifndef WS2812M_MAX_INSTANCES
#define WS2812M_MAX_INSTANCES  1  /**< จำนวน instances สูงสุด (NeoPixel รองรับ 1) */
#endif

/* ========== Wiring Pattern Enum ========== */

/**
 * @brief รูปแบบการต่อสาย LED Matrix
 *
 * **WIRING_ZIGZAG**: ซิกแซกเริ่มจากซ้าย (ใช้บ่อยที่สุด)
 * - แถวคู่: วิ่งซ้าย→ขวา
 * - แถวคี่: วิ่งขวา→ซ้าย
 *
 * **WIRING_SNAKE**: งูเลื้อยต่อเนื่อง
 * - ทุกแถววิ่งซ้าย→ขวา
 */
typedef enum {
    WIRING_ZIGZAG = 0,  /**< ซิกแซกเริ่มจากซ้าย (default) */
    WIRING_SNAKE  = 1   /**< งูเลื้อยแบบต่อเนื่อง */
} WS2812M_Wiring;

/* ========== Instance Structure ========== */

/**
 * @brief WS2812 Matrix Instance
 */
typedef struct {
    /* ----- Configuration ----- */
    uint8_t          data_pin;     /**< SimpleGPIO data pin (PC4, PD2, ...) */
    uint8_t          width;        /**< ความกว้าง (px) */
    uint8_t          height;       /**< ความสูง (px) */
    uint16_t         num_pixels;   /**< จำนวน LEDs ทั้งหมด (width × height) */
    WS2812M_Wiring   wiring;       /**< รูปแบบการต่อสาย */

    /* ----- Flags ----- */
    uint8_t initialized;           /**< Initialized flag */
} WS2812M_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น WS2812 LED Matrix
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param data_pin SimpleGPIO data pin (PC4, PD2, ...)
 * @param width ความกว้างของ matrix (px)
 * @param height ความสูงของ matrix (px)
 * @param wiring รูปแบบการต่อสาย (WIRING_ZIGZAG หรือ WIRING_SNAKE)
 * @return 1 = สำเร็จ, 0 = ล้มเหลว
 *
 * @example
 * WS2812M_Instance matrix;
 * WS2812M_Init(&matrix, PC4, 8, 8, WIRING_ZIGZAG);
 */
uint8_t WS2812M_Init(WS2812M_Instance* inst, uint8_t data_pin,
                     uint8_t width, uint8_t height, WS2812M_Wiring wiring);

/**
 * @brief ตั้งค่าสีของพิกเซลที่ตำแหน่ง (x, y)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X (0 = ซ้ายสุด)
 * @param y พิกัด Y (0 = บนสุด)
 * @param r ค่าสีแดง (0-255)
 * @param g ค่าสีเขียว (0-255)
 * @param b ค่าสีน้ำเงิน (0-255)
 *
 * @note ต้องเรียก WS2812M_Show() เพื่ออัพเดทการแสดงผล
 *
 * @example
 * WS2812M_SetPixel(&matrix, 3, 3, 255, 0, 0);
 * WS2812M_Show(&matrix);
 */
void WS2812M_SetPixel(WS2812M_Instance* inst, uint8_t x, uint8_t y,
                      uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief ตั้งค่าสีของพิกเซลด้วย 32-bit color
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param color สี 32-bit (0xRRGGBB)
 *
 * @example
 * WS2812M_SetPixelColor(&matrix, 3, 3, 0xFF0000);  // สีแดง
 */
void WS2812M_SetPixelColor(WS2812M_Instance* inst, uint8_t x, uint8_t y,
                           uint32_t color);

/**
 * @brief อ่านค่าสีของพิกเซล
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X
 * @param y พิกัด Y
 * @return สี 32-bit (0xRRGGBB) หรือ 0 ถ้า error
 *
 * @example
 * uint32_t color = WS2812M_GetPixel(&matrix, 3, 3);
 */
uint32_t WS2812M_GetPixel(WS2812M_Instance* inst, uint8_t x, uint8_t y);

/**
 * @brief ลบ Matrix ทั้งหมด (ดับทุกดวง)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 */
void WS2812M_Clear(WS2812M_Instance* inst);

/**
 * @brief เติมสีทั้ง Matrix
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 *
 * @example
 * WS2812M_Fill(&matrix, 0, 255, 0);  // เติมสีเขียว
 */
void WS2812M_Fill(WS2812M_Instance* inst, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief อัพเดทการแสดงผล (ส่งข้อมูลไปยัง LEDs)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 *
 * @note ต้องเรียกหลังจาก SetPixel / Clear / Fill
 *
 * @example
 * WS2812M_SetPixel(&matrix, 0, 0, 255, 0, 0);
 * WS2812M_Show(&matrix);
 */
void WS2812M_Show(WS2812M_Instance* inst);

/**
 * @brief ตั้งค่าความสว่าง (0-255)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param brightness ความสว่าง (0=ดับ, 255=สว่างสุด)
 *
 * @example
 * WS2812M_SetBrightness(&matrix, 50);  // ลดความสว่าง
 */
void WS2812M_SetBrightness(WS2812M_Instance* inst, uint8_t brightness);

/**
 * @brief หยุดการทำงานของ Matrix
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 */
void WS2812M_Deinit(WS2812M_Instance* inst);

/* ========== Drawing Primitives ========== */

/**
 * @brief วาดเส้นตรง
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x0 พิกัด X จุดเริ่มต้น
 * @param y0 พิกัด Y จุดเริ่มต้น
 * @param x1 พิกัด X จุดสิ้นสุด
 * @param y1 พิกัด Y จุดสิ้นสุด
 * @param r Red
 * @param g Green
 * @param b Blue
 *
 * ใช้ Bresenham's line algorithm
 */
void WS2812M_DrawLine(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดเส้นตรงด้วย 32-bit color
 */
void WS2812M_DrawLineColor(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1, uint32_t color);

/**
 * @brief วาดสี่เหลี่ยม (เฉพาะขอบ)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X มุมซ้ายบน
 * @param y พิกัด Y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 */
void WS2812M_DrawRect(WS2812M_Instance* inst, int16_t x, int16_t y,
                      uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดสี่เหลี่ยมแบบทึบ
 */
void WS2812M_FillRect(WS2812M_Instance* inst, int16_t x, int16_t y,
                      uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดวงกลม (เฉพาะขอบ)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x0 พิกัด X ศูนย์กลาง
 * @param y0 พิกัด Y ศูนย์กลาง
 * @param radius รัศมี
 */
void WS2812M_DrawCircle(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                        uint8_t radius, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดวงกลมแบบทึบ
 */
void WS2812M_FillCircle(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                        uint8_t radius, uint8_t r, uint8_t g, uint8_t b);

/* ========== Utility ========== */

/**
 * @brief แปลงพิกัด (x, y) เป็น LED index ตาม wiring pattern
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param width ความกว้างของ matrix
 * @param wiring รูปแบบการต่อสาย
 * @return LED index (0-based)
 *
 * @note ฟังก์ชัน internal แต่เปิดให้เรียกใช้เพื่อ debug
 *
 * @example
 * uint16_t idx = WS2812M_XYtoIndex(3, 3, 8, WIRING_ZIGZAG);
 */
uint16_t WS2812M_XYtoIndex(uint8_t x, uint8_t y, uint8_t width,
                           WS2812M_Wiring wiring);

/* ==================================================================
 *  Text Rendering (ต้อง include WS2812M_Fonts.h ด้วย)
 * ================================================================== */

/**
 * @brief วาดตัวอักษร ASCII 5x7
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param c ตัวอักษร (32-126)
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return ความกว้างของตัวอักษร (pixels) รวม spacing
 *
 * @example
 * WS2812M_DrawChar(&matrix, 0, 0, 'A', 255, 0, 0);
 * WS2812M_Show(&matrix);
 */
uint8_t WS2812M_DrawChar(WS2812M_Instance* inst, int16_t x, int16_t y,
                         char c, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดตัวอักษร ASCII ด้วย 32-bit color
 */
uint8_t WS2812M_DrawCharColor(WS2812M_Instance* inst, int16_t x, int16_t y,
                              char c, uint32_t color);

/**
 * @brief วาดข้อความ ASCII
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X เริ่มต้น
 * @param y พิกัด Y
 * @param text ข้อความ (null-terminated)
 * @param r Red
 * @param g Green
 * @param b Blue
 * @return ความกว้างรวม (pixels)
 *
 * @example
 * WS2812M_DrawText(&matrix, 0, 0, "Hello!", 0, 255, 0);
 * WS2812M_Show(&matrix);
 */
uint16_t WS2812M_DrawText(WS2812M_Instance* inst, int16_t x, int16_t y,
                          const char* text, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดข้อความ ASCII ด้วย 32-bit color
 */
uint16_t WS2812M_DrawTextColor(WS2812M_Instance* inst, int16_t x, int16_t y,
                               const char* text, uint32_t color);

#if (WS2812M_ENABLE_THAI)

/**
 * @brief วาดตัวอักษรไทย 8x8 (UTF-8 encoded)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param thai_char ตัวอักษรไทย UTF-8 (3 bytes)
 * @param color สี 32-bit
 * @return ความกว้าง (8 pixels) หรือ 0 ถ้าไม่พบฟอนต์
 *
 * @example
 * WS2812M_DrawCharThai(&matrix, 0, 0, "ก", COLOR_BLUE);
 * WS2812M_Show(&matrix);
 */
uint8_t WS2812M_DrawCharThai(WS2812M_Instance* inst, int16_t x, int16_t y,
                             const char* thai_char, uint32_t color);

/**
 * @brief วาดข้อความภาษาไทย (UTF-8, รองรับผสมภาษาอังกฤษ)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X เริ่มต้น
 * @param y พิกัด Y
 * @param text ข้อความ UTF-8 (null-terminated)
 * @param color สี 32-bit
 * @return ความกว้างรวม
 *
 * @example
 * WS2812M_DrawTextThai(&matrix, 0, 0, "สวัสดี", COLOR_CYAN);
 * WS2812M_Show(&matrix);
 */
uint16_t WS2812M_DrawTextThai(WS2812M_Instance* inst, int16_t x, int16_t y,
                              const char* text, uint32_t color);

#endif /* WS2812M_ENABLE_THAI */

/**
 * @brief คำนวณความกว้างของข้อความ (pixels)
 * @param text ข้อความ (ASCII หรือ UTF-8)
 * @return ความกว้างรวม
 *
 * @example
 * uint16_t w = WS2812M_GetTextWidth("Hello");
 */
uint16_t WS2812M_GetTextWidth(const char* text);

/* ==================================================================
 *  Scrolling Text
 * ================================================================== */

/**
 * @brief โครงสร้างสำหรับ Scrolling Text
 */
typedef struct {
    char     text[128];      /**< ข้อความที่จะเลื่อน */
    int16_t  position;       /**< ตำแหน่งปัจจุบัน */
    uint32_t color;          /**< สีของข้อความ */
    uint16_t speed;          /**< ความเร็ว (ms ต่อ step) */
    uint32_t last_update;    /**< เวลาอัพเดตล่าสุด (ms) */
    bool     active;         /**< เปิด/ปิด scrolling */
    bool     vertical;       /**< true=แนวตั้ง, false=แนวนอน */
} WS2812M_ScrollText;

/**
 * @brief เริ่มต้น scrolling text
 * @param scroll ตัวชี้ไปยัง WS2812M_ScrollText
 * @param text ข้อความ
 * @param color สี 32-bit
 * @param speed ความเร็ว (ms ต่อ step)
 * @param vertical true=แนวตั้ง, false=แนวนอน
 *
 * @example
 * WS2812M_ScrollText scroll;
 * WS2812M_ScrollTextInit(&scroll, "Hello", COLOR_RED, 100, false);
 */
void WS2812M_ScrollTextInit(WS2812M_ScrollText* scroll, const char* text,
                            uint32_t color, uint16_t speed, bool vertical);

/**
 * @brief อัปเดต scrolling text (เรียกใน loop)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param scroll ตัวชี้ไปยัง WS2812M_ScrollText
 * @param y แถว Y ที่แสดง (สำหรับแนวนอน)
 * @return true=มีการอัปเดต, false=ยังไม่ถึงเวลา
 *
 * @example
 * // ใน while(1):
 * WS2812M_ScrollTextUpdate(&matrix, &scroll, 0);
 */
bool WS2812M_ScrollTextUpdate(WS2812M_Instance* inst,
                              WS2812M_ScrollText* scroll, int16_t y);

/**
 * @brief หยุด scrolling text
 * @param scroll ตัวชี้ไปยัง WS2812M_ScrollText
 */
void WS2812M_ScrollTextStop(WS2812M_ScrollText* scroll);

/* ==================================================================
 *  Sprite / Bitmap
 * ================================================================== */

#if (WS2812M_ENABLE_SPRITES)

/**
 * @brief โครงสร้าง Sprite
 */
typedef struct {
    uint8_t         width;              /**< ความกว้าง (px) */
    uint8_t         height;             /**< ความสูง (px) */
    const uint32_t* data;               /**< ข้อมูลสี (array ของ 0xRRGGBB) */
    bool            has_transparency;   /**< มี transparency หรือไม่ */
    uint32_t        transparent_color;  /**< สีที่ถือว่าโปร่งใส */
} WS2812M_Sprite;

/**
 * @brief วาด Sprite
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param sprite ตัวชี้ไปยัง Sprite
 *
 * @example
 * const uint32_t heart[] = { 0, RED, RED, 0, RED, RED, RED, RED, ... };
 * WS2812M_Sprite spr = { 4, 4, heart, false, 0 };
 * WS2812M_DrawSprite(&matrix, 2, 2, &spr);
 */
void WS2812M_DrawSprite(WS2812M_Instance* inst, int16_t x, int16_t y,
                        const WS2812M_Sprite* sprite);

/**
 * @brief วาด Bitmap (1-bit monochrome)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param bitmap ข้อมูล bitmap (1 bit per pixel, MSB-first)
 * @param w ความกว้าง
 * @param h ความสูง
 * @param color สี 32-bit
 *
 * @example
 * const uint8_t arrow[] = { 0x08, 0x0C, 0xFE, 0x0C, 0x08 };
 * WS2812M_DrawBitmap(&matrix, 2, 2, arrow, 8, 5, COLOR_GREEN);
 */
void WS2812M_DrawBitmap(WS2812M_Instance* inst, int16_t x, int16_t y,
                        const uint8_t* bitmap, uint8_t w, uint8_t h,
                        uint32_t color);

#endif /* WS2812M_ENABLE_SPRITES */

/* ==================================================================
 *  Effects (blocking — ใช้ Delay_Ms ภายใน)
 * ================================================================== */

#if (WS2812M_ENABLE_EFFECTS)

/**
 * @brief Fade In — ค่อยๆ เพิ่มความสว่าง
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param duration_ms ระยะเวลาทั้งหมด (ms)
 * @param steps จำนวนขั้น
 *
 * @note ฟังก์ชันนี้เป็น blocking — ใช้ Delay_Ms ภายใน
 *
 * @example
 * WS2812M_FadeIn(&matrix, 500, 10);
 */
void WS2812M_FadeIn(WS2812M_Instance* inst, uint16_t duration_ms,
                    uint8_t steps);

/**
 * @brief Fade Out — ค่อยๆ ลดความสว่าง
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param duration_ms ระยะเวลาทั้งหมด (ms)
 * @param steps จำนวนขั้น
 */
void WS2812M_FadeOut(WS2812M_Instance* inst, uint16_t duration_ms,
                     uint8_t steps);

/**
 * @brief Wipe Transition — ค่อยๆ เติมสีจากซ้ายไปขวา
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param color สี 32-bit
 * @param delay_ms หน่วงเวลาต่อคอลัมน์
 */
void WS2812M_WipeTransition(WS2812M_Instance* inst, uint32_t color,
                            uint16_t delay_ms);

/**
 * @brief Slide Transition — ค่อยๆ เติมสีจากบนลงล่าง
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @param color สี 32-bit
 * @param delay_ms หน่วงเวลาต่อแถว
 */
void WS2812M_SlideTransition(WS2812M_Instance* inst, uint32_t color,
                             uint16_t delay_ms);

#endif /* WS2812M_ENABLE_EFFECTS */

/* ==================================================================
 *  Buffer Utilities
 * ================================================================== */

/**
 * @brief หมุน buffer 90° ตามเข็มนาฬิกา
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 * @note ใช้ได้เฉพาะ square matrix (width == height)
 */
void WS2812M_Rotate90CW(WS2812M_Instance* inst);

/**
 * @brief หมุน buffer 90° ทวนเข็มนาฬิกา
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 */
void WS2812M_Rotate90CCW(WS2812M_Instance* inst);

/**
 * @brief สะท้อน buffer แนวนอน (ซ้าย↔ขวา)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 */
void WS2812M_FlipHorizontal(WS2812M_Instance* inst);

/**
 * @brief สะท้อน buffer แนวตั้ง (บน↔ล่าง)
 * @param inst ตัวชี้ไปยัง WS2812M_Instance
 */
void WS2812M_FlipVertical(WS2812M_Instance* inst);

#ifdef __cplusplus
}
#endif

#endif /* __WS2812MATRIX_H */
