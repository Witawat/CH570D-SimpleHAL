/**
 * @file SimpleNeoPixel.c
 * @brief Simple NeoPixel Library Implementation
 * @version 1.0
 * @date 2025-12-12
 */

#include "NeoPixel.h"
#include "../../SimpleHAL/SimpleHAL.h"
#include <stdlib.h>

/* ========== Private Variables ========== */

static GPIO_TypeDef* neo_port = NULL;
static uint16_t neo_pin = 0;
static uint8_t _neo_pin_num = 0;
static uint16_t neo_num_leds = 0;
static uint8_t* neo_pixels = NULL;  // RGB buffer (3 bytes per LED)
static uint8_t neo_brightness = 255;

/* ========== WS2812 Timing (bit-banging) ========== */

/**
 * @brief Delay macros ที่ปรับตาม clock speed
 * 
 * WS2812 Timing Requirements:
 * - Bit 0: HIGH 0.4us, LOW 0.85us
 * - Bit 1: HIGH 0.8us, LOW 0.45us
 * 
 * @ 24MHz: 1 cycle = 0.042us
 * @ 48MHz: 1 cycle = 0.021us
 */
#if (SystemCoreClock == SYSCLK_FREQ_48MHZ_HSI)
    // 48MHz timing
    #define DELAY_T0H()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~19 cycles = 0.4us
    #define DELAY_T0L()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~40 cycles = 0.85us
    #define DELAY_T1H()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~38 cycles = 0.8us
    #define DELAY_T1L()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~21 cycles = 0.45us
#else
    // 24MHz timing (default)
    #define DELAY_T0H()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~10 cycles = 0.4us
    #define DELAY_T0L()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~20 cycles = 0.85us
    #define DELAY_T1H()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~19 cycles = 0.8us
    #define DELAY_T1L()  { __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); __asm__ volatile ("nop"); \
                           __asm__ volatile ("nop"); __asm__ volatile ("nop"); }  // ~10 cycles = 0.45us
#endif

/**
 * @brief ส่ง bit 0 (HIGH 0.4us, LOW 0.85us)
 */
static inline void SendBit0(void) {
    digitalWrite(_neo_pin_num, HIGH);
    DELAY_T0H();
    digitalWrite(_neo_pin_num, LOW);
    DELAY_T0L();
}

static inline void SendBit1(void) {
    digitalWrite(_neo_pin_num, HIGH);
    DELAY_T1H();
    digitalWrite(_neo_pin_num, LOW);
    DELAY_T1L();
}



/**
 * @brief ส่ง 1 byte
 */
