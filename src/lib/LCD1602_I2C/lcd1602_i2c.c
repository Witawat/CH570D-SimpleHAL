/**
 * @file lcd1602_i2c.c
 * @brief LCD1602 I2C Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "lcd1602_i2c.h"
#include <stdio.h>

/* ========== Private Helper Functions ========== */

/**
 * @brief เขียนข้อมูลไปยัง PCF8574 I/O Expander
 */
static void LCD_ExpanderWrite(LCD1602_Handle* lcd, uint8_t data) {
    uint8_t write_data = data | lcd->backlight;
    I2C_Write(lcd->i2c_addr, &write_data, 1);
}

/**
 * @brief สร้าง Enable pulse
 */
static void LCD_PulseEnable(LCD1602_Handle* lcd, uint8_t data) {
    LCD_ExpanderWrite(lcd, data | LCD_EN);   // EN high
    Delay_Us(1);                              // Enable pulse must be >450ns
    LCD_ExpanderWrite(lcd, data & ~LCD_EN);  // EN low
    Delay_Us(50);                             // Commands need >37us to settle
}

/**
 * @brief เขียน 4-bit data ไปยัง LCD
 */
static void LCD_WriteNibble(LCD1602_Handle* lcd, uint8_t nibble) {
    uint8_t data = (nibble << 4);
    LCD_PulseEnable(lcd, data);
}

/**
 * @brief เขียน 8-bit data ไปยัง LCD (แบบ 4-bit mode)
 */
static void LCD_WriteByte(LCD1602_Handle* lcd, uint8_t value, uint8_t mode) {
    uint8_t high_nibble = value & 0xF0;
    uint8_t low_nibble = (value << 4) & 0xF0;
    
    // เขียน high nibble
    LCD_PulseEnable(lcd, high_nibble | mode);
    
    // เขียน low nibble
    LCD_PulseEnable(lcd, low_nibble | mode);
}

/**
 * @brief ส่งคำสั่งไปยัง LCD
 */
static void LCD_SendCommand(LCD1602_Handle* lcd, uint8_t cmd) {
    LCD_WriteByte(lcd, cmd, 0);  // RS = 0 สำหรับ command
}

/**
 * @brief ส่งข้อมูลไปยัง LCD
 */
static void LCD_SendData(LCD1602_Handle* lcd, uint8_t data) {
    LCD_WriteByte(lcd, data, LCD_RS);  // RS = 1 สำหรับ data
}

/* ========== Core Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน LCD
 */
void LCD_Init(LCD1602_Handle* lcd, uint8_t i2c_addr, LCD_Size size) {
    // เก็บ configuration
    lcd->i2c_addr = i2c_addr;
    lcd->backlight = LCD_BL;  // เปิด backlight ตั้งแต่เริ่มต้น
    
    // ตั้งค่าขนาด LCD
    if (size == LCD_16x2) {
        lcd->cols = 16;
        lcd->rows = 2;
    } else {  // LCD_20x4
        lcd->cols = 20;
        lcd->rows = 4;
    }
    
    // รอให้ LCD พร้อมทำงาน (>40ms หลัง power on)
    Delay_Ms(50);
    
    // ตั้งค่า LCD เป็น 4-bit mode
    // ต้องส่งคำสั่งพิเศษ 3 ครั้งเพื่อ reset LCD
    LCD_WriteNibble(lcd, 0x03);
    Delay_Ms(5);
    
    LCD_WriteNibble(lcd, 0x03);
    Delay_Ms(5);
    
    LCD_WriteNibble(lcd, 0x03);
    Delay_Us(150);
    
    // ตั้งเป็น 4-bit mode
    LCD_WriteNibble(lcd, 0x02);
    Delay_Us(150);
    
    // ตั้งค่า function set
    lcd->display_function = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
    LCD_SendCommand(lcd, LCD_FUNCTIONSET | lcd->display_function);
    
    // ตั้งค่า display control
    lcd->display_control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    LCD_SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->display_control);
    
    // ล้างหน้าจอ
    LCD_Clear(lcd);
    
    // ตั้งค่า entry mode
    lcd->display_mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    LCD_SendCommand(lcd, LCD_ENTRYMODESET | lcd->display_mode);
    
    // กลับไปตำแหน่งเริ่มต้น
    LCD_Home(lcd);
}

