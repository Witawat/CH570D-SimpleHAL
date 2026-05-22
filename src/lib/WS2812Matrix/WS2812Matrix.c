/**
 * @file WS2812Matrix.c
 * @brief WS2812 LED Matrix Library Implementation
 * @version 1.1
 * @date 2026-05-03
 */

#include "WS2812Matrix.h"
#include "WS2812M_Fonts.h"
#include "../../SimpleHAL/SimpleDelay.h"

/* ========== Private Variables ========== */

static WS2812M_Instance* _active_instance = NULL;

/* ========== Private Function Prototypes ========== */

/**
 * @brief แปลง SimpleGPIO pin number เป็น GPIO_TypeDef*
 * @param pin SimpleGPIO pin (PC4=14, PD2=20, ...)
 * @param[out] out_port ตัวชี้ไปยัง GPIO port ที่จะรับค่า
 * @param[out] out_pin_mask ตัวชี้ไปยัง pin mask ที่จะรับค่า
 * @return 1 = สำเร็จ, 0 = pin ไม่ถูกต้อง
 */
static uint8_t resolve_pin(uint8_t pin, GPIO_TypeDef** out_port,
                           uint16_t* out_pin_mask);

/* ========== Private Functions ========== */

static uint8_t resolve_pin(uint8_t pin, GPIO_TypeDef** out_port,
                           uint16_t* out_pin_mask) {
    // PA1–PA2
    if (pin == 0)  { *out_port = GPIOA; *out_pin_mask = GPIO_Pin_1; return 1; }
    if (pin == 1)  { *out_port = GPIOA; *out_pin_mask = GPIO_Pin_2; return 1; }
    // PC0–PC7
    if (pin >= 10 && pin <= 17) {
        *out_port = GPIOC;
        *out_pin_mask = (uint16_t)(1 << (pin - 10));
        return 1;
    }
    // PD2–PD7
    if (pin >= 20 && pin <= 25) {
        *out_port = GPIOD;
        *out_pin_mask = (uint16_t)(1 << (pin - 18));
        return 1;
    }
    return 0;  // invalid pin
}

/* ========== Public Functions ========== */

uint8_t WS2812M_Init(WS2812M_Instance* inst, uint8_t data_pin,
                     uint8_t width, uint8_t height, WS2812M_Wiring wiring) {
    GPIO_TypeDef* port = NULL;
    uint16_t pin_mask = 0;

    // === Null check ===
    if (inst == NULL) return 0;

    // === Validate params ===
    if (width  == 0 || width  > WS2812M_MAX_WIDTH)  return 0;
    if (height == 0 || height > WS2812M_MAX_HEIGHT) return 0;
    if (!resolve_pin(data_pin, &port, &pin_mask))   return 0;

    // === Store config ===
    inst->data_pin   = data_pin;
    inst->width      = width;
    inst->height     = height;
    inst->num_pixels = (uint16_t)width * height;
    inst->wiring     = wiring;

    // === Init NeoPixel (low-level driver) ===
    NeoPixel_Init(port, pin_mask, inst->num_pixels);
    NeoPixel_Clear();
    NeoPixel_Show();

    // === Register active instance ===
    _active_instance = inst;

    // === Mark initialized ===
    inst->initialized = 1;

    return 1;
}

void WS2812M_SetPixel(WS2812M_Instance* inst, uint8_t x, uint8_t y,
                      uint8_t r, uint8_t g, uint8_t b) {
    uint16_t index;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (x >= inst->width || y >= inst->height) return;

    index = WS2812M_XYtoIndex(x, y, inst->width, inst->wiring);
    NeoPixel_SetPixelColor(index, r, g, b);
}

void WS2812M_SetPixelColor(WS2812M_Instance* inst, uint8_t x, uint8_t y,
                           uint32_t color) {
    uint16_t index;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (x >= inst->width || y >= inst->height) return;

    index = WS2812M_XYtoIndex(x, y, inst->width, inst->wiring);
    NeoPixel_SetPixelColor32(index, color);
}

