/**
 * @file SimpleTM1637.c
 * @brief TM1637 7-Segment Display Driver Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "TM1637.h"
#include <string.h>
#include <stdlib.h>

/* ========== Segment Encoding Tables ========== */

/**
 * @brief Segment patterns for digits 0-9
 */
static const uint8_t DIGIT_SEGMENTS[] = {
    0x3F,  // 0: ABCDEF
    0x06,  // 1: BC
    0x5B,  // 2: ABDEG
    0x4F,  // 3: ABCDG
    0x66,  // 4: BCFG
    0x6D,  // 5: ACDFG
    0x7D,  // 6: ACDEFG
    0x07,  // 7: ABC
    0x7F,  // 8: ABCDEFG
    0x6F   // 9: ABCDFG
};

/**
 * @brief Segment patterns for hex digits A-F
 */
static const uint8_t HEX_SEGMENTS[] = {
    0x77,  // A: ABCEFG
    0x7C,  // b: CDEFG
    0x39,  // C: ADEF
    0x5E,  // d: BCDEG
    0x79,  // E: ADEFG
    0x71   // F: AEFG
};

/**
 * @brief Segment patterns for special characters
 * @note Used by TM1637_CharToSegment() function
 */
__attribute__((unused))
static const uint8_t CHAR_SEGMENTS[] = {
    0x00,  // ' ' (space)
    0x40,  // '-' (minus)
    0x08,  // '_' (underscore)
    0x76,  // 'H'
    0x38,  // 'L'
    0x73,  // 'P'
    0x3E,  // 'U'
    0x50,  // 'r'
    0x5C,  // 'o'
    0x54,  // 'n'
    0x78,  // 't'
};

/* ========== Static Variables ========== */

static TM1637_Handle tm1637_instance;
static bool scroll_active = false;
static char scroll_text[64];
static uint8_t scroll_position = 0;
static uint32_t scroll_last_update = 0;
static uint16_t scroll_delay_ms = 0;

static bool blink_enabled = false;
static bool blink_state = false;
static uint32_t blink_last_update = 0;
static uint16_t blink_rate_ms = 0;

/* ========== Low-Level Protocol Functions ========== */

/**
 * @brief Delay for TM1637 timing (approximately 5us)
 */
static inline void TM1637_DelayUs(void) {
    Delay_Us(5);
}

/**
 * @brief Set CLK pin state
 */
static inline void TM1637_SetCLK(TM1637_Handle* handle, uint8_t state) {
    digitalWrite(handle->clk_pin, state);
}

/**
 * @brief Set DIO pin state
 */
static inline void TM1637_SetDIO(TM1637_Handle* handle, uint8_t state) {
    digitalWrite(handle->dio_pin, state);
}

/**
 * @brief Read DIO pin state
 */
static inline uint8_t TM1637_ReadDIO(TM1637_Handle* handle) {
    return digitalRead(handle->dio_pin);
}

/**
 * @brief Generate START condition
 */
static void TM1637_Start(TM1637_Handle* handle) {
    TM1637_SetDIO(handle, HIGH);
    TM1637_SetCLK(handle, HIGH);
    TM1637_DelayUs();
    TM1637_SetDIO(handle, LOW);
    TM1637_DelayUs();
}

/**
 * @brief Generate STOP condition
 */
static void TM1637_Stop(TM1637_Handle* handle) {
    TM1637_SetCLK(handle, LOW);
    TM1637_DelayUs();
    TM1637_SetDIO(handle, LOW);
    TM1637_DelayUs();
    TM1637_SetCLK(handle, HIGH);
    TM1637_DelayUs();
    TM1637_SetDIO(handle, HIGH);
    TM1637_DelayUs();
}

/**
 * @brief Write a byte to TM1637
 * @return ACK status (true if ACK received)
 */
static bool TM1637_WriteByte(TM1637_Handle* handle, uint8_t data) {
    // Write 8 bits
    for (uint8_t i = 0; i < 8; i++) {
        TM1637_SetCLK(handle, LOW);
        TM1637_DelayUs();
        
        if (data & 0x01) {
            TM1637_SetDIO(handle, HIGH);
        } else {
            TM1637_SetDIO(handle, LOW);
        }
        TM1637_DelayUs();
        
        TM1637_SetCLK(handle, HIGH);
        TM1637_DelayUs();
        
        data >>= 1;
    }
    
    // Wait for ACK
    TM1637_SetCLK(handle, LOW);
    pinMode(handle->dio_pin, PIN_MODE_INPUT_PULLUP);
    TM1637_DelayUs();
    
    TM1637_SetCLK(handle, HIGH);
    TM1637_DelayUs();
    bool ack = (TM1637_ReadDIO(handle) == LOW);
    
    TM1637_SetCLK(handle, LOW);
    pinMode(handle->dio_pin, PIN_MODE_OUTPUT);
    TM1637_DelayUs();
    
    return ack;
}

