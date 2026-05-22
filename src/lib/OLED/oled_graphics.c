/**
 * @file oled_graphics.c
 * @brief OLED Graphics Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "oled_graphics.h"

/* ========== Helper Functions ========== */

/**
 * @brief Swap two values
 */
static void swap_uint8(uint8_t* a, uint8_t* b) {
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Absolute value
 */
static uint8_t abs_int(int16_t x) {
    return (x < 0) ? -x : x;
}

/* ========== Line Drawing ========== */

/**
 * @brief วาดเส้นตรง (Bresenham's algorithm)
 */
void OLED_DrawLine(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, OLED_Color color) {
    int16_t dx = abs_int(x1 - x0);
    int16_t dy = abs_int(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while(1) {
        OLED_SetPixel(oled, x0, y0, color);
        
        if(x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if(e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if(e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief วาดเส้นแนวนอน
 */
void OLED_DrawHLine(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, OLED_Color color) {
    for(uint8_t i = 0; i < w; i++) {
        OLED_SetPixel(oled, x + i, y, color);
    }
}

/**
 * @brief วาดเส้นแนวตั้ง
 */
void OLED_DrawVLine(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t h, OLED_Color color) {
    for(uint8_t i = 0; i < h; i++) {
        OLED_SetPixel(oled, x, y + i, color);
    }
}

/* ========== Rectangle Drawing ========== */

/**
 * @brief วาดสี่เหลี่ยม (เส้นขอบ)
 */
void OLED_DrawRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_Color color) {
    OLED_DrawHLine(oled, x, y, w, color);
    OLED_DrawHLine(oled, x, y + h - 1, w, color);
    OLED_DrawVLine(oled, x, y, h, color);
    OLED_DrawVLine(oled, x + w - 1, y, h, color);
}

/**
 * @brief วาดสี่เหลี่ยมแบบเติมสี
 */
void OLED_FillRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_Color color) {
    for(uint8_t i = 0; i < h; i++) {
        OLED_DrawHLine(oled, x, y + i, w, color);
    }
}

/**
 * @brief Helper for drawing circle quadrants
 */
static void draw_circle_helper(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, uint8_t quadrant, OLED_Color color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while(x < y) {
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if(quadrant & 0x1) {
            OLED_SetPixel(oled, x0 + x, y0 - y, color);
            OLED_SetPixel(oled, x0 + y, y0 - x, color);
        }
        if(quadrant & 0x2) {
            OLED_SetPixel(oled, x0 - y, y0 - x, color);
            OLED_SetPixel(oled, x0 - x, y0 - y, color);
        }
        if(quadrant & 0x4) {
            OLED_SetPixel(oled, x0 - y, y0 + x, color);
            OLED_SetPixel(oled, x0 - x, y0 + y, color);
        }
        if(quadrant & 0x8) {
            OLED_SetPixel(oled, x0 + x, y0 + y, color);
            OLED_SetPixel(oled, x0 + y, y0 + x, color);
        }
    }
}

/**
 * @brief วาดสี่เหลี่ยมมุมมน (เส้นขอบ)
 */
void OLED_DrawRoundRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, OLED_Color color) {
    // Draw straight edges
    OLED_DrawHLine(oled, x + r, y, w - 2 * r, color);
    OLED_DrawHLine(oled, x + r, y + h - 1, w - 2 * r, color);
    OLED_DrawVLine(oled, x, y + r, h - 2 * r, color);
    OLED_DrawVLine(oled, x + w - 1, y + r, h - 2 * r, color);
    
    // Draw corners
    draw_circle_helper(oled, x + r, y + r, r, 0x1, color);
    draw_circle_helper(oled, x + w - r - 1, y + r, r, 0x2, color);
    draw_circle_helper(oled, x + w - r - 1, y + h - r - 1, r, 0x4, color);
    draw_circle_helper(oled, x + r, y + h - r - 1, r, 0x8, color);
}

/**
 * @brief Helper for filling circle quadrants
 */
static void fill_circle_helper(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, uint8_t quadrant, int16_t delta, OLED_Color color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while(x < y) {
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if(quadrant & 0x1) {
            OLED_DrawVLine(oled, x0 + x, y0 - y, 2 * y + 1 + delta, color);
            OLED_DrawVLine(oled, x0 + y, y0 - x, 2 * x + 1 + delta, color);
        }
        if(quadrant & 0x2) {
            OLED_DrawVLine(oled, x0 - x, y0 - y, 2 * y + 1 + delta, color);
            OLED_DrawVLine(oled, x0 - y, y0 - x, 2 * x + 1 + delta, color);
        }
    }
}

/**
 * @brief วาดสี่เหลี่ยมมุมมนแบบเติมสี
 */
void OLED_FillRoundRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, OLED_Color color) {
    // Fill center rectangle
    OLED_FillRect(oled, x + r, y, w - 2 * r, h, color);
    
    // Fill corners
    fill_circle_helper(oled, x + r, y + r, r, 0x1, h - 2 * r - 1, color);
    fill_circle_helper(oled, x + w - r - 1, y + r, r, 0x2, h - 2 * r - 1, color);
}

/* ========== Circle Drawing ========== */

/**
 * @brief วาดวงกลม (Midpoint circle algorithm)
 */
void OLED_DrawCircle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, OLED_Color color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    OLED_SetPixel(oled, x0, y0 + r, color);
    OLED_SetPixel(oled, x0, y0 - r, color);
    OLED_SetPixel(oled, x0 + r, y0, color);
    OLED_SetPixel(oled, x0 - r, y0, color);
    
    while(x < y) {
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        OLED_SetPixel(oled, x0 + x, y0 + y, color);
        OLED_SetPixel(oled, x0 - x, y0 + y, color);
        OLED_SetPixel(oled, x0 + x, y0 - y, color);
        OLED_SetPixel(oled, x0 - x, y0 - y, color);
        OLED_SetPixel(oled, x0 + y, y0 + x, color);
        OLED_SetPixel(oled, x0 - y, y0 + x, color);
        OLED_SetPixel(oled, x0 + y, y0 - x, color);
        OLED_SetPixel(oled, x0 - y, y0 - x, color);
    }
}

/**
 * @brief วาดวงกลมแบบเติมสี
 */
void OLED_FillCircle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, OLED_Color color) {
    OLED_DrawVLine(oled, x0, y0 - r, 2 * r + 1, color);
    fill_circle_helper(oled, x0, y0, r, 0x3, 0, color);
}

/* ========== Triangle Drawing ========== */

/**
 * @brief วาดสามเหลี่ยม (เส้นขอบ)
 */
void OLED_DrawTriangle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_Color color) {
    OLED_DrawLine(oled, x0, y0, x1, y1, color);
    OLED_DrawLine(oled, x1, y1, x2, y2, color);
    OLED_DrawLine(oled, x2, y2, x0, y0, color);
}

