/**
 * @file oled_fonts.c
 * @brief OLED Font Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "oled_fonts.h"
#include "oled_graphics.h"
#include "../WS2815Matrix/fonts.h"
#include <stdio.h>

/* ========== Font Data (6x8) ========== */

// Font 6x8 - Basic ASCII (32-126)
static const uint8_t font_6x8_data[] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x00, 0x00, 0x5F, 0x00, 0x00, 0x00,
    // " (34)
    0x00, 0x07, 0x00, 0x07, 0x00, 0x00,
    // # (35)
    0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00,
    // $ (36)
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00,
    // % (37)
    0x23, 0x13, 0x08, 0x64, 0x62, 0x00,
    // & (38)
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00,
    // ' (39)
    0x00, 0x05, 0x03, 0x00, 0x00, 0x00,
    // ( (40)
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00,
    // ) (41)
    0x00, 0x41, 0x22, 0x1C, 0x00, 0x00,
    // * (42)
    0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00,
    // + (43)
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00,
    // , (44)
    0x00, 0x50, 0x30, 0x00, 0x00, 0x00,
    // - (45)
    0x08, 0x08, 0x08, 0x08, 0x08, 0x00,
    // . (46)
    0x00, 0x60, 0x60, 0x00, 0x00, 0x00,
    // / (47)
    0x20, 0x10, 0x08, 0x04, 0x02, 0x00,
    // 0-9 (48-57)
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00,  // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, 0x00,  // 1
    0x42, 0x61, 0x51, 0x49, 0x46, 0x00,  // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, 0x00,  // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x00,  // 4
    0x27, 0x45, 0x45, 0x45, 0x39, 0x00,  // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00,  // 6
    0x01, 0x71, 0x09, 0x05, 0x03, 0x00,  // 7
    0x36, 0x49, 0x49, 0x49, 0x36, 0x00,  // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, 0x00,  // 9
    // : ; < = > ? @ (58-64)
    0x00, 0x36, 0x36, 0x00, 0x00, 0x00,
    0x00, 0x56, 0x36, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x14, 0x22, 0x41, 0x00,
    0x14, 0x14, 0x14, 0x14, 0x14, 0x00,
    0x41, 0x22, 0x14, 0x08, 0x00, 0x00,
    0x02, 0x01, 0x51, 0x09, 0x06, 0x00,
    0x32, 0x49, 0x79, 0x41, 0x3E, 0x00,
    // A-Z (65-90)
    0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00,  // A
    0x7F, 0x49, 0x49, 0x49, 0x36, 0x00,  // B
    0x3E, 0x41, 0x41, 0x41, 0x22, 0x00,  // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00,  // D
    0x7F, 0x49, 0x49, 0x49, 0x41, 0x00,  // E
    0x7F, 0x09, 0x09, 0x01, 0x01, 0x00,  // F
    0x3E, 0x41, 0x41, 0x51, 0x32, 0x00,  // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00,  // H
    0x00, 0x41, 0x7F, 0x41, 0x00, 0x00,  // I
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x00,  // J
    0x7F, 0x08, 0x14, 0x22, 0x41, 0x00,  // K
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x00,  // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00,  // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00,  // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00,  // O
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x00,  // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00,  // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, 0x00,  // R
    0x46, 0x49, 0x49, 0x49, 0x31, 0x00,  // S
    0x01, 0x01, 0x7F, 0x01, 0x01, 0x00,  // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00,  // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00,  // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, 0x00,  // W
    0x63, 0x14, 0x08, 0x14, 0x63, 0x00,  // X
    0x03, 0x04, 0x78, 0x04, 0x03, 0x00,  // Y
    0x61, 0x51, 0x49, 0x45, 0x43, 0x00,  // Z
    // [ \ ] ^ _ ` (91-96)
    0x00, 0x00, 0x7F, 0x41, 0x41, 0x00,
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00,
    0x41, 0x41, 0x7F, 0x00, 0x00, 0x00,
    0x04, 0x02, 0x01, 0x02, 0x04, 0x00,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00,
    0x00, 0x01, 0x02, 0x04, 0x00, 0x00,
    // a-z (97-122)
    0x20, 0x54, 0x54, 0x54, 0x78, 0x00,  // a
    0x7F, 0x48, 0x44, 0x44, 0x38, 0x00,  // b
    0x38, 0x44, 0x44, 0x44, 0x20, 0x00,  // c
    0x38, 0x44, 0x44, 0x48, 0x7F, 0x00,  // d
    0x38, 0x54, 0x54, 0x54, 0x18, 0x00,  // e
    0x08, 0x7E, 0x09, 0x01, 0x02, 0x00,  // f
    0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00,  // g
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00,  // h
    0x00, 0x44, 0x7D, 0x40, 0x00, 0x00,  // i
    0x20, 0x40, 0x44, 0x3D, 0x00, 0x00,  // j
    0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,  // k
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x00,  // l
    0x7C, 0x04, 0x18, 0x04, 0x78, 0x00,  // m
    0x7C, 0x08, 0x04, 0x04, 0x78, 0x00,  // n
    0x38, 0x44, 0x44, 0x44, 0x38, 0x00,  // o
    0x7C, 0x14, 0x14, 0x14, 0x08, 0x00,  // p
    0x08, 0x14, 0x14, 0x18, 0x7C, 0x00,  // q
    0x7C, 0x08, 0x04, 0x04, 0x08, 0x00,  // r
    0x48, 0x54, 0x54, 0x54, 0x20, 0x00,  // s
    0x04, 0x3F, 0x44, 0x40, 0x20, 0x00,  // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00,  // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00,  // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00,  // w
    0x44, 0x28, 0x10, 0x28, 0x44, 0x00,  // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00,  // y
    0x44, 0x64, 0x54, 0x4C, 0x44, 0x00,  // z
    // { | } ~ (123-126)
    0x00, 0x08, 0x36, 0x41, 0x00, 0x00,
    0x00, 0x00, 0x7F, 0x00, 0x00, 0x00,
    0x00, 0x41, 0x36, 0x08, 0x00, 0x00,
    0x08, 0x04, 0x08, 0x10, 0x08, 0x00,
};

