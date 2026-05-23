/**
 * @file SimpleWS2815Matrix.c
 * @brief WS2815 LED Matrix Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "WS2815Matrix.h"
#include "fonts.h"
#include "../../SimpleHAL/SimpleHAL.h"
#include <string.h>
#include <stdlib.h>

/* ========== Private Variables ========== */

static Matrix_Config_t matrix_config = {0};
static uint32_t matrix_buffer[MATRIX_MAX_PIXELS] = {0};

/* ========== Private Function Prototypes ========== */

static uint16_t xy_to_index_zigzag_left(int16_t x, int16_t y);
static uint16_t xy_to_index_snake(int16_t x, int16_t y);
static uint16_t xy_to_index_zigzag_right(int16_t x, int16_t y);
static uint16_t xy_to_index_columns(int16_t x, int16_t y);
static uint16_t utf8_to_unicode(const char* utf8_char);
static int8_t get_thai_font_index(uint16_t unicode);

/* ========== Initialization Functions ========== */

/**
 * @brief เริ่มต้น WS2815 LED matrix (12V version)
 *
 * @param port - GPIO port สำหรับ data pin
 * @param pin - pin mask (16-bit)
 * @param width - จำนวน LED ในแนวนอน
 * @param height - จำนวน LED ในแนวตั้ง
 * @param wiring - รูปแบบการเรียง LED
 *
 * @note ใช้ NeoPixel low-level driver สำหรับ SPI bitbang
 *       WS2815 ใช้ protocol timing เดียวกับ WS2812
 *       รองรับความกว้างสูงสุด MATRIX_MAX_WIDTH, ความสูง MATRIX_MAX_HEIGHT
 */
void Matrix_Init(GPIO_TypeDef* port, uint16_t pin, uint8_t width, uint8_t height, WiringPattern_e wiring) {
    // ตรวจสอบขนาด
    if (width > MATRIX_MAX_WIDTH || height > MATRIX_MAX_HEIGHT) {
        return;
    }
    
    // บันทึกการตั้งค่า
    matrix_config.gpio_port = port;
    matrix_config.gpio_pin = pin;
    matrix_config.width = width;
    matrix_config.height = height;
    matrix_config.num_pixels = width * height;
    matrix_config.wiring = wiring;
    matrix_config.initialized = true;
    
    // เริ่มต้น NeoPixel
    NeoPixel_Init(port, pin, matrix_config.num_pixels);
    
    // ล้าง buffer
    Matrix_Clear();
    Matrix_Show();
}

/**
 * @brief กำหนด wiring pattern
 *
 * @param wiring - รูปแบบ (ZIGZAG, SNAKE, COLUMNS)
 */
void Matrix_SetWiringPattern(WiringPattern_e wiring) {
    matrix_config.wiring = wiring;
}

/**
 * @brief อ่าน config ปัจจุบัน
 *
 * @return pointer ไปยัง Matrix_Config_t
 */
Matrix_Config_t* Matrix_GetConfig(void) {
    return &matrix_config;
}

/* ========== Basic Drawing Functions ========== */

/**
 * @brief ตั้งค่าสี LED ที่ตำแหน่ง (x, y)
 *
 * @param x - พิกัดแนวนอน
 * @param y - พิกัดแนวตั้ง
 * @param r - สีแดง (0-255)
 * @param g - สีเขียว (0-255)
 * @param b - สีน้ำเงิน (0-255)
 *
 * @note ตรวจสอบ bounds ก่อน set
 *       แปลง (x,y) เป็น LED index ตาม wiring pattern
 *       ต้องเรียก Matrix_Show() เพื่ออัปเดต hardware
 */
void Matrix_SetPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (!Matrix_IsInBounds(x, y)) {
        return;
    }
    
    uint16_t index = Matrix_XYtoIndex(x, y);
    if (index < matrix_config.num_pixels) {
        NeoPixel_SetPixelColor(index, r, g, b);
        matrix_buffer[index] = NeoPixel_Color(r, g, b);
    }
}

/**
 * @brief ตั้งค่าสี LED ด้วย 32-bit color
 *
 * @param x - พิกัดแนวนอน
 * @param y - พิกัดแนวตั้ง
 * @param color - 32-bit color (0x00RRGGBB)
 *
 * @note ใช้ NeoPixel_SetPixelColor32() ภายใน
 */
