/**
 * @file lcd1602_i2c.h
 * @brief LCD1602 I2C Library สำหรับ CH57x แบบ Arduino-style
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library นี้ควบคุม LCD1602/LCD2004 ผ่าน I2C โดยใช้ PCF8574 I/O Expander
 * รูปแบบการใช้งานคล้าย Arduino LiquidCrystal_I2C แต่ปรับให้เหมาะกับ CH57x
 * 
 * **คุณสมบัติ:**
 * - รองรับ LCD ขนาด 16x2 และ 20x4
 * - ควบคุมผ่าน I2C (PCF8574/PCF8574A)
 * - API แบบ Arduino-style ใช้งานง่าย
 * - รองรับ custom characters (สูงสุด 8 ตัว)
 * - ควบคุม backlight ได้
 * - รองรับ scrolling และ animation
 * 
 * @example
 * // ตัวอย่างการใช้งานพื้นฐาน
 * #include "main.h"
 * #include "Lib/LCD1602_I2C/lcd1602_i2c.h"
 * 
 * LCD1602_Handle lcd;
 * 
 * int main(void) {
 *     // ตั้งค่าระบบ
 *     SystemCoreClockUpdate();
 *     NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
 *     
 *     // เริ่มต้น I2C และ Timer
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *     Timer_Init();
 *     
 *     // เริ่มต้น LCD (address 0x27, ขนาด 16x2)
 *     LCD_Init(&lcd, 0x27, LCD_16x2);
 *     
 *     // แสดงข้อความ
 *     LCD_Print(&lcd, "Hello World!");
 *     LCD_SetCursor(&lcd, 0, 1);
 *     LCD_Print(&lcd, "CH57x");
 *     
 *     while(1) {
 *         // วนลูปการทำงาน
 *     }
 * }
 * 
 * @note 
 * - ต้องเรียก SystemCoreClockUpdate() เป็นอย่างแรก
 * - ต้องเรียก I2C_SimpleInit() ก่อนใช้งาน LCD
 * - ต้องเรียก Timer_Init() เพื่อใช้ delay functions
 * - ต้องต่อ pull-up resistor (4.7kΩ) ที่ SDA (PC1) และ SCL (PC2)
 */

#ifndef __LCD1602_I2C_H
#define __LCD1602_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleI2C.h"
#include "../../SimpleHAL/SimpleDelay.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* ========== LCD Commands ========== */

// Commands
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT    0x10
#define LCD_FUNCTIONSET    0x20
#define LCD_SETCGRAMADDR   0x40
#define LCD_SETDDRAMADDR   0x80

// Entry Mode flags
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Display Control flags
#define LCD_DISPLAYON  0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON   0x02
#define LCD_CURSOROFF  0x00
#define LCD_BLINKON    0x01
#define LCD_BLINKOFF   0x00

// Cursor/Display Shift flags
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT   0x04
#define LCD_MOVELEFT    0x00

// Function Set flags
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE    0x08
#define LCD_1LINE    0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS  0x00

/* ========== PCF8574 Pin Mapping ========== */

// PCF8574 pins connected to LCD
#define LCD_RS 0x01  // P0 - Register Select
#define LCD_RW 0x02  // P1 - Read/Write (not used, tied to GND)
#define LCD_EN 0x04  // P2 - Enable
#define LCD_BL 0x08  // P3 - Backlight
#define LCD_D4 0x10  // P4 - Data bit 4
#define LCD_D5 0x20  // P5 - Data bit 5
#define LCD_D6 0x40  // P6 - Data bit 6
#define LCD_D7 0x80  // P7 - Data bit 7

/* ========== Enumerations ========== */

/**
 * @brief ขนาด LCD ที่รองรับ
 */
typedef enum {
    LCD_16x2 = 0,  /**< LCD ขนาด 16 คอลัมน์ x 2 แถว */
    LCD_20x4 = 1   /**< LCD ขนาด 20 คอลัมน์ x 4 แถว */
} LCD_Size;

/**
 * @brief สถานะ Backlight
 */
typedef enum {
    LCD_BACKLIGHT_OFF = 0,  /**< ปิด backlight */
    LCD_BACKLIGHT_ON = 1    /**< เปิด backlight */
} LCD_BacklightState;

/* ========== Structures ========== */

/**
 * @brief โครงสร้างข้อมูลสำหรับ LCD Handle
 */