const OLED_Font Font_6x8 = {
    .width = 6,
    .height = 8,
    .first_char = 32,
    .last_char = 126,
    .data = font_6x8_data
};

/* Note: Font_8x16 and Font_12x16 would be defined similarly but with larger bitmaps */
/* For brevity, we'll create placeholder fonts that reference the 6x8 data */

const OLED_Font Font_8x16 = {
    .width = 8,
    .height = 16,
    .first_char = 32,
    .last_char = 126,
    .data = font_6x8_data  // Placeholder - would be actual 8x16 data
};

const OLED_Font Font_12x16 = {
    .width = 12,
    .height = 16,
    .first_char = 32,
    .last_char = 126,
    .data = font_6x8_data  // Placeholder - would be actual 12x16 data
};

/* ========== Global Variables ========== */

static const OLED_Font* current_font = &Font_6x8;
static uint8_t current_text_scale = 1;

typedef struct {
    uint16_t unicode;
    const uint8_t* glyph;
} OLED_ThaiGlyphMap;

/* Standalone Thai symbols/vowels that are not covered by WS2815 consonant table. */
static const uint8_t thai_glyph_sara_a[8]      = {0x00, 0x1C, 0x22, 0x02, 0x02, 0x22, 0x1C, 0x00}; /* ะ */
static const uint8_t thai_glyph_sara_aa[8]     = {0x00, 0x1C, 0x22, 0x22, 0x22, 0x3E, 0x20, 0x00}; /* า */
static const uint8_t thai_glyph_sara_am[8]     = {0x08, 0x00, 0x1C, 0x22, 0x22, 0x3E, 0x20, 0x00}; /* ำ */
static const uint8_t thai_glyph_sara_ii[8]     = {0x00, 0x3C, 0x24, 0x04, 0x04, 0x24, 0x3C, 0x00}; /* ี standalone fallback */
static const uint8_t thai_glyph_sara_uee[8]    = {0x00, 0x3C, 0x24, 0x24, 0x04, 0x24, 0x3C, 0x00}; /* ื standalone fallback */
static const uint8_t thai_glyph_sara_uu[8]     = {0x00, 0x3E, 0x22, 0x22, 0x02, 0x02, 0x02, 0x00}; /* ู standalone fallback */
static const uint8_t thai_glyph_sara_ue[8]     = {0x00, 0x3E, 0x22, 0x22, 0x02, 0x22, 0x3E, 0x00}; /* ึ standalone fallback */
static const uint8_t thai_glyph_maihanakat[8]  = {0x00, 0x08, 0x00, 0x3E, 0x22, 0x22, 0x3E, 0x00}; /* ั standalone fallback */
static const uint8_t thai_glyph_ru[8]          = {0x00, 0x1E, 0x12, 0x1E, 0x10, 0x10, 0x38, 0x00}; /* ฤ */
static const uint8_t thai_glyph_lu[8]          = {0x00, 0x3E, 0x22, 0x3E, 0x22, 0x22, 0x3E, 0x00}; /* ฦ */
static const uint8_t thai_glyph_lakkhangyao[8] = {0x00, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1C, 0x00}; /* ฯ */
static const uint8_t thai_glyph_maiyamok[8]    = {0x00, 0x22, 0x14, 0x08, 0x14, 0x22, 0x00, 0x00}; /* ๆ */
static const uint8_t thai_glyph_angkhankhu[8]  = {0x00, 0x36, 0x36, 0x00, 0x36, 0x36, 0x00, 0x00}; /* ๚ */
static const uint8_t thai_glyph_khomut[8]      = {0x00, 0x3C, 0x24, 0x5A, 0x5A, 0x24, 0x3C, 0x00}; /* ๛ */