/**
 * @brief Write command to TM1637
 */
static void TM1637_WriteCommand(TM1637_Handle* handle, uint8_t cmd) {
    TM1637_Start(handle);
    TM1637_WriteByte(handle, cmd);
    TM1637_Stop(handle);
}

/**
 * @brief Write data to display
 */
static void TM1637_WriteData(TM1637_Handle* handle, uint8_t addr, const uint8_t* data, uint8_t length) {
    TM1637_Start(handle);
    TM1637_WriteByte(handle, TM1637_CMD_ADDRESS | addr);
    
    for (uint8_t i = 0; i < length; i++) {
        TM1637_WriteByte(handle, data[i]);
    }
    
    TM1637_Stop(handle);
}

/* ========== Initialization Functions ========== */

TM1637_Handle* TM1637_Init(uint8_t clk_pin, uint8_t dio_pin, uint8_t num_digits) {
    TM1637_Handle* handle = &tm1637_instance;
    
    // Validate parameters
    if (num_digits > TM1637_MAX_DIGITS) {
        num_digits = TM1637_MAX_DIGITS;
    }
    
    // Initialize handle
    handle->clk_pin = clk_pin;
    handle->dio_pin = dio_pin;
    handle->num_digits = num_digits;
    handle->brightness = 5;
    handle->display_on = true;
    
    // Clear buffer
    memset(handle->buffer, 0, TM1637_MAX_DIGITS);
    
    // Configure GPIO pins
    pinMode(clk_pin, PIN_MODE_OUTPUT);
    pinMode(dio_pin, PIN_MODE_OUTPUT);
    
    TM1637_SetCLK(handle, HIGH);
    TM1637_SetDIO(handle, HIGH);
    
    // Initialize display
    TM1637_WriteCommand(handle, TM1637_CMD_DATA | 0x00);  // Auto increment mode
    TM1637_Clear(handle);
    TM1637_SetBrightness(handle, handle->brightness);
    TM1637_DisplayControl(handle, true);
    
    return handle;
}

void TM1637_SetBrightness(TM1637_Handle* handle, uint8_t brightness) {
    if (brightness > TM1637_BRIGHTNESS_MAX) {
        brightness = TM1637_BRIGHTNESS_MAX;
    }
    
    handle->brightness = brightness;
    
    uint8_t cmd = TM1637_CMD_DISPLAY | brightness;
    if (handle->display_on) {
        cmd |= 0x08;
    }
    
    TM1637_WriteCommand(handle, cmd);
}

void TM1637_DisplayControl(TM1637_Handle* handle, bool on) {
    handle->display_on = on;
    TM1637_SetBrightness(handle, handle->brightness);
}

void TM1637_Clear(TM1637_Handle* handle) {
    memset(handle->buffer, 0, handle->num_digits);
    TM1637_Update(handle);
}

/* ========== Basic Display Functions ========== */

void TM1637_DisplayNumber(TM1637_Handle* handle, int16_t number, bool leading_zero) {
    bool negative = (number < 0);
    if (negative) {
        number = -number;
    }
    
    // Clear buffer
    memset(handle->buffer, 0, handle->num_digits);
    
    // Convert number to digits
    uint8_t pos = handle->num_digits - 1;
    
    do {
        uint8_t digit = number % 10;
        handle->buffer[pos] = DIGIT_SEGMENTS[digit];
        number /= 10;
        
        if (pos == 0) break;
        pos--;
    } while (number > 0 || (leading_zero && pos < handle->num_digits - 1));
    
    // Add minus sign if negative
    if (negative && pos > 0) {
        handle->buffer[pos - 1] = 0x40;  // Minus sign
    }
    
    TM1637_Update(handle);
}

