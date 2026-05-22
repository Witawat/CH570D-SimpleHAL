/**
 * @file SimpleMAX7219.c
 * @brief MAX7219 LED Matrix Driver Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "MAX7219.h"
#include "max7219_fonts.h"
#if (MAX7219_ENABLE_THAI_FULL)
#include "max7219_fonts_thai.h"
#endif
#include <stdlib.h>

/* ========== Private Variables ========== */

static MAX7219_Handle g_max7219_handle;  // Global handle instance

/* ========== Private Helper Functions ========== */

/**
 * @brief ส่งข้อมูลไปยัง MAX7219 ผ่าน SPI
 */
static void MAX7219_WriteRegister(MAX7219_Handle* handle, uint8_t reg, uint8_t data) {
    if (!handle) return;
    
    // Pull CS low
    digitalWrite(handle->cs_pin, LOW);
    Delay_Us(1);
    
    // Send register address and data for each device in cascade
    for (int i = 0; i < handle->num_devices; i++) {
        SPI_Transfer(reg);
        SPI_Transfer(data);
    }
    
    // Pull CS high to latch data
    digitalWrite(handle->cs_pin, HIGH);
    Delay_Us(1);
}

/**
 * @brief ส่งข้อมูลไปยัง device เฉพาะใน cascade
 * @note Currently unused but kept for future multi-device support
 */
__attribute__((unused))
static void MAX7219_WriteRegisterToDevice(MAX7219_Handle* handle, uint8_t device_idx, uint8_t reg, uint8_t data) {
    if (!handle || device_idx >= handle->num_devices) return;
    
    // Pull CS low
    digitalWrite(handle->cs_pin, LOW);
    Delay_Us(1);
    
    // Send data to specific device, NO-OP to others
    for (int i = handle->num_devices - 1; i >= 0; i--) {
        if (i == device_idx) {
            SPI_Transfer(reg);
            SPI_Transfer(data);
        } else {
            SPI_Transfer(MAX7219_REG_NOOP);
            SPI_Transfer(0x00);
        }
    }
    
    // Pull CS high to latch data
    digitalWrite(handle->cs_pin, HIGH);
    Delay_Us(1);
}

/**
 * @brief Swap function for sorting
 */
