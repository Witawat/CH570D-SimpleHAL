/**
 * @file SimpleNeoPixel.h
 * @brief Simple NeoPixel (WS2812) Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-12
 * 
 * @details
 * Library นี้ควบคุม WS2812/WS2812B RGB LEDs (NeoPixel)
 * ใช้ bit-banging เพื่อสร้าง timing ที่ถูกต้อง
 * 
 * **คุณสมบัติ:**
 * - ควบคุม RGB LEDs หลายดวง
 * - Color functions (RGB, HSV, Wheel)
 * - Built-in effects (Rainbow, Chase, Fade, etc.)
 * - Brightness control
 * 
 * **WS2812 Timing:**
 * - 0 bit: HIGH 0.4us, LOW 0.85us
 * - 1 bit: HIGH 0.8us, LOW 0.45us
 * - Reset: LOW > 50us
 * 
 * @example
 * #include "NeoPixel.h"
 * 
 * int main(void) {
 *     // เริ่มต้น 8 LEDs บน pin PC4
 *     NeoPixel_Init(GPIOC, GPIO_Pin_4, 8);
 *     
 *     // ตั้งสี LED ดวงที่ 0 เป็นสีแดง
 *     NeoPixel_SetPixelColor(0, 255, 0, 0);
 *     NeoPixel_Show();
 * }
 * 
 * @note ต้องปิด interrupt ขณะส่งข้อมูล เพื่อ timing ที่แม่นยำ
 */

#ifndef __NEOPIXEL_H
#define __NEOPIXEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include "debug.h"
#include <stdint.h>

/* ========== Configuration ========== */

#define NEOPIXEL_MAX_LEDS  64  /**< จำนวน LEDs สูงสุด */

/* ========== Color Presets ========== */

#define COLOR_RED       0xFF0000
#define COLOR_GREEN     0x00FF00
#define COLOR_BLUE      0x0000FF
#define COLOR_YELLOW    0xFFFF00
#define COLOR_CYAN      0x00FFFF
#define COLOR_MAGENTA   0xFF00FF
#define COLOR_WHITE     0xFFFFFF
#define COLOR_ORANGE    0xFF8000
#define COLOR_PURPLE    0x8000FF
#define COLOR_PINK      0xFF1493
#define COLOR_OFF       0x000000

/* ========== Structures ========== */

/**
 * @brief โครงสร้างสี RGB
 */
typedef struct {
    uint8_t r;  /**< Red (0-255) */
    uint8_t g;  /**< Green (0-255) */
    uint8_t b;  /**< Blue (0-255) */
} RGB_Color;

/**
 * @brief โครงสร้างสี HSV
 */
typedef struct {
    uint16_t h;  /**< Hue (0-359) */
    uint8_t s;   /**< Saturation (0-255) */
    uint8_t v;   /**< Value/Brightness (0-255) */
} HSV_Color;

/**
 * @brief โครงสร้างสำหรับ Non-blocking Effect
 */
typedef struct {
    uint8_t active;        /**< Effect active flag */
    uint8_t type;          /**< Effect type */
    uint32_t last_update;  /**< Last update time (ms) */
    uint16_t step;         /**< Current step */
    uint16_t speed;        /**< Speed (ms per step) */
    uint32_t param1;       /**< Effect parameter 1 */
    uint32_t param2;       /**< Effect parameter 2 */
} NeoPixel_Effect_t;

/* Effect Types */
#define EFFECT_NONE           0
#define EFFECT_RAINBOW        1
#define EFFECT_BREATHING      2
#define EFFECT_COMET          3
#define EFFECT_SCANNER        4
#define EFFECT_TWINKLE        5
#define EFFECT_COLOR_CHASE    6

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้นการใช้งาน NeoPixel
 * @param port GPIO port (เช่น GPIOC)
 * @param pin GPIO pin (เช่น GPIO_Pin_4)
 * @param num_leds จำนวน LEDs
 * 
 * @example
 * NeoPixel_Init(GPIOC, GPIO_Pin_4, 8);
 */
void NeoPixel_Init(GPIO_TypeDef* port, uint16_t pin, uint16_t num_leds);

/**
 * @brief ตั้งค่าสีของ LED ดวงที่ระบุ (RGB)
 * @param pixel หมายเลข LED (0-based)
 * @param r ค่าสีแดง (0-255)
 * @param g ค่าสีเขียว (0-255)
 * @param b ค่าสีน้ำเงิน (0-255)
 * 
 * @example
 * NeoPixel_SetPixelColor(0, 255, 0, 0);  // LED แรกเป็นสีแดง
 */