void TM1637_DisplayFloat(TM1637_Handle* handle, float number, uint8_t decimal_places) {
    if (decimal_places > 3) decimal_places = 3;
    
    // Convert to integer by multiplying
    int32_t multiplier = 1;
    for (uint8_t i = 0; i < decimal_places; i++) {
        multiplier *= 10;
    }
    
    int32_t int_value = (int32_t)(number * multiplier);
    bool negative = (int_value < 0);
    if (negative) {
        int_value = -int_value;
    }
    
    // Clear buffer
    memset(handle->buffer, 0, handle->num_digits);
    
    // Fill digits from right to left
    uint8_t pos = handle->num_digits - 1;
    uint8_t digit_count = 0;
    
    do {
        uint8_t digit = int_value % 10;
        handle->buffer[pos] = DIGIT_SEGMENTS[digit];
        
        // Add decimal point
        if (digit_count == decimal_places - 1 && decimal_places > 0) {
            handle->buffer[pos] |= SEG_DP;
        }
        
        int_value /= 10;
        digit_count++;
        
        if (pos == 0) break;
        pos--;
    } while (int_value > 0 || digit_count <= decimal_places);
    
    // Add minus sign if negative
    if (negative && pos > 0) {
        handle->buffer[pos - 1] = 0x40;
    }
    
    TM1637_Update(handle);
}

void TM1637_DisplayHex(TM1637_Handle* handle, uint16_t number, bool leading_zero) {
    memset(handle->buffer, 0, handle->num_digits);
    
    uint8_t pos = handle->num_digits - 1;
    
    do {
        uint8_t digit = number & 0x0F;
        
        if (digit < 10) {
            handle->buffer[pos] = DIGIT_SEGMENTS[digit];
        } else {
            handle->buffer[pos] = HEX_SEGMENTS[digit - 10];
        }
        
        number >>= 4;
        
        if (pos == 0) break;
        pos--;
    } while (number > 0 || (leading_zero && pos < handle->num_digits - 1));
    
    TM1637_Update(handle);
}

void TM1637_DisplayDigit(TM1637_Handle* handle, uint8_t position, uint8_t digit, bool show_dp) {
    if (position >= handle->num_digits || digit > 9) return;
    
    handle->buffer[position] = DIGIT_SEGMENTS[digit];
    if (show_dp) {
        handle->buffer[position] |= SEG_DP;
    }
    
    TM1637_Update(handle);
}

void TM1637_DisplayRaw(TM1637_Handle* handle, uint8_t position, uint8_t segments) {
    if (position >= handle->num_digits) return;
    
    handle->buffer[position] = segments;
    TM1637_Update(handle);
}

/* ========== Character Display Functions ========== */

uint8_t TM1637_CharToSegment(char ch) {
    if (ch >= '0' && ch <= '9') {
        return DIGIT_SEGMENTS[ch - '0'];
    } else if (ch >= 'A' && ch <= 'F') {
        return HEX_SEGMENTS[ch - 'A'];
    } else if (ch >= 'a' && ch <= 'f') {
        return HEX_SEGMENTS[ch - 'a'];
    }
    
    // Special characters
    switch (ch) {
        case ' ': return 0x00;
        case '-': return 0x40;
        case '_': return 0x08;
        case 'H': case 'h': return 0x76;
        case 'L': case 'l': return 0x38;
        case 'P': case 'p': return 0x73;
        case 'U': case 'u': return 0x3E;
        case 'r': case 'R': return 0x50;
        case 'o': case 'O': return 0x5C;
        case 'n': case 'N': return 0x54;
        case 't': case 'T': return 0x78;
        default: return 0x00;
    }
}

void TM1637_DisplayChar(TM1637_Handle* handle, uint8_t position, char ch, bool show_dp) {
    if (position >= handle->num_digits) return;
    
    handle->buffer[position] = TM1637_CharToSegment(ch);
    if (show_dp) {
        handle->buffer[position] |= SEG_DP;
    }
    
    TM1637_Update(handle);
}

void TM1637_DisplayString(TM1637_Handle* handle, const char* text, uint8_t start_pos) {
    if (text == NULL) return;
    
    uint8_t len = strlen(text);
    uint8_t pos = start_pos;
    
    for (uint8_t i = 0; i < len && pos < handle->num_digits; i++, pos++) {
        handle->buffer[pos] = TM1637_CharToSegment(text[i]);
    }
    
    TM1637_Update(handle);
}

/* ========== Advanced Display Functions ========== */

void TM1637_SetBlink(TM1637_Handle* handle, bool enable, uint16_t blink_rate) {
    blink_enabled = enable;
    blink_rate_ms = blink_rate;
    blink_last_update = Get_CurrentMs();
    blink_state = true;
}

void TM1637_UpdateBlink(TM1637_Handle* handle) {
    if (!blink_enabled) return;
    
    uint32_t current_ms = Get_CurrentMs();
    if (ELAPSED_TIME(blink_last_update, current_ms) >= blink_rate_ms) {
        blink_state = !blink_state;
        blink_last_update = current_ms;
        
        TM1637_DisplayControl(handle, blink_state);
    }
}