typedef struct {
    uint8_t i2c_addr;           /**< I2C address (7-bit) */
    uint8_t cols;               /**< จำนวนคอลัมน์ */
    uint8_t rows;               /**< จำนวนแถว */
    uint8_t backlight;          /**< สถานะ backlight (LCD_BL หรือ 0x00) */
    uint8_t display_control;    /**< Display control flags */
    uint8_t display_mode;       /**< Display mode flags */
    uint8_t display_function;   /**< Function set flags */
} LCD1602_Handle;

/* ========== Core Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน LCD
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param i2c_addr ที่อยู่ I2C ของ PCF8574 (7-bit address, ปกติ 0x27 หรือ 0x3F)
 * @param size ขนาดของ LCD (LCD_16x2 หรือ LCD_20x4)
 * 
 * @note ต้องเรียก I2C_SimpleInit() และ Timer_Init() ก่อนใช้ฟังก์ชันนี้
 * 
 * @example
 * LCD1602_Handle lcd;
 * LCD_Init(&lcd, 0x27, LCD_16x2);
 */
void LCD_Init(LCD1602_Handle* lcd, uint8_t i2c_addr, LCD_Size size);

/**
 * @brief ล้างหน้าจอ LCD
 * @param lcd ตัวชี้ไปยัง LCD handle
 * 
 * @note ฟังก์ชันนี้ใช้เวลาประมาณ 2ms
 * 
 * @example
 * LCD_Clear(&lcd);
 */
void LCD_Clear(LCD1602_Handle* lcd);

/**
 * @brief กลับไปตำแหน่งเริ่มต้น (0,0)
 * @param lcd ตัวชี้ไปยัง LCD handle
 * 
 * @note ฟังก์ชันนี้ใช้เวลาประมาณ 2ms
 * 
 * @example
 * LCD_Home(&lcd);
 */
void LCD_Home(LCD1602_Handle* lcd);

/**
 * @brief ตั้งตำแหน่ง cursor
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param col คอลัมน์ (0-15 สำหรับ 16x2, 0-19 สำหรับ 20x4)
 * @param row แถว (0-1 สำหรับ 16x2, 0-3 สำหรับ 20x4)
 * 
 * @example
 * LCD_SetCursor(&lcd, 0, 1);  // ไปที่แถวที่ 2 คอลัมน์แรก
 */
void LCD_SetCursor(LCD1602_Handle* lcd, uint8_t col, uint8_t row);

/**
 * @brief แสดงข้อความ
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param str ข้อความที่ต้องการแสดง (null-terminated string)
 * 
 * @example
 * LCD_Print(&lcd, "Hello World!");
 */
void LCD_Print(LCD1602_Handle* lcd, const char* str);

/**
 * @brief แสดงตัวอักษรเดียว
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param c ตัวอักษรที่ต้องการแสดง
 * 
 * @example
 * LCD_PrintChar(&lcd, 'A');
 */
void LCD_PrintChar(LCD1602_Handle* lcd, char c);

/* ========== Display Control Functions ========== */

/**
 * @brief เปิด/ปิดการแสดงผล
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param on 1 = เปิด, 0 = ปิด
 * 
 * @note การปิดจะไม่ลบข้อมูลใน RAM
 * 
 * @example
 * LCD_Display(&lcd, 0);  // ปิดการแสดงผล
 * LCD_Display(&lcd, 1);  // เปิดการแสดงผล
 */
void LCD_Display(LCD1602_Handle* lcd, uint8_t on);

/**
 * @brief เปิด/ปิด cursor
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param on 1 = เปิด, 0 = ปิด
 * 
 * @example
 * LCD_Cursor(&lcd, 1);  // แสดง cursor
 */
void LCD_Cursor(LCD1602_Handle* lcd, uint8_t on);

/**
 * @brief เปิด/ปิด cursor blink
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param on 1 = เปิด, 0 = ปิด
 * 
 * @example
 * LCD_Blink(&lcd, 1);  // cursor กระพริบ
 */
void LCD_Blink(LCD1602_Handle* lcd, uint8_t on);

/**
 * @brief เปิด/ปิด backlight
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param on 1 = เปิด, 0 = ปิด
 * 
 * @example
 * LCD_Backlight(&lcd, 1);  // เปิด backlight
 */
void LCD_Backlight(LCD1602_Handle* lcd, uint8_t on);

/* ========== Advanced Functions ========== */

/**
 * @brief สร้าง custom character
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param location ตำแหน่งใน CGRAM (0-7)
 * @param charmap array ของ pixel data (8 bytes สำหรับ 5x8 dots)
 * 
 * @note แต่ละ byte แทน 1 แถว, 5 bits ล่างสุดแทน pixels
 * 
 * @example
 * uint8_t heart[8] = {
 *     0b00000,
 *     0b01010,
 *     0b11111,
 *     0b11111,
 *     0b01110,
 *     0b00100,
 *     0b00000,
 *     0b00000
 * };
 * LCD_CreateChar(&lcd, 0, heart);
 * LCD_PrintChar(&lcd, 0);  // แสดง custom character
 */