static const OLED_ThaiGlyphMap thai_extra_glyphs[] = {
    {0x0E24, thai_glyph_ru},
    {0x0E26, thai_glyph_lu},
    {0x0E2F, thai_glyph_lakkhangyao},
    {0x0E30, thai_glyph_sara_a},
    {0x0E32, thai_glyph_sara_aa},
    {0x0E33, thai_glyph_sara_am},
    {0x0E31, thai_glyph_maihanakat},
    {0x0E35, thai_glyph_sara_ii},
    {0x0E37, thai_glyph_sara_uee},
    {0x0E39, thai_glyph_sara_uu},
    {0x0E3A, thai_glyph_sara_ue},
    {0x0E45, thai_glyph_sara_aa},
    {0x0E46, thai_glyph_maiyamok},
    {0x0E5A, thai_glyph_angkhankhu},
    {0x0E5B, thai_glyph_khomut}
};

/* ========== Private Helpers ========== */

static uint8_t _clamp_scale(uint8_t scale) {
    if(scale < 1) return 1;
    if(scale > 4) return 4;
    return scale;
}

static void _fill_scaled_pixel(OLED_Handle* oled, int16_t x, int16_t y, uint8_t scale, OLED_Color color) {
    for(uint8_t sy = 0; sy < scale; sy++) {
        for(uint8_t sx = 0; sx < scale; sx++) {
            OLED_SetPixel(oled, x + sx, y + sy, color);
        }
    }
}

/* Phase 1 uses 8-pixel tall glyph rows from current bitmap table. */
static uint8_t _glyph_height_from_table(void) {
    return (current_font->height > 8) ? 8 : current_font->height;
}

static uint8_t _is_thai_combining_upper(uint16_t unicode) {
    return (unicode == 0x0E31) ||
           (unicode >= 0x0E34 && unicode <= 0x0E37) ||
           (unicode >= 0x0E47 && unicode <= 0x0E4E);
}

static uint8_t _is_thai_combining_lower(uint16_t unicode) {
    return (unicode >= 0x0E38 && unicode <= 0x0E3A);
}

static uint8_t _is_thai_leading_vowel(uint16_t unicode) {
    return (unicode >= 0x0E40 && unicode <= 0x0E44);
}

static uint8_t _is_thai_base_char(uint16_t unicode) {
    if(!OLED_IsThaiChar(unicode)) {
        return 0;
    }

    if(_is_thai_combining_upper(unicode) || _is_thai_combining_lower(unicode)) {
        return 0;
    }

    return 1;
}

static uint8_t _thai_advance(uint8_t scale) {
    return (uint8_t)((current_font->width + 2) * scale);
}

static const uint8_t* _lookup_thai_glyph(uint16_t unicode) {
    if(unicode >= 0x0E50 && unicode <= 0x0E59) {
        return font_thai_numbers_8x8[unicode - 0x0E50];
    }

    for(uint8_t i = 0; i < THAI_CONSONANT_COUNT; i++) {
        if(thai_consonant_map[i].unicode == unicode) {
            return font_thai_8x8[thai_consonant_map[i].index];
        }
    }

    for(uint8_t i = 0; i < (sizeof(thai_extra_glyphs) / sizeof(thai_extra_glyphs[0])); i++) {
        if(thai_extra_glyphs[i].unicode == unicode) {
            return thai_extra_glyphs[i].glyph;
        }
    }

    return NULL;
}

static void _draw_bitmap8_scaled(OLED_Handle* oled, int16_t x, int16_t y, const uint8_t* glyph, OLED_Color color, uint8_t scale) {
    if(glyph == NULL) {
        return;
    }

    for(uint8_t col = 0; col < 8; col++) {
        uint8_t line = glyph[col];
        for(uint8_t row = 0; row < 8; row++) {
            if(line & (1u << row)) {
                _fill_scaled_pixel(oled,
                                   x + (int16_t)(col * scale),
                                   y + (int16_t)(row * scale),
                                   scale,
                                   color);
            }
        }
    }
}

static uint8_t _draw_ascii_scaled_internal(OLED_Handle* oled, int16_t x, int16_t y, char c, OLED_Color color, uint8_t scale) {
    if(c < current_font->first_char || c > current_font->last_char) {
        return 0;
    }

    uint8_t glyph_h = _glyph_height_from_table();
    uint8_t char_index = (uint8_t)(c - current_font->first_char);
    uint16_t offset = char_index * current_font->width;

    for(uint8_t i = 0; i < current_font->width; i++) {
        uint8_t line = current_font->data[offset + i];

        for(uint8_t j = 0; j < glyph_h; j++) {
            if(line & (1u << j)) {
                _fill_scaled_pixel(oled,
                                   x + (int16_t)(i * scale),
                                   y + (int16_t)(j * scale),
                                   scale,
                                   color);
            } else if(color != OLED_COLOR_INVERT) {
                _fill_scaled_pixel(oled,
                                   x + (int16_t)(i * scale),
                                   y + (int16_t)(j * scale),
                                   scale,
                                   OLED_COLOR_BLACK);
            }
        }
    }

    return (uint8_t)(current_font->width * scale);
}