void Matrix_SetPixelColor(int16_t x, int16_t y, uint32_t color) {
    if (!Matrix_IsInBounds(x, y)) {
        return;
    }
    
    uint16_t index = Matrix_XYtoIndex(x, y);
    if (index < matrix_config.num_pixels) {
        NeoPixel_SetPixelColor32(index, color);
        matrix_buffer[index] = color;
    }
}

/**
 * @brief อ่านสี LED ที่ตำแหน่ง (x, y)
 *
 * @param x - พิกัดแนวนอน
 * @param y - พิกัดแนวตั้ง
 *
 * @return 32-bit color (0x00RRGGBB), 0 ถ้า out of bounds
 *
 * @note อ่านจาก buffer ภายใน
 */
uint32_t Matrix_GetPixel(int16_t x, int16_t y) {
    if (!Matrix_IsInBounds(x, y)) {
        return 0;
    }
    
    uint16_t index = Matrix_XYtoIndex(x, y);
    if (index < matrix_config.num_pixels) {
        return matrix_buffer[index];
    }
    return 0;
}

/**
 * @brief ดับ LED ทั้งหมด
 *
 * @note เรียก NeoPixel_Clear() และ memset buffer เป็น 0
 *       ต้องเรียก Matrix_Show() เพื่ออัปเดต hardware
 */
void Matrix_Clear(void) {
    NeoPixel_Clear();
    memset(matrix_buffer, 0, sizeof(matrix_buffer));
}

/**
 * @brief เติมสีเดียวกันทุก LED
 *
 * @param r - สีแดง (0-255)
 * @param g - สีเขียว (0-255)
 * @param b - สีน้ำเงิน (0-255)
 *
 * @note เรียก NeoPixel_Fill() และ update buffer
 *       ต้องเรียก Matrix_Show() เพื่ออัปเดต hardware
 */
void Matrix_Fill(uint8_t r, uint8_t g, uint8_t b) {
    NeoPixel_Fill(r, g, b);
    uint32_t color = NeoPixel_Color(r, g, b);
    for (uint16_t i = 0; i < matrix_config.num_pixels; i++) {
        matrix_buffer[i] = color;
    }
}

/**
 * @brief เติมสี 32-bit เดียวกันทุก LED
 *
 * @param color - 32-bit color (0x00RRGGBB)
 *
 * @note update ทั้ง NeoPixel และ buffer ภายใน
 */
void Matrix_FillColor(uint32_t color) {
    for (uint16_t i = 0; i < matrix_config.num_pixels; i++) {
        NeoPixel_SetPixelColor32(i, color);
        matrix_buffer[i] = color;
    }
}

/**
 * @brief ส่งข้อมูลสีทั้งหมดไปยัง LED (update hardware)
 *
 * @note เรียก NeoPixel_Show() สำหรับ SPI bitbang
 *       ปิด interrupt ชั่วคราวเพื่อ timing ที่แม่นยำ
 */
void Matrix_Show(void) {
    NeoPixel_Show();
}

/* ========== Shape Drawing Functions ========== */

/**
 * @brief วาดเส้นตรงระหว่าง 2 จุด (Bresenham)
 *
 * @param x0, y0 - จุดเริ่มต้น
 * @param x1, y1 - จุดสิ้นสุด
 * @param r, g, b - สี RGB
 */
void Matrix_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b) {
    // Bresenham's line algorithm
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1) {
        Matrix_SetPixel(x0, y0, r, g, b);
        
        if (x0 == x1 && y0 == y1) {
            break;
        }
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief วาดเส้นด้วย 32-bit color
 *
 * @param x0, y0, x1, y1 - จุดเริ่มและสิ้นสุด
 * @param color - 32-bit color
 */
void Matrix_DrawLineColor(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    Matrix_DrawLine(x0, y0, x1, y1, r, g, b);
}

/**
 * @brief วาดสี่เหลี่ยม (เฉพาะขอบ)
 *
 * @param x, y - มุมซ้ายบน
 * @param w - ความกว้าง
 * @param h - ความสูง
 * @param r, g, b - สี RGB
 */
void Matrix_DrawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b) {
    // วาด 4 เส้น
    Matrix_DrawLine(x, y, x + w - 1, y, r, g, b);           // บน
    Matrix_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, r, g, b); // ล่าง
    Matrix_DrawLine(x, y, x, y + h - 1, r, g, b);           // ซ้าย
    Matrix_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, r, g, b); // ขวา
}

