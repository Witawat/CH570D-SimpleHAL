/**
 * @file oled_graphics.h
 * @brief OLED Graphics Library - Graphics Primitives
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับวาดรูปทรงเรขาคณิต, bitmap, และ sprite บน OLED
 * 
 * **คุณสมบัติ:**
 * - Drawing primitives (line, rectangle, circle, triangle)
 * - Filled shapes
 * - Bitmap/image display
 * - Sprite system for animations
 * - Pattern fills
 * 
 * @example
 * #include "oled_graphics.h"
 * 
 * OLED_DrawLine(&oled, 0, 0, 127, 63, OLED_COLOR_WHITE);
 * OLED_DrawRect(&oled, 10, 10, 50, 30, OLED_COLOR_WHITE);
 * OLED_FillCircle(&oled, 64, 32, 20, OLED_COLOR_WHITE);
 */

#ifndef __OLED_GRAPHICS_H
#define __OLED_GRAPHICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "oled_i2c.h"
#include <stdlib.h>

/* ========== Structures ========== */

/**
 * @brief โครงสร้างสำหรับ Bitmap
 */
typedef struct {
    uint8_t width;          /**< ความกว้าง (pixels) */
    uint8_t height;         /**< ความสูง (pixels) */
    const uint8_t* data;    /**< ข้อมูล bitmap */
} OLED_Bitmap;

/**
 * @brief โครงสร้างสำหรับ Sprite
 */
typedef struct {
    uint8_t x;              /**< ตำแหน่ง x */
    uint8_t y;              /**< ตำแหน่ง y */
    uint8_t width;          /**< ความกว้าง */
    uint8_t height;         /**< ความสูง */
    uint8_t frame;          /**< Frame ปัจจุบัน */
    uint8_t frame_count;    /**< จำนวน frames */
    const uint8_t** frames; /**< Array ของ frame data */
} OLED_Sprite;

/* ========== Line Drawing ========== */

/**
 * @brief วาดเส้นตรง
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x0 จุดเริ่มต้น x
 * @param y0 จุดเริ่มต้น y
 * @param x1 จุดสิ้นสุด x
 * @param y1 จุดสิ้นสุด y
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawLine(&oled, 0, 0, 127, 63, OLED_COLOR_WHITE);
 */
void OLED_DrawLine(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, OLED_Color color);

/**
 * @brief วาดเส้นแนวนอน
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x จุดเริ่มต้น x
 * @param y ตำแหน่ง y
 * @param w ความยาว
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawHLine(&oled, 10, 32, 100, OLED_COLOR_WHITE);
 */
void OLED_DrawHLine(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, OLED_Color color);

/**
 * @brief วาดเส้นแนวตั้ง
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y จุดเริ่มต้น y
 * @param h ความสูง
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawVLine(&oled, 64, 10, 40, OLED_COLOR_WHITE);
 */
void OLED_DrawVLine(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t h, OLED_Color color);

/* ========== Rectangle Drawing ========== */

/**
 * @brief วาดสี่เหลี่ยม (เส้นขอบ)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x มุมซ้ายบน
 * @param y ตำแหน่ง y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawRect(&oled, 10, 10, 50, 30, OLED_COLOR_WHITE);
 */
void OLED_DrawRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_Color color);

/**
 * @brief วาดสี่เหลี่ยมแบบเติมสี
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x มุมซ้ายบน
 * @param y ตำแหน่ง y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 * @param color สีเติม
 * 
 * @example
 * OLED_FillRect(&oled, 10, 10, 50, 30, OLED_COLOR_WHITE);
 */
void OLED_FillRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_Color color);

/**
 * @brief วาดสี่เหลี่ยมมุมมน (เส้นขอบ)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x มุมซ้ายบน
 * @param y ตำแหน่ง y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 * @param r รัศมีมุม
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawRoundRect(&oled, 10, 10, 50, 30, 5, OLED_COLOR_WHITE);
 */
void OLED_DrawRoundRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, OLED_Color color);

/**
 * @brief วาดสี่เหลี่ยมมุมมนแบบเติมสี
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x มุมซ้ายบน
 * @param y ตำแหน่ง y มุมซ้ายบน
 * @param w ความกว้าง
 * @param h ความสูง
 * @param r รัศมีมุม
 * @param color สีเติม
 * 
 * @example
 * OLED_FillRoundRect(&oled, 10, 10, 50, 30, 5, OLED_COLOR_WHITE);
 */
void OLED_FillRoundRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, OLED_Color color);

/* ========== Circle Drawing ========== */

/**
 * @brief วาดวงกลม (เส้นขอบ)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x0 ตำแหน่ง x ของจุดศูนย์กลาง
 * @param y0 ตำแหน่ง y ของจุดศูนย์กลาง
 * @param r รัศมี
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawCircle(&oled, 64, 32, 20, OLED_COLOR_WHITE);
 */
void OLED_DrawCircle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, OLED_Color color);

/**
 * @brief วาดวงกลมแบบเติมสี
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x0 ตำแหน่ง x ของจุดศูนย์กลาง
 * @param y0 ตำแหน่ง y ของจุดศูนย์กลาง
 * @param r รัศมี
 * @param color สีเติม
 * 
 * @example
 * OLED_FillCircle(&oled, 64, 32, 20, OLED_COLOR_WHITE);
 */
void OLED_FillCircle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, OLED_Color color);

/* ========== Triangle Drawing ========== */