static void _draw_thai_base_placeholder(OLED_Handle* oled, int16_t x, int16_t y, uint16_t unicode, OLED_Color color, uint8_t scale) {
    const uint8_t* glyph = _lookup_thai_glyph(unicode);
    if(glyph != NULL) {
        _draw_bitmap8_scaled(oled, x, y, glyph, color, scale);
        return;
    }

    uint8_t w = _thai_advance(scale);
    uint8_t h = (uint8_t)(8 * scale);

    OLED_DrawRect(oled, (uint8_t)x, (uint8_t)y, w, h, color);

    /* Draw a deterministic inner pattern so each unicode looks different. */
    uint8_t pattern = (uint8_t)((unicode ^ (unicode >> 4)) & 0x07);
    uint8_t px = (uint8_t)(x + scale + pattern);
    uint8_t py = (uint8_t)(y + scale + ((unicode >> 1) & 0x03));
    if(px < x + w && py < y + h) {
        OLED_SetPixel(oled, px, py, color);
    }
}

static void _draw_thai_mark_placeholder(OLED_Handle* oled,
                                        int16_t base_x,
                                        int16_t base_y,
                                        uint16_t unicode,
                                        OLED_Color color,
                                        uint8_t scale) {
    int16_t x = base_x;
    int16_t y = base_y;
    uint8_t w = (uint8_t)((scale > 1) ? (scale + 1) : 1);
    uint8_t h = w;
    uint8_t glyph_h = _glyph_height_from_table();

    if(_is_thai_leading_vowel(unicode)) {
        x = base_x - (int16_t)(3 * scale);
        y = base_y + (int16_t)(2 * scale);
        OLED_DrawVLine(oled, (uint8_t)x, (uint8_t)y, (uint8_t)(4 * scale), color);
        OLED_DrawVLine(oled, (uint8_t)(x + scale), (uint8_t)y, (uint8_t)(4 * scale), color);
        return;
    } else if(_is_thai_combining_upper(unicode)) {
        x = base_x + (int16_t)(2 * scale);
        y = base_y - (int16_t)(2 * scale);

        if(unicode >= 0x0E48 && unicode <= 0x0E4B) {
            OLED_DrawLine(oled, (uint8_t)x, (uint8_t)y,
                          (uint8_t)(x + scale + 1), (uint8_t)(y + scale), color);
            return;
        }

        if(unicode == 0x0E4C || unicode == 0x0E4D || unicode == 0x0E4E) {
            OLED_DrawCircle(oled, (uint8_t)x, (uint8_t)y, (uint8_t)scale, color);
            return;
        }
    } else if(_is_thai_combining_lower(unicode)) {
        x = base_x + (int16_t)(2 * scale);
        y = base_y + (int16_t)((glyph_h - 1) * scale);
    } else {
        x = base_x + (int16_t)(4 * scale);
        y = base_y + (int16_t)(2 * scale);
    }

    OLED_FillRect(oled, (uint8_t)x, (uint8_t)y, w, h, color);
}

static OLED_TextConfig _default_text_config(void) {
    OLED_TextConfig cfg;
    cfg.scale = current_text_scale;
    cfg.letter_spacing = 0;
    cfg.thai_mode = OLED_THAI_RENDER_COMBINING;
    return cfg;
}

static uint16_t _measure_utf8_internal(const char* str, const OLED_TextConfig* config) {
    if(str == NULL) {
        return 0;
    }

    OLED_TextConfig cfg = _default_text_config();
    if(config != NULL) {
        cfg = *config;
    }

    cfg.scale = _clamp_scale(cfg.scale);

    uint16_t total = 0;
    const char* p = str;
    uint8_t have_base = 0;

    while(*p) {
        uint16_t unicode = 0;
        uint8_t bytes = OLED_UTF8ToUnicode(p, &unicode);
        if(bytes == 0) {
            p++;
            continue;
        }

        if(unicode < 0x80) {
            if(unicode >= current_font->first_char && unicode <= current_font->last_char) {
                total += (uint16_t)(current_font->width * cfg.scale);
                total += cfg.letter_spacing;
            }
            have_base = 0;
        } else if(cfg.thai_mode != OLED_THAI_RENDER_OFF && OLED_IsThaiChar(unicode)) {
            if(_is_thai_base_char(unicode)) {
                total += (uint16_t)(current_font->width * cfg.scale);
                total += cfg.letter_spacing;
                have_base = 1;
            } else if(_is_thai_leading_vowel(unicode)) {
                total += _thai_advance(cfg.scale);
                total += cfg.letter_spacing;
                have_base = 1;
            } else if(_is_thai_combining_upper(unicode) || _is_thai_combining_lower(unicode)) {
                if(!have_base) {
                    total += _thai_advance(cfg.scale);
                    total += cfg.letter_spacing;
                }
            } else {
                total += _thai_advance(cfg.scale);
                total += cfg.letter_spacing;
                have_base = 1;
            }
        } else {
            total += (uint16_t)(current_font->width * cfg.scale);
            total += cfg.letter_spacing;
            have_base = 0;
        }

        p += bytes;
    }

    return total;
}