/**
 * @brief วาดสามเหลี่ยมแบบเติมสี
 */
void OLED_FillTriangle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_Color color) {
    int16_t a, b, y, last;
    
    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if(y0 > y1) {
        swap_uint8(&y0, &y1);
        swap_uint8(&x0, &x1);
    }
    if(y1 > y2) {
        swap_uint8(&y2, &y1);
        swap_uint8(&x2, &x1);
    }
    if(y0 > y1) {
        swap_uint8(&y0, &y1);
        swap_uint8(&x0, &x1);
    }
    
    if(y0 == y2) {
        // Degenerate triangle
        a = b = x0;
        if(x1 < a) a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a) a = x2;
        else if(x2 > b) b = x2;
        OLED_DrawHLine(oled, a, y0, b - a + 1, color);
        return;
    }
    
    int16_t dx01 = x1 - x0;
    int16_t dy01 = y1 - y0;
    int16_t dx02 = x2 - x0;
    int16_t dy02 = y2 - y0;
    int16_t dx12 = x2 - x1;
    int16_t dy12 = y2 - y1;
    int32_t sa = 0;
    int32_t sb = 0;
    
    // Upper part of triangle
    if(y1 == y2) last = y1;
    else last = y1 - 1;
    
    for(y = y0; y <= last; y++) {
        a = x0 + sa / dy02;
        b = x0 + sb / dy01;
        sa += dx02;
        sb += dx01;
        if(a > b) swap_uint8((uint8_t*)&a, (uint8_t*)&b);
        OLED_DrawHLine(oled, a, y, b - a + 1, color);
    }
    
    // Lower part of triangle
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if(a > b) swap_uint8((uint8_t*)&a, (uint8_t*)&b);
        OLED_DrawHLine(oled, a, y, b - a + 1, color);
    }
}

/* ========== Bitmap Drawing ========== */

/**
 * @brief วาด Bitmap
 */