/**
 * @brief วาดสี่เหลี่ยมทึบ
 *
 * @param x, y - มุมซ้ายบน
 * @param w - ความกว้าง
 * @param h - ความสูง
 * @param r, g, b - สี RGB
 */
void Matrix_FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b) {
    for (int16_t j = y; j < y + h; j++) {
        for (int16_t i = x; i < x + w; i++) {
            Matrix_SetPixel(i, j, r, g, b);
        }
    }
}

/**
 * @brief วาดวงกลม (เฉพาะขอบ)
 *
 * @param x0, y0 - จุดศูนย์กลาง
 * @param radius - รัศมี
 * @param r, g, b - สี RGB
 *
 * @note ใช้ midpoint circle algorithm
 */
void Matrix_DrawCircle(int16_t x0, int16_t y0, uint8_t radius, uint8_t r, uint8_t g, uint8_t b) {
    // Midpoint circle algorithm
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        Matrix_SetPixel(x0 + x, y0 + y, r, g, b);
        Matrix_SetPixel(x0 + y, y0 + x, r, g, b);
        Matrix_SetPixel(x0 - y, y0 + x, r, g, b);
        Matrix_SetPixel(x0 - x, y0 + y, r, g, b);
        Matrix_SetPixel(x0 - x, y0 - y, r, g, b);
        Matrix_SetPixel(x0 - y, y0 - x, r, g, b);
        Matrix_SetPixel(x0 + y, y0 - x, r, g, b);
        Matrix_SetPixel(x0 + x, y0 - y, r, g, b);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

/**
 * @brief วาดวงกลมทึบ
 *
 * @param x0, y0 - จุดศูนย์กลาง
 * @param radius - รัศมี
 * @param r, g, b - สี RGB
 */
void Matrix_FillCircle(int16_t x0, int16_t y0, uint8_t radius, uint8_t r, uint8_t g, uint8_t b) {
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        Matrix_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, r, g, b);
        Matrix_DrawLine(x0 - y, y0 + x, x0 + y, y0 + x, r, g, b);
        Matrix_DrawLine(x0 - x, y0 - y, x0 + x, y0 - y, r, g, b);
        Matrix_DrawLine(x0 - y, y0 - x, x0 + y, y0 - x, r, g, b);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

/* ========== Text Drawing Functions ========== */

/**
 * @brief วาดตัวอักษร 5x7
 *
 * @param x, y - ตำแหน่ง
 * @param c - ตัวอักษร (ASCII)
 * @param color - 32-bit color
 *
 * @return ความกว้าง 6 pixel, 0 ถ้าไม่อยู่ในช่วง ASCII
 */
uint8_t Matrix_DrawChar(int16_t x, int16_t y, char c, uint32_t color) {
    if (c < 32 || c > 126) {
        return 0;  // ตัวอักษรไม่รองรับ
    }
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    const uint8_t* glyph = font_5x7[c - 32];
    
    // วาดตัวอักษร 5x7
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                Matrix_SetPixel(x + col, y + row, r, g, b);
            }
        }
    }
    
    return 6;  // ความกว้าง 5 + spacing 1
}

/**
 * @brief วาดข้อความ
 *
 * @param x, y - ตำแหน่งเริ่มต้น
 * @param text - ข้อความ
 * @param color - 32-bit color
 *
 * @return ความกว้างทั้งหมด
 */
uint16_t Matrix_DrawText(int16_t x, int16_t y, const char* text, uint32_t color) {
    int16_t cursor_x = x;
    uint16_t total_width = 0;
    
    while (*text) {
        uint8_t char_width = Matrix_DrawChar(cursor_x, y, *text, color);
        cursor_x += char_width;
        total_width += char_width;
        text++;
    }
    
    return total_width;
}

/**
 * @brief วาดตัวอักษรไทย (UTF-8) ขนาด 8x8
 *
 * @param x, y - ตำแหน่ง
 * @param thai_char - ตัวอักษรไทย (3-byte UTF-8)
 * @param color - 32-bit color
 *
 * @return ความกว้าง 8 pixel, 0 ถ้าไม่พบฟอนต์
 */