/* ========== Font Selection ========== */

/**
 * @brief Sets active font
 *
 * @param oled - pointer to OLED_Handle (unused)
 * @param font - pointer to OLED_Font struct (width, height, data)
 *
 * @note ใช้ static variable ปัจจุบันรองรับ Font_6x8 / Font_8x16 / Font_12x16
 */
void OLED_SetFont(OLED_Handle* oled, const OLED_Font* font) {
    (void)oled;  // Unused
    current_font = font;
}

/**
 * @brief Gets current active font
 *
 * @param oled - pointer to OLED_Handle (unused)
 *
 * @return pointer to current OLED_Font
 */
const OLED_Font* OLED_GetFont(OLED_Handle* oled) {
    (void)oled;  // Unused
    return current_font;
}

/* ========== Character Drawing ========== */

/**
 * @brief Draws a character at given position (uses current text scale)
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param c - ASCII character
 * @param color - OLED_Color
 *
 * @return character width in pixels
 *
 * @note เรียก OLED_DrawCharScaled ด้วย text scale ปัจจุบัน
 */
uint8_t OLED_DrawChar(OLED_Handle* oled, uint8_t x, uint8_t y, char c, OLED_Color color) {
    return OLED_DrawCharScaled(oled, x, y, c, color, current_text_scale);
}

/**
 * @brief Draws a scaled ASCII character
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param c - ASCII character
 * @param color - OLED_Color
 * @param scale - scale factor 1–4 (clamped)
 *
 * @return character width in pixels (font_width × scale)
 *
 * @note ใช้ _draw_ascii_scaled_internal, รองรับ font ปัจจุบัน
 *       scale < 1 → 1, scale > 4 → 4
 */
uint8_t OLED_DrawCharScaled(OLED_Handle* oled, uint8_t x, uint8_t y, char c, OLED_Color color, uint8_t scale) {
    scale = _clamp_scale(scale);
    return _draw_ascii_scaled_internal(oled, x, y, c, color, scale);
}

/**
 * @brief Draws a character with inverted colors (white bg, black fg)
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param c - ASCII character
 *
 * @return character width in pixels
 *
 * @note วาด FillRect สีขาว = background แล้ววาดตัวอักษรสีดำทับ
 */
uint8_t OLED_DrawCharInverse(OLED_Handle* oled, uint8_t x, uint8_t y, char c) {
    uint8_t scale = current_text_scale;
    uint8_t glyph_h = _glyph_height_from_table();

    // Draw background
    OLED_FillRect(oled, x, y,
                  (uint8_t)(current_font->width * scale),
                  (uint8_t)(glyph_h * scale),
                  OLED_COLOR_WHITE);
    
    // Draw character in black
    return OLED_DrawCharScaled(oled, x, y, c, OLED_COLOR_BLACK, scale);
}

/* ========== String Drawing ========== */

/**
 * @brief Draws an ASCII string at given position
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param str - null-terminated string
 * @param color - OLED_Color
 *
 * @return total width in pixels
 *
 * @note เรียก OLED_DrawStringScaled ด้วย text scale ปัจจุบัน
 */
uint16_t OLED_DrawString(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color) {
    return OLED_DrawStringScaled(oled, x, y, str, color, current_text_scale);
}

/**
 * @brief Draws a scaled ASCII string
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param str - null-terminated string
 * @param color - OLED_Color
 * @param scale - scale factor 1–4
 *
 * @return total width in pixels
 *
 * @note รองรับเฉพาะ ASCII (non-UTF8), scale clamped
 */
uint16_t OLED_DrawStringScaled(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, uint8_t scale) {
    uint16_t total_width = 0;
    scale = _clamp_scale(scale);

    if(str == NULL) {
        return 0;
    }
    
    while(*str) {
        uint8_t char_width = OLED_DrawCharScaled(oled, (uint8_t)(x + total_width), y, *str, color, scale);
        total_width += char_width;
        str++;
    }
    
    return total_width;
}

