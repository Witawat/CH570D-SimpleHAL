/**
 * @file SimpleWS2815Matrix.h
 * @brief WS2815 LED Matrix Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับควบคุม WS2815 LED Matrix แบบ 8x8
 * รองรับรูปแบบการต่อสายแบบ Zigzag และ Snake
 * มีฟังก์ชันวาดรูปและแสดงข้อความภาษาไทย/อังกฤษ
 * 
 * **คุณสมบัติ:**
 * - รองรับ Matrix ขนาด 8x8 (ขยายได้ถึง 32x32)
 * - รูปแบบการต่อสาย: Zigzag (ซิกแซก) และ Snake (งูเลื้อย)
 * - ระบบพิกกัด (x, y) ที่ใช้งานง่าย
 * - ฟังก์ชันวาดรูปพื้นฐาน (เส้น, สี่เหลี่ยม, วงกลม)
 * - รองรับข้อความภาษาไทยและอังกฤษ
 * - Effect และ Animation ในตัว
 * - Non-blocking operation
 * 
 * **WS2815 vs WS2812:**
 * - WS2815: 12V, มี backup data line, เสถียรกว่า
 * - WS2812: 5V, ราคาถูกกว่า
 * - Timing protocol เหมือนกัน
 * 
 * @example
 * #include "WS2815Matrix.h"
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Delay_Init();
 *     
 *     // เริ่มต้น Matrix 8x8 แบบ Zigzag บน pin PC4
 *     Matrix_Init(GPIOC, GPIO_Pin_4, 8, 8, WIRING_ZIGZAG_LEFT);
 *     
 *     // วาดพิกเซลที่ตำแหน่ง (3, 3) สีแดง
 *     Matrix_SetPixel(3, 3, 255, 0, 0);
 *     Matrix_Show();
 *     
 *     // แสดงข้อความ
 *     Matrix_DrawText(0, 0, "Hi", COLOR_GREEN);
 *     Matrix_Show();
 * }
 * 
 * @note ใช้ร่วมกับ SimpleNeoPixel library
 */

#ifndef __WS2815MATRIX_H
#define __WS2815MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../NeoPixel/NeoPixel.h"
#include "debug.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

#define MATRIX_MAX_WIDTH   32  /**< ความกว้างสูงสุด */
#define MATRIX_MAX_HEIGHT  32  /**< ความสูงสูงสุด */
#define MATRIX_MAX_PIXELS  (MATRIX_MAX_WIDTH * MATRIX_MAX_HEIGHT)

/* ========== Wiring Pattern Enums ========== */

/**
 * @brief รูปแบบการต่อสาย LED Matrix
 * 
 * @details
 * **WIRING_ZIGZAG_LEFT**: ซิกแซกเริ่มจากซ้าย
 * ```
 * Row 0:  0→ 1→ 2→ 3→ 4→ 5→ 6→ 7
 * Row 1: 15←14←13←12←11←10← 9← 8
 * Row 2: 16→17→18→19→20→21→22→23
 * Row 3: 31←30←29←28←27←26←25←24
 * ```
 * 
 * **WIRING_SNAKE**: งูเลื้อยต่อเนื่อง
 * ```
 * Row 0:  0→ 1→ 2→ 3→ 4→ 5→ 6→ 7
 * Row 1:  8→ 9→10→11→12→13→14→15
 * Row 2: 16→17→18→19→20→21→22→23
 * Row 3: 24→25→26→27→28→29→30→31
 * ```
 */
typedef enum {
    WIRING_ZIGZAG_LEFT = 0,  /**< ซิกแซกเริ่มจากซ้าย (ใช้บ่อยที่สุด) */
    WIRING_SNAKE = 1,        /**< งูเลื้อยแบบต่อเนื่อง */
    WIRING_ZIGZAG_RIGHT = 2, /**< ซิกแซกเริ่มจากขวา */
    WIRING_COLUMNS = 3       /**< เรียงตามคอลัมน์ */
} WiringPattern_e;

/* ========== Structures ========== */

/**
 * @brief โครงสร้างจุด 2D
 */
typedef struct {
    int16_t x;  /**< พิกัด X */
    int16_t y;  /**< พิกัด Y */
} Point_t;

/**
 * @brief โครงสร้างสี่เหลี่ยม
 */
typedef struct {
    int16_t x;       /**< พิกัด X มุมซ้ายบน */
    int16_t y;       /**< พิกัด Y มุมซ้ายบน */
    uint8_t width;   /**< ความกว้าง */
    uint8_t height;  /**< ความสูง */
} Rect_t;

