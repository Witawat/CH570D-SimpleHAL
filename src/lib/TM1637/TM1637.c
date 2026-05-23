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
 * @return None
 * @note Provides a ~5µs delay for the TM1637 two-wire protocol timing.
 *       Uses the system Delay_Us function.
 */
static inline void TM1637_DelayUs(void) {
    Delay_Us(5);
}

/**
 * @brief Set CLK pin state
 * @param handle Pointer to TM1637_Handle
 * @param state  Pin state (HIGH or LOW)
 * @return None
 * @note Writes the clock pin via digitalWrite.
 */
static inline void TM1637_SetCLK(TM1637_Handle* handle, uint8_t state) {
    digitalWrite(handle->clk_pin, state);
}

/**
 * @brief Set DIO pin state
 * @param handle Pointer to TM1637_Handle
 * @param state  Pin state (HIGH or LOW)
 * @return None
 * @note Writes the data pin via digitalWrite.
 */
static inline void TM1637_SetDIO(TM1637_Handle* handle, uint8_t state) {
    digitalWrite(handle->dio_pin, state);
}

/**
 * @brief Read DIO pin state
 * @param handle Pointer to TM1637_Handle
 * @return Current logic level of the DIO pin (HIGH or LOW)
 * @note Reads the data pin via digitalRead. Used during ACK polling
 *       after byte writes.
 */
static inline uint8_t TM1637_ReadDIO(TM1637_Handle* handle) {
    return digitalRead(handle->dio_pin);
}

/**
 * @brief Generate START condition
 * @param handle Pointer to TM1637_Handle
 * @return None
 * @note START: DIO goes LOW while CLK is HIGH. Sets up the TM1637 for
 *       command or data transfer.
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
 * @param handle Pointer to TM1637_Handle
 * @return None
 * @note STOP: CLK goes HIGH, then DIO goes HIGH. Terminates the current
 *       transfer.
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
 * @param handle Pointer to TM1637_Handle
 * @param data   Byte to transmit (LSB first)
 * @return true if ACK received from TM1637, false otherwise
 * @note Shifts out 8 bits LSB-first on DIO, clocked by CLK. After the
 *       8th bit, releases DIO (input pull-up) and clocks a 9th cycle to
 *       read the ACK bit from the TM1637. Restores DIO to output after ACK.
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
 * @param handle Pointer to TM1637_Handle
 * @param cmd    Command byte to send
 * @return None
 * @note Sends a command frame: START + command byte + STOP. Used for
 *       display control, data mode, and address commands.
 */
static void TM1637_WriteCommand(TM1637_Handle* handle, uint8_t cmd) {
    TM1637_Start(handle);
    TM1637_WriteByte(handle, cmd);
    TM1637_Stop(handle);
}

/**
 * @brief Write data to display
 * @param handle Pointer to TM1637_Handle
 * @param addr   Starting address (typically 0x00 for first digit)
 * @param data   Pointer to segment data array
 * @param length Number of bytes to write
 * @return None
 * @note Sends an address command followed by data bytes: START + address
 *       byte + data bytes + STOP. Uses auto-increment addressing mode
 *       for consecutive digits.
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

/**
 * @brief Initialize the TM1637 display
 * @param clk_pin    GPIO pin for CLK
 * @param dio_pin    GPIO pin for DIO
 * @param num_digits Number of digits (1–TM1637_MAX_DIGITS, clamped)
 * @return Pointer to the TM1637_Handle singleton, or NULL on failure
 * @note Configures CLK and DIO as outputs, sets both HIGH. Sends data
 *       mode command (auto-increment), clears display, sets default
 *       brightness (5), and enables display. Uses a single static handle.
 */
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

/**
 * @brief Set display brightness
 * @param handle     Pointer to TM1637_Handle
 * @param brightness Brightness level (0–7, clamped to TM1637_BRIGHTNESS_MAX)
 * @return None
 * @note Sends a display control command combining brightness with the
 *       display on/off flag. If the display is off, brightness setting
 *       is still stored but the display remains off.
 */
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

/**
 * @brief Turn display on or off
 * @param handle Pointer to TM1637_Handle
 * @param on     true to turn on, false to turn off
 * @return None
 * @note Updates the display_on flag and reapplies brightness setting
 *       with the new on/off state. Turning off blanks all segments
 *       but preserves the buffer contents.
 */
void TM1637_DisplayControl(TM1637_Handle* handle, bool on) {
    handle->display_on = on;
    TM1637_SetBrightness(handle, handle->brightness);
}

/**
 * @brief Clear the display (all segments off)
 * @param handle Pointer to TM1637_Handle
 * @return None
 * @note Zeros out the internal buffer and updates the display.
 */
void TM1637_Clear(TM1637_Handle* handle) {
    memset(handle->buffer, 0, handle->num_digits);
    TM1637_Update(handle);
}