/**
 * @brief Draws UTF-8 string with full text configuration
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param str - UTF-8 string
 * @param color - OLED_Color
 * @param config - text config (scale, letter_spacing, thai_mode) or NULL for defaults
 *
 * @return total width in pixels
 *
 * @note รองรับ ASCII + ภาษาไทย (configurable thai_mode)
 *       Thai rendering: base chars + leading vowels + combining marks
 *       scale = 1–4, letter_spacing default = 0
 */
uint16_t OLED_DrawStringUTF8Ex(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, const OLED_TextConfig* config) {
    if(str == NULL) {
        return 0;
    }

    OLED_TextConfig cfg = _default_text_config();
    if(config != NULL) {
        cfg = *config;
    }
    cfg.scale = _clamp_scale(cfg.scale);

    uint16_t total_width = 0;
    const char* p = str;
    int16_t last_base_x = -1;
    int16_t last_base_y = (int16_t)y;

    while(*p) {
        uint16_t unicode = 0;
        uint8_t bytes = OLED_UTF8ToUnicode(p, &unicode);
        if(bytes == 0) {
            p++;
            continue;
        }

        if(unicode < 0x80) {
            uint8_t w = OLED_DrawCharScaled(oled, (uint8_t)(x + total_width), y, (char)unicode, color, cfg.scale);
            total_width += w;
            total_width += cfg.letter_spacing;
            last_base_x = -1;
        } else if(cfg.thai_mode != OLED_THAI_RENDER_OFF && OLED_IsThaiChar(unicode)) {
            int16_t cx = (int16_t)(x + total_width);
            if(_is_thai_base_char(unicode)) {
                _draw_thai_base_placeholder(oled, cx, y, unicode, color, cfg.scale);
                last_base_x = cx;
                last_base_y = y;
                total_width += _thai_advance(cfg.scale);
                total_width += cfg.letter_spacing;
            } else if(_is_thai_leading_vowel(unicode)) {
                _draw_thai_mark_placeholder(oled, cx + (int16_t)(2 * cfg.scale), y, unicode, color, cfg.scale);
                total_width += _thai_advance(cfg.scale);
                total_width += cfg.letter_spacing;
                last_base_x = cx;
                last_base_y = y;
            } else if(_is_thai_combining_upper(unicode) || _is_thai_combining_lower(unicode)) {
                if(last_base_x >= 0) {
                    _draw_thai_mark_placeholder(oled, last_base_x, last_base_y, unicode, color, cfg.scale);
                } else {
                    _draw_thai_base_placeholder(oled, cx, y, unicode, color, cfg.scale);
                    total_width += _thai_advance(cfg.scale);
                    total_width += cfg.letter_spacing;
                }
            } else {
                _draw_thai_base_placeholder(oled, cx, y, unicode, color, cfg.scale);
                last_base_x = cx;
                last_base_y = y;
                total_width += _thai_advance(cfg.scale);
                total_width += cfg.letter_spacing;
            }
        } else {
            _draw_thai_base_placeholder(oled, (int16_t)(x + total_width), y, unicode, color, cfg.scale);
            total_width += _thai_advance(cfg.scale);
            total_width += cfg.letter_spacing;
            last_base_x = -1;
        }

        p += bytes;
    }

    return total_width;
}

/**
 * @brief Draws string with text alignment (left/center/right)
 *
 * @param oled - pointer to OLED_Handle
 * @param x - reference X position
 * @param y - Y position
 * @param str - string
 * @param color - OLED_Color
 * @param align - OLED_ALIGN_LEFT / CENTER / RIGHT
 *
 * @note คำนวณ offset จากความกว้าง string ก่อนวาด
 */
void OLED_DrawStringAlign(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, OLED_TextAlign align) {
    uint16_t width = OLED_GetStringWidth(oled, str);
    uint8_t start_x = x;
    
    switch(align) {
        case OLED_ALIGN_CENTER:
            start_x = x - (width / 2);
            break;
        case OLED_ALIGN_RIGHT:
            start_x = x - width;
            break;
        case OLED_ALIGN_LEFT:
        default:
            start_x = x;
            break;
    }
    
    OLED_DrawString(oled, start_x, y, str, color);
}

/**
 * @brief Draws string with inverted colors (white bg, black fg)
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param str - string
 *
 * @return total width in pixels
 *
 * @note เรียก OLED_DrawCharInverse ทีละ character
 */
uint16_t OLED_DrawStringInverse(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str) {
    uint16_t total_width = 0;
    
    while(*str) {
        uint8_t char_width = OLED_DrawCharInverse(oled, x + total_width, y, *str);
        total_width += char_width;
        str++;
    }
    
    return total_width;
}

/* ========== Number Drawing ========== */

/**
 * @brief Draws an integer as string
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param num - integer value
 * @param color - OLED_Color
 *
 * @return total width in pixels
 *
 * @note ใช้ snprintf("%ld") แล้วเรียก DrawString
 */