void OLED_DrawBitmap(OLED_Handle* oled, uint8_t x, uint8_t y, const OLED_Bitmap* bitmap, OLED_Color color) {
    for(uint8_t j = 0; j < bitmap->height; j++) {
        for(uint8_t i = 0; i < bitmap->width; i++) {
            uint16_t byte_index = (j * bitmap->width + i) / 8;
            uint8_t bit_index = 7 - ((j * bitmap->width + i) % 8);
            
            if(bitmap->data[byte_index] & (1 << bit_index)) {
                OLED_SetPixel(oled, x + i, y + j, color);
            }
        }
    }
}

/**
 * @brief วาด Bitmap แบบมี transparency
 */
void OLED_DrawBitmapMask(OLED_Handle* oled, uint8_t x, uint8_t y, const OLED_Bitmap* bitmap, const OLED_Bitmap* mask, OLED_Color color) {
    for(uint8_t j = 0; j < bitmap->height; j++) {
        for(uint8_t i = 0; i < bitmap->width; i++) {
            uint16_t byte_index = (j * bitmap->width + i) / 8;
            uint8_t bit_index = 7 - ((j * bitmap->width + i) % 8);
            
            // Check mask
            if(mask->data[byte_index] & (1 << bit_index)) {
                // Check bitmap
                if(bitmap->data[byte_index] & (1 << bit_index)) {
                    OLED_SetPixel(oled, x + i, y + j, color);
                }
            }
        }
    }
}

/* ========== Sprite System ========== */

/**
 * @brief สร้าง Sprite
 */
void OLED_CreateSprite(OLED_Sprite* sprite, uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t** frames, uint8_t frame_count) {
    sprite->x = x;
    sprite->y = y;
    sprite->width = width;
    sprite->height = height;
    sprite->frame = 0;
    sprite->frame_count = frame_count;
    sprite->frames = frames;
}

/**
 * @brief วาด Sprite
 */
void OLED_DrawSprite(OLED_Handle* oled, OLED_Sprite* sprite, OLED_Color color) {
    OLED_Bitmap bitmap = {
        .width = sprite->width,
        .height = sprite->height,
        .data = sprite->frames[sprite->frame]
    };
    OLED_DrawBitmap(oled, sprite->x, sprite->y, &bitmap, color);
}

/**
 * @brief เปลี่ยน Frame ของ Sprite
 */
void OLED_SetSpriteFrame(OLED_Sprite* sprite, uint8_t frame) {
    if(frame < sprite->frame_count) {
        sprite->frame = frame;
    }
}

/**
 * @brief ไปยัง Frame ถัดไป
 */
void OLED_NextSpriteFrame(OLED_Sprite* sprite) {
    sprite->frame = (sprite->frame + 1) % sprite->frame_count;
}

/**
 * @brief ย้าย Sprite
 */
void OLED_MoveSprite(OLED_Sprite* sprite, uint8_t x, uint8_t y) {
    sprite->x = x;
    sprite->y = y;
}

/* ========== Advanced Graphics ========== */

/**
 * @brief วาด Progress Bar
 */
void OLED_DrawProgressBar(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t progress) {
    // Clamp progress to 0-100
    if(progress > 100) progress = 100;
    
    // Draw border
    OLED_DrawRect(oled, x, y, w, h, OLED_COLOR_WHITE);
    
    // Calculate fill width
    uint8_t fill_w = ((uint16_t)(w - 2) * progress) / 100;
    
    // Draw fill
    if(fill_w > 0) {
        OLED_FillRect(oled, x + 1, y + 1, fill_w, h - 2, OLED_COLOR_WHITE);
    }
}

/**
 * @brief วาดกราฟเส้น
 */
void OLED_DrawGraph(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, int16_t* data, uint8_t data_count, int16_t min_val, int16_t max_val) {
    if(data_count < 2) return;
    
    // Draw border
    OLED_DrawRect(oled, x, y, w, h, OLED_COLOR_WHITE);
    
    // Calculate scale
    int16_t range = max_val - min_val;
    if(range == 0) range = 1;
    
    // Draw data points
    for(uint8_t i = 0; i < data_count - 1; i++) {
        // Calculate positions
        uint8_t x1 = x + 1 + ((uint16_t)i * (w - 2)) / (data_count - 1);
        uint8_t y1 = y + h - 2 - (((uint16_t)(data[i] - min_val) * (h - 3)) / range);
        
        uint8_t x2 = x + 1 + ((uint16_t)(i + 1) * (w - 2)) / (data_count - 1);
        uint8_t y2 = y + h - 2 - (((uint16_t)(data[i + 1] - min_val) * (h - 3)) / range);
        
        // Draw line
        OLED_DrawLine(oled, x1, y1, x2, y2, OLED_COLOR_WHITE);
    }
}