/* ========== Basic Display Functions ========== */

/**
 * @brief Display an integer number
 * @param handle       Pointer to TM1637_Handle
 * @param number       Integer to display (-999 to 9999 range typical)
 * @param leading_zero If true, leading digits show 0 instead of blank
 * @return None
 * @note Converts the number to digit segments, right-aligned. Negative
 *       numbers show a minus sign in the leftmost position. Digits beyond
 *       the display width are truncated.
 */
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

/**
 * @brief Display a floating-point number
 * @param handle         Pointer to TM1637_Handle
 * @param number         Float value to display
 * @param decimal_places Number of decimal places (0–3, clamped)
 * @return None
 * @note Multiplies by 10^decimal_places, converts to integer, displays with
 *       decimal point at the appropriate digit. Shows minus sign for negative
 *       values. Truncates if the result exceeds display capacity.
 */
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

/**
 * @brief Display a hexadecimal number
 * @param handle       Pointer to TM1637_Handle
 * @param number       16-bit value to display in hex
 * @param leading_zero If true, leading hex digits show 0 instead of blank
 * @return None
 * @note Converts nibbles to digit (0–9) or hex segment (A–F). Right-aligned.
 *       Uses HEX_SEGMENTS for A–F letters. Truncates to display width.
 */
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

/**
 * @brief Display a single digit at a specific position
 * @param handle   Pointer to TM1637_Handle
 * @param position Digit position (0 = leftmost, up to num_digits-1)
 * @param digit    Digit value (0–9)
 * @param show_dp  If true, turn on the decimal point at this position
 * @return None
 * @note Sets the segment pattern for the given digit and optionally enables
 *       the decimal point. No-op if position is out of range or digit > 9.
 */
void TM1637_DisplayDigit(TM1637_Handle* handle, uint8_t position, uint8_t digit, bool show_dp) {
    if (position >= handle->num_digits || digit > 9) return;
    
    handle->buffer[position] = DIGIT_SEGMENTS[digit];
    if (show_dp) {
        handle->buffer[position] |= SEG_DP;
    }
    
    TM1637_Update(handle);
}

/**
 * @brief Display raw segment data at a specific position
 * @param handle   Pointer to TM1637_Handle
 * @param position Digit position (0 = leftmost)
 * @param segments Raw segment bitmask to display
 * @return None
 * @note Allows arbitrary segment patterns (e.g., custom symbols).
 *       No-op if position is out of range.
 */
void TM1637_DisplayRaw(TM1637_Handle* handle, uint8_t position, uint8_t segments) {
    if (position >= handle->num_digits) return;
    
    handle->buffer[position] = segments;
    TM1637_Update(handle);
}

/* ========== Character Display Functions ========== */

/**
 * @brief Convert a character to its 7-segment pattern
 * @param ch Character to convert (0–9, A–F, a–f, and special chars)
 * @return Segment bitmask for the character, or 0x00 if unrecognized
 * @note Supports: 0–9 (DIGIT_SEGMENTS), A–F/a–f (HEX_SEGMENTS),
 *       space, minus, underscore, H, L, P, U, r, o, n, t.
 */
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

/**
 * @brief Display a character at a specific position
 * @param handle   Pointer to TM1637_Handle
 * @param position Digit position (0 = leftmost)
 * @param ch       Character to display (via TM1637_CharToSegment)
 * @param show_dp  If true, enable decimal point at this position
 * @return None
 * @note Converts character to segment pattern and writes to buffer.
 *       No-op if position is out of range.
 */
void TM1637_DisplayChar(TM1637_Handle* handle, uint8_t position, char ch, bool show_dp) {
    if (position >= handle->num_digits) return;
    
    handle->buffer[position] = TM1637_CharToSegment(ch);
    if (show_dp) {
        handle->buffer[position] |= SEG_DP;
    }
    
    TM1637_Update(handle);
}

/**
 * @brief Display a string starting at a given position
 * @param handle    Pointer to TM1637_Handle
 * @param text      Null-terminated string to display
 * @param start_pos Starting digit position (0 = leftmost)
 * @return None
 * @brief Writes characters from the string to the display buffer starting
 *        at start_pos. Stops at null terminator or when all digits are filled.
 *        Unused positions remain as previously set.
 */
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

/**
 * @brief Enable or disable display blinking
 * @param handle     Pointer to TM1637_Handle
 * @param enable     true to enable blinking, false to disable
 * @param blink_rate Blink interval in milliseconds
 * @return None
 * @note Sets blink state variables. Blinking toggles the display on/off
 *       at the specified rate via TM1637_UpdateBlink, which must be called
 *       periodically.
 */
void TM1637_SetBlink(TM1637_Handle* handle, bool enable, uint16_t blink_rate) {
    blink_enabled = enable;
    blink_rate_ms = blink_rate;
    blink_last_update = Get_CurrentMs();
    blink_state = true;
}