uint16_t OLED_DrawInt(OLED_Handle* oled, uint8_t x, uint8_t y, int32_t num, OLED_Color color) {
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%ld", (long)num);
    return OLED_DrawString(oled, x, y, buffer, color);
}

/**
 * @brief Draws a float as string
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param num - float value
 * @param decimals - decimal places
 * @param color - OLED_Color
 *
 * @return total width in pixels
 *
 * @note ใช้ snprintf ร่วมกับ format string เช่น "%.2f"
 */
uint16_t OLED_DrawFloat(OLED_Handle* oled, uint8_t x, uint8_t y, float num, uint8_t decimals, OLED_Color color) {
    char buffer[16];
    char format[8];
    snprintf(format, sizeof(format), "%%.%df", decimals);
    snprintf(buffer, sizeof(buffer), format, num);
    return OLED_DrawString(oled, x, y, buffer, color);
}

/* ========== Text Measurement ========== */

/**
 * @brief Gets width of a string in pixels (using current text scale)
 *
 * @param oled - pointer to OLED_Handle
 * @param str - string
 *
 * @return width in pixels
 *
 * @note เรียก OLED_GetStringWidthScaled ด้วย text scale ปัจจุบัน
 */
uint16_t OLED_GetStringWidth(OLED_Handle* oled, const char* str) {
    return OLED_GetStringWidthScaled(oled, str, current_text_scale);
}

/**
 * @brief Gets width of a string with given scale (ASCII only)
 *
 * @param oled - pointer to OLED_Handle (unused)
 * @param str - ASCII string
 * @param scale - scale factor 1–4
 *
 * @return width in pixels (strlen × font_width × scale)
 *
 * @note คำนวณจากความยาว string อย่างเดียว ไม่นับ UTF-8
 */
uint16_t OLED_GetStringWidthScaled(OLED_Handle* oled, const char* str, uint8_t scale) {
    (void)oled;
    if(str == NULL) {
        return 0;
    }
    scale = _clamp_scale(scale);
    return (uint16_t)(strlen(str) * current_font->width * scale);
}

/**
 * @brief Measures pixel width of a UTF-8 string (including Thai)
 *
 * @param oled - pointer to OLED_Handle (unused)
 * @param str - UTF-8 string
 * @param config - text config (or NULL for defaults)
 *
 * @return width in pixels
 *
 * @note ใช้ _measure_utf8_internal, รองรับการผสมภาษาไทย
 */
uint16_t OLED_MeasureStringUTF8(OLED_Handle* oled, const char* str, const OLED_TextConfig* config) {
    (void)oled;
    return _measure_utf8_internal(str, config);
}

/**
 * @brief Gets current font height in pixels (considering text scale)
 *
 * @param oled - pointer to OLED_Handle (unused)
 *
 * @return height in pixels
 */
uint8_t OLED_GetFontHeight(OLED_Handle* oled) {
    (void)oled;  // Unused
    return (uint8_t)(_glyph_height_from_table() * current_text_scale);
}

/**
 * @brief Sets text scale (affects DrawChar, DrawString, etc.)
 *
 * @param oled - pointer to OLED_Handle (unused)
 * @param scale - 1–4 (clamped)
 */
void OLED_SetTextScale(OLED_Handle* oled, uint8_t scale) {
    (void)oled;
    current_text_scale = _clamp_scale(scale);
}

/**
 * @brief Gets current text scale
 *
 * @param oled - pointer to OLED_Handle (unused)
 *
 * @return current scale (1–4)
 */
uint8_t OLED_GetTextScale(OLED_Handle* oled) {
    (void)oled;
    return current_text_scale;
}

/* ========== Advanced Text Features ========== */

/**
 * @brief Draws multiple lines of text (splits on newline)
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - start position
 * @param str - multi-line string (\\n-separated)
 * @param color - OLED_Color
 * @param line_spacing - extra spacing between lines (px)
 *
 * @note รองรับ UTF-8, line_wrap = newline character
 *       ความสูงแต่ละบรรทัด = font_height + line_spacing
 */
void OLED_DrawMultiLine(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, uint8_t line_spacing) {
    uint8_t current_y = y;
    const char* line_start = str;
    
    while(*str) {
        if(*str == '\n' || *(str + 1) == '\0') {
            // Draw line
            char line[128];
            uint8_t len = str - line_start;
            if(*(str + 1) == '\0' && *str != '\n') len++;
            
            if(len > 0 && len < sizeof(line)) {
                memcpy(line, line_start, len);
                line[len] = '\0';
                OLED_DrawStringUTF8Ex(oled, x, current_y, line, color, NULL);
            }
            
            current_y += OLED_GetFontHeight(oled) + line_spacing;
            line_start = str + 1;
        }
        str++;
    }
}