/**
 * @brief ล้างหน้าจอ LCD
 */
void LCD_Clear(LCD1602_Handle* lcd) {
    LCD_SendCommand(lcd, LCD_CLEARDISPLAY);
    Delay_Ms(2);  // Clear command ใช้เวลานาน
}

/**
 * @brief กลับไปตำแหน่งเริ่มต้น (0,0)
 */
void LCD_Home(LCD1602_Handle* lcd) {
    LCD_SendCommand(lcd, LCD_RETURNHOME);
    Delay_Ms(2);  // Home command ใช้เวลานาน
}

/**
 * @brief ตั้งตำแหน่ง cursor
 */
void LCD_SetCursor(LCD1602_Handle* lcd, uint8_t col, uint8_t row) {
    // Row offsets สำหรับ LCD แต่ละแบบ
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    
    if (row >= lcd->rows) {
        row = lcd->rows - 1;  // จำกัดไม่ให้เกิน
    }
    
    LCD_SendCommand(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

/**
 * @brief แสดงข้อความ
 */
void LCD_Print(LCD1602_Handle* lcd, const char* str) {
    while (*str) {
        LCD_SendData(lcd, *str++);
    }
}

/**
 * @brief แสดงตัวอักษรเดียว
 */
void LCD_PrintChar(LCD1602_Handle* lcd, char c) {
    LCD_SendData(lcd, c);
}

/* ========== Display Control Functions ========== */

/**
 * @brief เปิด/ปิดการแสดงผล
 */
void LCD_Display(LCD1602_Handle* lcd, uint8_t on) {
    if (on) {
        lcd->display_control |= LCD_DISPLAYON;
    } else {
        lcd->display_control &= ~LCD_DISPLAYON;
    }
    LCD_SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->display_control);
}

/**
 * @brief เปิด/ปิด cursor
 */
void LCD_Cursor(LCD1602_Handle* lcd, uint8_t on) {
    if (on) {
        lcd->display_control |= LCD_CURSORON;
    } else {
        lcd->display_control &= ~LCD_CURSORON;
    }
    LCD_SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->display_control);
}

/**
 * @brief เปิด/ปิด cursor blink
 */
void LCD_Blink(LCD1602_Handle* lcd, uint8_t on) {
    if (on) {
        lcd->display_control |= LCD_BLINKON;
    } else {
        lcd->display_control &= ~LCD_BLINKON;
    }
    LCD_SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->display_control);
}

/**
 * @brief เปิด/ปิด backlight
 */
void LCD_Backlight(LCD1602_Handle* lcd, uint8_t on) {
    if (on) {
        lcd->backlight = LCD_BL;
    } else {
        lcd->backlight = 0x00;
    }
    LCD_ExpanderWrite(lcd, 0);
}

/* ========== Advanced Functions ========== */

/**
 * @brief สร้าง custom character
 */
void LCD_CreateChar(LCD1602_Handle* lcd, uint8_t location, uint8_t charmap[8]) {
    location &= 0x7;  // เรามี 8 ตำแหน่ง (0-7)
    LCD_SendCommand(lcd, LCD_SETCGRAMADDR | (location << 3));
    
    for (int i = 0; i < 8; i++) {
        LCD_SendData(lcd, charmap[i]);
    }
}

/**
 * @brief เลื่อนหน้าจอไปทางซ้าย
 */
