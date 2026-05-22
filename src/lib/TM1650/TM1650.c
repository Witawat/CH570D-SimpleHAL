/**
 * @file TM1650.c
 * @brief TM1650 I2C 7-Segment Display + Key Scan Driver Implementation
 * @version 1.0
 * @date 2026-05-15
 */

#include "TM1650.h"
#include <string.h>

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

/* ========== Private: I2C Helpers ========== */

/**
 * @brief Write a single command byte to TM1650
 */
static TM1650_Status _write_cmd(TM1650_Handle* handle, uint8_t cmd) {
    if (I2C_Write(handle->i2c_addr, &cmd, 1) != I2C_OK) {
        return TM1650_ERROR_I2C;
    }
    return TM1650_OK;
}

/**
 * @brief Write display control command (brightness + on/off + mode)
 */
static TM1650_Status _write_display_ctrl(TM1650_Handle* handle) {
    uint8_t cmd = 0x48;  /* Display control register base */

    if (handle->display_on) {
        cmd |= 0x08;      /* Bit 3: Display ON */
    }
    cmd |= (handle->brightness & 0x07);   /* Bits 2-0: Brightness */
    if (handle->mode == TM1650_MODE_7SEG) {
        cmd |= 0x10;      /* Bit 4: 7-segment mode */
    }

    return _write_cmd(handle, cmd);
}

/**
 * @brief Write segment buffer to display using auto-increment mode
 */
static TM1650_Status _write_segments(TM1650_Handle* handle) {
    /*
     * Protocol:
     *   Byte 0: 0x40 — auto-increment data write mode
     *   Byte 1: 0xC0 — start address (grid 1)
     *   Bytes 2-5: segment data for grids 1-4
     */
    uint8_t data[6];
    data[0] = 0x40;  /* Auto-increment mode */
    data[1] = 0xC0;  /* Start at grid 1 address */
    data[2] = handle->buffer[0];
    data[3] = handle->buffer[1];
    data[4] = handle->buffer[2];
    data[5] = handle->buffer[3];

    if (I2C_Write(handle->i2c_addr, data, 6) != I2C_OK) {
        return TM1650_ERROR_I2C;
    }
    return TM1650_OK;
}

/* ========== Public: Initialization ========== */

TM1650_Status TM1650_Init(TM1650_Handle* handle, uint8_t i2c_addr) {
    if (handle == NULL) {
        return TM1650_ERROR_PARAM;
    }

    /* Populate handle with defaults */
    handle->i2c_addr      = i2c_addr;
    handle->brightness    = 5;                  /* Default: mid brightness */
    handle->display_on    = true;
    handle->mode          = TM1650_MODE_8SEG;   /* Default: 8-segment mode */
    handle->raw_keys      = 0;
    handle->last_key_mask = 0;
    handle->press_start_ms = 0;
    handle->is_pressed    = 0;
    handle->initialized   = 0;

    /* Clear display buffer */
    memset(handle->buffer, 0, TM1650_NUM_DIGITS);

    /* Check device presence on I2C bus */
    if (!I2C_IsDeviceReady(i2c_addr)) {
        return TM1650_ERROR_I2C;
    }

    /* Initialize TM1650:
     * 1. Set display mode + brightness + ON
     * 2. Clear all segments
     */
    TM1650_Status status;
    status = _write_display_ctrl(handle);
    if (status != TM1650_OK) return status;

    status = _write_segments(handle);
    if (status != TM1650_OK) return status;

    handle->initialized = 1;
    return TM1650_OK;
}

TM1650_Status TM1650_SetBrightness(TM1650_Handle* handle, uint8_t brightness) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;
    if (brightness > TM1650_BRIGHTNESS_MAX) {
        brightness = TM1650_BRIGHTNESS_MAX;
    }

    handle->brightness = brightness;
    return _write_display_ctrl(handle);
}

TM1650_Status TM1650_DisplayOn(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    handle->display_on = true;
    return _write_display_ctrl(handle);
}

TM1650_Status TM1650_DisplayOff(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    handle->display_on = false;
    return _write_display_ctrl(handle);
}

TM1650_Status TM1650_Clear(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    memset(handle->buffer, 0, TM1650_NUM_DIGITS);
    return _write_segments(handle);
}

TM1650_Status TM1650_SetMode(TM1650_Handle* handle, TM1650_Mode mode) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    handle->mode = mode;

    /* Clear buffer on mode change */
    memset(handle->buffer, 0, TM1650_NUM_DIGITS);

    /* Apply new mode */
    TM1650_Status status = _write_display_ctrl(handle);
    if (status != TM1650_OK) return status;

    return _write_segments(handle);
}

/* ========== Public: Display Data ========== */

