/**
 * @file ShiftReg595.c
 * @brief 74HC595 Shift Register Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "ShiftReg595.h"

/* ========== Private Functions ========== */

/**
 * @brief Pulse CLK pin (rising edge → shift 1 bit in)
 */
static void _clk_pulse(ShiftReg595_Instance* sr) {
    digitalWrite(sr->pin_clk, HIGH);
    Delay_Us(1);
    digitalWrite(sr->pin_clk, LOW);
    Delay_Us(1);
}

/**
 * @brief Pulse LATCH pin (rising edge → copy shift register → output)
 */
static void _latch_pulse(ShiftReg595_Instance* sr) {
    digitalWrite(sr->pin_latch, HIGH);
    Delay_Us(1);
    digitalWrite(sr->pin_latch, LOW);
    Delay_Us(1);
}

/**
 * @brief ส่ง 1 byte ออก shift register (ยังไม่ latch)
 * @note ส่งจาก IC สุดท้ายก่อน (cascade: IC0 output ถูก shift ออกไป ICn)
 */
static void _shift_out_byte(ShiftReg595_Instance* sr, uint8_t value) {
    if (sr->bit_order == SR595_MSBFIRST) {
        for (int8_t i = 7; i >= 0; i--) {
            digitalWrite(sr->pin_data, (value >> i) & 0x01);
            _clk_pulse(sr);
        }
    } else {
        for (uint8_t i = 0; i < 8; i++) {
            digitalWrite(sr->pin_data, (value >> i) & 0x01);
            _clk_pulse(sr);
        }
    }
}

/**
 * @brief ส่ง buffer ทั้งหมดออก hardware และ latch
 */
static void _flush(ShiftReg595_Instance* sr) {
    /* Cascade: ส่ง IC สุดท้ายก่อน → IC แรกถูก latch ออกมา */
    for (int8_t i = (int8_t)(sr->num_ics - 1); i >= 0; i--) {
        _shift_out_byte(sr, sr->buffer[i]);
    }
    _latch_pulse(sr);
}

/* ========== Public Functions ========== */

void ShiftReg595_Init(ShiftReg595_Instance* sr,
                      uint8_t data, uint8_t clk, uint8_t latch,
                      uint8_t num_ics) {
    if (sr == NULL) return;
    if (num_ics == 0 || num_ics > SHIFTREG595_MAX_ICS) num_ics = 1;

    sr->pin_data   = data;
    sr->pin_clk    = clk;
    sr->pin_latch  = latch;
    sr->num_ics    = num_ics;
    sr->bit_order  = SR595_MSBFIRST;
    sr->initialized = 1;

    memset(sr->buffer, 0, sizeof(sr->buffer));

    /* ตั้ง GPIO เป็น output */
    pinMode(data,  PIN_MODE_OUTPUT);
    pinMode(clk,   PIN_MODE_OUTPUT);
    pinMode(latch, PIN_MODE_OUTPUT);

    digitalWrite(data,  LOW);
    digitalWrite(clk,   LOW);
    digitalWrite(latch, LOW);

    /* Clear outputs */
    _flush(sr);
}

void ShiftReg595_WriteByte(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t value) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics) return;

    sr->buffer[ic_idx] = value;
    _flush(sr);
}

void ShiftReg595_WriteAll(ShiftReg595_Instance* sr, const uint8_t* values) {
    if (sr == NULL || !sr->initialized || values == NULL) return;

    for (uint8_t i = 0; i < sr->num_ics; i++) {
        sr->buffer[i] = values[i];
    }
    _flush(sr);
}

void ShiftReg595_Latch(ShiftReg595_Instance* sr) {
    if (sr == NULL || !sr->initialized) return;
    _flush(sr);
}

void ShiftReg595_SetBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics || bit_num > 7) return;

    sr->buffer[ic_idx] |= (1u << bit_num);
    _flush(sr);
}

void ShiftReg595_ClearBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics || bit_num > 7) return;

    sr->buffer[ic_idx] &= ~(1u << bit_num);
    _flush(sr);
}

void ShiftReg595_ToggleBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics || bit_num > 7) return;

    sr->buffer[ic_idx] ^= (1u << bit_num);
    _flush(sr);
}

uint8_t ShiftReg595_GetBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return 0;
    if (ic_idx >= sr->num_ics || bit_num > 7) return 0;

    return (sr->buffer[ic_idx] >> bit_num) & 0x01;
}

void ShiftReg595_Clear(ShiftReg595_Instance* sr) {
    if (sr == NULL || !sr->initialized) return;
    memset(sr->buffer, 0x00, sr->num_ics);
    _flush(sr);
}

void ShiftReg595_SetAll(ShiftReg595_Instance* sr) {
    if (sr == NULL || !sr->initialized) return;
    memset(sr->buffer, 0xFF, sr->num_ics);
    _flush(sr);
}

void ShiftReg595_ShiftLeft(ShiftReg595_Instance* sr, uint8_t wrap) {
    if (sr == NULL || !sr->initialized) return;

    uint8_t msb = 0;
    if (wrap) {
        /* บันทึก MSB ของ IC สุดท้าย */
        msb = (sr->buffer[sr->num_ics - 1] >> 7) & 0x01;
    }

    /* Shift แต่ละ IC */
    for (int8_t i = (int8_t)(sr->num_ics - 1); i >= 0; i--) {
        sr->buffer[i] <<= 1;
        if (i > 0) {
            /* รับ MSB จาก IC ก่อนหน้า */
            sr->buffer[i] |= (sr->buffer[i - 1] >> 7) & 0x01;
        }
    }

    /* Wrap: เอา MSB เดิมมาใส่ LSB ของ IC แรก */
    if (wrap) {
        sr->buffer[0] |= msb;
    }

    _flush(sr);
}

void ShiftReg595_ShiftRight(ShiftReg595_Instance* sr, uint8_t wrap) {
    if (sr == NULL || !sr->initialized) return;

    uint8_t lsb = 0;
    if (wrap) {
        /* บันทึก LSB ของ IC แรก */
        lsb = sr->buffer[0] & 0x01;
    }

    /* Shift แต่ละ IC */
    for (uint8_t i = 0; i < sr->num_ics; i++) {
        sr->buffer[i] >>= 1;
        if (i < sr->num_ics - 1) {
            /* รับ LSB จาก IC ถัดไป */
            sr->buffer[i] |= (sr->buffer[i + 1] & 0x01) << 7;
        }
    }

    /* Wrap: เอา LSB เดิมมาใส่ MSB ของ IC สุดท้าย */
    if (wrap) {
        sr->buffer[sr->num_ics - 1] |= (lsb << 7);
    }

    _flush(sr);
}