uint32_t WS2812M_GetPixel(WS2812M_Instance* inst, uint8_t x, uint8_t y) {
    uint16_t index;

    if (inst == NULL) return 0;
    if (!inst->initialized) return 0;
    if (x >= inst->width || y >= inst->height) return 0;

    index = WS2812M_XYtoIndex(x, y, inst->width, inst->wiring);
    return NeoPixel_GetPixelColor(index);
}

void WS2812M_Clear(WS2812M_Instance* inst) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    NeoPixel_Clear();
}

void WS2812M_Fill(WS2812M_Instance* inst, uint8_t r, uint8_t g, uint8_t b) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    NeoPixel_Fill(r, g, b);
}

void WS2812M_Show(WS2812M_Instance* inst) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    NeoPixel_Show();
}

void WS2812M_SetBrightness(WS2812M_Instance* inst, uint8_t brightness) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    NeoPixel_SetBrightness(brightness);
}

void WS2812M_Deinit(WS2812M_Instance* inst) {
    if (inst == NULL) return;
    if (!inst->initialized) return;

    _active_instance = NULL;
    inst->initialized = 0;
}

/* ========== XY-to-Index Mapping ========== */

uint16_t WS2812M_XYtoIndex(uint8_t x, uint8_t y, uint8_t width,
                           WS2812M_Wiring wiring) {
    if (wiring == WIRING_ZIGZAG) {
        // Zigzag: even rows → left-to-right, odd rows → right-to-left
        if (y & 1) {
            return (uint16_t)(y * width) + (uint16_t)(width - 1 - x);
        } else {
            return (uint16_t)(y * width) + x;
        }
    } else {
        // Snake: all rows left-to-right
        return (uint16_t)(y * width) + x;
    }
}

/* ========== Drawing Primitives ========== */