/**
 * @brief Update blink state (call periodically)
 * @param handle Pointer to TM1637_Handle
 * @return None
 * @note Toggles display on/off when the blink interval elapses. No-op if
 *       blinking is not enabled. Must be called at a rate faster than the
 *       blink_rate for smooth blinking.
 */
void TM1637_UpdateBlink(TM1637_Handle* handle) {
    if (!blink_enabled) return;
    
    uint32_t current_ms = Get_CurrentMs();
    if (ELAPSED_TIME(blink_last_update, current_ms) >= blink_rate_ms) {
        blink_state = !blink_state;
        blink_last_update = current_ms;
        
        TM1637_DisplayControl(handle, blink_state);
    }
}

/**
 * @brief Scroll text across the display (blocking)
 * @param handle       Pointer to TM1637_Handle
 * @param text         Null-terminated text to scroll
 * @param scroll_delay Delay in milliseconds between scroll steps
 * @return None
 * @brief Shifts the text from right to left across the display. The text
 *        scrolls until the last character exits the left side. Blocks
 *        during the entire scroll animation.
 */
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

/**
 * @brief Start non-blocking scroll animation
 * @param handle       Pointer to TM1637_Handle
 * @param text         Null-terminated text to scroll
 * @param scroll_delay Delay in milliseconds between scroll steps
 * @return None
 * @brief Initializes scroll state variables. Actual scrolling is driven
 *        by TM1637_UpdateScroll, which must be called periodically.
 *        Copies the text to an internal buffer (max 63 chars).
 */
void TM1637_StartScroll(TM1637_Handle* handle, const char* text, uint16_t scroll_delay) {
    if (text == NULL) return;
    
    strncpy(scroll_text, text, sizeof(scroll_text) - 1);
    scroll_text[sizeof(scroll_text) - 1] = '\0';
    
    scroll_active = true;
    scroll_position = 0;
    scroll_delay_ms = scroll_delay;
    scroll_last_update = Get_CurrentMs();
}

/**
 * @brief Update non-blocking scroll animation (call periodically)
 * @param handle Pointer to TM1637_Handle
 * @return true if scrolling is still active, false when complete
 * @note Advances the scroll by one position when the delay elapses.
 *       Returns false when all characters have scrolled off the display.
 */
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

/**
 * @brief Stop the non-blocking scroll animation
 * @param handle Pointer to TM1637_Handle
 * @return None
 * @brief Clears the scroll_active flag. The display shows whatever was
 *        last written to the buffer.
 */
void TM1637_StopScroll(TM1637_Handle* handle) {
    scroll_active = false;
}

/**
 * @brief Play a frame-based animation (blocking)
 * @param handle      Pointer to TM1637_Handle
 * @param frames      Array of frame data (each frame is TM1637_MAX_DIGITS bytes)
 * @param num_frames  Number of frames in the animation
 * @param frame_delay Delay in milliseconds between frames
 * @param repeat      Number of times to repeat (0 = infinite)
 * @return None
 * @brief Copies each frame to the display buffer and updates the display.
 *        Blocks during the entire animation. Supports infinite looping
 *        when repeat is 0.
 */
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

/**
 * @brief Show or hide the colon (typically at position 1)
 * @param handle Pointer to TM1637_Handle
 * @param show   true to enable colon, false to disable
 * @return None
 * @brief Sets or clears the decimal point segment on digit position 1
 *        (between digits 1 and 2 on a 4-digit display). No-op if the
 *        display has fewer than 4 digits.
 */
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

/**
 * @brief Display time in HH:MM format
 * @param handle     Pointer to TM1637_Handle
 * @param hours      Hours value (0–23, clamped)
 * @param minutes    Minutes value (0–59, clamped)
 * @param show_colon If true, enable the colon separator
 * @return None
 * @brief Formats hours and minutes across 4 digits (HHMM). Requires at
 *        least 4 digits. Colon is shown at position 1 when show_colon is true.
 */
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

/**
 * @brief Send the internal buffer to the display hardware
 * @param handle Pointer to TM1637_Handle
 * @return None
 * @brief Writes all digit segment data starting at address 0x00 using
 *        TM1637_WriteData. Called internally by most display functions.
 */
void TM1637_Update(TM1637_Handle* handle) {
    TM1637_WriteData(handle, 0x00, handle->buffer, handle->num_digits);
}

/**
 * @brief Convert a numeric digit to its 7-segment pattern
 * @param digit Digit value (0–9)
 * @return Segment bitmask, or 0x00 if digit > 9
 * @note Looks up the digit in the DIGIT_SEGMENTS table.
 */
uint8_t TM1637_DigitToSegment(uint8_t digit) {
    if (digit > 9) return 0;
    return DIGIT_SEGMENTS[digit];
}