void NeoPixel_SetPixelColor(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief ตั้งค่าสีของ LED ด้วย 32-bit color
 * @param pixel หมายเลข LED
 * @param color สี 32-bit (0xRRGGBB)
 * 
 * @example
 * NeoPixel_SetPixelColor32(0, COLOR_RED);
 */
void NeoPixel_SetPixelColor32(uint16_t pixel, uint32_t color);

/**
 * @brief ตั้งค่าสีของ LED ด้วย HSV
 * @param pixel หมายเลข LED
 * @param h Hue (0-359)
 * @param s Saturation (0-255)
 * @param v Value (0-255)
 * 
 * @example
 * NeoPixel_SetPixelColorHSV(0, 0, 255, 255);  // สีแดงสด
 */
void NeoPixel_SetPixelColorHSV(uint16_t pixel, uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief อัพเดทการแสดงผล (ส่งข้อมูลไปยัง LEDs)
 * 
 * @note ต้องเรียกฟังก์ชันนี้หลังจาก Set color
 * 
 * @example
 * NeoPixel_SetPixelColor(0, 255, 0, 0);
 * NeoPixel_Show();
 */
void NeoPixel_Show(void);

/**
 * @brief ดับ LEDs ทั้งหมด
 * 
 * @example
 * NeoPixel_Clear();
 * NeoPixel_Show();
 */
void NeoPixel_Clear(void);

/**
 * @brief ตั้งค่าสีเดียวกันให้ทุก LEDs
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @example
 * NeoPixel_Fill(255, 0, 0);  // ทุกดวงเป็นสีแดง
 * NeoPixel_Show();
 */
void NeoPixel_Fill(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief ตั้งค่าความสว่างทั้งหมด
 * @param brightness ความสว่าง (0-255)
 * 
 * @note ค่า default = 255 (สว่างสุด)
 * 
 * @example
 * NeoPixel_SetBrightness(50);  // ลดความสว่าง
 */
void NeoPixel_SetBrightness(uint8_t brightness);

/**
 * @brief อ่านค่าสีของ LED
 * @param pixel หมายเลข LED
 * @return สี 32-bit (0xRRGGBB)
 * 
 * @example
 * uint32_t color = NeoPixel_GetPixelColor(0);
 */
uint32_t NeoPixel_GetPixelColor(uint16_t pixel);

/* ========== Color Utility Functions ========== */

/**
 * @brief สร้างสี 32-bit จาก RGB
 * @param r Red
 * @param g Green
 * @param b Blue
 * @return สี 32-bit
 * 
 * @example
 * uint32_t color = NeoPixel_Color(255, 0, 0);
 */
uint32_t NeoPixel_Color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief แปลง HSV เป็น RGB
 * @param h Hue (0-359)
 * @param s Saturation (0-255)
 * @param v Value (0-255)
 * @return สี 32-bit
 * 
 * @example
 * uint32_t color = NeoPixel_ColorHSV(120, 255, 255);  // สีเขียว
 */
uint32_t NeoPixel_ColorHSV(uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief สร้างสีจาก Color Wheel (0-255)
 * @param pos ตำแหน่งบน wheel (0-255)
 * @return สี 32-bit
 * 
 * @note 0=แดง, 85=เขียว, 170=น้ำเงิน
 * 
 * @example
 * uint32_t color = NeoPixel_Wheel(128);
 */
uint32_t NeoPixel_Wheel(uint8_t pos);

/* ========== Effect Functions ========== */

/**
 * @brief แสดง Rainbow effect
 * @param wait_ms เวลารอระหว่างแต่ละ step (ms)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_Rainbow(20, 5);
 */
void NeoPixel_Rainbow(uint16_t wait_ms, uint8_t cycles);

/**
 * @brief แสดง Theater Chase effect
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param wait_ms เวลารอ (ms)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_TheaterChase(255, 0, 0, 50, 10);
 */
void NeoPixel_TheaterChase(uint8_t r, uint8_t g, uint8_t b, uint16_t wait_ms, uint8_t cycles);

/**
 * @brief แสดง Color Wipe effect
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param wait_ms เวลารอ (ms)
 * 
 * @example
 * NeoPixel_ColorWipe(255, 0, 0, 50);
 */
void NeoPixel_ColorWipe(uint8_t r, uint8_t g, uint8_t b, uint16_t wait_ms);

/**
 * @brief แสดง Fade effect
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param steps จำนวน steps
 * @param wait_ms เวลารอ (ms)
 * 
 * @example
 * NeoPixel_Fade(255, 0, 0, 50, 10);
 */
void NeoPixel_Fade(uint8_t r, uint8_t g, uint8_t b, uint8_t steps, uint16_t wait_ms);

/**
 * @brief แสดง Sparkle effect
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param count จำนวนครั้ง
 * @param wait_ms เวลารอ (ms)
 * 
 * @example
 * NeoPixel_Sparkle(255, 255, 255, 50, 20);
 */
void NeoPixel_Sparkle(uint8_t r, uint8_t g, uint8_t b, uint16_t count, uint16_t wait_ms);

/* ========== Advanced Effect Functions ========== */

/**
 * @brief แสดง Breathing effect (หายใจเข้าออก)
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param speed ความเร็ว (ms per step)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_Breathing(255, 0, 0, 20, 5);
 */
void NeoPixel_Breathing(uint8_t r, uint8_t g, uint8_t b, uint16_t speed, uint8_t cycles);

/**
 * @brief แสดง Comet/Meteor effect
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param tail_length ความยาวหาง
 * @param speed ความเร็ว (ms)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_Comet(255, 255, 255, 5, 50, 3);
 */
void NeoPixel_Comet(uint8_t r, uint8_t g, uint8_t b, uint8_t tail_length, uint16_t speed, uint8_t cycles);

/**
 * @brief แสดง KITT Scanner effect
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param eye_size ขนาดของ eye
 * @param speed ความเร็ว (ms)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_Scanner(255, 0, 0, 3, 30, 5);
 */
void NeoPixel_Scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t eye_size, uint16_t speed, uint8_t cycles);

/**
 * @brief แสดง Running Lights effect (ไฟวิ่งพร้อมหาง)
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param wave_delay ความล่าช้าของคลื่น
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_RunningLights(255, 0, 0, 50, 5);
 */
void NeoPixel_RunningLights(uint8_t r, uint8_t g, uint8_t b, uint16_t wave_delay, uint8_t cycles);

/**
 * @brief แสดง Twinkle Random effect
 * @param count จำนวนครั้งที่กระพริบ
 * @param speed ความเร็ว (ms)
 * 
 * @example
 * NeoPixel_TwinkleRandom(100, 50);
 */
void NeoPixel_TwinkleRandom(uint16_t count, uint16_t speed);

/**
 * @brief แสดง Color Chase effect (ไล่สีหลายสี)
 * @param colors อาร์เรย์ของสี
 * @param num_colors จำนวนสี
 * @param speed ความเร็ว (ms)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * uint32_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE};
 * NeoPixel_ColorChase(colors, 3, 50, 5);
 */
void NeoPixel_ColorChase(uint32_t* colors, uint8_t num_colors, uint16_t speed, uint8_t cycles);

/**
 * @brief แสดง Rainbow Cycle effect
 * @param speed ความเร็ว (ms)
 * @param cycles จำนวนรอบ
 * 
 * @example
 * NeoPixel_RainbowCycle(20, 5);
 */
void NeoPixel_RainbowCycle(uint16_t speed, uint8_t cycles);

/**
 * @brief แสดง Strobe effect (กระพริบเร็ว)
 * @param r Red
 * @param g Green
 * @param b Blue
 * @param count จำนวนครั้ง
 * @param flash_delay เวลากระพริบ (ms)
 * @param end_pause เวลาหยุดท้าย (ms)
 * 
 * @example
 * NeoPixel_Strobe(255, 255, 255, 10, 50, 1000);
 */
void NeoPixel_Strobe(uint8_t r, uint8_t g, uint8_t b, uint16_t count, uint16_t flash_delay, uint16_t end_pause);

/* ========== Advanced Color Utility Functions ========== */

/**
 * @brief ปรับสีด้วย Gamma Correction
 * @param color สี 32-bit
 * @return สีที่ปรับแล้ว
 * 
 * @note ทำให้สีดูเป็นธรรมชาติมากขึ้น
 * 
 * @example
 * uint32_t corrected = NeoPixel_GammaCorrect(COLOR_RED);
 */
uint32_t NeoPixel_GammaCorrect(uint32_t color);

/**
 * @brief ผสมสี 2 สีเข้าด้วยกัน
 * @param color1 สีที่ 1
 * @param color2 สีที่ 2
 * @param blend จำนวนการผสม (0-255, 0=color1, 255=color2)
 * @return สีที่ผสมแล้ว
 * 
 * @example
 * uint32_t mixed = NeoPixel_ColorBlend(COLOR_RED, COLOR_BLUE, 128);
 */
uint32_t NeoPixel_ColorBlend(uint32_t color1, uint32_t color2, uint8_t blend);

/**
 * @brief Interpolate ระหว่างสี 2 สี
 * @param color1 สีเริ่มต้น
 * @param color2 สีปลายทาง
 * @param position ตำแหน่ง (0-255)
 * @return สีที่ interpolate แล้ว
 * 
 * @example
 * uint32_t mid = NeoPixel_ColorInterpolate(COLOR_RED, COLOR_BLUE, 128);
 */
uint32_t NeoPixel_ColorInterpolate(uint32_t color1, uint32_t color2, uint8_t position);

/**
 * @brief ลดความสว่างของสี
 * @param color สี 32-bit
 * @param percent เปอร์เซ็นต์ (0-100)
 * @return สีที่ลดความสว่างแล้ว
 * 
 * @example
 * uint32_t dimmed = NeoPixel_DimColor(COLOR_RED, 50); // ลด 50%
 */
uint32_t NeoPixel_DimColor(uint32_t color, uint8_t percent);

/* ========== Advanced Control Functions ========== */

/**
 * @brief ตั้งค่าสีสำหรับช่วง LEDs
 * @param start LED เริ่มต้น
 * @param end LED สุดท้าย
 * @param r Red
 * @param g Green
 * @param b Blue
 * 
 * @example
 * NeoPixel_SetPixelRange(0, 3, 255, 0, 0); // LED 0-3 เป็นสีแดง
 */
void NeoPixel_SetPixelRange(uint16_t start, uint16_t end, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief หมุน pixel buffer ไปทางซ้าย
 * @param positions จำนวนตำแหน่งที่หมุน
 * 
 * @example
 * NeoPixel_RotateLeft(1);
 */
void NeoPixel_RotateLeft(uint16_t positions);

/**
 * @brief หมุน pixel buffer ไปทางขวา
 * @param positions จำนวนตำแหน่งที่หมุน
 * 
 * @example
 * NeoPixel_RotateRight(1);
 */
void NeoPixel_RotateRight(uint16_t positions);

/**
 * @brief เติมสีแบบ Gradient
 * @param start_color สีเริ่มต้น
 * @param end_color สีสุดท้าย
 * 
 * @example
 * NeoPixel_FillGradient(COLOR_RED, COLOR_BLUE);
 */
void NeoPixel_FillGradient(uint32_t start_color, uint32_t end_color);

/**
 * @brief ตั้งค่าความสว่างสำหรับช่วง LEDs
 * @param start LED เริ่มต้น
 * @param end LED สุดท้าย
 * @param brightness ความสว่าง (0-255)
 * 
 * @example
 * NeoPixel_SetBrightnessRange(0, 3, 50);
 */
void NeoPixel_SetBrightnessRange(uint16_t start, uint16_t end, uint8_t brightness);

/* ========== Non-blocking Effect Framework ========== */

/**
 * @brief เริ่มต้น Effect แบบ Non-blocking
 * @param effect ตัวชี้ไปยัง Effect structure
 * @param type ประเภทของ Effect
 * @param speed ความเร็ว (ms per step)
 * @param param1 Parameter 1 (ขึ้นกับ effect type)
 * @param param2 Parameter 2 (ขึ้นกับ effect type)
 * 
 * @example
 * NeoPixel_Effect_t effect;
 * NeoPixel_StartEffect(&effect, EFFECT_RAINBOW, 20, 0, 0);
 */
void NeoPixel_StartEffect(NeoPixel_Effect_t* effect, uint8_t type, uint16_t speed, uint32_t param1, uint32_t param2);

/**
 * @brief อัพเดท Effect (เรียกใน main loop)
 * @param effect ตัวชี้ไปยัง Effect structure
 * @return 1 = มีการอัพเดท, 0 = ยังไม่ถึงเวลา
 * 
 * @example
 * while(1) {
 *     if(NeoPixel_UpdateEffect(&effect)) {
 *         NeoPixel_Show();
 *     }
 * }
 */
uint8_t NeoPixel_UpdateEffect(NeoPixel_Effect_t* effect);

/**
 * @brief หยุด Effect
 * @param effect ตัวชี้ไปยัง Effect structure
 * 
 * @example
 * NeoPixel_StopEffect(&effect);
 */
void NeoPixel_StopEffect(NeoPixel_Effect_t* effect);

/* ========== Utility Functions ========== */

/**
 * @brief อ่านจำนวน LEDs
 * @return จำนวน LEDs
 */
uint16_t NeoPixel_GetNumLEDs(void);

/**
 * @brief อ่านค่าความสว่างปัจจุบัน
 * @return ความสว่าง (0-255)
 */
uint8_t NeoPixel_GetBrightness(void);

#ifdef __cplusplus
}
#endif

#endif  // __NEOPIXEL_H