/**
 * @brief Draws horizontally scrolling text (seamless loop)
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param w - viewport width
 * @param str - text to scroll
 * @param color - OLED_Color
 * @param offset - pixel offset (increment for animation)
 *
 * @note วาดซ้ำ 2 ครั้งเพื่อ seamless loop เมื่อ text สั้นกว่าหน้าต่าง
 *       ใช้ offset mod (text_width + w) สำหรับ scroll position
 */
void OLED_DrawScrollText(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, const char* str, OLED_Color color, uint16_t offset) {
    uint16_t text_width = OLED_GetStringWidth(oled, str);
    
    // Calculate scroll position
    int16_t scroll_x = x - (offset % (text_width + w));
    
    // Draw text at scrolled position
    OLED_DrawStringUTF8Ex(oled, (uint8_t)scroll_x, y, str, color, NULL);
    
    // Draw again for seamless loop
    if(scroll_x + text_width < x + w) {
        OLED_DrawStringUTF8Ex(oled, (uint8_t)(scroll_x + text_width + w), y, str, color, NULL);
    }
}

/**
 * @brief Draws a bordered text box with centered text
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - box top-left
 * @param w, h - box dimensions
 * @param str - text content
 * @param color - OLED_Color
 * @param align - OLED_ALIGN_LEFT / CENTER / RIGHT
 *
 * @note วาดกรอบสี่เหลี่ยม + ข้อความอยู่กึ่งกลางแนวตั้ง
 */
void OLED_DrawTextBox(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, const char* str, OLED_Color color, OLED_TextAlign align) {
    // Draw box
    OLED_DrawRect(oled, x, y, w, h, color);
    
    // Calculate text position
    uint8_t text_y = y + (h - OLED_GetFontHeight(oled)) / 2;
    uint8_t text_x = x + w / 2;
    
    if(align == OLED_ALIGN_LEFT) {
        text_x = x + 2;
    } else if(align == OLED_ALIGN_RIGHT) {
        text_x = x + w - 2;
    }
    
    OLED_DrawStringAlign(oled, text_x, text_y, str, color, align);
}

/* ========== UTF-8 Helper Functions ========== */

/**
 * @brief Converts UTF-8 bytes to Unicode code point
 *
 * @param utf8 - UTF-8 encoded character
 * @param unicode - output 16-bit Unicode code point
 *
 * @return number of UTF-8 bytes consumed (0 on error)
 *
 * @note รองรับ 1–3 byte sequences (4-byte returns '?')
 *       ไม่ advance pointer
 */
uint8_t OLED_UTF8ToUnicode(const char* utf8, uint16_t* unicode) {
    if(utf8 == NULL || unicode == NULL) {
        return 0;
    }

    uint8_t c = (uint8_t)*utf8;
    
    if(c < 0x80) {
        // 1-byte character (ASCII)
        *unicode = c;
        return 1;
    } else if((c & 0xE0) == 0xC0) {
        // 2-byte character
        *unicode = ((c & 0x1F) << 6) | (utf8[1] & 0x3F);
        return 2;
    } else if((c & 0xF0) == 0xE0) {
        // 3-byte character
        *unicode = ((c & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
        return 3;
    } else if((c & 0xF8) == 0xF0) {
        // 4-byte character not supported by uint16_t result type in this API.
        *unicode = '?';
        return 4;
    }
    
    return 0;
}

/**
 * @brief Checks if Unicode is in Thai range (0x0E00–0x0E7F)
 *
 * @param unicode - Unicode code point
 *
 * @return 1 if Thai character, 0 otherwise
 */
uint8_t OLED_IsThaiChar(uint16_t unicode) {
    // Thai Unicode range: 0x0E00 - 0x0E7F
    return (unicode >= 0x0E00 && unicode <= 0x0E7F);
}

/* ========== Thai Text Support ========== */

// Note: Full Thai font implementation would require ~100 characters x 32 bytes = 3.2KB
// This is a placeholder implementation

/**
 * @brief Draws a Thai-language string
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param str - UTF-8 Thai string
 * @param color - OLED_Color
 *
 * @return total width in pixels
 *
 * @note เรียก OLED_DrawStringUTF8Ex ด้วย thai_mode = OLED_THAI_RENDER_COMBINING
 */
uint16_t OLED_DrawStringThai(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color) {
    OLED_TextConfig cfg = _default_text_config();
    cfg.thai_mode = OLED_THAI_RENDER_COMBINING;
    return OLED_DrawStringUTF8Ex(oled, x, y, str, color, &cfg);
}

/**
 * @brief Draws mixed ASCII/Thai string (auto-detect language)
 *
 * @param oled - pointer to OLED_Handle
 * @param x, y - position
 * @param str - mixed UTF-8 string
 * @param color - OLED_Color
 *
 * @return total width in pixels
 *
 * @note เรียก OLED_DrawStringUTF8Ex โดยตรง, ใช้ config default
 */
uint16_t OLED_DrawStringMixed(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color) {
    return OLED_DrawStringUTF8Ex(oled, x, y, str, color, NULL);
}