void WS2812M_DrawLine(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b) {
    int16_t dx, dy, sx, sy, err, e2;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    // Bresenham's line algorithm
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;
    err = dx - dy;

    while (1) {
        if (x0 >= 0 && y0 >= 0 &&
            x0 < (int16_t)inst->width && y0 < (int16_t)inst->height) {
            WS2812M_SetPixel(inst, (uint8_t)x0, (uint8_t)y0, r, g, b);
        }

        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void WS2812M_DrawLineColor(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    WS2812M_DrawLine(inst, x0, y0, x1, y1, r, g, b);
}

void WS2812M_DrawRect(WS2812M_Instance* inst, int16_t x, int16_t y,
                      uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b) {
    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (w == 0 || h == 0) return;

    WS2812M_DrawLine(inst, x, y, x + w - 1, y, r, g, b);                  // top
    WS2812M_DrawLine(inst, x, y + h - 1, x + w - 1, y + h - 1, r, g, b); // bottom
    WS2812M_DrawLine(inst, x, y, x, y + h - 1, r, g, b);                  // left
    WS2812M_DrawLine(inst, x + w - 1, y, x + w - 1, y + h - 1, r, g, b); // right
}

void WS2812M_FillRect(WS2812M_Instance* inst, int16_t x, int16_t y,
                      uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b) {
    int16_t i, j;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (w == 0 || h == 0) return;

    for (j = y; j < y + (int16_t)h; j++) {
        for (i = x; i < x + (int16_t)w; i++) {
            if (i >= 0 && j >= 0 &&
                i < (int16_t)inst->width && j < (int16_t)inst->height) {
                WS2812M_SetPixel(inst, (uint8_t)i, (uint8_t)j, r, g, b);
            }
        }
    }
}

void WS2812M_DrawCircle(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                        uint8_t radius, uint8_t r, uint8_t g, uint8_t b) {
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    while (x >= y) {
        WS2812M_SetPixel(inst, x0 + x, y0 + y, r, g, b);
        WS2812M_SetPixel(inst, x0 + y, y0 + x, r, g, b);
        WS2812M_SetPixel(inst, x0 - y, y0 + x, r, g, b);
        WS2812M_SetPixel(inst, x0 - x, y0 + y, r, g, b);
        WS2812M_SetPixel(inst, x0 - x, y0 - y, r, g, b);
        WS2812M_SetPixel(inst, x0 - y, y0 - x, r, g, b);
        WS2812M_SetPixel(inst, x0 + y, y0 - x, r, g, b);
        WS2812M_SetPixel(inst, x0 + x, y0 - y, r, g, b);

        if (err <= 0) { y += 1; err += 2 * y + 1; }
        if (err > 0)  { x -= 1; err -= 2 * x + 1; }
    }
}

void WS2812M_FillCircle(WS2812M_Instance* inst, int16_t x0, int16_t y0,
                        uint8_t radius, uint8_t r, uint8_t g, uint8_t b) {
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    while (x >= y) {
        WS2812M_DrawLine(inst, x0 - x, y0 + y, x0 + x, y0 + y, r, g, b);
        WS2812M_DrawLine(inst, x0 - y, y0 + x, x0 + y, y0 + x, r, g, b);
        WS2812M_DrawLine(inst, x0 - x, y0 - y, x0 + x, y0 - y, r, g, b);
        WS2812M_DrawLine(inst, x0 - y, y0 - x, x0 + y, y0 - x, r, g, b);

        if (err <= 0) { y += 1; err += 2 * y + 1; }
        if (err > 0)  { x -= 1; err -= 2 * x + 1; }
    }
}

/* ==================================================================
 *  Text Rendering
 * ================================================================== */

uint8_t WS2812M_DrawChar(WS2812M_Instance* inst, int16_t x, int16_t y,
                         char c, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t col, row;
    uint8_t line;

    if (inst == NULL) return 0;
    if (!inst->initialized) return 0;
    if (c < 32 || c > 126) return 0;

    const uint8_t* glyph = font_5x7[c - 32];

    for (col = 0; col < 5; col++) {
        line = glyph[col];
        for (row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                WS2812M_SetPixel(inst, (uint8_t)(x + col), (uint8_t)(y + row),
                                 r, g, b);
            }
        }
    }

    return 6;  // 5px width + 1px spacing
}

uint8_t WS2812M_DrawCharColor(WS2812M_Instance* inst, int16_t x, int16_t y,
                              char c, uint32_t color) {
    uint8_t rr = (color >> 16) & 0xFF;
    uint8_t gg = (color >> 8) & 0xFF;
    uint8_t bb = color & 0xFF;
    return WS2812M_DrawChar(inst, x, y, c, rr, gg, bb);
}

uint16_t WS2812M_DrawText(WS2812M_Instance* inst, int16_t x, int16_t y,
                          const char* text, uint8_t r, uint8_t g, uint8_t b) {
    int16_t cursor_x = x;
    uint16_t total = 0;

    if (inst == NULL) return 0;
    if (!inst->initialized) return 0;
    if (text == NULL) return 0;

    while (*text) {
        total += WS2812M_DrawChar(inst, cursor_x, y, *text, r, g, b);
        cursor_x += 6;
        text++;
    }

    return total;
}

uint16_t WS2812M_DrawTextColor(WS2812M_Instance* inst, int16_t x, int16_t y,
                               const char* text, uint32_t color) {
    uint8_t rr = (color >> 16) & 0xFF;
    uint8_t gg = (color >> 8) & 0xFF;
    uint8_t bb = color & 0xFF;
    return WS2812M_DrawText(inst, x, y, text, rr, gg, bb);
}

#if (WS2812M_ENABLE_THAI)

/**
 * @brief แปลง UTF-8 3-byte sequence เป็น Unicode code point
 */
static uint16_t utf8_to_unicode(const char* utf8_char) {
    if ((utf8_char[0] & 0xE0) == 0xE0) {
        // 3-byte UTF-8
        return ((uint16_t)(utf8_char[0] & 0x0F) << 12)
             | ((uint16_t)(utf8_char[1] & 0x3F) << 6)
             | (uint16_t)(utf8_char[2] & 0x3F);
    }
    return (uint16_t)(uint8_t)utf8_char[0];
}

/**
 * @brief ค้นหา index ของฟอนต์ไทยจาก Unicode code point
 */
static int8_t get_thai_font_index(uint16_t unicode) {
    uint8_t i;

    // ค้นหาใน thai_consonant_map
    for (i = 0; i < sizeof(thai_consonant_map) / sizeof(ThaiCharMap_t); i++) {
        if (thai_consonant_map[i].unicode == unicode) {
            return (int8_t)thai_consonant_map[i].index;
        }
    }

    return -1;  // ไม่พบ
}

uint8_t WS2812M_DrawCharThai(WS2812M_Instance* inst, int16_t x, int16_t y,
                             const char* thai_char, uint32_t color) {
    uint16_t unicode;
    int8_t font_index;
    uint8_t col, row;
    const uint8_t* glyph = NULL;
    uint8_t rr, gg, bb;

    if (inst == NULL) return 0;
    if (!inst->initialized) return 0;
    if (thai_char == NULL) return 0;

    unicode = utf8_to_unicode(thai_char);

    rr = (color >> 16) & 0xFF;
    gg = (color >> 8) & 0xFF;
    bb = color & 0xFF;

    // ตรวจสอบว่าเป็นเลขไทยหรือไม่
    if (unicode >= 0x0E50 && unicode <= 0x0E59) {
        glyph = font_thai_numbers_8x8[unicode - 0x0E50];
    } else {
        font_index = get_thai_font_index(unicode);
        if (font_index < 0) return 0;  // ไม่พบฟอนต์
        glyph = font_thai_8x8[font_index];
    }

    if (glyph == NULL) return 0;

    for (col = 0; col < 8; col++) {
        for (row = 0; row < 8; row++) {
            if (glyph[col] & (1 << row)) {
                WS2812M_SetPixel(inst, (uint8_t)(x + col), (uint8_t)(y + row),
                                 rr, gg, bb);
            }
        }
    }

    return 8;  // 8px width
}

uint16_t WS2812M_DrawTextThai(WS2812M_Instance* inst, int16_t x, int16_t y,
                              const char* text, uint32_t color) {
    int16_t cursor_x = x;
    uint16_t total = 0;

    if (inst == NULL) return 0;
    if (!inst->initialized) return 0;
    if (text == NULL) return 0;

    while (*text) {
        if ((*text & 0xE0) == 0xE0) {
            // UTF-8 3-byte (Thai)
            total += WS2812M_DrawCharThai(inst, cursor_x, y, text, color);
            cursor_x += 8;
            text += 3;
        } else {
            // ASCII
            uint8_t rr = (color >> 16) & 0xFF;
            uint8_t gg = (color >> 8) & 0xFF;
            uint8_t bb = color & 0xFF;
            total += WS2812M_DrawChar(inst, cursor_x, y, *text, rr, gg, bb);
            cursor_x += 6;
            text++;
        }
    }

    return total;
}

#endif /* WS2812M_ENABLE_THAI */

uint16_t WS2812M_GetTextWidth(const char* text) {
    uint16_t width = 0;

    if (text == NULL) return 0;

    while (*text) {
        if ((*text & 0xE0) == 0xE0) {
            width += 8;   // Thai char
            text += 3;
        } else {
            width += 6;   // ASCII char
            text++;
        }
    }

    return width;
}

/* ==================================================================
 *  Scrolling Text
 * ================================================================== */

void WS2812M_ScrollTextInit(WS2812M_ScrollText* scroll, const char* text,
                            uint32_t color, uint16_t speed, bool vertical) {
    if (scroll == NULL) return;
    if (text == NULL) return;

    strncpy(scroll->text, text, sizeof(scroll->text) - 1);
    scroll->text[sizeof(scroll->text) - 1] = '\0';
    scroll->color = color;
    scroll->speed = speed;
    scroll->vertical = vertical;
    scroll->active = true;
    scroll->last_update = Get_CurrentMs();
    scroll->position = 0;
}

bool WS2812M_ScrollTextUpdate(WS2812M_Instance* inst,
                              WS2812M_ScrollText* scroll, int16_t y) {
    uint32_t now;

    if (inst == NULL) return false;
    if (scroll == NULL) return false;
    if (!scroll->active) return false;

    now = Get_CurrentMs();
    if ((now - scroll->last_update) < scroll->speed) return false;

    scroll->last_update = now;

    if (scroll->vertical) {
        WS2812M_Clear(inst);
#if (WS2812M_ENABLE_THAI)
        WS2812M_DrawTextThai(inst, 0, scroll->position, scroll->text,
                             scroll->color);
#else
        uint8_t rr = (scroll->color >> 16) & 0xFF;
        uint8_t gg = (scroll->color >> 8) & 0xFF;
        uint8_t bb = scroll->color & 0xFF;
        WS2812M_DrawText(inst, 0, scroll->position, scroll->text, rr, gg, bb);
#endif
        WS2812M_Show(inst);
        scroll->position--;
        if (scroll->position < -8) {
            scroll->position = (int16_t)inst->height;
        }
    } else {
        WS2812M_Clear(inst);
#if (WS2812M_ENABLE_THAI)
        WS2812M_DrawTextThai(inst, scroll->position, y, scroll->text,
                             scroll->color);
#else
        uint8_t rr = (scroll->color >> 16) & 0xFF;
        uint8_t gg = (scroll->color >> 8) & 0xFF;
        uint8_t bb = scroll->color & 0xFF;
        WS2812M_DrawText(inst, scroll->position, y, scroll->text, rr, gg, bb);
#endif
        WS2812M_Show(inst);
        scroll->position--;

        {
            uint16_t tw = WS2812M_GetTextWidth(scroll->text);
            if (scroll->position < -(int16_t)tw) {
                scroll->position = (int16_t)inst->width;
            }
        }
    }

    return true;
}

void WS2812M_ScrollTextStop(WS2812M_ScrollText* scroll) {
    if (scroll == NULL) return;
    scroll->active = false;
}

/* ==================================================================
 *  Sprite / Bitmap
 * ================================================================== */

#if (WS2812M_ENABLE_SPRITES)

void WS2812M_DrawSprite(WS2812M_Instance* inst, int16_t x, int16_t y,
                        const WS2812M_Sprite* sprite) {
    uint8_t j, i;
    uint32_t color;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (sprite == NULL) return;

    for (j = 0; j < sprite->height; j++) {
        for (i = 0; i < sprite->width; i++) {
            color = sprite->data[j * sprite->width + i];

            if (sprite->has_transparency && color == sprite->transparent_color) {
                continue;
            }

            WS2812M_SetPixelColor(inst, (uint8_t)(x + i), (uint8_t)(y + j),
                                  color);
        }
    }
}

void WS2812M_DrawBitmap(WS2812M_Instance* inst, int16_t x, int16_t y,
                        const uint8_t* bitmap, uint8_t w, uint8_t h,
                        uint32_t color) {
    uint8_t j, i;
    uint8_t line;
    uint8_t rr, gg, bb;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (bitmap == NULL) return;

    rr = (color >> 16) & 0xFF;
    gg = (color >> 8) & 0xFF;
    bb = color & 0xFF;

    for (j = 0; j < h; j++) {
        line = bitmap[j];
        for (i = 0; i < w; i++) {
            if (line & (0x80 >> i)) {
                WS2812M_SetPixel(inst, (uint8_t)(x + i), (uint8_t)(y + j),
                                 rr, gg, bb);
            }
        }
    }
}

#endif /* WS2812M_ENABLE_SPRITES */

/* ==================================================================
 *  Effects (blocking — ใช้ Delay_Ms ภายใน)
 * ================================================================== */

#if (WS2812M_ENABLE_EFFECTS)

void WS2812M_FadeIn(WS2812M_Instance* inst, uint16_t duration_ms,
                    uint8_t steps) {
    uint8_t i;
    uint16_t step_delay;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (steps == 0) return;

    step_delay = duration_ms / steps;

    for (i = 0; i <= steps; i++) {
        uint8_t brightness = (uint8_t)((255u * i) / steps);
        WS2812M_SetBrightness(inst, brightness);
        WS2812M_Show(inst);
        Delay_Ms(step_delay);
    }
}

void WS2812M_FadeOut(WS2812M_Instance* inst, uint16_t duration_ms,
                     uint8_t steps) {
    uint8_t i;
    uint16_t step_delay;

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (steps == 0) return;

    step_delay = duration_ms / steps;

    for (i = steps; i > 0; i--) {
        uint8_t brightness = (uint8_t)((255u * i) / steps);
        WS2812M_SetBrightness(inst, brightness);
        WS2812M_Show(inst);
        Delay_Ms(step_delay);
    }

    WS2812M_SetBrightness(inst, 0);
    WS2812M_Show(inst);
}

void WS2812M_WipeTransition(WS2812M_Instance* inst, uint32_t color,
                            uint16_t delay_ms) {
    uint8_t x, y;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    for (x = 0; x < inst->width; x++) {
        for (y = 0; y < inst->height; y++) {
            WS2812M_SetPixelColor(inst, x, y, color);
        }
        WS2812M_Show(inst);
        Delay_Ms(delay_ms);
    }
}

void WS2812M_SlideTransition(WS2812M_Instance* inst, uint32_t color,
                             uint16_t delay_ms) {
    int16_t y;
    uint8_t x;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    for (y = (int16_t)inst->height - 1; y >= 0; y--) {
        for (x = 0; x < inst->width; x++) {
            WS2812M_SetPixelColor(inst, x, (uint8_t)y, color);
        }
        WS2812M_Show(inst);
        Delay_Ms(delay_ms);
    }
}

#endif /* WS2812M_ENABLE_EFFECTS */

/* ==================================================================
 *  Buffer Utilities
 * ================================================================== */

void WS2812M_Rotate90CW(WS2812M_Instance* inst) {
    uint8_t n;
    int16_t x, y;
    uint32_t temp_buf[64];  // stack buffer for rotation (max 8×8)

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (inst->width != inst->height) return;  // square only

    n = inst->width;
    if ((uint16_t)n * n > 64) return;  // too large for stack buffer

    // Copy to temp
    for (y = 0; y < (int16_t)n; y++) {
        for (x = 0; x < (int16_t)n; x++) {
            temp_buf[y * n + x] = WS2812M_GetPixel(inst, (uint8_t)x, (uint8_t)y);
        }
    }

    // Write rotated
    for (y = 0; y < (int16_t)n; y++) {
        for (x = 0; x < (int16_t)n; x++) {
            uint32_t c = temp_buf[(uint16_t)(n - 1 - y) * n + x];
            WS2812M_SetPixelColor(inst, (uint8_t)y, (uint8_t)x, c);
        }
    }
}

void WS2812M_Rotate90CCW(WS2812M_Instance* inst) {
    uint8_t n;
    int16_t x, y;
    uint32_t temp_buf[64];

    if (inst == NULL) return;
    if (!inst->initialized) return;
    if (inst->width != inst->height) return;

    n = inst->width;
    if ((uint16_t)n * n > 64) return;

    for (y = 0; y < (int16_t)n; y++) {
        for (x = 0; x < (int16_t)n; x++) {
            temp_buf[y * n + x] = WS2812M_GetPixel(inst, (uint8_t)x, (uint8_t)y);
        }
    }

    for (y = 0; y < (int16_t)n; y++) {
        for (x = 0; x < (int16_t)n; x++) {
            uint32_t c = temp_buf[y * n + (uint16_t)(n - 1 - x)];
            WS2812M_SetPixelColor(inst, (uint8_t)y, (uint8_t)x, c);
        }
    }
}

void WS2812M_FlipHorizontal(WS2812M_Instance* inst) {
    uint8_t x, y;
    uint32_t temp;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    for (y = 0; y < inst->height; y++) {
        for (x = 0; x < inst->width / 2; x++) {
            uint8_t rx = inst->width - 1 - x;
            temp = WS2812M_GetPixel(inst, x, y);
            WS2812M_SetPixelColor(inst, x, y,
                                  WS2812M_GetPixel(inst, rx, y));
            WS2812M_SetPixelColor(inst, rx, y, temp);
        }
    }
}

void WS2812M_FlipVertical(WS2812M_Instance* inst) {
    uint8_t x, y;
    uint32_t temp;

    if (inst == NULL) return;
    if (!inst->initialized) return;

    for (x = 0; x < inst->width; x++) {
        for (y = 0; y < inst->height / 2; y++) {
            uint8_t ry = inst->height - 1 - y;
            temp = WS2812M_GetPixel(inst, x, y);
            WS2812M_SetPixelColor(inst, x, y,
                                  WS2812M_GetPixel(inst, x, ry));
            WS2812M_SetPixelColor(inst, x, ry, temp);
        }
    }
}