void TM1637_ScrollText(TM1637_Handle* handle, const char* text, uint16_t scroll_delay) {
    if (text == NULL) return;
    
    uint8_t text_len = strlen(text);
    uint8_t total_positions = text_len + handle->num_digits;
    
    for (uint8_t scroll_pos = 0; scroll_pos < total_positions; scroll_pos++) {
        memset(handle->buffer, 0, handle->num_digits);
        
        for (uint8_t i = 0; i < handle->num_digits; i++) {
            int16_t text_index = scroll_pos - handle->num_digits + i + 1;
            if (text_index >= 0 && text_index < text_len) {
                handle->buffer[i] = TM1637_CharToSegment(text[text_index]);
            }
        }
        
        TM1637_Update(handle);
        Delay_Ms(scroll_delay);
    }
}

void TM1637_StartScroll(TM1637_Handle* handle, const char* text, uint16_t scroll_delay) {
    if (text == NULL) return;
    
    strncpy(scroll_text, text, sizeof(scroll_text) - 1);
    scroll_text[sizeof(scroll_text) - 1] = '\0';
    
    scroll_active = true;
    scroll_position = 0;
    scroll_delay_ms = scroll_delay;
    scroll_last_update = Get_CurrentMs();
}

bool TM1637_UpdateScroll(TM1637_Handle* handle) {
    if (!scroll_active) return false;
    
    uint32_t current_ms = Get_CurrentMs();
    if (ELAPSED_TIME(scroll_last_update, current_ms) < scroll_delay_ms) {
        return true;
    }
    
    scroll_last_update = current_ms;
    
    uint8_t text_len = strlen(scroll_text);
    uint8_t total_positions = text_len + handle->num_digits;
    
    if (scroll_position >= total_positions) {
        scroll_active = false;
        return false;
    }
    
    memset(handle->buffer, 0, handle->num_digits);
    
    for (uint8_t i = 0; i < handle->num_digits; i++) {
        int16_t text_index = scroll_position - handle->num_digits + i + 1;
        if (text_index >= 0 && text_index < text_len) {
            handle->buffer[i] = TM1637_CharToSegment(scroll_text[text_index]);
        }
    }
    
    TM1637_Update(handle);
    scroll_position++;
    
    return true;
}

void TM1637_StopScroll(TM1637_Handle* handle) {
    scroll_active = false;
}

void TM1637_PlayAnimation(TM1637_Handle* handle, const uint8_t frames[][TM1637_MAX_DIGITS], 
                          uint8_t num_frames, uint16_t frame_delay, uint8_t repeat) {
    uint8_t count = 0;
    
    do {
        for (uint8_t frame = 0; frame < num_frames; frame++) {
            memcpy(handle->buffer, frames[frame], handle->num_digits);
            TM1637_Update(handle);
            Delay_Ms(frame_delay);
        }
        count++;
    } while (repeat == 0 || count < repeat);
}

void TM1637_SetColon(TM1637_Handle* handle, bool show) {
    // Colon is typically at position 1 (between digit 1 and 2)
    if (handle->num_digits >= 4) {
        if (show) {
            handle->buffer[1] |= SEG_DP;
        } else {
            handle->buffer[1] &= ~SEG_DP;
        }
        TM1637_Update(handle);
    }
}

void TM1637_DisplayTime(TM1637_Handle* handle, uint8_t hours, uint8_t minutes, bool show_colon) {
    if (handle->num_digits < 4) return;
    
    // Validate input
    if (hours > 23) hours = 23;
    if (minutes > 59) minutes = 59;
    
    // Display hours
    handle->buffer[0] = DIGIT_SEGMENTS[hours / 10];
    handle->buffer[1] = DIGIT_SEGMENTS[hours % 10];
    
    // Display minutes
    handle->buffer[2] = DIGIT_SEGMENTS[minutes / 10];
    handle->buffer[3] = DIGIT_SEGMENTS[minutes % 10];
    
    // Colon
    if (show_colon) {
        handle->buffer[1] |= SEG_DP;
    }
    
    TM1637_Update(handle);
}

/* ========== Utility Functions ========== */

void TM1637_Update(TM1637_Handle* handle) {
    TM1637_WriteData(handle, 0x00, handle->buffer, handle->num_digits);
}

uint8_t TM1637_DigitToSegment(uint8_t digit) {
    if (digit > 9) return 0;
    return DIGIT_SEGMENTS[digit];
}