static void SendByte(uint8_t byte) {
    for(uint8_t bit = 0; bit < 8; bit++) {
        if(byte & 0x80) {
            SendBit1();
        } else {
            SendBit0();
        }
        byte <<= 1;
    }
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน NeoPixel
 */
void NeoPixel_Init(GPIO_TypeDef* port, uint16_t pin, uint16_t num_leds) {
    if(num_leds > NEOPIXEL_MAX_LEDS) {
        num_leds = NEOPIXEL_MAX_LEDS;
    }
    neo_port = port;
    neo_pin = pin;
    _neo_pin_num = 0;
    while (pin > 1) { pin >>= 1; _neo_pin_num++; }
    neo_num_leds = num_leds;
    neo_pixels = (uint8_t*)malloc(num_leds * 3);
    if(neo_pixels == NULL) {
        return;
    }
    for(uint16_t i = 0; i < num_leds * 3; i++) {
        neo_pixels[i] = 0;
    }
    pinMode(_neo_pin_num, PIN_MODE_OUTPUT);
    digitalWrite(_neo_pin_num, LOW);
}

/**
 * @brief ตั้งค่าสีของ LED (RGB)
 */
void NeoPixel_SetPixelColor(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b) {
    if(pixel >= neo_num_leds || neo_pixels == NULL) return;
    
    // ปรับความสว่าง
    if(neo_brightness != 255) {
        r = (r * neo_brightness) >> 8;
        g = (g * neo_brightness) >> 8;
        b = (b * neo_brightness) >> 8;
    }
    
    // WS2812 ใช้ลำดับ GRB
    uint16_t offset = pixel * 3;
    neo_pixels[offset + 0] = g;
    neo_pixels[offset + 1] = r;
    neo_pixels[offset + 2] = b;
}

/**
 * @brief ตั้งค่าสีด้วย 32-bit color
 */
void NeoPixel_SetPixelColor32(uint16_t pixel, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    NeoPixel_SetPixelColor(pixel, r, g, b);
}

/**
 * @brief ตั้งค่าสีด้วย HSV
 */
void NeoPixel_SetPixelColorHSV(uint16_t pixel, uint16_t h, uint8_t s, uint8_t v) {
    uint32_t color = NeoPixel_ColorHSV(h, s, v);
    NeoPixel_SetPixelColor32(pixel, color);
}

/**
 * @brief อัพเดทการแสดงผล
 */
void NeoPixel_Show(void) {
    if(neo_pixels == NULL) return;
    
    // ปิด interrupt เพื่อ timing ที่แม่นยำ
    __disable_irq();
    
    // ส่งข้อมูลทุก LEDs
    for(uint16_t i = 0; i < neo_num_leds * 3; i++) {
        SendByte(neo_pixels[i]);
    }
    
    // เปิด interrupt กลับ
    __enable_irq();
    
    // Reset pulse (LOW > 50us)
    digitalWrite(_neo_pin_num, LOW);
    Delay_Us(60);
}

/**
 * @brief ดับ LEDs ทั้งหมด
 */
void NeoPixel_Clear(void) {
    if(neo_pixels == NULL) return;
    
    for(uint16_t i = 0; i < neo_num_leds * 3; i++) {
        neo_pixels[i] = 0;
    }
}

/**
 * @brief ตั้งค่าสีเดียวกันให้ทุก LEDs
 */
void NeoPixel_Fill(uint8_t r, uint8_t g, uint8_t b) {
    for(uint16_t i = 0; i < neo_num_leds; i++) {
        NeoPixel_SetPixelColor(i, r, g, b);
    }
}

/**
 * @brief ตั้งค่าความสว่าง
 */
void NeoPixel_SetBrightness(uint8_t brightness) {
    neo_brightness = brightness;
}

/**
 * @brief อ่านค่าสีของ LED
 */
uint32_t NeoPixel_GetPixelColor(uint16_t pixel) {
    if(pixel >= neo_num_leds || neo_pixels == NULL) return 0;
    
    uint16_t offset = pixel * 3;
    uint8_t g = neo_pixels[offset + 0];
    uint8_t r = neo_pixels[offset + 1];
    uint8_t b = neo_pixels[offset + 2];
    
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/* ========== Color Utility Functions ========== */

/**
 * @brief สร้างสี 32-bit จาก RGB
 */
uint32_t NeoPixel_Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/**
 * @brief แปลง HSV เป็น RGB
 */
uint32_t NeoPixel_ColorHSV(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    
    // Normalize hue to 0-359
    h = h % 360;
    
    uint8_t region = h / 60;
    uint8_t remainder = (h % 60) * 255 / 60;
    
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    
    switch(region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    
    return NeoPixel_Color(r, g, b);
}

/**
 * @brief สร้างสีจาก Color Wheel
 */
uint32_t NeoPixel_Wheel(uint8_t pos) {
    pos = 255 - pos;
    if(pos < 85) {
        return NeoPixel_Color(255 - pos * 3, 0, pos * 3);
    } else if(pos < 170) {
        pos -= 85;
        return NeoPixel_Color(0, pos * 3, 255 - pos * 3);
    } else {
        pos -= 170;
        return NeoPixel_Color(pos * 3, 255 - pos * 3, 0);
    }
}

/* ========== Effect Functions ========== */

/**
 * @brief Rainbow effect
 */
void NeoPixel_Rainbow(uint16_t wait_ms, uint8_t cycles) {
    for(uint8_t j = 0; j < 256 * cycles; j++) {
        for(uint16_t i = 0; i < neo_num_leds; i++) {
            uint32_t color = NeoPixel_Wheel((i + j) & 255);
            NeoPixel_SetPixelColor32(i, color);
        }
        NeoPixel_Show();
        Delay_Ms(wait_ms);
    }
}

/**
 * @brief Theater Chase effect
 */
void NeoPixel_TheaterChase(uint8_t r, uint8_t g, uint8_t b, uint16_t wait_ms, uint8_t cycles) {
    for(uint8_t j = 0; j < cycles * 3; j++) {
        for(uint16_t q = 0; q < 3; q++) {
            NeoPixel_Clear();
            for(uint16_t i = 0; i < neo_num_leds; i += 3) {
                NeoPixel_SetPixelColor(i + q, r, g, b);
            }
            NeoPixel_Show();
            Delay_Ms(wait_ms);
        }
    }
}

/**
 * @brief Color Wipe effect
 */
void NeoPixel_ColorWipe(uint8_t r, uint8_t g, uint8_t b, uint16_t wait_ms) {
    for(uint16_t i = 0; i < neo_num_leds; i++) {
        NeoPixel_SetPixelColor(i, r, g, b);
        NeoPixel_Show();
        Delay_Ms(wait_ms);
    }
}

/**
 * @brief Fade effect
 */
void NeoPixel_Fade(uint8_t r, uint8_t g, uint8_t b, uint8_t steps, uint16_t wait_ms) {
    // Fade in
    for(uint8_t i = 0; i <= steps; i++) {
        uint8_t brightness = (255 * i) / steps;
        NeoPixel_SetBrightness(brightness);
        NeoPixel_Fill(r, g, b);
        NeoPixel_Show();
        Delay_Ms(wait_ms);
    }
    
    // Fade out
    for(uint8_t i = steps; i > 0; i--) {
        uint8_t brightness = (255 * i) / steps;
        NeoPixel_SetBrightness(brightness);
        NeoPixel_Fill(r, g, b);
        NeoPixel_Show();
        Delay_Ms(wait_ms);
    }
    
    NeoPixel_SetBrightness(255);  // Reset brightness
}

/**
 * @brief Sparkle effect
 */
void NeoPixel_Sparkle(uint8_t r, uint8_t g, uint8_t b, uint16_t count, uint16_t wait_ms) {
    for(uint16_t i = 0; i < count; i++) {
        // Random pixel
        uint16_t pixel = rand() % neo_num_leds;
        
        // Flash
        NeoPixel_SetPixelColor(pixel, r, g, b);
        NeoPixel_Show();
        Delay_Ms(wait_ms);
        
        // Off
        NeoPixel_SetPixelColor(pixel, 0, 0, 0);
        NeoPixel_Show();
    }
}

/* ========== Gamma Correction Table ========== */

/**
 * @brief Gamma correction lookup table (gamma = 2.8)
 * ทำให้สีดูเป็นธรรมชาติมากขึ้นบนตา LED
 */
static const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

/* ========== Advanced Effect Functions ========== */

/**
 * @brief Breathing effect
 */
void NeoPixel_Breathing(uint8_t r, uint8_t g, uint8_t b, uint16_t speed, uint8_t cycles) {
    for(uint8_t cycle = 0; cycle < cycles; cycle++) {
        // Fade in
        for(uint16_t i = 0; i < 256; i++) {
            uint8_t brightness = i;
            NeoPixel_SetBrightness(brightness);
            NeoPixel_Fill(r, g, b);
            NeoPixel_Show();
            Delay_Ms(speed);
        }
        
        // Fade out
        for(uint16_t i = 255; i > 0; i--) {
            uint8_t brightness = i;
            NeoPixel_SetBrightness(brightness);
            NeoPixel_Fill(r, g, b);
            NeoPixel_Show();
            Delay_Ms(speed);
        }
    }
    NeoPixel_SetBrightness(255);  // Reset brightness
}

/**
 * @brief Comet/Meteor effect
 */
void NeoPixel_Comet(uint8_t r, uint8_t g, uint8_t b, uint8_t tail_length, uint16_t speed, uint8_t cycles) {
    for(uint8_t cycle = 0; cycle < cycles; cycle++) {
        for(uint16_t i = 0; i < neo_num_leds + tail_length; i++) {
            NeoPixel_Clear();
            
            // Draw comet with tail
            for(uint8_t j = 0; j < tail_length; j++) {
                if(i >= j && (i - j) < neo_num_leds) {
                    uint8_t brightness = 255 - (j * 255 / tail_length);
                    uint8_t dim_r = (r * brightness) >> 8;
                    uint8_t dim_g = (g * brightness) >> 8;
                    uint8_t dim_b = (b * brightness) >> 8;
                    NeoPixel_SetPixelColor(i - j, dim_r, dim_g, dim_b);
                }
            }
            
            NeoPixel_Show();
            Delay_Ms(speed);
        }
    }
}

/**
 * @brief KITT Scanner effect
 */
void NeoPixel_Scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t eye_size, uint16_t speed, uint8_t cycles) {
    for(uint8_t cycle = 0; cycle < cycles; cycle++) {
        // Scan right
        for(uint16_t i = 0; i < neo_num_leds - eye_size - 2; i++) {
            NeoPixel_Clear();
            NeoPixel_SetPixelColor(i, r/10, g/10, b/10);
            for(uint8_t j = 1; j <= eye_size; j++) {
                NeoPixel_SetPixelColor(i + j, r, g, b);
            }
            NeoPixel_SetPixelColor(i + eye_size + 1, r/10, g/10, b/10);
            NeoPixel_Show();
            Delay_Ms(speed);
        }
        
        // Scan left
        for(uint16_t i = neo_num_leds - eye_size - 2; i > 0; i--) {
            NeoPixel_Clear();
            NeoPixel_SetPixelColor(i, r/10, g/10, b/10);
            for(uint8_t j = 1; j <= eye_size; j++) {
                NeoPixel_SetPixelColor(i + j, r, g, b);
            }
            NeoPixel_SetPixelColor(i + eye_size + 1, r/10, g/10, b/10);
            NeoPixel_Show();
            Delay_Ms(speed);
        }
    }
}

/**
 * @brief Running Lights effect
 */
void NeoPixel_RunningLights(uint8_t r, uint8_t g, uint8_t b, uint16_t wave_delay, uint8_t cycles) {
    for(uint8_t cycle = 0; cycle < cycles; cycle++) {
        for(uint16_t j = 0; j < 256; j++) {
            for(uint16_t i = 0; i < neo_num_leds; i++) {
                // Calculate wave position
                uint8_t level = (uint8_t)(127 + 127 * ((i + j) % 256) / 256);
                NeoPixel_SetPixelColor(i, (r * level) >> 8, (g * level) >> 8, (b * level) >> 8);
            }
            NeoPixel_Show();
            Delay_Ms(wave_delay);
        }
    }
}

/**
 * @brief Twinkle Random effect
 */
void NeoPixel_TwinkleRandom(uint16_t count, uint16_t speed) {
    NeoPixel_Clear();
    NeoPixel_Show();
    
    for(uint16_t i = 0; i < count; i++) {
        uint16_t pixel = rand() % neo_num_leds;
        uint32_t color = NeoPixel_Wheel(rand() % 256);
        
        NeoPixel_SetPixelColor32(pixel, color);
        NeoPixel_Show();
        Delay_Ms(speed);
        
        NeoPixel_SetPixelColor(pixel, 0, 0, 0);
        NeoPixel_Show();
    }
}

/**
 * @brief Color Chase effect
 */
void NeoPixel_ColorChase(uint32_t* colors, uint8_t num_colors, uint16_t speed, uint8_t cycles) {
    for(uint8_t cycle = 0; cycle < cycles; cycle++) {
        for(uint8_t color_idx = 0; color_idx < num_colors; color_idx++) {
            for(uint16_t i = 0; i < neo_num_leds; i++) {
                NeoPixel_Clear();
                NeoPixel_SetPixelColor32(i, colors[color_idx]);
                NeoPixel_Show();
                Delay_Ms(speed);
            }
        }
    }
}

/**
 * @brief Rainbow Cycle effect
 */
void NeoPixel_RainbowCycle(uint16_t speed, uint8_t cycles) {
    for(uint8_t j = 0; j < 256 * cycles; j++) {
        for(uint16_t i = 0; i < neo_num_leds; i++) {
            uint32_t color = NeoPixel_Wheel(((i * 256 / neo_num_leds) + j) & 255);
            NeoPixel_SetPixelColor32(i, color);
        }
        NeoPixel_Show();
        Delay_Ms(speed);
    }
}

/**
 * @brief Strobe effect
 */
void NeoPixel_Strobe(uint8_t r, uint8_t g, uint8_t b, uint16_t count, uint16_t flash_delay, uint16_t end_pause) {
    for(uint16_t i = 0; i < count; i++) {
        NeoPixel_Fill(r, g, b);
        NeoPixel_Show();
        Delay_Ms(flash_delay);
        
        NeoPixel_Clear();
        NeoPixel_Show();
        Delay_Ms(flash_delay);
    }
    Delay_Ms(end_pause);
}

/* ========== Advanced Color Utility Functions ========== */

/**
 * @brief Gamma correction
 */
uint32_t NeoPixel_GammaCorrect(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    r = gamma8[r];
    g = gamma8[g];
    b = gamma8[b];
    
    return NeoPixel_Color(r, g, b);
}

/**
 * @brief Color blending
 */
uint32_t NeoPixel_ColorBlend(uint32_t color1, uint32_t color2, uint8_t blend) {
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;
    
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;
    
    uint8_t r = ((r1 * (255 - blend)) + (r2 * blend)) >> 8;
    uint8_t g = ((g1 * (255 - blend)) + (g2 * blend)) >> 8;
    uint8_t b_out = ((b1 * (255 - blend)) + (b2 * blend)) >> 8;
    
    return NeoPixel_Color(r, g, b_out);
}

/**
 * @brief Color interpolation
 */
uint32_t NeoPixel_ColorInterpolate(uint32_t color1, uint32_t color2, uint8_t position) {
    return NeoPixel_ColorBlend(color1, color2, position);
}

/**
 * @brief Dim color by percentage
 */
uint32_t NeoPixel_DimColor(uint32_t color, uint8_t percent) {
    if(percent > 100) percent = 100;
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    r = (r * percent) / 100;
    g = (g * percent) / 100;
    b = (b * percent) / 100;
    
    return NeoPixel_Color(r, g, b);
}

/* ========== Advanced Control Functions ========== */

/**
 * @brief Set pixel range
 */
void NeoPixel_SetPixelRange(uint16_t start, uint16_t end, uint8_t r, uint8_t g, uint8_t b) {
    if(end >= neo_num_leds) end = neo_num_leds - 1;
    
    for(uint16_t i = start; i <= end; i++) {
        NeoPixel_SetPixelColor(i, r, g, b);
    }
}

/**
 * @brief Rotate left
 */
void NeoPixel_RotateLeft(uint16_t positions) {
    if(neo_pixels == NULL || neo_num_leds == 0) return;
    
    positions = positions % neo_num_leds;
    
    for(uint16_t p = 0; p < positions; p++) {
        // Save first pixel
        uint8_t temp_g = neo_pixels[0];
        uint8_t temp_r = neo_pixels[1];
        uint8_t temp_b = neo_pixels[2];
        
        // Shift all pixels left
        for(uint16_t i = 0; i < neo_num_leds - 1; i++) {
            neo_pixels[i * 3 + 0] = neo_pixels[(i + 1) * 3 + 0];
            neo_pixels[i * 3 + 1] = neo_pixels[(i + 1) * 3 + 1];
            neo_pixels[i * 3 + 2] = neo_pixels[(i + 1) * 3 + 2];
        }
        
        // Put first pixel at end
        neo_pixels[(neo_num_leds - 1) * 3 + 0] = temp_g;
        neo_pixels[(neo_num_leds - 1) * 3 + 1] = temp_r;
        neo_pixels[(neo_num_leds - 1) * 3 + 2] = temp_b;
    }
}

/**
 * @brief Rotate right
 */
void NeoPixel_RotateRight(uint16_t positions) {
    if(neo_pixels == NULL || neo_num_leds == 0) return;
    
    positions = positions % neo_num_leds;
    
    for(uint16_t p = 0; p < positions; p++) {
        // Save last pixel
        uint8_t temp_g = neo_pixels[(neo_num_leds - 1) * 3 + 0];
        uint8_t temp_r = neo_pixels[(neo_num_leds - 1) * 3 + 1];
        uint8_t temp_b = neo_pixels[(neo_num_leds - 1) * 3 + 2];
        
        // Shift all pixels right
        for(uint16_t i = neo_num_leds - 1; i > 0; i--) {
            neo_pixels[i * 3 + 0] = neo_pixels[(i - 1) * 3 + 0];
            neo_pixels[i * 3 + 1] = neo_pixels[(i - 1) * 3 + 1];
            neo_pixels[i * 3 + 2] = neo_pixels[(i - 1) * 3 + 2];
        }
        
        // Put last pixel at start
        neo_pixels[0] = temp_g;
        neo_pixels[1] = temp_r;
        neo_pixels[2] = temp_b;
    }
}

/**
 * @brief Fill gradient
 */
void NeoPixel_FillGradient(uint32_t start_color, uint32_t end_color) {
    if(neo_num_leds == 0) return;
    
    for(uint16_t i = 0; i < neo_num_leds; i++) {
        uint8_t position = (i * 255) / (neo_num_leds - 1);
        uint32_t color = NeoPixel_ColorInterpolate(start_color, end_color, position);
        NeoPixel_SetPixelColor32(i, color);
    }
}

/**
 * @brief Set brightness range
 */
void NeoPixel_SetBrightnessRange(uint16_t start, uint16_t end, uint8_t brightness) {
    if(end >= neo_num_leds) end = neo_num_leds - 1;
    if(neo_pixels == NULL) return;
    
    for(uint16_t i = start; i <= end; i++) {
        uint16_t offset = i * 3;
        
        // Apply brightness to existing color
        neo_pixels[offset + 0] = (neo_pixels[offset + 0] * brightness) >> 8;
        neo_pixels[offset + 1] = (neo_pixels[offset + 1] * brightness) >> 8;
        neo_pixels[offset + 2] = (neo_pixels[offset + 2] * brightness) >> 8;
    }
}

/* ========== Non-blocking Effect Framework ========== */

/**
 * @brief Start non-blocking effect
 */
void NeoPixel_StartEffect(NeoPixel_Effect_t* effect, uint8_t type, uint16_t speed, uint32_t param1, uint32_t param2) {
    if(effect == NULL) return;
    
    effect->active = 1;
    effect->type = type;
    effect->speed = speed;
    effect->step = 0;
    effect->param1 = param1;
    effect->param2 = param2;
    effect->last_update = Get_CurrentMs();
}

/**
 * @brief Update non-blocking effect
 */
uint8_t NeoPixel_UpdateEffect(NeoPixel_Effect_t* effect) {
    if(effect == NULL || !effect->active) return 0;
    
    uint32_t current_time = Get_CurrentMs();
    
    // Check if it's time to update
    if((current_time - effect->last_update) < effect->speed) {
        return 0;
    }
    
    effect->last_update = current_time;
    
    // Update based on effect type
    switch(effect->type) {
        case EFFECT_RAINBOW:
            for(uint16_t i = 0; i < neo_num_leds; i++) {
                uint32_t color = NeoPixel_Wheel((i + effect->step) & 255);
                NeoPixel_SetPixelColor32(i, color);
            }
            effect->step++;
            break;
            
        case EFFECT_BREATHING: {
            uint8_t brightness = (uint8_t)(127 + 127 * (effect->step % 256) / 256);
            if(effect->step >= 256) {
                brightness = 255 - brightness;
            }
            NeoPixel_SetBrightness(brightness);
            uint8_t r = (effect->param1 >> 16) & 0xFF;
            uint8_t g = (effect->param1 >> 8) & 0xFF;
            uint8_t b = effect->param1 & 0xFF;
            NeoPixel_Fill(r, g, b);
            effect->step++;
            if(effect->step >= 512) effect->step = 0;
            break;
        }
            
        case EFFECT_COMET: {
            NeoPixel_Clear();
            uint8_t tail_length = (uint8_t)(effect->param1 & 0xFF);
            uint8_t r = (effect->param2 >> 16) & 0xFF;
            uint8_t g = (effect->param2 >> 8) & 0xFF;
            uint8_t b = effect->param2 & 0xFF;
            
            uint16_t pos = effect->step % (neo_num_leds + tail_length);
            for(uint8_t j = 0; j < tail_length; j++) {
                if(pos >= j && (pos - j) < neo_num_leds) {
                    uint8_t brightness = 255 - (j * 255 / tail_length);
                    NeoPixel_SetPixelColor(pos - j, (r * brightness) >> 8, 
                                                     (g * brightness) >> 8, 
                                                     (b * brightness) >> 8);
                }
            }
            effect->step++;
            break;
        }
            
        case EFFECT_SCANNER: {
            NeoPixel_Clear();
            uint8_t eye_size = (uint8_t)(effect->param1 & 0xFF);
            uint8_t r = (effect->param2 >> 16) & 0xFF;
            uint8_t g = (effect->param2 >> 8) & 0xFF;
            uint8_t b = effect->param2 & 0xFF;
            
            uint16_t max_pos = (neo_num_leds - eye_size - 2) * 2;
            uint16_t pos = effect->step % max_pos;
            
            if(pos < (neo_num_leds - eye_size - 2)) {
                // Scan right
                NeoPixel_SetPixelColor(pos, r/10, g/10, b/10);
                for(uint8_t j = 1; j <= eye_size; j++) {
                    NeoPixel_SetPixelColor(pos + j, r, g, b);
                }
                NeoPixel_SetPixelColor(pos + eye_size + 1, r/10, g/10, b/10);
            } else {
                // Scan left
                pos = max_pos - pos;
                NeoPixel_SetPixelColor(pos, r/10, g/10, b/10);
                for(uint8_t j = 1; j <= eye_size; j++) {
                    NeoPixel_SetPixelColor(pos + j, r, g, b);
                }
                NeoPixel_SetPixelColor(pos + eye_size + 1, r/10, g/10, b/10);
            }
            effect->step++;
            break;
        }
            
        case EFFECT_TWINKLE: {
            uint16_t pixel = rand() % neo_num_leds;
            uint32_t color = NeoPixel_Wheel(rand() % 256);
            
            if(effect->step % 2 == 0) {
                NeoPixel_SetPixelColor32(pixel, color);
            } else {
                NeoPixel_SetPixelColor(pixel, 0, 0, 0);
            }
            effect->step++;
            break;
        }
            
        case EFFECT_COLOR_CHASE: {
            NeoPixel_Clear();
            uint8_t num_colors = (uint8_t)(effect->param1 & 0xFF);
            uint32_t* colors = (uint32_t*)effect->param2;
            
            if(colors != NULL && num_colors > 0) {
                uint8_t color_idx = (effect->step / neo_num_leds) % num_colors;
                uint16_t pos = effect->step % neo_num_leds;
                NeoPixel_SetPixelColor32(pos, colors[color_idx]);
            }
            effect->step++;
            break;
        }
            
        default:
            return 0;
    }
    
    return 1;  // Effect was updated
}

/**
 * @brief Stop effect
 */
void NeoPixel_StopEffect(NeoPixel_Effect_t* effect) {
    if(effect == NULL) return;
    effect->active = 0;
}

/* ========== Utility Functions ========== */

/**
 * @brief Get number of LEDs
 */
uint16_t NeoPixel_GetNumLEDs(void) {
    return neo_num_leds;
}

/**
 * @brief Get current brightness
 */
uint8_t NeoPixel_GetBrightness(void) {
    return neo_brightness;
}
