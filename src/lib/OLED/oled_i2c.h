/**
 * @file oled_i2c.h
 * @brief OLED I2C Library สำหรับ CH57x (SSD1306 Controller)
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับควบคุม OLED Display ผ่าน I2C โดยรองรับ SSD1306 controller
 * รองรับหลายขนาดหน้าจอ และมีฟีเจอร์ครบครันตั้งแต่ขั้นพื้นฐานถึงขั้นสูง
 * 
 * **คุณสมบัติ:**
 * - รองรับหลายขนาด: 128x64, 128x32, 64x48
 * - Single/Double buffering
 * - Partial screen update
 * - Hardware scrolling
 * - Display control (on/off, contrast, invert)
 * 
 * @example
 * // ตัวอย่างการใช้งานพื้นฐาน
 * #include "oled_i2c.h"
 * 
 * OLED_Handle oled;
 * 
 * int main(void) {
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *     Timer_Init();
 *     
 *     OLED_Init(&oled, OLED_128x64, 0x3C);
 *     OLED_Clear(&oled);
 *     OLED_SetPixel(&oled, 64, 32, 1);
 *     OLED_Update(&oled);
 * }
 * 
 * @note 
 * - ต้องเรียก I2C_SimpleInit() และ Timer_Init() ก่อนใช้งาน
 * - ต้องต่อ pull-up resistor (4.7kΩ) ที่ SDA และ SCL
 * - SSD1306 รองรับ I2C address 0x3C หรือ 0x3D
 */

#ifndef __OLED_I2C_H
#define __OLED_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <string.h>

/* ========== SSD1306 Commands ========== */

// Fundamental Commands
#define SSD1306_SET_CONTRAST            0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME   0xA4
#define SSD1306_DISPLAY_ALL_ON          0xA5
#define SSD1306_NORMAL_DISPLAY          0xA6
#define SSD1306_INVERT_DISPLAY          0xA7
#define SSD1306_DISPLAY_OFF             0xAE
#define SSD1306_DISPLAY_ON              0xAF

// Scrolling Commands
#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A
#define SSD1306_DEACTIVATE_SCROLL                    0x2E
#define SSD1306_ACTIVATE_SCROLL                      0x2F
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3

// Addressing Commands
#define SSD1306_SET_LOWER_COLUMN        0x00
#define SSD1306_SET_HIGHER_COLUMN       0x10
#define SSD1306_MEMORY_MODE             0x20
#define SSD1306_COLUMN_ADDR             0x21
#define SSD1306_PAGE_ADDR               0x22

// Hardware Configuration Commands
#define SSD1306_SET_START_LINE          0x40
#define SSD1306_SET_SEGMENT_REMAP       0xA0
#define SSD1306_SET_MULTIPLEX_RATIO     0xA8
#define SSD1306_COM_SCAN_INC            0xC0
#define SSD1306_COM_SCAN_DEC            0xC8
#define SSD1306_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_SET_COM_PINS            0xDA

// Timing & Driving Scheme Commands
#define SSD1306_SET_DISPLAY_CLOCK_DIV   0xD5
#define SSD1306_SET_PRECHARGE           0xD9
#define SSD1306_SET_VCOM_DETECT         0xDB
#define SSD1306_CHARGE_PUMP             0x8D

/* ========== Enumerations ========== */

/**
 * @brief ขนาด OLED ที่รองรับ
 */
typedef enum {
    OLED_128x64 = 0,  /**< 128x64 pixels (0.96", 1.3") */
    OLED_128x32 = 1,  /**< 128x32 pixels (0.91") */
    OLED_64x48  = 2   /**< 64x48 pixels (0.66") */
} OLED_Size;

/**
 * @brief โหมด Buffering
 */
typedef enum {
    OLED_SINGLE_BUFFER = 0,  /**< Single buffer (saves RAM) */
    OLED_DOUBLE_BUFFER = 1   /**< Double buffer (smooth animations) */
} OLED_BufferMode;

/**
 * @brief สี Pixel
 */
typedef enum {
    OLED_COLOR_BLACK = 0,  /**< Pixel off */
    OLED_COLOR_WHITE = 1,  /**< Pixel on */
    OLED_COLOR_INVERT = 2  /**< Invert pixel */
} OLED_Color;

/**
 * @brief ทิศทาง Scroll
 */
typedef enum {
    OLED_SCROLL_RIGHT = 0,
    OLED_SCROLL_LEFT = 1,
    OLED_SCROLL_DIAG_RIGHT = 2,
    OLED_SCROLL_DIAG_LEFT = 3
} OLED_ScrollDir;

/* ========== Structures ========== */

/**
 * @brief โครงสร้างข้อมูลสำหรับ OLED Handle
 */
typedef struct {
    uint8_t i2c_addr;           /**< I2C address (7-bit) */
    uint8_t width;              /**< Display width in pixels */
    uint8_t height;             /**< Display height in pixels */
    uint8_t pages;              /**< Number of pages (height/8) */
    OLED_BufferMode buffer_mode; /**< Buffering mode */
    uint8_t* buffer;            /**< Display buffer */
    uint8_t* back_buffer;       /**< Back buffer (if double buffering) */
    uint8_t initialized;        /**< Initialization flag */
} OLED_Handle;

/* ========== Core Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน OLED
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param size ขนาดของ OLED
 * @param i2c_addr ที่อยู่ I2C (ปกติ 0x3C หรือ 0x3D)
 * @return 1 = สำเร็จ, 0 = ล้มเหลว
 * 
 * @note ต้องเรียก I2C_SimpleInit() และ Timer_Init() ก่อนใช้ฟังก์ชันนี้
 * 
 * @example
 * OLED_Handle oled;
 * OLED_Init(&oled, OLED_128x64, 0x3C);
 */
