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
 *
 * @param sr - instance ที่ผ่านการ init แล้ว
 *
 * @note digitalWrite HIGH → Delay_Us(1) → LOW
 *       74HC595: ทุก rising edge ที่ SHCP (pin 11)
 *       จะ shift 1 bit จาก DS เข้า shift register
 */
static void _clk_pulse(ShiftReg595_Instance* sr) {
    digitalWrite(sr->pin_clk, HIGH);
    Delay_Us(1);
    digitalWrite(sr->pin_clk, LOW);
    Delay_Us(1);
}

/**
 * @brief Pulse LATCH pin (rising edge → copy shift register → output)
 *
 * @param sr - instance ที่ผ่านการ init แล้ว
 *
 * @note digitalWrite HIGH → Delay_Us(1) → LOW
 *       74HC595: rising edge ที่ STCP (pin 12)
 *       copy 8-bit shift register → output latch
 */
static void _latch_pulse(ShiftReg595_Instance* sr) {
    digitalWrite(sr->pin_latch, HIGH);
    Delay_Us(1);
    digitalWrite(sr->pin_latch, LOW);
    Delay_Us(1);
}

/**
 * @brief ส่ง 1 byte ออก shift register (ยังไม่ latch)
 *
 * @param sr    - instance ที่ผ่านการ init แล้ว
 * @param value - 8-bit value ที่จะ shift ออก (bit by bit)
 *
 * @note bit-bang SPI: set DATA pin → _clk_pulse() × 8
 *       MSBFIRST: for i = 7..0, (value >> i) & 0x01
 *       LSBFIRST: for i = 0..7, (value >> i) & 0x01
 *       ส่งจาก IC สุดท้ายก่อน (cascade)
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
 *
 * @param sr - instance ที่ผ่านการ init แล้ว
 *
 * @note cascade: shift IC สุดท้ายก่อน → IC แรก
 *       เรียก _shift_out_byte() สำหรับทุก IC
 *       ตามด้วย _latch_pulse() เพื่อ update output
 */
static void _flush(ShiftReg595_Instance* sr) {
    /* Cascade: ส่ง IC สุดท้ายก่อน → IC แรกถูก latch ออกมา */
    for (int8_t i = (int8_t)(sr->num_ics - 1); i >= 0; i--) {
        _shift_out_byte(sr, sr->buffer[i]);
    }
    _latch_pulse(sr);
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น ShiftReg595
 *
 * @param sr      - instance (NULL → return ทันที)
 * @param data    - GPIO pin สำหรับ DATA (DS, pin 14)
 * @param clk     - GPIO pin สำหรับ CLK (SHCP, pin 11)
 * @param latch   - GPIO pin สำหรับ LATCH (STCP, pin 12)
 * @param num_ics - จำนวน IC cascade (0 หรือ > MAX → ปรับเป็น 1)
 *
 * @note ตั้ง DATA/CLK/LATCH เป็น OUTPUT, idle LOW
 *       bit_order เริ่มต้น SR595_MSBFIRST
 *       buffer ถูกล้าง (0x00) → _flush() เพื่อ clear output
 */
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

/**
 * @brief เขียน 1 byte ไปยัง IC ที่กำหนด
 *
 * @param sr     - instance (NULL หรือ !initialized → return ทันที)
 * @param ic_idx - หมายเลข IC (0-based, ≥ num_ics → return ทันที)
 * @param value  - ค่า 8-bit ที่ต้องการ output
 *
 * @note อัปเดต buffer[ic_idx] → _flush() ทั้งหมด
 *       cascade: IC0 = byte ที่ latch ใกล้ DATA pin, IC1 = ถัดไป
 */
void ShiftReg595_WriteByte(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t value) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics) return;

    sr->buffer[ic_idx] = value;
    _flush(sr);
}

/**
 * @brief เขียนทุก IC พร้อมกัน
 *
 * @param sr     - instance (NULL/!initialized → return ทันที)
 * @param values - array ของค่า (size = num_ics), NULL → return ทันที
 *
 * @note copy values[] → buffer[] → _flush()
 *       เขียนพร้อมกันครั้งเดียว ไม่เหมือน WriteByte ที่ flush ทีละ IC
 */
void ShiftReg595_WriteAll(ShiftReg595_Instance* sr, const uint8_t* values) {
    if (sr == NULL || !sr->initialized || values == NULL) return;

    for (uint8_t i = 0; i < sr->num_ics; i++) {
        sr->buffer[i] = values[i];
    }
    _flush(sr);
}

/**
 * @brief ส่งออก buffer ปัจจุบันไปยัง hardware (latch)
 *
 * @param sr - instance (NULL/!initialized → return ทันที)
 *
 * @note เรียก _flush() — shift buffer ทั้งหมด + latch pulse
 *       ใช้หลัง SetBit/ClearBit/ToggleBit เพื่ออัปเดต output
 *       ถ้าใช้ WriteByte/WriteAll ไม่ต้องเรียกเพราะ flush อัตโนมัติ
 */