/**
 * @brief วาดสามเหลี่ยม (เส้นขอบ)
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x0 จุดที่ 1 x
 * @param y0 จุดที่ 1 y
 * @param x1 จุดที่ 2 x
 * @param y1 จุดที่ 2 y
 * @param x2 จุดที่ 3 x
 * @param y2 จุดที่ 3 y
 * @param color สีเส้น
 * 
 * @example
 * OLED_DrawTriangle(&oled, 64, 10, 40, 50, 88, 50, OLED_COLOR_WHITE);
 */
void OLED_DrawTriangle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_Color color);

/**
 * @brief วาดสามเหลี่ยมแบบเติมสี
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x0 จุดที่ 1 x
 * @param y0 จุดที่ 1 y
 * @param x1 จุดที่ 2 x
 * @param y1 จุดที่ 2 y
 * @param x2 จุดที่ 3 x
 * @param y2 จุดที่ 3 y
 * @param color สีเติม
 * 
 * @example
 * OLED_FillTriangle(&oled, 64, 10, 40, 50, 88, 50, OLED_COLOR_WHITE);
 */
void OLED_FillTriangle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_Color color);

/* ========== Bitmap Drawing ========== */

/**
 * @brief วาด Bitmap
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param bitmap ตัวชี้ไปยัง bitmap
 * @param color สี (OLED_COLOR_WHITE, OLED_COLOR_BLACK, OLED_COLOR_INVERT)
 * 
 * @example
 * const uint8_t icon_data[] = {...};
 * OLED_Bitmap icon = {16, 16, icon_data};
 * OLED_DrawBitmap(&oled, 10, 10, &icon, OLED_COLOR_WHITE);
 */
void OLED_DrawBitmap(OLED_Handle* oled, uint8_t x, uint8_t y, const OLED_Bitmap* bitmap, OLED_Color color);

/**
 * @brief วาด Bitmap แบบมี transparency
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param bitmap ตัวชี้ไปยัง bitmap
 * @param mask ตัวชี้ไปยัง mask bitmap (1 = แสดง, 0 = โปร่งใส)
 * @param color สี
 * 
 * @example
 * OLED_DrawBitmapMask(&oled, 10, 10, &icon, &icon_mask, OLED_COLOR_WHITE);
 */
void OLED_DrawBitmapMask(OLED_Handle* oled, uint8_t x, uint8_t y, const OLED_Bitmap* bitmap, const OLED_Bitmap* mask, OLED_Color color);

/* ========== Sprite System ========== */

/**
 * @brief สร้าง Sprite
 * @param sprite ตัวชี้ไปยัง sprite
 * @param x ตำแหน่ง x เริ่มต้น
 * @param y ตำแหน่ง y เริ่มต้น
 * @param width ความกว้าง
 * @param height ความสูง
 * @param frames Array ของ frame data
 * @param frame_count จำนวน frames
 * 
 * @example
 * const uint8_t* anim_frames[] = {frame1, frame2, frame3};
 * OLED_Sprite sprite;
 * OLED_CreateSprite(&sprite, 10, 10, 16, 16, anim_frames, 3);
 */
void OLED_CreateSprite(OLED_Sprite* sprite, uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t** frames, uint8_t frame_count);

/**
 * @brief วาด Sprite
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param sprite ตัวชี้ไปยัง sprite
 * @param color สี
 * 
 * @example
 * OLED_DrawSprite(&oled, &sprite, OLED_COLOR_WHITE);
 */
void OLED_DrawSprite(OLED_Handle* oled, OLED_Sprite* sprite, OLED_Color color);

/**
 * @brief เปลี่ยน Frame ของ Sprite
 * @param sprite ตัวชี้ไปยัง sprite
 * @param frame หมายเลข frame
 * 
 * @example
 * OLED_SetSpriteFrame(&sprite, 1);
 */
void OLED_SetSpriteFrame(OLED_Sprite* sprite, uint8_t frame);

/**
 * @brief ไปยัง Frame ถัดไป
 * @param sprite ตัวชี้ไปยัง sprite
 * 
 * @example
 * OLED_NextSpriteFrame(&sprite);
 */
void OLED_NextSpriteFrame(OLED_Sprite* sprite);

/**
 * @brief ย้าย Sprite
 * @param sprite ตัวชี้ไปยัง sprite
 * @param x ตำแหน่ง x ใหม่
 * @param y ตำแหน่ง y ใหม่
 * 
 * @example
 * OLED_MoveSprite(&sprite, 20, 20);
 */
void OLED_MoveSprite(OLED_Sprite* sprite, uint8_t x, uint8_t y);

/* ========== Advanced Graphics ========== */

/**
 * @brief วาด Progress Bar
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param w ความกว้าง
 * @param h ความสูง
 * @param progress ความคืบหน้า (0-100)
 * 
 * @example
 * OLED_DrawProgressBar(&oled, 10, 30, 100, 10, 75);
 */
void OLED_DrawProgressBar(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t progress);

/**
 * @brief วาดกราฟเส้น
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @param w ความกว้าง
 * @param h ความสูง
 * @param data Array ของข้อมูล
 * @param data_count จำนวนข้อมูล
 * @param min_val ค่าต่ำสุด
 * @param max_val ค่าสูงสุด
 * 
 * @example
 * int16_t data[] = {10, 20, 15, 30, 25};
 * OLED_DrawGraph(&oled, 10, 10, 100, 40, data, 5, 0, 50);
 */
void OLED_DrawGraph(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, int16_t* data, uint8_t data_count, int16_t min_val, int16_t max_val);

#ifdef __cplusplus
}
#endif

#endif  // __OLED_GRAPHICS_H