uint8_t Matrix_DrawCharThai(int16_t x, int16_t y, const char* thai_char, uint32_t color) {
    // แปลง UTF-8 เป็น Unicode
    uint16_t unicode = utf8_to_unicode(thai_char);
    
    // หา index ของฟอนต์
    int8_t font_index = get_thai_font_index(unicode);
    if (font_index < 0) {
        return 0;  // ไม่พบตัวอักษร
    }
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    const uint8_t* glyph = NULL;
    
    // เลือกฟอนต์ที่เหมาะสม
    if (unicode >= 0x0E50 && unicode <= 0x0E59) {
        // เลขไทย
        glyph = font_thai_numbers_8x8[unicode - 0x0E50];
    } else {
        // พยัญชนะไทย
        glyph = font_thai_8x8[font_index];
    }
    
    if (glyph == NULL) {
        return 0;
    }
    
    // วาดตัวอักษร 8x8
    for (uint8_t col = 0; col < 8; col++) {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                Matrix_SetPixel(x + col, y + row, r, g, b);
            }
        }
    }
    
    return 8;  // ความกว้าง 8 pixels
}

/**
 * @brief วาดข้อความไทย (ผสม UTF-8 และ ASCII)
 *
 * @param x, y - ตำแหน่ง
 * @param text - ข้อความ
 * @param color - 32-bit color
 *
 * @return ความกว้างทั้งหมด
 */
uint16_t Matrix_DrawTextThai(int16_t x, int16_t y, const char* text, uint32_t color) {
    int16_t cursor_x = x;
    uint16_t total_width = 0;
    
    while (*text) {
        // ตรวจสอบว่าเป็น UTF-8 Thai character หรือไม่
        if ((*text & 0xE0) == 0xE0) {
            // UTF-8 3-byte character (Thai)
            uint8_t char_width = Matrix_DrawCharThai(cursor_x, y, text, color);
            cursor_x += char_width;
            total_width += char_width;
            text += 3;  // ข้ามไป 3 bytes
        } else {
            // ASCII character
            uint8_t char_width = Matrix_DrawChar(cursor_x, y, *text, color);
            cursor_x += char_width;
            total_width += char_width;
            text++;
        }
    }
    
    return total_width;
}

/**
 * @brief คำนวณความกว้างข้อความ (pixel)
 *
 * @param text - ข้อความ
 *
 * @return ความกว้างรวม (6/ASCII, 8/Thai)
 */
uint16_t Matrix_GetTextWidth(const char* text) {
    uint16_t width = 0;
    
    while (*text) {
        if ((*text & 0xE0) == 0xE0) {
            // Thai character (8 pixels)
            width += 8;
            text += 3;
        } else {
            // ASCII character (6 pixels)
            width += 6;
            text++;
        }
    }
    
    return width;
}

/* ========== Scrolling Text Functions ========== */

/**
 * @brief เริ่ม scroll ข้อความ
 *
 * @param scroll - pointer ไปยัง scroll struct
 * @param text - ข้อความ
 * @param color - 32-bit color
 * @param speed - ความเร็ว (ms)
 * @param vertical - true = แนวตั้ง, false = แนวนอน
 *
 * @note ต้องเรียก Matrix_ScrollTextUpdate() ใน loop
 */
void Matrix_ScrollTextInit(ScrollText_t* scroll, const char* text, uint32_t color, uint16_t speed, bool vertical) {
    strncpy(scroll->text, text, sizeof(scroll->text) - 1);
    scroll->text[sizeof(scroll->text) - 1] = '\0';
    scroll->color = color;
    scroll->speed = speed;
    scroll->vertical = vertical;
    scroll->active = true;
    scroll->last_update = Get_CurrentMs();
    
    if (vertical) {
        scroll->position = matrix_config.height;
    } else {
        scroll->position = matrix_config.width;
    }
}

/**
 * @brief อัปเดต scroll (เรียกใน loop)
 *
 * @param scroll - pointer ไปยัง scroll struct
 * @param y - ตำแหน่ง y สำหรับ horizontal scroll
 *
 * @return true = ยัง scroll, false = เสร็จ
 */