void ShiftReg595_Latch(ShiftReg595_Instance* sr) {
    if (sr == NULL || !sr->initialized) return;
    _flush(sr);
}

/**
 * @brief ตั้ง bit ที่กำหนดเป็น HIGH
 *
 * @param sr      - instance (NULL/!initialized → return ทันที)
 * @param ic_idx  - หมายเลข IC (0-based, ≥ num_ics → return)
 * @param bit_num - หมายเลข bit 0-7 (0=QA, 7=QH, >7 → return)
 *
 * @note buffer[ic_idx] |= (1u << bit_num) → _flush()
 *       bit 0 = QA, bit 7 = QH
 */
void ShiftReg595_SetBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics || bit_num > 7) return;

    sr->buffer[ic_idx] |= (1u << bit_num);
    _flush(sr);
}

/**
 * @brief ตั้ง bit ที่กำหนดเป็น LOW
 *
 * @param sr      - instance (NULL/!initialized → return ทันที)
 * @param ic_idx  - หมายเลข IC (0-based)
 * @param bit_num - หมายเลข bit 0-7
 *
 * @note buffer[ic_idx] &= ~(1u << bit_num) → _flush()
 */
void ShiftReg595_ClearBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics || bit_num > 7) return;

    sr->buffer[ic_idx] &= ~(1u << bit_num);
    _flush(sr);
}

/**
 * @brief สลับค่า bit ที่กำหนด (HIGH ↔ LOW)
 *
 * @param sr      - instance (NULL/!initialized → return ทันที)
 * @param ic_idx  - หมายเลข IC (0-based)
 * @param bit_num - หมายเลข bit 0-7
 *
 * @note buffer[ic_idx] ^= (1u << bit_num) → _flush()
 */
void ShiftReg595_ToggleBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return;
    if (ic_idx >= sr->num_ics || bit_num > 7) return;

    sr->buffer[ic_idx] ^= (1u << bit_num);
    _flush(sr);
}

/**
 * @brief อ่านสถานะ bit ปัจจุบัน (จาก buffer, ไม่ได้อ่าน hardware)
 *
 * @param sr      - instance (NULL/!initialized → return 0)
 * @param ic_idx  - หมายเลข IC (0-based, ≥ num_ics → return 0)
 * @param bit_num - หมายเลข bit 0-7 (>7 → return 0)
 * @return 1 = HIGH, 0 = LOW
 *
 * @note อ่านจาก buffer เท่านั้น (shadow register)
 *       ไม่ได้อ่านจาก physical pin ของ 74HC595
 */
uint8_t ShiftReg595_GetBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num) {
    if (sr == NULL || !sr->initialized) return 0;
    if (ic_idx >= sr->num_ics || bit_num > 7) return 0;

    return (sr->buffer[ic_idx] >> bit_num) & 0x01;
}

/**
 * @brief ปิด output ทั้งหมด (เขียน 0x00 ทุก IC)
 *
 * @param sr - instance (NULL/!initialized → return ทันที)
 *
 * @note memset buffer → 0x00 → _flush()
 *       ใช้ sizeof(sr->buffer[0]) × num_ics
 */
void ShiftReg595_Clear(ShiftReg595_Instance* sr) {
    if (sr == NULL || !sr->initialized) return;
    memset(sr->buffer, 0x00, sr->num_ics);
    _flush(sr);
}

/**
 * @brief เปิด output ทั้งหมด (เขียน 0xFF ทุก IC)
 *
 * @param sr - instance (NULL/!initialized → return ทันที)
 *
 * @note memset buffer → 0xFF → _flush()
 */
void ShiftReg595_SetAll(ShiftReg595_Instance* sr) {
    if (sr == NULL || !sr->initialized) return;
    memset(sr->buffer, 0xFF, sr->num_ics);
    _flush(sr);
}

/**
 * @brief Shift buffer ทั้งหมดไปทางซ้าย 1 bit (LED chase effect)
 *
 * @param sr   - instance (NULL/!initialized → return ทันที)
 * @param wrap - 1 = MSB วนกลับมา LSB ของ IC แรก, 0 = LSB เติม 0
 *
 * @note shift แต่ละ IC ← 1, รับ MSB จาก IC ก่อนหน้า
 *       cascade: buffer[0] ← 0, buffer[1] ← bit7 ของ buffer[0], ...
 *       wrap=1: bit7 ของ IC สุดท้าย → bit0 ของ IC แรก
 */
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

/**
 * @brief Shift buffer ทั้งหมดไปทางขวา 1 bit
 *
 * @param sr   - instance (NULL/!initialized → return ทันที)
 * @param wrap - 1 = LSB วนกลับมา MSB ของ IC สุดท้าย, 0 = MSB เติม 0
 *
 * @note shift แต่ละ IC → 1, รับ LSB จาก IC ถัดไป
 *       cascade: buffer[last] ← 0, buffer[last-1] ← bit0 ของ buffer[last], ...
 *       wrap=1: bit0 ของ IC แรก → bit7 ของ IC สุดท้าย
 */
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