void LCD_CreateChar(LCD1602_Handle* lcd, uint8_t location, uint8_t charmap[8]);

/**
 * @brief เลื่อนหน้าจอไปทางซ้าย
 * @param lcd ตัวชี้ไปยัง LCD handle
 * 
 * @example
 * LCD_ScrollDisplayLeft(&lcd);
 */
void LCD_ScrollDisplayLeft(LCD1602_Handle* lcd);

/**
 * @brief เลื่อนหน้าจอไปทางขวา
 * @param lcd ตัวชี้ไปยัง LCD handle
 * 
 * @example
 * LCD_ScrollDisplayRight(&lcd);
 */
void LCD_ScrollDisplayRight(LCD1602_Handle* lcd);

/**
 * @brief ตั้งทิศทางการเขียนจากซ้ายไปขวา
 * @param lcd ตัวชี้ไปยัง LCD handle
 * 
 * @example
 * LCD_LeftToRight(&lcd);
 */
void LCD_LeftToRight(LCD1602_Handle* lcd);

/**
 * @brief ตั้งทิศทางการเขียนจากขวาไปซ้าย
 * @param lcd ตัวชี้ไปยัง LCD handle
 * 
 * @example
 * LCD_RightToLeft(&lcd);
 */
void LCD_RightToLeft(LCD1602_Handle* lcd);

/**
 * @brief เปิด/ปิด auto scroll
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param on 1 = เปิด, 0 = ปิด
 * 
 * @note เมื่อเปิด auto scroll หน้าจอจะเลื่อนตามทิศทางที่ตั้งไว้
 * 
 * @example
 * LCD_AutoScroll(&lcd, 1);
 */
void LCD_AutoScroll(LCD1602_Handle* lcd, uint8_t on);

/* ========== Helper Functions ========== */

/**
 * @brief แสดงตัวเลข integer
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param num ตัวเลขที่ต้องการแสดง
 * 
 * @example
 * LCD_PrintInt(&lcd, 12345);
 */
void LCD_PrintInt(LCD1602_Handle* lcd, int32_t num);

/**
 * @brief แสดงตัวเลข float
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param num ตัวเลขที่ต้องการแสดง
 * @param decimals จำนวนทศนิยม
 * 
 * @example
 * LCD_PrintFloat(&lcd, 3.14159, 2);  // แสดง "3.14"
 */
void LCD_PrintFloat(LCD1602_Handle* lcd, float num, uint8_t decimals);

/**
 * @brief แสดงข้อความที่ตำแหน่งที่กำหนด
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param col คอลัมน์
 * @param row แถว
 * @param str ข้อความ
 * 
 * @example
 * LCD_PrintAt(&lcd, 0, 1, "Line 2");
 */
void LCD_PrintAt(LCD1602_Handle* lcd, uint8_t col, uint8_t row, const char* str);

/**
 * @brief แสดงผลแบบ formatted string (เหมือน printf)
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param format รูปแบบข้อความ
 * @param ... พารามิเตอร์ต่างๆ
 * 
 * @example
 * LCD_Printf(&lcd, "Temp: %d C", temp);
 */
void LCD_Printf(LCD1602_Handle* lcd, const char* format, ...);

/**
 * @brief ล้างข้อความเฉพาะบรรทัด
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param row แถวที่ต้องการล้าง (0-1 หรือ 0-3)
 * 
 * @example
 * LCD_ClearLine(&lcd, 1);
 */
void LCD_ClearLine(LCD1602_Handle* lcd, uint8_t row);

/**
 * @brief แสดงข้อความกึ่งกลางบรรทัด
 * @param lcd ตัวชี้ไปยัง LCD handle
 * @param row แถวที่ต้องการแสดง
 * @param str ข้อความ
 * 
 * @example
 * LCD_CenterPrint(&lcd, 0, "Welcome");
 */
void LCD_CenterPrint(LCD1602_Handle* lcd, uint8_t row, const char* str);

/**
 * @brief สลับสถานะ Backlight (เปิด <-> ปิด)
 * @param lcd ตัวชี้ไปยัง LCD handle
 */
void LCD_ToggleBacklight(LCD1602_Handle* lcd);

#ifdef __cplusplus
}
#endif

#endif  // __LCD1602_I2C_H