bool Matrix_ScrollTextUpdate(ScrollText_t* scroll, int16_t y) {
    if (!scroll->active) {
        return false;
    }
    
    uint32_t current_time = Get_CurrentMs();
    if (ELAPSED_TIME(scroll->last_update, current_time) < scroll->speed) {
        return false;
    }
    
    scroll->last_update = current_time;
    
    if (scroll->vertical) {
        // Vertical scrolling
        Matrix_Clear();
        Matrix_DrawTextThai(0, scroll->position, scroll->text, scroll->color);
        scroll->position--;
        
        if (scroll->position < -8) {
            scroll->position = matrix_config.height;
        }
    } else {
        // Horizontal scrolling
        Matrix_Clear();
        Matrix_DrawTextThai(scroll->position, y, scroll->text, scroll->color);
        scroll->position--;
        
        uint16_t text_width = Matrix_GetTextWidth(scroll->text);
        if (scroll->position < -(int16_t)text_width) {
            scroll->position = matrix_config.width;
        }
    }
    
    return true;
}

/**
 * @brief หยุด scroll
 *
 * @param scroll - pointer ไปยัง scroll struct
 */
void Matrix_ScrollTextStop(ScrollText_t* scroll) {
    scroll->active = false;
}

/* ========== Sprite Functions ========== */

/**
 * @brief วาด sprite (32-bit color pixels)
 *
 * @param x, y - ตำแหน่ง
 * @param sprite - pointer ไปยัง sprite struct
 *
 * @note รองรับ transparency
 */
void Matrix_DrawSprite(int16_t x, int16_t y, const Sprite_t* sprite) {
    for (uint8_t j = 0; j < sprite->height; j++) {
        for (uint8_t i = 0; i < sprite->width; i++) {
            uint32_t color = sprite->data[j * sprite->width + i];
            
            // ตรวจสอบ transparency
            if (sprite->has_transparency && color == sprite->transparent_color) {
                continue;
            }
            
            Matrix_SetPixelColor(x + i, y + j, color);
        }
    }
}

/**
 * @brief วาด bitmap 1 bpp
 *
 * @param x, y - ตำแหน่ง
 * @param bitmap - bitmap data
 * @param w, h - ขนาด
 * @param color - 32-bit color สำหรับ pixel = 1
 */
void Matrix_DrawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t w, uint8_t h, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    for (uint8_t j = 0; j < h; j++) {
        uint8_t line = bitmap[j];
        for (uint8_t i = 0; i < w; i++) {
            if (line & (0x80 >> i)) {
                Matrix_SetPixel(x + i, y + j, r, g, b);
            }
        }
    }
}

/* ========== Advanced Effects ========== */

/**
 * @brief fade in (blocking)
 *
 * @param duration - ระยะเวลา (ms)
 * @param steps - จำนวนขั้น
 */
void Matrix_FadeIn(uint16_t duration, uint8_t steps) {
    uint16_t step_delay = duration / steps;
    
    for (uint8_t i = 0; i <= steps; i++) {
        uint8_t brightness = (255 * i) / steps;
        Matrix_SetBrightness(brightness);
        Matrix_Show();
        Delay_Ms(step_delay);
    }
}

/**
 * @brief fade out (blocking)
 *
 * @param duration - ระยะเวลา (ms)
 * @param steps - จำนวนขั้น
 */
void Matrix_FadeOut(uint16_t duration, uint8_t steps) {
    uint16_t step_delay = duration / steps;
    
    for (uint8_t i = steps; i > 0; i--) {
        uint8_t brightness = (255 * i) / steps;
        Matrix_SetBrightness(brightness);
        Matrix_Show();
        Delay_Ms(step_delay);
    }
    
    Matrix_SetBrightness(0);
    Matrix_Show();
}

/**
 * @brief wipe transition (blocking)
 *
 * @param color - 32-bit color
 * @param delay_ms - หน่วงเวลาแต่ละ column
 */
void Matrix_WipeTransition(uint32_t color, uint16_t delay_ms) {
    for (uint8_t x = 0; x < matrix_config.width; x++) {
        for (uint8_t y = 0; y < matrix_config.height; y++) {
            Matrix_SetPixelColor(x, y, color);
        }
        Matrix_Show();
        Delay_Ms(delay_ms);
    }
}

/**
 * @brief slide transition (blocking)
 *
 * @param color - 32-bit color
 * @param delay_ms - หน่วงเวลาแต่ละ row
 */