void LCD_ScrollDisplayLeft(LCD1602_Handle* lcd) {
    LCD_SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

/**
 * @brief เลื่อนหน้าจอไปทางขวา
 */
void LCD_ScrollDisplayRight(LCD1602_Handle* lcd) {
    LCD_SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

/**
 * @brief ตั้งทิศทางการเขียนจากซ้ายไปขวา
 */
void LCD_LeftToRight(LCD1602_Handle* lcd) {
    lcd->display_mode |= LCD_ENTRYLEFT;
    LCD_SendCommand(lcd, LCD_ENTRYMODESET | lcd->display_mode);
}

/**
 * @brief ตั้งทิศทางการเขียนจากขวาไปซ้าย
 */
void LCD_RightToLeft(LCD1602_Handle* lcd) {
    lcd->display_mode &= ~LCD_ENTRYLEFT;
    LCD_SendCommand(lcd, LCD_ENTRYMODESET | lcd->display_mode);
}

/**
 * @brief เปิด/ปิด auto scroll
 */
void LCD_AutoScroll(LCD1602_Handle* lcd, uint8_t on) {
    if (on) {
        lcd->display_mode |= LCD_ENTRYSHIFTINCREMENT;
    } else {
        lcd->display_mode &= ~LCD_ENTRYSHIFTINCREMENT;
    }
    LCD_SendCommand(lcd, LCD_ENTRYMODESET | lcd->display_mode);
}

/* ========== Helper Functions ========== */

/**
 * @brief แสดงตัวเลข integer
 */
void LCD_PrintInt(LCD1602_Handle* lcd, int32_t num) {
    char buffer[12];  // เพียงพอสำหรับ int32_t (-2147483648 ถึง 2147483647)
    sprintf(buffer, "%d", num);
    LCD_Print(lcd, buffer);
}

/**
 * @brief แสดงตัวเลข float
 */
void LCD_PrintFloat(LCD1602_Handle* lcd, float num, uint8_t decimals) {
    char buffer[16];
    char format[8];
    
    // สร้าง format string เช่น "%.2f"
    sprintf(format, "%%.%df", decimals);
    sprintf(buffer, format, num);
    
    LCD_Print(lcd, buffer);
}

/**
 * @brief แสดงข้อความที่ตำแหน่งที่กำหนด
 */
void LCD_PrintAt(LCD1602_Handle* lcd, uint8_t col, uint8_t row, const char* str) {
    LCD_SetCursor(lcd, col, row);
    LCD_Print(lcd, str);
}

/**
 * @brief แสดงผลแบบ formatted string (เหมือน printf)
 */
void LCD_Printf(LCD1602_Handle* lcd, const char* format, ...) {
    char buffer[32]; // ขนาดบัฟเฟอร์ที่เหมาะสมสำหรับ LCD 16x2 หรือ 20x4
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    LCD_Print(lcd, buffer);
}

/**
 * @brief ล้างข้อความเฉพาะบรรทัด
 */
void LCD_ClearLine(LCD1602_Handle* lcd, uint8_t row) {
    LCD_SetCursor(lcd, 0, row);
    for (int i = 0; i < lcd->cols; i++) {
        LCD_SendData(lcd, ' ');
    }
    LCD_SetCursor(lcd, 0, row);
}

/**
 * @brief แสดงข้อความกึ่งกลางบรรทัด
 */
void LCD_CenterPrint(LCD1602_Handle* lcd, uint8_t row, const char* str) {
    uint8_t len = strlen(str);
    if (len >= lcd->cols) {
        LCD_SetCursor(lcd, 0, row);
        LCD_Print(lcd, str);
    } else {
        uint8_t pos = (lcd->cols - len) / 2;
        LCD_ClearLine(lcd, row);
        LCD_SetCursor(lcd, pos, row);
        LCD_Print(lcd, str);
    }
}

/**
 * @brief สลับสถานะ Backlight
 */
void LCD_ToggleBacklight(LCD1602_Handle* lcd) {
    if (lcd->backlight == LCD_BL) {
        LCD_Backlight(lcd, 0);
    } else {
        LCD_Backlight(lcd, 1);
    }
}