TM1650_Status TM1650_DisplayNumber(TM1650_Handle* handle, int16_t number, bool leading_zero) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    bool negative = (number < 0);
    if (negative) {
        number = -number;
    }

    /* Clear buffer */
    memset(handle->buffer, 0, TM1650_NUM_DIGITS);

    /* Convert number to digits (right-to-left) */
    int8_t pos = TM1650_NUM_DIGITS - 1;
    uint8_t digit_count = 0;

    if (number == 0 && leading_zero) {
        /* Show "0000" */
        for (uint8_t i = 0; i < TM1650_NUM_DIGITS; i++) {
            handle->buffer[i] = DIGIT_SEGMENTS[0];
        }
    } else {
        do {
            uint8_t digit = number % 10;
            handle->buffer[pos] = DIGIT_SEGMENTS[digit];
            number /= 10;
            digit_count++;

            if (pos == 0) break;
            pos--;
        } while (number > 0);

        /* Fill leading zeros if requested */
        if (leading_zero) {
            while (pos > 0 && digit_count < TM1650_NUM_DIGITS) {
                pos--;
                handle->buffer[pos] = DIGIT_SEGMENTS[0];
                digit_count++;
            }
        }
    }

    /* Add minus sign if negative and there's room to the left */
    if (negative) {
        /* Find leftmost non-blank digit */
        int8_t leftmost = 0;
        while (leftmost < TM1650_NUM_DIGITS && handle->buffer[leftmost] == 0) {
            leftmost++;
        }
        if (leftmost > 0) {
            handle->buffer[leftmost - 1] = 0x40;  /* '-' segment */
        }
    }

    return _write_segments(handle);
}

TM1650_Status TM1650_DisplayHex(TM1650_Handle* handle, uint16_t number, bool leading_zero) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    memset(handle->buffer, 0, TM1650_NUM_DIGITS);

    int8_t pos = TM1650_NUM_DIGITS - 1;
    uint8_t digit_count = 0;

    if (number == 0 && leading_zero) {
        for (uint8_t i = 0; i < TM1650_NUM_DIGITS; i++) {
            handle->buffer[i] = DIGIT_SEGMENTS[0];
        }
    } else {
        do {
            uint8_t digit = number & 0x0F;

            if (digit < 10) {
                handle->buffer[pos] = DIGIT_SEGMENTS[digit];
            } else {
                handle->buffer[pos] = HEX_SEGMENTS[digit - 10];
            }

            number >>= 4;
            digit_count++;

            if (pos == 0) break;
            pos--;
        } while (number > 0);

        if (leading_zero) {
            while (pos >= 0) {
                handle->buffer[pos] = DIGIT_SEGMENTS[0];
                if (pos == 0) break;
                pos--;
            }
        }
    }

    return _write_segments(handle);
}

TM1650_Status TM1650_DisplayDigit(TM1650_Handle* handle, uint8_t position, uint8_t digit, bool show_dp) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;
    if (position >= TM1650_NUM_DIGITS || digit > 9) return TM1650_ERROR_PARAM;

    handle->buffer[position] = DIGIT_SEGMENTS[digit];
    if (show_dp) {
        handle->buffer[position] |= SEG_DP;
    }

    return _write_segments(handle);
}

TM1650_Status TM1650_DisplayRaw(TM1650_Handle* handle, uint8_t position, uint8_t segments) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;
    if (position >= TM1650_NUM_DIGITS) return TM1650_ERROR_PARAM;

    handle->buffer[position] = segments;
    return _write_segments(handle);
}

TM1650_Status TM1650_Update(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return TM1650_ERROR_PARAM;

    return _write_segments(handle);
}

/* ========== Public: Key Scan ========== */

/**
 * @brief Read raw key byte from TM1650 via I2C
 * @return Raw key byte, or 0 if I2C error
 */
static uint8_t _read_key_byte(TM1650_Handle* handle) {
    uint8_t key_byte = 0;
    if (I2C_Read(handle->i2c_addr, &key_byte, 1) != I2C_OK) {
        return 0;
    }
    return key_byte;
}

uint8_t TM1650_GetKeys(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return 0;

    uint8_t current = _read_key_byte(handle);
    handle->raw_keys = current;

    if (current != 0) {
        /* A key is pressed */
        if (!handle->is_pressed) {
            /* New press — start debounce timer */
            handle->is_pressed     = 1;
            handle->press_start_ms = Get_CurrentMs();
            handle->last_key_mask  = current;
            return 0;  /* Wait for debounce */
        }

        /* Check debounce time */
        if (ELAPSED_TIME(handle->press_start_ms, Get_CurrentMs()) >= TM1650_KEY_DEBOUNCE_MS) {
            /* Debounce passed — check if same key still held */
            if (handle->last_key_mask == current) {
                uint8_t result = handle->last_key_mask;
                handle->last_key_mask = 0;  /* Prevent repeat */
                return result;
            }
        }
    } else {
        /* No key pressed — reset state */
        handle->is_pressed    = 0;
        handle->last_key_mask = 0;
    }

    return 0;
}

uint8_t TM1650_GetRawKeys(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return 0;

    uint8_t raw = _read_key_byte(handle);
    handle->raw_keys = raw;
    return raw;
}

bool TM1650_IsKeyPressed(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return false;

    return (_read_key_byte(handle) != 0);
}

uint8_t TM1650_WaitKey(TM1650_Handle* handle) {
    if (handle == NULL || !handle->initialized) return 0;

    uint8_t key;
    /* Wait for a key press */
    do {
        key = _read_key_byte(handle);
        if (key != 0) {
            Delay_Ms(TM1650_KEY_DEBOUNCE_MS);
            /* Re-read to confirm */
            if (_read_key_byte(handle) == key) {
                break;
            }
        }
        Delay_Ms(10);
    } while (1);

    /* Wait for key release */
    while (_read_key_byte(handle) != 0) {
        Delay_Ms(10);
    }
    Delay_Ms(TM1650_KEY_DEBOUNCE_MS);

    return key;
}

/* ========== Public: Utility ========== */

uint8_t TM1650_DigitToSegment(uint8_t digit) {
    if (digit > 9) return 0;
    return DIGIT_SEGMENTS[digit];
}