uint8_t OLED_Init(OLED_Handle* oled, OLED_Size size, uint8_t i2c_addr);

/**
 * @brief เปิดใช้งาน Double Buffering
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param back_buffer ตัวชี้ไปยัง back buffer (ต้องจัดสรรก่อน)
 * 
 * @note ต้องจัดสรร memory สำหรับ back_buffer ก่อน
 * 
 * @example
 * uint8_t back_buf[1024];
 * OLED_EnableDoubleBuffer(&oled, back_buf);
 */
void OLED_EnableDoubleBuffer(OLED_Handle* oled, uint8_t* back_buffer);

/**
 * @brief อัพเดทหน้าจอทั้งหมด
 * @param oled ตัวชี้ไปยัง OLED handle
 * 
 * @note ฟังก์ชันนี้จะส่งข้อมูลทั้ง buffer ไปยัง OLED
 * 
 * @example
 * OLED_Update(&oled);
 */
void OLED_Update(OLED_Handle* oled);

/**
 * @brief อัพเดทหน้าจอบางส่วน
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x เริ่มต้น x (0-127)
 * @param y เริ่มต้น y (0-63)
 * @param w ความกว้าง
 * @param h ความสูง
 * 
 * @note ใช้สำหรับ optimization เมื่ออัพเดทเฉพาะส่วนที่เปลี่ยน
 * 
 * @example
 * OLED_UpdateArea(&oled, 0, 0, 64, 32);
 */
void OLED_UpdateArea(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief สลับ buffer (สำหรับ double buffering)
 * @param oled ตัวชี้ไปยัง OLED handle
 * 
 * @note ใช้ร่วมกับ OLED_EnableDoubleBuffer()
 * 
 * @example
 * // Draw to back buffer
 * OLED_SwapBuffers(&oled);
 * OLED_Update(&oled);
 */
void OLED_SwapBuffers(OLED_Handle* oled);

/**
 * @brief ล้างหน้าจอ
 * @param oled ตัวชี้ไปยัง OLED handle
 * 
 * @example
 * OLED_Clear(&oled);
 */
void OLED_Clear(OLED_Handle* oled);

/**
 * @brief เติมหน้าจอด้วยสี
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param color สีที่ต้องการเติม
 * 
 * @example
 * OLED_Fill(&oled, OLED_COLOR_WHITE);
 */
void OLED_Fill(OLED_Handle* oled, OLED_Color color);

/* ========== Display Control Functions ========== */

/**
 * @brief เปิด/ปิดหน้าจอ
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param on 1 = เปิด, 0 = ปิด
 * 
 * @example
 * OLED_DisplayOn(&oled, 1);  // เปิดหน้าจอ
 */
void OLED_DisplayOn(OLED_Handle* oled, uint8_t on);

/**
 * @brief ตั้งค่า Contrast
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param contrast ค่า contrast (0-255)
 * 
 * @example
 * OLED_SetContrast(&oled, 128);
 */
void OLED_SetContrast(OLED_Handle* oled, uint8_t contrast);

/**
 * @brief กลับสีหน้าจอ
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param invert 1 = กลับสี, 0 = ปกติ
 * 
 * @example
 * OLED_InvertDisplay(&oled, 1);
 */
void OLED_InvertDisplay(OLED_Handle* oled, uint8_t invert);

/* ========== Pixel Operations ========== */

/**
 * @brief ตั้งค่า pixel
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x (0-width-1)
 * @param y ตำแหน่ง y (0-height-1)
 * @param color สี pixel
 * 
 * @example
 * OLED_SetPixel(&oled, 64, 32, OLED_COLOR_WHITE);
 */
void OLED_SetPixel(OLED_Handle* oled, uint8_t x, uint8_t y, OLED_Color color);

/**
 * @brief อ่านค่า pixel
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param x ตำแหน่ง x
 * @param y ตำแหน่ง y
 * @return สี pixel (0 หรือ 1)
 * 
 * @example
 * uint8_t pixel = OLED_GetPixel(&oled, 64, 32);
 */
uint8_t OLED_GetPixel(OLED_Handle* oled, uint8_t x, uint8_t y);

/* ========== Hardware Scrolling ========== */

/**
 * @brief เริ่ม Scrolling
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param dir ทิศทาง scroll
 * @param start หน้าเริ่มต้น
 * @param end หน้าสุดท้าย
 * @param interval ความเร็ว (0-7, เร็วสุด-ช้าสุด)
 * 
 * @example
 * OLED_StartScroll(&oled, OLED_SCROLL_RIGHT, 0, 7, 0);
 */
void OLED_StartScroll(OLED_Handle* oled, OLED_ScrollDir dir, uint8_t start, uint8_t end, uint8_t interval);

/**
 * @brief หยุด Scrolling
 * @param oled ตัวชี้ไปยัง OLED handle
 * 
 * @example
 * OLED_StopScroll(&oled);
 */
void OLED_StopScroll(OLED_Handle* oled);

/* ========== Low-level Functions ========== */

/**
 * @brief ส่ง Command ไปยัง OLED
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param cmd คำสั่ง
 */
void OLED_WriteCommand(OLED_Handle* oled, uint8_t cmd);

/**
 * @brief ส่ง Data ไปยัง OLED
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param data ข้อมูล
 */
void OLED_WriteData(OLED_Handle* oled, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif  // __OLED_I2C_H