/**
 * @brief โครงสร้าง Sprite
 */
typedef struct {
    uint8_t width;          /**< ความกว้าง */
    uint8_t height;         /**< ความสูง */
    const uint32_t* data;   /**< ข้อมูลสี (array ของ RGB) */
    bool has_transparency;  /**< มี transparency หรือไม่ */
    uint32_t transparent_color; /**< สีที่ถือว่าโปร่งใส */
} Sprite_t;

/**
 * @brief โครงสร้างการตั้งค่า Matrix
 */
typedef struct {
    GPIO_TypeDef* gpio_port;   /**< GPIO port */
    uint16_t gpio_pin;         /**< GPIO pin */
    uint8_t width;             /**< ความกว้าง */
    uint8_t height;            /**< ความสูง */
    uint16_t num_pixels;       /**< จำนวนพิกเซลทั้งหมด */
    WiringPattern_e wiring;    /**< รูปแบบการต่อสาย */
    bool initialized;          /**< สถานะการเริ่มต้น */
} Matrix_Config_t;

/**
 * @brief โครงสร้างสำหรับ Scrolling Text
 */
typedef struct {
    char text[128];         /**< ข้อความที่จะเลื่อน */
    int16_t position;       /**< ตำแหน่งปัจจุบัน */
    uint32_t color;         /**< สีของข้อความ */
    uint16_t speed;         /**< ความเร็ว (ms per step) */
    uint32_t last_update;   /**< เวลาอัพเดทล่าสุด */
    bool active;            /**< สถานะการทำงาน */
    bool vertical;          /**< เลื่อนแนวตั้งหรือไม่ */
} ScrollText_t;

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน LED Matrix
 * @param port GPIO port (เช่น GPIOC)
 * @param pin GPIO pin (เช่น GPIO_Pin_4)
 * @param width ความกว้างของ Matrix
 * @param height ความสูงของ Matrix
 * @param wiring รูปแบบการต่อสาย
 * 
 * @example
 * Matrix_Init(GPIOC, GPIO_Pin_4, 8, 8, WIRING_ZIGZAG_LEFT);
 */
void Matrix_Init(GPIO_TypeDef* port, uint16_t pin, uint8_t width, uint8_t height, WiringPattern_e wiring);

/**
 * @brief เปลี่ยนรูปแบบการต่อสาย
 * @param wiring รูปแบบการต่อสายใหม่
 * 
 * @example
 * Matrix_SetWiringPattern(WIRING_SNAKE);
 */
void Matrix_SetWiringPattern(WiringPattern_e wiring);

/**
 * @brief อ่านข้อมูลการตั้งค่า Matrix
 * @return ตัวชี้ไปยังโครงสร้างการตั้งค่า
 */
Matrix_Config_t* Matrix_GetConfig(void);

/* ========== Basic Drawing Functions ========== */

/**
 * @brief ตั้งค่าสีของพิกเซลที่ตำแหน่ง (x, y)
 * @param x พิกัด X (0 = ซ้ายสุด)
 * @param y พิกัด Y (0 = บนสุด)
 * @param r ค่าสีแดง (0-255)
 * @param g ค่าสีเขียว (0-255)
 * @param b ค่าสีน้ำเงิน (0-255)
 * 
 * @note ต้องเรียก Matrix_Show() เพื่ออัพเดทการแสดงผล
 * 
 * @example
 * Matrix_SetPixel(3, 3, 255, 0, 0);  // พิกเซลกลาง Matrix สีแดง
 * Matrix_Show();
 */
void Matrix_SetPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief ตั้งค่าสีของพิกเซลด้วย 32-bit color
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param color สี 32-bit (0xRRGGBB)
 * 
 * @example
 * Matrix_SetPixelColor(3, 3, COLOR_RED);
 */
void Matrix_SetPixelColor(int16_t x, int16_t y, uint32_t color);

/**
 * @brief อ่านค่าสีของพิกเซล
 * @param x พิกัด X
 * @param y พิกัด Y
 * @return สี 32-bit (0xRRGGBB)
 * 
 * @example
 * uint32_t color = Matrix_GetPixel(3, 3);
 */
uint32_t Matrix_GetPixel(int16_t x, int16_t y);

/**
 * @brief ล้าง Matrix ทั้งหมด (ดับทุกดวง)
 * 
 * @note ต้องเรียก Matrix_Show() เพื่ออัพเดทการแสดงผล
 * 
 * @example
 * Matrix_Clear();
 * Matrix_Show();
 */
void Matrix_Clear(void);