void Matrix_SlideTransition(uint32_t color, uint16_t delay_ms) {
    for (int16_t y = matrix_config.height - 1; y >= 0; y--) {
        for (uint8_t x = 0; x < matrix_config.width; x++) {
            Matrix_SetPixelColor(x, y, color);
        }
        Matrix_Show();
        Delay_Ms(delay_ms);
    }
}

/* ========== Utility Functions ========== */

/**
 * @brief หมุน buffer 90° ตามเข็ม
 *
 * @note square matrix only
 *       update NeoPixel หลังจากหมุน
 */
void Matrix_RotateBuffer90CW(void) {
    if (matrix_config.width != matrix_config.height) {
        return;  // ใช้ได้เฉพาะ square matrix
    }
    
    uint32_t temp_buffer[MATRIX_MAX_PIXELS];
    memcpy(temp_buffer, matrix_buffer, sizeof(matrix_buffer));
    
    uint8_t n = matrix_config.width;
    for (uint8_t y = 0; y < n; y++) {
        for (uint8_t x = 0; x < n; x++) {
            uint16_t old_index = Matrix_XYtoIndex(x, y);
            uint16_t new_index = Matrix_XYtoIndex(n - 1 - y, x);
            matrix_buffer[new_index] = temp_buffer[old_index];
        }
    }
    
    // อัพเดทไปยัง NeoPixel
    for (uint16_t i = 0; i < matrix_config.num_pixels; i++) {
        NeoPixel_SetPixelColor32(i, matrix_buffer[i]);
    }
}

/**
 * @brief หมุน buffer 90° ทวนเข็ม
 *
 * @note square matrix only
 */
void Matrix_RotateBuffer90CCW(void) {
    if (matrix_config.width != matrix_config.height) {
        return;
    }
    
    uint32_t temp_buffer[MATRIX_MAX_PIXELS];
    memcpy(temp_buffer, matrix_buffer, sizeof(matrix_buffer));
    
    uint8_t n = matrix_config.width;
    for (uint8_t y = 0; y < n; y++) {
        for (uint8_t x = 0; x < n; x++) {
            uint16_t old_index = Matrix_XYtoIndex(x, y);
            uint16_t new_index = Matrix_XYtoIndex(y, n - 1 - x);
            matrix_buffer[new_index] = temp_buffer[old_index];
        }
    }
    
    for (uint16_t i = 0; i < matrix_config.num_pixels; i++) {
        NeoPixel_SetPixelColor32(i, matrix_buffer[i]);
    }
}

/**
 * @brief สะท้อนแนวนอน (ซ้าย↔ขวา)
 *
 * @note update NeoPixel หลังสะท้อน
 */
void Matrix_MirrorH(void) {
    for (uint8_t y = 0; y < matrix_config.height; y++) {
        for (uint8_t x = 0; x < matrix_config.width / 2; x++) {
            uint16_t left_index = Matrix_XYtoIndex(x, y);
            uint16_t right_index = Matrix_XYtoIndex(matrix_config.width - 1 - x, y);
            
            uint32_t temp = matrix_buffer[left_index];
            matrix_buffer[left_index] = matrix_buffer[right_index];
            matrix_buffer[right_index] = temp;
        }
    }
    
    for (uint16_t i = 0; i < matrix_config.num_pixels; i++) {
        NeoPixel_SetPixelColor32(i, matrix_buffer[i]);
    }
}

/**
 * @brief สะท้อนแนวตั้ง (บน↔ล่าง)
 *
 * @note update NeoPixel หลังสะท้อน
 */
void Matrix_MirrorV(void) {
    for (uint8_t y = 0; y < matrix_config.height / 2; y++) {
        for (uint8_t x = 0; x < matrix_config.width; x++) {
            uint16_t top_index = Matrix_XYtoIndex(x, y);
            uint16_t bottom_index = Matrix_XYtoIndex(x, matrix_config.height - 1 - y);
            
            uint32_t temp = matrix_buffer[top_index];
            matrix_buffer[top_index] = matrix_buffer[bottom_index];
            matrix_buffer[bottom_index] = temp;
        }
    }
    
    for (uint16_t i = 0; i < matrix_config.num_pixels; i++) {
        NeoPixel_SetPixelColor32(i, matrix_buffer[i]);
    }
}

/**
 * @brief ตั้งค่าความสว่างรวม
 *
 * @param brightness - ค่าความสว่าง (0-255)
 *
 * @note ส่งต่อไปยัง NeoPixel_SetBrightness()
 *       brightness ถูก apply ตอน SetPixelColor
 */