static void swap_int16(int16_t* a, int16_t* b) {
    int16_t temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Absolute value
 */
static int16_t abs_int16(int16_t x) {
    return (x < 0) ? -x : x;
}

/* ========== Initialization Functions ========== */

MAX7219_Handle* MAX7219_Init(uint8_t clk_pin, uint8_t mosi_pin, uint8_t cs_pin, uint8_t num_devices) {
    if (num_devices == 0 || num_devices > MAX7219_MAX_DEVICES) {
        return NULL;
    }
    
    MAX7219_Handle* handle = &g_max7219_handle;
    
    // Store configuration
    handle->clk_pin = clk_pin;
    handle->mosi_pin = mosi_pin;
    handle->cs_pin = cs_pin;
    handle->num_devices = num_devices;
    handle->intensity = 8;
    handle->display_on = true;
    handle->font = &font_5x7;  // Default font
    
    // Initialize animation state
    handle->animation.active = false;
    handle->animation.frames = NULL;
    handle->animation.num_frames = 0;
    handle->animation.current_frame = 0;
    handle->animation.frame_delay = 0;
    handle->animation.last_update = 0;
    handle->animation.loop = false;
    
    // Initialize scrolling state
    handle->scroll.active = false;
    handle->scroll.text = NULL;
    handle->scroll.offset = 0;
    handle->scroll.scroll_delay = 0;
    handle->scroll.last_update = 0;
    handle->scroll.vertical = false;
    handle->scroll.font = &font_5x7;
    
    // Clear display buffer
    memset(handle->buffer, 0, sizeof(handle->buffer));
    
    // Initialize SPI
    SPI_SimpleInit(SPI_MODE0, SPI_2MHZ, SPI_PINS_DEFAULT);
    
    // Initialize CS pin
    pinMode(cs_pin, PIN_MODE_OUTPUT);
    digitalWrite(cs_pin, HIGH);
    
    // Initialize MAX7219 registers for all devices
    MAX7219_WriteRegister(handle, MAX7219_REG_SCANLIMIT, 0x07);    // Scan all 8 digits
    MAX7219_WriteRegister(handle, MAX7219_REG_DECODE, 0x00);       // No decode mode
    MAX7219_WriteRegister(handle, MAX7219_REG_DISPLAYTEST, 0x00);  // Normal operation
    MAX7219_WriteRegister(handle, MAX7219_REG_INTENSITY, handle->intensity);
    MAX7219_WriteRegister(handle, MAX7219_REG_SHUTDOWN, 0x01);     // Normal operation (not shutdown)
    
    // Clear display
    MAX7219_Clear(handle, true);
    
    return handle;
}

void MAX7219_SetIntensity(MAX7219_Handle* handle, uint8_t intensity) {
    if (!handle) return;
    
    if (intensity > MAX7219_INTENSITY_MAX) {
        intensity = MAX7219_INTENSITY_MAX;
    }
    
    handle->intensity = intensity;
    MAX7219_WriteRegister(handle, MAX7219_REG_INTENSITY, intensity);
}

void MAX7219_DisplayControl(MAX7219_Handle* handle, bool on) {
    if (!handle) return;
    
    handle->display_on = on;
    MAX7219_WriteRegister(handle, MAX7219_REG_SHUTDOWN, on ? 0x01 : 0x00);
}

void MAX7219_Clear(MAX7219_Handle* handle, bool update) {
    if (!handle) return;
    
    // Clear buffer
    memset(handle->buffer, 0, sizeof(handle->buffer));
    
    // Update hardware if requested
    if (update) {
        MAX7219_Update(handle);
    }
}

void MAX7219_Update(MAX7219_Handle* handle) {
    if (!handle) return;
    
    // Send each row to all devices
    for (uint8_t row = 0; row < MAX7219_MATRIX_SIZE; row++) {
        digitalWrite(handle->cs_pin, LOW);
        Delay_Us(1);
        
        // Send row data for each device (reverse order for cascading)
        for (int dev = handle->num_devices - 1; dev >= 0; dev--) {
            SPI_Transfer(MAX7219_REG_DIGIT0 + row);
            SPI_Transfer(handle->buffer[dev][row]);
        }
        
        digitalWrite(handle->cs_pin, HIGH);
        Delay_Us(1);
    }
}

void MAX7219_DisplayTest(MAX7219_Handle* handle, bool test) {
    if (!handle) return;
    
    MAX7219_WriteRegister(handle, MAX7219_REG_DISPLAYTEST, test ? 0x01 : 0x00);
}

/* ========== Graphics Primitives ========== */

void MAX7219_SetPixel(MAX7219_Handle* handle, int16_t x, int16_t y, bool on) {
    if (!handle) return;
    
    // Calculate which device and local x coordinate
    uint8_t device_idx = x / MAX7219_MATRIX_SIZE;
    uint8_t local_x = x % MAX7219_MATRIX_SIZE;
    
    // Check bounds
    if (device_idx >= handle->num_devices || y < 0 || y >= MAX7219_MATRIX_SIZE) {
        return;
    }
    
    // Set or clear pixel in buffer
    if (on) {
        handle->buffer[device_idx][y] |= (1 << (7 - local_x));
    } else {
        handle->buffer[device_idx][y] &= ~(1 << (7 - local_x));
    }
}

bool MAX7219_GetPixel(MAX7219_Handle* handle, int16_t x, int16_t y) {
    if (!handle) return false;
    
    // Calculate which device and local x coordinate
    uint8_t device_idx = x / MAX7219_MATRIX_SIZE;
    uint8_t local_x = x % MAX7219_MATRIX_SIZE;
    
    // Check bounds
    if (device_idx >= handle->num_devices || y < 0 || y >= MAX7219_MATRIX_SIZE) {
        return false;
    }
    
    return (handle->buffer[device_idx][y] & (1 << (7 - local_x))) != 0;
}

void MAX7219_DrawLine(MAX7219_Handle* handle, int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on) {
    if (!handle) return;
    
    // Bresenham's line algorithm
    int16_t dx = abs_int16(x1 - x0);
    int16_t dy = abs_int16(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1) {
        MAX7219_SetPixel(handle, x0, y0, on);
        
        if (x0 == x1 && y0 == y1) break;
        
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

void MAX7219_DrawRect(MAX7219_Handle* handle, int16_t x, int16_t y, int16_t w, int16_t h, bool filled, bool on) {
    if (!handle) return;
    
    if (filled) {
        // Draw filled rectangle
        for (int16_t i = 0; i < h; i++) {
            for (int16_t j = 0; j < w; j++) {
                MAX7219_SetPixel(handle, x + j, y + i, on);
            }
        }
    } else {
        // Draw rectangle outline
        MAX7219_DrawLine(handle, x, y, x + w - 1, y, on);           // Top
        MAX7219_DrawLine(handle, x, y + h - 1, x + w - 1, y + h - 1, on);  // Bottom
        MAX7219_DrawLine(handle, x, y, x, y + h - 1, on);           // Left
        MAX7219_DrawLine(handle, x + w - 1, y, x + w - 1, y + h - 1, on);  // Right
    }
}

void MAX7219_DrawCircle(MAX7219_Handle* handle, int16_t x0, int16_t y0, int16_t r, bool filled, bool on) {
    if (!handle) return;
    
    // Bresenham's circle algorithm
    int16_t x = 0;
    int16_t y = r;
    int16_t d = 3 - 2 * r;
    
    while (x <= y) {
        if (filled) {
            // Draw horizontal lines to fill circle
            MAX7219_DrawLine(handle, x0 - x, y0 + y, x0 + x, y0 + y, on);
            MAX7219_DrawLine(handle, x0 - x, y0 - y, x0 + x, y0 - y, on);
            MAX7219_DrawLine(handle, x0 - y, y0 + x, x0 + y, y0 + x, on);
            MAX7219_DrawLine(handle, x0 - y, y0 - x, x0 + y, y0 - x, on);
        } else {
            // Draw circle outline (8 octants)
            MAX7219_SetPixel(handle, x0 + x, y0 + y, on);
            MAX7219_SetPixel(handle, x0 - x, y0 + y, on);
            MAX7219_SetPixel(handle, x0 + x, y0 - y, on);
            MAX7219_SetPixel(handle, x0 - x, y0 - y, on);
            MAX7219_SetPixel(handle, x0 + y, y0 + x, on);
            MAX7219_SetPixel(handle, x0 - y, y0 + x, on);
            MAX7219_SetPixel(handle, x0 + y, y0 - x, on);
            MAX7219_SetPixel(handle, x0 - y, y0 - x, on);
        }
        
        x++;
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            y--;
            d = d + 4 * (x - y) + 10;
        }
    }
}

void MAX7219_DrawTriangle(MAX7219_Handle* handle, int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                          int16_t x2, int16_t y2, bool filled, bool on) {
    if (!handle) return;
    
    if (filled) {
        // Sort vertices by y coordinate
        if (y0 > y1) { swap_int16(&y0, &y1); swap_int16(&x0, &x1); }
        if (y1 > y2) { swap_int16(&y1, &y2); swap_int16(&x1, &x2); }
        if (y0 > y1) { swap_int16(&y0, &y1); swap_int16(&x0, &x1); }
        
        // Draw filled triangle using horizontal lines
        int16_t total_height = y2 - y0;
        for (int16_t i = 0; i < total_height; i++) {
            bool second_half = i > y1 - y0 || y1 == y0;
            int16_t segment_height = second_half ? y2 - y1 : y1 - y0;
            
            if (segment_height == 0) continue;
            
            float alpha = (float)i / total_height;
            float beta = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;
            
            int16_t ax = x0 + (x2 - x0) * alpha;
            int16_t bx = second_half ? x1 + (x2 - x1) * beta : x0 + (x1 - x0) * beta;
            
            if (ax > bx) swap_int16(&ax, &bx);
            
            for (int16_t j = ax; j <= bx; j++) {
                MAX7219_SetPixel(handle, j, y0 + i, on);
            }
        }
    } else {
        // Draw triangle outline
        MAX7219_DrawLine(handle, x0, y0, x1, y1, on);
        MAX7219_DrawLine(handle, x1, y1, x2, y2, on);
        MAX7219_DrawLine(handle, x2, y2, x0, y0, on);
    }
}

void MAX7219_DrawBitmap(MAX7219_Handle* handle, int16_t x, int16_t y, const uint8_t* bitmap, 
                        uint8_t width, uint8_t height) {
    if (!handle || !bitmap) return;
    
    for (uint8_t row = 0; row < height; row++) {
        uint8_t byte_data = bitmap[row];
        for (uint8_t col = 0; col < width; col++) {
            bool pixel_on = (byte_data & (1 << (7 - col))) != 0;
            MAX7219_SetPixel(handle, x + col, y + row, pixel_on);
        }
    }
}

/* ========== Text Functions ========== */

void MAX7219_SetFont(MAX7219_Handle* handle, const MAX7219_Font* font) {
    if (!handle || !font) return;
    handle->font = font;
}

uint8_t MAX7219_DrawChar(MAX7219_Handle* handle, int16_t x, int16_t y, char ch) {
    if (!handle || !handle->font) return 0;
    
    const MAX7219_Font* font = handle->font;
    
    // Check if character is in font range
    if (ch < font->first_char || ch > font->last_char) {
        return 0;
    }
    
    // Calculate character index
    uint16_t char_idx = ch - font->first_char;
    uint16_t data_offset = char_idx * font->width;
    
    // Draw character
    for (uint8_t col = 0; col < font->width; col++) {
        uint8_t col_data = font->data[data_offset + col];
        for (uint8_t row = 0; row < font->height; row++) {
            bool pixel_on = (col_data & (1 << row)) != 0;
            MAX7219_SetPixel(handle, x + col, y + row, pixel_on);
        }
    }
    
    return font->width + 1;  // Return character width + spacing
}

uint16_t MAX7219_DrawString(MAX7219_Handle* handle, int16_t x, int16_t y, const char* text) {
    if (!handle || !text) return 0;
    
    uint16_t cursor_x = x;
    
    while (*text) {
        uint8_t char_width = MAX7219_DrawChar(handle, cursor_x, y, *text);
        cursor_x += char_width;
        text++;
    }
    
    return cursor_x - x;  // Return total width
}

uint16_t MAX7219_GetStringWidth(MAX7219_Handle* handle, const char* text) {
    if (!handle || !text || !handle->font) return 0;
    
    uint16_t width = 0;
    const MAX7219_Font* font = handle->font;
    
    while (*text) {
        if (*text >= font->first_char && *text <= font->last_char) {
            width += font->width + 1;  // Character width + spacing
        }
        text++;
    }
    
    return width > 0 ? width - 1 : 0;  // Remove last spacing
}

/* ========== Scrolling Functions ========== */

void MAX7219_StartScrollText(MAX7219_Handle* handle, const char* text, uint16_t scroll_delay) {
    if (!handle || !text) return;
    
    handle->scroll.text = text;
    handle->scroll.offset = handle->num_devices * MAX7219_MATRIX_SIZE;  // Start from right
    handle->scroll.scroll_delay = scroll_delay;
    handle->scroll.last_update = Get_CurrentMs();
    handle->scroll.active = true;
    handle->scroll.vertical = false;
    handle->scroll.font = handle->font;
}

bool MAX7219_UpdateScroll(MAX7219_Handle* handle) {
    if (!handle || !handle->scroll.active) return false;
    
    uint32_t current_time = Get_CurrentMs();
    
    // Check if it's time to update
    if (current_time - handle->scroll.last_update < handle->scroll.scroll_delay) {
        return true;
    }
    
    handle->scroll.last_update = current_time;
    
    // Clear display
    MAX7219_Clear(handle, false);
    
    // Draw text at current offset
    if (handle->scroll.vertical) {
        // Vertical scroll (bottom to top)
#if (MAX7219_ENABLE_THAI_FULL)
        MAX7219_DrawStringThai(handle, 0, handle->scroll.offset,
                               handle->scroll.text, 8);
#else
        MAX7219_DrawString(handle, 0, handle->scroll.offset, handle->scroll.text);
#endif
    } else {
        // Horizontal scroll (right to left)
        MAX7219_DrawString(handle, handle->scroll.offset, 0, handle->scroll.text);
    }
    
    // Update display
    MAX7219_Update(handle);
    
    // Move offset
    if (handle->scroll.vertical) {
        handle->scroll.offset--;  // move up
        
        // Check if scrolling is complete
        if (handle->scroll.offset < -8) {
            handle->scroll.active = false;
            return false;
        }
    } else {
        handle->scroll.offset--;  // move left
        
        // Check if scrolling is complete
        int16_t text_width = MAX7219_GetStringWidth(handle, handle->scroll.text);
        if (handle->scroll.offset < -text_width) {
            handle->scroll.active = false;
            return false;  // Scrolling complete
        }
    }
    
    return true;  // Still scrolling
}

void MAX7219_StopScroll(MAX7219_Handle* handle) {
    if (!handle) return;
    handle->scroll.active = false;
}

/* ========== Animation Functions ========== */

void MAX7219_StartAnimation(MAX7219_Handle* handle, const uint8_t** frames, uint8_t num_frames, 
                           uint16_t frame_delay, bool loop) {
    if (!handle || !frames || num_frames == 0) return;
    
    handle->animation.frames = frames;
    handle->animation.num_frames = num_frames;
    handle->animation.current_frame = 0;
    handle->animation.frame_delay = frame_delay;
    handle->animation.last_update = Get_CurrentMs();
    handle->animation.loop = loop;
    handle->animation.active = true;
}

bool MAX7219_UpdateAnimation(MAX7219_Handle* handle) {
    if (!handle || !handle->animation.active) return false;
    
    uint32_t current_time = Get_CurrentMs();
    
    // Check if it's time to update
    if (current_time - handle->animation.last_update < handle->animation.frame_delay) {
        return true;  // Still animating, but not time to update yet
    }
    
    handle->animation.last_update = current_time;
    
    // Display current frame
    const uint8_t* frame = handle->animation.frames[handle->animation.current_frame];
    MAX7219_DrawBitmap(handle, 0, 0, frame, 8, 8);
    MAX7219_Update(handle);
    
    // Move to next frame
    handle->animation.current_frame++;
    
    // Check if animation is complete
    if (handle->animation.current_frame >= handle->animation.num_frames) {
        if (handle->animation.loop) {
            handle->animation.current_frame = 0;  // Loop back to first frame
        } else {
            handle->animation.active = false;
            return false;  // Animation complete
        }
    }
    
    return true;  // Still animating
}

void MAX7219_StopAnimation(MAX7219_Handle* handle) {
    if (!handle) return;
    handle->animation.active = false;
}

/* ========== Sprite Functions ========== */

void MAX7219_DrawSprite(MAX7219_Handle* handle, int16_t x, int16_t y, const uint8_t* sprite, 
                       const uint8_t* mask, uint8_t width, uint8_t height) {
    if (!handle || !sprite) return;
    
    for (uint8_t row = 0; row < height; row++) {
        uint8_t sprite_data = sprite[row];
        uint8_t mask_data = mask ? mask[row] : 0xFF;  // Default: all pixels visible
        
        for (uint8_t col = 0; col < width; col++) {
            bool sprite_pixel = (sprite_data & (1 << (7 - col))) != 0;
            bool mask_pixel = (mask_data & (1 << (7 - col))) != 0;
            
            // Only draw if mask allows
            if (mask_pixel) {
                MAX7219_SetPixel(handle, x + col, y + row, sprite_pixel);
            }
        }
    }
}

/* ========== Utility Functions ========== */

void MAX7219_FadeIn(MAX7219_Handle* handle, uint16_t duration) {
    if (!handle) return;
    
    uint16_t steps = MAX7219_INTENSITY_MAX + 1;
    uint16_t delay_per_step = duration / steps;
    
    for (uint8_t i = 0; i <= MAX7219_INTENSITY_MAX; i++) {
        MAX7219_SetIntensity(handle, i);
        Delay_Ms(delay_per_step);
    }
}

void MAX7219_FadeOut(MAX7219_Handle* handle, uint16_t duration) {
    if (!handle) return;
    
    uint16_t steps = MAX7219_INTENSITY_MAX + 1;
    uint16_t delay_per_step = duration / steps;
    
    for (int8_t i = MAX7219_INTENSITY_MAX; i >= 0; i--) {
        MAX7219_SetIntensity(handle, i);
        Delay_Ms(delay_per_step);
    }
}

void MAX7219_Invert(MAX7219_Handle* handle) {
    if (!handle) return;
    
    // Invert all pixels in buffer
    for (uint8_t dev = 0; dev < handle->num_devices; dev++) {
        for (uint8_t row = 0; row < MAX7219_MATRIX_SIZE; row++) {
            handle->buffer[dev][row] = ~handle->buffer[dev][row];
        }
    }
}

/* ==================================================================
 *  Thai Text Functions (UTF-8)
 * ================================================================== */

#if (MAX7219_ENABLE_THAI_FULL)

/**
 * @brief แปลง UTF-8 3-byte → Unicode
 */
uint16_t m_utf8_to_unicode(const char* utf8_char) {
    if ((utf8_char[0] & 0xE0) == 0xE0) {
        return ((uint16_t)(utf8_char[0] & 0x0F) << 12)
             | ((uint16_t)(utf8_char[1] & 0x3F) << 6)
             | (uint16_t)(utf8_char[2] & 0x3F);
    }
    return (uint16_t)(uint8_t)utf8_char[0];
}

/**
 * @brief ค้นหา Thai 8x8 font index จาก Unicode
 */
int8_t m_get_thai_idx(uint16_t unicode) {
    uint8_t i;
    for (i = 0; i < sizeof(thai_con_map) / sizeof(M_ThaiCharMap); i++) {
        if (thai_con_map[i].unicode == unicode) return (int8_t)thai_con_map[i].index;
    }
    return -1;
}

uint8_t MAX7219_DrawCharThai(MAX7219_Handle* handle, int16_t x, int16_t y,
                             const char* thai_char, uint8_t intensity) {
    uint16_t unicode;
    int8_t font_idx;
    uint8_t col, row;
    const uint8_t* glyph = NULL;

    if (!handle || !thai_char) return 0;

    unicode = m_utf8_to_unicode(thai_char);

    // Thai digits
    if (unicode >= 0x0E50 && unicode <= 0x0E59) {
        glyph = &font_thai_numbers_8x8_data[(unicode - 0x0E50) * 8];
    } else {
        font_idx = m_get_thai_idx(unicode);
        if (font_idx < 0) return 0;
        glyph = &font_thai_8x8_data[font_idx * 8];
    }

    if (!glyph) return 0;

    for (col = 0; col < 8; col++) {
        uint8_t col_data = glyph[col];
        for (row = 0; row < 8; row++) {
            if (col_data & (1 << row)) {
                MAX7219_SetPixel(handle, x + col, y + row, true);
            }
        }
    }

    return 9;  // 8px width + 1px spacing
}

uint16_t MAX7219_DrawStringThai(MAX7219_Handle* handle, int16_t x, int16_t y,
                                const char* text, uint8_t intensity) {
    int16_t cx = x;
    uint16_t total = 0;

    if (!handle || !text) return 0;

    while (*text) {
        if ((*text & 0xE0) == 0xE0) {
            total += MAX7219_DrawCharThai(handle, cx, y, text, intensity);
            cx += 9;
            text += 3;
        } else {
            total += MAX7219_DrawChar(handle, cx, y, *text);
            cx += handle->font ? (handle->font->width + 1) : 6;
            text++;
        }
    }

    return total;
}

#endif /* MAX7219_ENABLE_THAI_FULL */

/* ==================================================================
 *  Effects — Wipe
 * ================================================================== */

#if (MAX7219_ENABLE_EFFECTS)

void MAX7219_WipeLeft(MAX7219_Handle* handle, uint16_t delay_ms) {
    uint8_t x, y, dev;
    if (!handle) return;

    for (dev = 0; dev < handle->num_devices; dev++) {
        for (x = 0; x < MAX7219_MATRIX_SIZE; x++) {
            for (y = 0; y < MAX7219_MATRIX_SIZE; y++) {
                handle->buffer[dev][y] |= (1 << (7 - x));
            }
            MAX7219_Update(handle);
            Delay_Ms(delay_ms);
        }
    }
}

void MAX7219_WipeRight(MAX7219_Handle* handle, uint16_t delay_ms) {
    uint8_t x, y, dev;
    if (!handle) return;

    for (dev = 0; dev < handle->num_devices; dev++) {
        for (x = 0; x < MAX7219_MATRIX_SIZE; x++) {
            for (y = 0; y < MAX7219_MATRIX_SIZE; y++) {
                handle->buffer[dev][y] |= (1 << x);
            }
            MAX7219_Update(handle);
            Delay_Ms(delay_ms);
        }
    }
}

void MAX7219_WipeUp(MAX7219_Handle* handle, uint16_t delay_ms) {
    uint8_t x, y, dev;
    if (!handle) return;

    for (y = 0; y < MAX7219_MATRIX_SIZE; y++) {
        for (dev = 0; dev < handle->num_devices; dev++) {
            for (x = 0; x < MAX7219_MATRIX_SIZE; x++) {
                handle->buffer[dev][y] |= (1 << (7 - x));
            }
        }
        MAX7219_Update(handle);
        Delay_Ms(delay_ms);
    }
}

void MAX7219_WipeDown(MAX7219_Handle* handle, uint16_t delay_ms) {
    int16_t y;
    uint8_t x, dev;
    if (!handle) return;

    for (y = MAX7219_MATRIX_SIZE - 1; y >= 0; y--) {
        for (dev = 0; dev < handle->num_devices; dev++) {
            for (x = 0; x < MAX7219_MATRIX_SIZE; x++) {
                handle->buffer[dev][(uint8_t)y] |= (1 << (7 - x));
            }
        }
        MAX7219_Update(handle);
        Delay_Ms(delay_ms);
    }
}

void MAX7219_Blink(MAX7219_Handle* handle, uint16_t count,
                   uint16_t on_ms, uint16_t off_ms) {
    uint16_t i;
    if (!handle) return;

    for (i = 0; i < count; i++) {
        MAX7219_DisplayControl(handle, true);
        MAX7219_Update(handle);
        Delay_Ms(on_ms);
        MAX7219_DisplayControl(handle, false);
        Delay_Ms(off_ms);
    }
    MAX7219_DisplayControl(handle, true);
}

void MAX7219_Sparkle(MAX7219_Handle* handle, uint16_t duration_ms,
                     uint8_t density) {
    uint32_t start, now;
    uint8_t dev, x, y;
    uint16_t total_pixels, num_sparkle, s;

    if (!handle) return;
    if (density > 100) density = 100;

    total_pixels = (uint16_t)handle->num_devices * MAX7219_MATRIX_SIZE * MAX7219_MATRIX_SIZE;
    num_sparkle = (total_pixels * density) / 100;

    start = Get_CurrentMs();
    do {
        // Clear random pixels
        for (s = 0; s < num_sparkle; s++) {
            dev = rand() % handle->num_devices;
            x = rand() % MAX7219_MATRIX_SIZE;
            y = rand() % MAX7219_MATRIX_SIZE;
            handle->buffer[dev][y] ^= (1 << (7 - x));
        }
        MAX7219_Update(handle);
        Delay_Ms(30);

        now = Get_CurrentMs();
    } while ((now - start) < duration_ms);

    // Restore — clear all
    MAX7219_Clear(handle, true);
}

void MAX7219_MarqueeBorder(MAX7219_Handle* handle, uint16_t speed_ms,
                           uint8_t rounds) {
    int16_t total_w;
    uint16_t step;
    uint8_t r;

    if (!handle) return;

    total_w = (int16_t)handle->num_devices * MAX7219_MATRIX_SIZE;
    // Perimeter = 2W + 2H - 4 (but for a line of modules, just top+bottom+left+right of combined)
    // Simplified: run a single pixel around the entire composite perimeter
    // Perimeter path: top (L→R) → right (T→B) → bottom (R→L) → left (B→T)
    int16_t perimeter = 2 * total_w + 2 * MAX7219_MATRIX_SIZE - 4;
    if (perimeter <= 0) return;

    r = rounds;
    while (rounds == 0 || r > 0) {
        for (step = 0; step < (uint16_t)perimeter; step++) {
            MAX7219_Clear(handle, false);

            int16_t px, py;
            if (step < total_w) {
                // Top edge
                px = step; py = 0;
            } else if (step < total_w + MAX7219_MATRIX_SIZE - 1) {
                // Right edge
                px = total_w - 1; py = step - total_w + 1;
            } else if (step < 2 * total_w + MAX7219_MATRIX_SIZE - 2) {
                // Bottom edge
                px = 2 * total_w + MAX7219_MATRIX_SIZE - 3 - step;
                py = MAX7219_MATRIX_SIZE - 1;
            } else {
                // Left edge
                px = 0;
                py = perimeter - step;
            }

            if (px >= 0 && py >= 0 && py < MAX7219_MATRIX_SIZE) {
                MAX7219_SetPixel(handle, (int16_t)px, (int16_t)py, true);
            }
            MAX7219_Update(handle);
            Delay_Ms(speed_ms);
        }
        if (rounds > 0) r--;
    }
}

void MAX7219_RainEffect(MAX7219_Handle* handle, uint16_t duration_ms,
                        uint16_t speed_ms) {
    int16_t total_w;
    uint8_t drops[64];  // max 64 columns
    uint8_t drop_y[64];
    uint8_t i;
    uint32_t start, last_update, now;

    if (!handle) return;

    total_w = (int16_t)handle->num_devices * MAX7219_MATRIX_SIZE;
    if (total_w > 64) total_w = 64;

    memset(drops, 0, sizeof(drops));
    memset(drop_y, 0, sizeof(drop_y));

    start = Get_CurrentMs();
    last_update = start;

    do {
        now = Get_CurrentMs();
        if ((now - last_update) < speed_ms) continue;
        last_update = now;

        // Shift all drops down
        for (i = 0; i < (uint8_t)total_w; i++) {
            if (drops[i]) {
                // Erase old position
                MAX7219_SetPixel(handle, (int16_t)i, (int16_t)drop_y[i], false);
                if (drop_y[i] > 0) {
                    // Erase trail
                    MAX7219_SetPixel(handle, (int16_t)i, (int16_t)(drop_y[i] - 1), false);
                }

                drop_y[i]++;

                if (drop_y[i] >= MAX7219_MATRIX_SIZE) {
                    drops[i] = 0;  // Drop reached bottom
                } else {
                    // Draw at new position with trail
                    MAX7219_SetPixel(handle, (int16_t)i, (int16_t)drop_y[i], true);
                    if (drop_y[i] > 0) {
                        MAX7219_SetPixel(handle, (int16_t)i, (int16_t)(drop_y[i] - 1), true);
                    }
                }
            }
        }

        // Occasionally create new drops
        if (rand() % 3 == 0) {
            uint8_t col = rand() % (uint8_t)total_w;
            if (!drops[col]) {
                drops[col] = 1;
                drop_y[col] = 0;
                MAX7219_SetPixel(handle, (int16_t)col, 0, true);
            }
        }

        MAX7219_Update(handle);
    } while ((Get_CurrentMs() - start) < duration_ms);

    MAX7219_Clear(handle, true);
}

void MAX7219_RunningLight(MAX7219_Handle* handle, uint16_t speed_ms) {
    int16_t total_w;
    int16_t perimeter;
    uint16_t step;
    uint32_t last_update, now;

    if (!handle) return;

    total_w = (int16_t)handle->num_devices * MAX7219_MATRIX_SIZE;
    perimeter = 2 * total_w + 2 * MAX7219_MATRIX_SIZE - 4;

    last_update = Get_CurrentMs();
    step = 0;

    while (1) {
        now = Get_CurrentMs();
        if ((now - last_update) < speed_ms) continue;
        last_update = now;

        MAX7219_Clear(handle, false);

        int16_t px, py;
        if (step < (uint16_t)total_w) {
            px = (int16_t)step; py = 0;
        } else if (step < (uint16_t)(total_w + MAX7219_MATRIX_SIZE - 1)) {
            px = total_w - 1; py = (int16_t)(step - total_w + 1);
        } else if (step < (uint16_t)(2 * total_w + MAX7219_MATRIX_SIZE - 2)) {
            px = 2 * total_w + MAX7219_MATRIX_SIZE - 3 - (int16_t)step;
            py = MAX7219_MATRIX_SIZE - 1;
        } else {
            px = 0;
            py = perimeter - (int16_t)step;
        }

        if (px >= 0 && py >= 0 && py < MAX7219_MATRIX_SIZE) {
            MAX7219_SetPixel(handle, px, py, true);
        }
        MAX7219_Update(handle);

        step++;
        if (step >= (uint16_t)perimeter) step = 0;
    }
}

#endif /* MAX7219_ENABLE_EFFECTS */

/* ==================================================================
 *  Vertical Scrolling
 * ================================================================== */

void MAX7219_StartScrollTextVertical(MAX7219_Handle* handle, const char* text,
                                     uint16_t scroll_delay) {
    if (!handle || !text) return;

    handle->scroll.text = text;
    handle->scroll.offset = MAX7219_MATRIX_SIZE;  // Start from bottom
    handle->scroll.scroll_delay = scroll_delay;
    handle->scroll.last_update = Get_CurrentMs();
    handle->scroll.active = true;
    handle->scroll.vertical = true;
    handle->scroll.font = handle->font;
}

/* ==================================================================
 *  Buffer Utilities
 * ================================================================== */

void MAX7219_ShiftLeft(MAX7219_Handle* handle, uint8_t pixels) {
    uint8_t dev, row, p;
    if (!handle) return;

    for (p = 0; p < pixels; p++) {
        for (dev = 0; dev < handle->num_devices; dev++) {
            for (row = 0; row < MAX7219_MATRIX_SIZE; row++) {
                handle->buffer[dev][row] <<= 1;
                // Carry bit from next device
                if (dev + 1 < handle->num_devices) {
                    if (handle->buffer[dev + 1][row] & 0x80) {
                        handle->buffer[dev][row] |= 0x01;
                    }
                }
            }
        }
    }
}

void MAX7219_ShiftRight(MAX7219_Handle* handle, uint8_t pixels) {
    int16_t dev;
    uint8_t row, p;
    if (!handle) return;

    for (p = 0; p < pixels; p++) {
        for (dev = (int16_t)handle->num_devices - 1; dev >= 0; dev--) {
            for (row = 0; row < MAX7219_MATRIX_SIZE; row++) {
                handle->buffer[dev][row] >>= 1;
                // Carry bit from previous device
                if (dev > 0) {
                    if (handle->buffer[dev - 1][row] & 0x01) {
                        handle->buffer[dev][row] |= 0x80;
                    }
                }
            }
        }
    }
}

void MAX7219_ShiftUp(MAX7219_Handle* handle, uint8_t pixels) {
    uint8_t dev, row, p;
    if (!handle) return;

    for (p = 0; p < pixels; p++) {
        for (dev = 0; dev < handle->num_devices; dev++) {
            for (row = 0; row < MAX7219_MATRIX_SIZE - 1; row++) {
                handle->buffer[dev][row] = handle->buffer[dev][row + 1];
            }
            handle->buffer[dev][MAX7219_MATRIX_SIZE - 1] = 0;
        }
    }
}

void MAX7219_ShiftDown(MAX7219_Handle* handle, uint8_t pixels) {
    int16_t row;
    uint8_t dev, p;
    if (!handle) return;

    for (p = 0; p < pixels; p++) {
        for (dev = 0; dev < handle->num_devices; dev++) {
            for (row = MAX7219_MATRIX_SIZE - 1; row > 0; row--) {
                handle->buffer[dev][row] = handle->buffer[dev][row - 1];
            }
            handle->buffer[dev][0] = 0;
        }
    }
}

// Internal timer data for ScrollBuffer
static uint32_t _scroll_last = 0;
static uint8_t  _scroll_dir = 0;  // 0=left, 1=right

void MAX7219_ScrollBufferLeft(MAX7219_Handle* handle, uint16_t speed_ms) {
    uint32_t now;
    if (!handle) return;

    now = Get_CurrentMs();
    if ((now - _scroll_last) < speed_ms) return;
    _scroll_last = now;

    _scroll_dir = 0;
    MAX7219_ShiftLeft(handle, 1);
    MAX7219_Update(handle);
}

void MAX7219_ScrollBufferRight(MAX7219_Handle* handle, uint16_t speed_ms) {
    uint32_t now;
    if (!handle) return;

    now = Get_CurrentMs();
    if ((now - _scroll_last) < speed_ms) return;
    _scroll_last = now;

    _scroll_dir = 1;
    MAX7219_ShiftRight(handle, 1);
    MAX7219_Update(handle);
}

void MAX7219_ProgressBar(MAX7219_Handle* handle, uint8_t percent,
                         bool vertical) {
    int16_t total_w;
    uint16_t total_pixels, pixels_on, p;
    uint8_t dev, x, y;

    if (!handle) return;
    if (percent > 100) percent = 100;

    if (vertical) {
        total_w = MAX7219_MATRIX_SIZE;
        total_pixels = total_w;
        pixels_on = (total_pixels * percent) / 100;

        for (dev = 0; dev < handle->num_devices; dev++) {
            for (y = 0; y < MAX7219_MATRIX_SIZE; y++) {
                uint8_t row_val = 0;
                if ((uint8_t)(MAX7219_MATRIX_SIZE - 1 - y) < pixels_on) {
                    row_val = 0xFF;
                }
                handle->buffer[dev][y] = row_val;
            }
        }
    } else {
        total_w = (int16_t)handle->num_devices * MAX7219_MATRIX_SIZE;
        total_pixels = (uint16_t)total_w;
        pixels_on = (total_pixels * percent) / 100;

        for (p = 0; p < total_pixels; p++) {
            dev = p / MAX7219_MATRIX_SIZE;
            x = p % MAX7219_MATRIX_SIZE;

            for (y = 0; y < MAX7219_MATRIX_SIZE; y++) {
                if (p < pixels_on) {
                    handle->buffer[dev][y] |= (1 << (7 - x));
                } else {
                    handle->buffer[dev][y] &= ~(1 << (7 - x));
                }
            }
        }
    }
}