/**
 * @brief เติมสีทั้ง Matrix
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @example
 * Matrix_Fill(255, 0, 0);  // เติมสีแดงทั้งหมด
 * Matrix_Show();
 */
void Matrix_Fill(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief เติมสีทั้ง Matrix ด้วย 32-bit color
 * @param color สี 32-bit
 * 
 * @example
 * Matrix_FillColor(COLOR_BLUE);
 * Matrix_Show();
 */
void Matrix_FillColor(uint32_t color);

/**
 * @brief อัพเดทการแสดงผล (ส่งข้อมูลไปยัง LEDs)
 * 
 * @note ต้องเรียกฟังก์ชันนี้หลังจากเปลี่ยนแปลงพิกเซล
 * 
 * @example
 * Matrix_SetPixel(0, 0, 255, 0, 0);
 * Matrix_Show();
 */
void Matrix_Show(void);

/* ========== Shape Drawing Functions ========== */

/**
 * @brief วาดเส้นตรง
 * @param x0 พิกัด X จุดเริ่มต้น
 * @param y0 พิกัด Y จุดเริ่มต้น
 * @param x1 พิกัด X จุดสิ้นสุด
 * @param y1 พิกัด Y จุดสิ้นสุด
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @note ใช้ Bresenham's algorithm
 * 
 * @example
 * Matrix_DrawLine(0, 0, 7, 7, 255, 0, 0);  // เส้นทแยงสีแดง
 */
void Matrix_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดเส้นตรงด้วย 32-bit color
 */
void Matrix_DrawLineColor(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color);

/**
 * @brief วาดสี่เหลี่ยม (เส้นขอบ)
 * @param x พิกัด X มุมซ้ายบน
 * @param y พิกัด Y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @example
 * Matrix_DrawRect(1, 1, 6, 6, 0, 255, 0);  // กรอบสีเขียว
 */
void Matrix_DrawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดสี่เหลี่ยมเติมสี
 * @param x พิกัด X มุมซ้ายบน
 * @param y พิกัด Y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @example
 * Matrix_FillRect(2, 2, 4, 4, 0, 0, 255);  // สี่เหลี่ยมสีน้ำเงิน
 */
void Matrix_FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดวงกลม (เส้นขอบ)
 * @param x0 พิกัด X จุดศูนย์กลาง
 * @param y0 พิกัด Y จุดศูนย์กลาง
 * @param radius รัศมี
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @note ใช้ Midpoint Circle algorithm
 * 
 * @example
 * Matrix_DrawCircle(3, 3, 3, 255, 255, 0);  // วงกลมสีเหลือง
 */
void Matrix_DrawCircle(int16_t x0, int16_t y0, uint8_t radius, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief วาดวงกลมเติมสี
 * @param x0 พิกัด X จุดศูนย์กลาง
 * @param y0 พิกัด Y จุดศูนย์กลาง
 * @param radius รัศมี
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @example
 * Matrix_FillCircle(3, 3, 2, 255, 0, 255);  // วงกลมสีม่วง
 */
void Matrix_FillCircle(int16_t x0, int16_t y0, uint8_t radius, uint8_t r, uint8_t g, uint8_t b);

/* ========== Text Drawing Functions ========== */

/**
 * @brief วาดตัวอักษร ASCII
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param c ตัวอักษร
 * @param color สี 32-bit
 * @return ความกว้างของตัวอักษร (pixels)
 * 
 * @note ใช้ฟอนต์ 5x7 pixels
 * 
 * @example
 * Matrix_DrawChar(0, 0, 'A', COLOR_RED);
 */
uint8_t Matrix_DrawChar(int16_t x, int16_t y, char c, uint32_t color);

/**
 * @brief วาดข้อความ ASCII
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param text ข้อความ
 * @param color สี 32-bit
 * @return ความกว้างของข้อความทั้งหมด (pixels)
 * 
 * @example
 * Matrix_DrawText(0, 0, "Hi", COLOR_GREEN);
 * Matrix_Show();
 */
uint16_t Matrix_DrawText(int16_t x, int16_t y, const char* text, uint32_t color);

/**
 * @brief วาดตัวอักษรภาษาไทย (UTF-8)
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param thai_char ตัวอักษรไทย (UTF-8, 3 bytes)
 * @param color สี 32-bit
 * @return ความกว้างของตัวอักษร (pixels)
 * 
 * @note ใช้ฟอนต์ 8x8 pixels สำหรับภาษาไทย
 * 
 * @example
 * Matrix_DrawCharThai(0, 0, "ก", COLOR_BLUE);
 */
uint8_t Matrix_DrawCharThai(int16_t x, int16_t y, const char* thai_char, uint32_t color);

/**
 * @brief วาดข้อความภาษาไทย (UTF-8)
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param text ข้อความภาษาไทย (UTF-8)
 * @param color สี 32-bit
 * @return ความกว้างของข้อความทั้งหมด (pixels)
 * 
 * @example
 * Matrix_DrawTextThai(0, 0, "สวัสดี", COLOR_CYAN);
 * Matrix_Show();
 */
uint16_t Matrix_DrawTextThai(int16_t x, int16_t y, const char* text, uint32_t color);

/**
 * @brief คำนวณความกว้างของข้อความ
 * @param text ข้อความ
 * @return ความกว้าง (pixels)
 */
uint16_t Matrix_GetTextWidth(const char* text);

/* ========== Scrolling Text Functions ========== */

/**
 * @brief เริ่มต้นการเลื่อนข้อความ
 * @param scroll ตัวชี้ไปยังโครงสร้าง ScrollText_t
 * @param text ข้อความที่จะเลื่อน
 * @param color สีของข้อความ
 * @param speed ความเร็ว (ms per step)
 * @param vertical เลื่อนแนวตั้ง (true) หรือแนวนอน (false)
 * 
 * @example
 * ScrollText_t scroll;
 * Matrix_ScrollTextInit(&scroll, "Hello World", COLOR_RED, 100, false);
 */
void Matrix_ScrollTextInit(ScrollText_t* scroll, const char* text, uint32_t color, uint16_t speed, bool vertical);

/**
 * @brief อัพเดทการเลื่อนข้อความ (เรียกใน main loop)
 * @param scroll ตัวชี้ไปยังโครงสร้าง ScrollText_t
 * @param y พิกัด Y (สำหรับ horizontal scroll)
 * @return true = มีการอัพเดท, false = ยังไม่ถึงเวลา
 * 
 * @example
 * while(1) {
 *     if(Matrix_ScrollTextUpdate(&scroll, 0)) {
 *         Matrix_Show();
 *     }
 * }
 */
bool Matrix_ScrollTextUpdate(ScrollText_t* scroll, int16_t y);

/**
 * @brief หยุดการเลื่อนข้อความ
 * @param scroll ตัวชี้ไปยังโครงสร้าง ScrollText_t
 */
void Matrix_ScrollTextStop(ScrollText_t* scroll);

/* ========== Sprite Functions ========== */

/**
 * @brief วาด Sprite
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param sprite ตัวชี้ไปยังโครงสร้าง Sprite_t
 * 
 * @example
 * const uint32_t heart_data[] = {
 *     0x000000, 0xFF0000, 0x000000, 0xFF0000, 0x000000,
 *     0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000,
 *     0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000,
 *     0x000000, 0xFF0000, 0xFF0000, 0xFF0000, 0x000000,
 *     0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000
 * };
 * Sprite_t heart = {5, 5, heart_data, false, 0};
 * Matrix_DrawSprite(1, 1, &heart);
 */
void Matrix_DrawSprite(int16_t x, int16_t y, const Sprite_t* sprite);

/**
 * @brief วาด Bitmap (ข้อมูลแบบ 1-bit)
 * @param x พิกัด X
 * @param y พิกัด Y
 * @param bitmap ข้อมูล bitmap
 * @param w ความกว้าง
 * @param h ความสูง
 * @param color สี
 * 
 * @example
 * const uint8_t smile[] = {
 *     0b00111100,
 *     0b01000010,
 *     0b10100101,
 *     0b10000001,
 *     0b10100101,
 *     0b10011001,
 *     0b01000010,
 *     0b00111100
 * };
 * Matrix_DrawBitmap(0, 0, smile, 8, 8, COLOR_YELLOW);
 */
void Matrix_DrawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t w, uint8_t h, uint32_t color);

/* ========== Advanced Effects ========== */

/**
 * @brief Fade In effect
 * @param duration ระยะเวลา (ms)
 * @param steps จำนวน steps
 * 
 * @note ฟังก์ชันนี้เป็นแบบ blocking
 * 
 * @example
 * Matrix_Fill(255, 0, 0);
 * Matrix_FadeIn(1000, 50);
 */
void Matrix_FadeIn(uint16_t duration, uint8_t steps);

/**
 * @brief Fade Out effect
 * @param duration ระยะเวลา (ms)
 * @param steps จำนวน steps
 * 
 * @example
 * Matrix_FadeOut(1000, 50);
 */
void Matrix_FadeOut(uint16_t duration, uint8_t steps);

/**
 * @brief Wipe Transition (เช็ดจากซ้ายไปขวา)
 * @param color สีใหม่
 * @param delay_ms ความล่าช้าระหว่างคอลัมน์
 * 
 * @example
 * Matrix_WipeTransition(COLOR_BLUE, 50);
 */
void Matrix_WipeTransition(uint32_t color, uint16_t delay_ms);

/**
 * @brief Slide Transition (เลื่อนจากล่างขึ้นบน)
 * @param color สีใหม่
 * @param delay_ms ความล่าช้าระหว่างแถว
 * 
 * @example
 * Matrix_SlideTransition(COLOR_GREEN, 50);
 */
void Matrix_SlideTransition(uint32_t color, uint16_t delay_ms);

/* ========== Utility Functions ========== */

/**
 * @brief หมุน buffer 90 องศาตามเข็มนาฬิกา
 * 
 * @note ใช้ได้กับ Matrix สี่เหลี่ยมจัตุรัสเท่านั้น (8x8)
 * 
 * @example
 * Matrix_RotateBuffer90CW();
 * Matrix_Show();
 */
void Matrix_RotateBuffer90CW(void);

/**
 * @brief หมุน buffer 90 องศาทวนเข็มนาฬิกา
 */
void Matrix_RotateBuffer90CCW(void);

/**
 * @brief กลับด้านแนวนอน (Mirror Horizontal)
 * 
 * @example
 * Matrix_MirrorH();
 * Matrix_Show();
 */
void Matrix_MirrorH(void);

/**
 * @brief กลับด้านแนวตั้ง (Mirror Vertical)
 * 
 * @example
 * Matrix_MirrorV();
 * Matrix_Show();
 */
void Matrix_MirrorV(void);

/**
 * @brief ตั้งค่าความสว่างทั้งหมด
 * @param brightness ความสว่าง (0-255)
 * 
 * @example
 * Matrix_SetBrightness(50);  // ลดความสว่าง
 */
void Matrix_SetBrightness(uint8_t brightness);

/**
 * @brief อ่านค่าความสว่างปัจจุบัน
 * @return ความสว่าง (0-255)
 */
uint8_t Matrix_GetBrightness(void);

/**
 * @brief แปลงพิกัด (x, y) เป็น LED index
 * @param x พิกัด X
 * @param y พิกัด Y
 * @return LED index (0-based)
 * 
 * @note ฟังก์ชันนี้ใช้ภายใน library แต่เปิดให้ใช้ได้
 * 
 * @example
 * uint16_t index = Matrix_XYtoIndex(3, 3);
 */
uint16_t Matrix_XYtoIndex(int16_t x, int16_t y);

/**
 * @brief ตรวจสอบว่าพิกัดอยู่ในขอบเขตหรือไม่
 * @param x พิกัด X
 * @param y พิกัด Y
 * @return true = อยู่ในขอบเขต, false = นอกขอบเขต
 */
bool Matrix_IsInBounds(int16_t x, int16_t y);

/* ========== Pattern Generation ========== */

/**
 * @brief สร้าง Checkerboard pattern
 * @param color1 สีที่ 1
 * @param color2 สีที่ 2
 * 
 * @example
 * Matrix_PatternCheckerboard(COLOR_RED, COLOR_BLACK);
 * Matrix_Show();
 */
void Matrix_PatternCheckerboard(uint32_t color1, uint32_t color2);

/**
 * @brief สร้าง Gradient แนวนอน
 * @param start_color สีเริ่มต้น
 * @param end_color สีสิ้นสุด
 * 
 * @example
 * Matrix_PatternGradientH(COLOR_RED, COLOR_BLUE);
 * Matrix_Show();
 */
void Matrix_PatternGradientH(uint32_t start_color, uint32_t end_color);

/**
 * @brief สร้าง Gradient แนวตั้ง
 * @param start_color สีเริ่มต้น
 * @param end_color สีสิ้นสุด
 * 
 * @example
 * Matrix_PatternGradientV(COLOR_GREEN, COLOR_YELLOW);
 * Matrix_Show();
 */
void Matrix_PatternGradientV(uint32_t start_color, uint32_t end_color);

/**
 * @brief สร้าง Random noise
 * @param density ความหนาแน่น (0-100%)
 * @param color สี
 * 
 * @example
 * Matrix_PatternRandom(50, COLOR_WHITE);  // 50% ความหนาแน่น
 * Matrix_Show();
 */
void Matrix_PatternRandom(uint8_t density, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif  // __WS2815MATRIX_H