void Matrix_SetBrightness(uint8_t brightness) {
    NeoPixel_SetBrightness(brightness);
}

uint8_t Matrix_GetBrightness(void) {
    return NeoPixel_GetBrightness();
}

/**
 * @brief แปลงพิกัด (x, y) เป็น LED index
 *
 * @param x - พิกัดแนวนอน
 * @param y - พิกัดแนวตั้ง
 *
 * @return LED index (0 = LED แรก)
 *
 * @note เลือก mapping function ตาม wiring pattern
 *       รองรับ ZIGZAG_LEFT, SNAKE, ZIGZAG_RIGHT, COLUMNS
 */
uint16_t Matrix_XYtoIndex(int16_t x, int16_t y) {
    switch (matrix_config.wiring) {
        case WIRING_ZIGZAG_LEFT:
            return xy_to_index_zigzag_left(x, y);
        case WIRING_SNAKE:
            return xy_to_index_snake(x, y);
        case WIRING_ZIGZAG_RIGHT:
            return xy_to_index_zigzag_right(x, y);
        case WIRING_COLUMNS:
            return xy_to_index_columns(x, y);
        default:
            return xy_to_index_zigzag_left(x, y);
    }
}

/**
 * @brief ตรวจสอบว่าพิกัด (x, y) อยู่ในขอบเขต matrix หรือไม่
 *
 * @param x - พิกัดแนวนอน
 * @param y - พิกัดแนวตั้ง
 *
 * @return true = อยู่ในขอบเขต, false = อยู่นอกขอบเขต
 */
bool Matrix_IsInBounds(int16_t x, int16_t y) {
    return (x >= 0 && x < matrix_config.width && y >= 0 && y < matrix_config.height);
}

/* ========== Pattern Generation ========== */

/**
 * @brief สร้างลายตาหมากรุก
 *
 * @param color1 - สีแรก
 * @param color2 - สีที่สอง
 */
void Matrix_PatternCheckerboard(uint32_t color1, uint32_t color2) {
    for (uint8_t y = 0; y < matrix_config.height; y++) {
        for (uint8_t x = 0; x < matrix_config.width; x++) {
            uint32_t color = ((x + y) % 2 == 0) ? color1 : color2;
            Matrix_SetPixelColor(x, y, color);
        }
    }
}

/**
 * @brief สร้าง gradient แนวนอน
 *
 * @param start_color - สีเริ่มต้น (ซ้าย)
 * @param end_color - สีสิ้นสุด (ขวา)
 */
void Matrix_PatternGradientH(uint32_t start_color, uint32_t end_color) {
    uint8_t start_r = (start_color >> 16) & 0xFF;
    uint8_t start_g = (start_color >> 8) & 0xFF;
    uint8_t start_b = start_color & 0xFF;
    
    uint8_t end_r = (end_color >> 16) & 0xFF;
    uint8_t end_g = (end_color >> 8) & 0xFF;
    uint8_t end_b = end_color & 0xFF;
    
    for (uint8_t x = 0; x < matrix_config.width; x++) {
        uint8_t r = start_r + ((end_r - start_r) * x) / (matrix_config.width - 1);
        uint8_t g = start_g + ((end_g - start_g) * x) / (matrix_config.width - 1);
        uint8_t b = start_b + ((end_b - start_b) * x) / (matrix_config.width - 1);
        
        for (uint8_t y = 0; y < matrix_config.height; y++) {
            Matrix_SetPixel(x, y, r, g, b);
        }
    }
}

/**
 * @brief สร้าง gradient แนวตั้ง
 *
 * @param start_color - สีเริ่มต้น (บน)
 * @param end_color - สีสิ้นสุด (ล่าง)
 */
void Matrix_PatternGradientV(uint32_t start_color, uint32_t end_color) {
    uint8_t start_r = (start_color >> 16) & 0xFF;
    uint8_t start_g = (start_color >> 8) & 0xFF;
    uint8_t start_b = start_color & 0xFF;
    
    uint8_t end_r = (end_color >> 16) & 0xFF;
    uint8_t end_g = (end_color >> 8) & 0xFF;
    uint8_t end_b = end_color & 0xFF;
    
    for (uint8_t y = 0; y < matrix_config.height; y++) {
        uint8_t r = start_r + ((end_r - start_r) * y) / (matrix_config.height - 1);
        uint8_t g = start_g + ((end_g - start_g) * y) / (matrix_config.height - 1);
        uint8_t b = start_b + ((end_b - start_b) * y) / (matrix_config.height - 1);
        
        for (uint8_t x = 0; x < matrix_config.width; x++) {
            Matrix_SetPixel(x, y, r, g, b);
        }
    }
}

/**
 * @brief สร้าง pattern สุ่ม
 *
 * @param density - ความหนาแน่น (0-100%)
 * @param color - 32-bit color
 */
void Matrix_PatternRandom(uint8_t density, uint32_t color) {
    Matrix_Clear();
    
    uint16_t num_pixels = (matrix_config.num_pixels * density) / 100;
    
    for (uint16_t i = 0; i < num_pixels; i++) {
        uint8_t x = rand() % matrix_config.width;
        uint8_t y = rand() % matrix_config.height;
        Matrix_SetPixelColor(x, y, color);
    }
}

/* ========== Private Functions ========== */

/**
 * @brief แปลง (x,y) → index แบบ zigzag (แถวคู่กลับด้านซ้าย)
 *
 * @param x - พิกัดแนวนอน
 * @param y - พิกัดแนวตั้ง
 *
 * @return LED index
 */
static uint16_t xy_to_index_zigzag_left(int16_t x, int16_t y) {
    if (y % 2 == 0) {
        // แถวคู่: ซ้ายไปขวา
        return y * matrix_config.width + x;
    } else {
        // แถวคี่: ขวาไปซ้าย
        return y * matrix_config.width + (matrix_config.width - 1 - x);
    }
}

/**
 * @brief แปลง (x,y) → index แบบ snake (ซ้าย→ขวาทุกแถว)
 *
 * @param x, y - พิกัด
 *
 * @return LED index
 */
static uint16_t xy_to_index_snake(int16_t x, int16_t y) {
    // งูเลื้อยต่อเนื่อง (ซ้ายไปขวาทุกแถว)
    return y * matrix_config.width + x;
}

/**
 * @brief แปลง (x,y) → index แบบ zigzag (แถวคู่กลับด้านขวา)
 *
 * @param x, y - พิกัด
 *
 * @return LED index
 */
static uint16_t xy_to_index_zigzag_right(int16_t x, int16_t y) {
    if (y % 2 == 0) {
        // แถวคู่: ขวาไปซ้าย
        return y * matrix_config.width + (matrix_config.width - 1 - x);
    } else {
        // แถวคี่: ซ้ายไปขวา
        return y * matrix_config.width + x;
    }
}

/**
 * @brief แปลง (x,y) → index แบบ column-major
 *
 * @param x, y - พิกัด
 *
 * @return LED index
 */
static uint16_t xy_to_index_columns(int16_t x, int16_t y) {
    // เรียงตามคอลัมน์
    if (x % 2 == 0) {
        return x * matrix_config.height + y;
    } else {
        return x * matrix_config.height + (matrix_config.height - 1 - y);
    }
}

/**
 * @brief แปลง UTF-8 3-byte → Unicode
 *
 * @param utf8_char - pointer ไปยัง UTF-8 char
 *
 * @return Unicode code point
 */
static uint16_t utf8_to_unicode(const char* utf8_char) {
    // แปลง UTF-8 3-byte เป็น Unicode (สำหรับภาษาไทย)
    if ((utf8_char[0] & 0xE0) == 0xE0) {
        uint16_t unicode = ((utf8_char[0] & 0x0F) << 12) |
                          ((utf8_char[1] & 0x3F) << 6) |
                          (utf8_char[2] & 0x3F);
        return unicode;
    }
    return 0;
}

/**
 * @brief ค้นหา font index จาก Unicode
 *
 * @param unicode - Unicode code point
 *
 * @return font index, -1 ถ้าไม่พบ
 */
static int8_t get_thai_font_index(uint16_t unicode) {
    // ค้นหา index ของฟอนต์จาก Unicode
    for (uint8_t i = 0; i < THAI_CONSONANT_COUNT; i++) {
        if (thai_consonant_map[i].unicode == unicode) {
            return thai_consonant_map[i].index;
        }
    }
    return -1;  // ไม่พบ
}
