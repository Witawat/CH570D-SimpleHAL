/**
 * @file ShiftReg595.h
 * @brief 74HC595 Shift Register Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับควบคุม 74HC595 8-bit Serial-In Parallel-Out Shift Register
 * ขยาย GPIO output โดยใช้เพียง 3 pins (DATA, CLK, LATCH)
 * รองรับการ cascade หลาย IC ต่อกัน
 *
 * **คุณสมบัติ:**
 * - ใช้ 3 GPIO pins ควบคุม output สูงสุด 8×N bits
 * - รองรับ cascade หลาย IC (N×8 outputs)
 * - ควบคุมแต่ละ bit ด้วย SetBit/ClearBit/ToggleBit
 * - เขียนทั้ง byte ด้วย WriteByte
 * - รองรับ MSB first และ LSB first
 * - มีฟังก์ชัน ShiftLeft/ShiftRight สำหรับ LED chase effect
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          74HC595
 *   DATA (PC0) -----> DS   (pin 14)
 *   CLK  (PC1) -----> SHCP (pin 11)  ← Shift Clock
 *   LATCH(PC2) -----> STCP (pin 12)  ← Storage Clock
 *   GND        -----> GND  (pin 8)
 *   3.3V       -----> VCC  (pin 16)
 *   3.3V       -----> MR   (pin 10)  ← ต่อ HIGH (disable reset)
 *   3.3V       -----> OE   (pin 13)  ← ต่อ LOW (enable output)
 *
 *   Outputs: QA(1) QB(2) QC(3) QD(4) QE(5) QF(6) QG(7) QH(9)
 *
 * Cascade 2 IC:
 *   IC1 QH' (pin 9) -----> IC2 DS (pin 14)
 *   CLK และ LATCH ใช้ร่วมกัน
 * ```
 *
 * @example
 * // ตัวอย่างการใช้งานพื้นฐาน
 * #include "SimpleHAL.h"
 * #include "ShiftReg595.h"
 *
 * ShiftReg595_Instance sr;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     // Init: DATA=PC0, CLK=PC1, LATCH=PC2, จำนวน IC = 1
 *     ShiftReg595_Init(&sr, PC0, PC1, PC2, 1);
 *
 *     // เปิด LED ทุกดวง (byte 0xFF)
 *     ShiftReg595_WriteByte(&sr, 0, 0xFF);
 *     Delay_Ms(500);
 *
 *     // เปิดแค่ bit 0 และ bit 7
 *     ShiftReg595_WriteByte(&sr, 0, 0x81);
 *
 *     while (1) {}
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __SHIFTREG595_H
#define __SHIFTREG595_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ========== Configuration ========== */

/** @brief จำนวน 74HC595 IC สูงสุดที่รองรับ (cascade) */
#ifndef SHIFTREG595_MAX_ICS
#define SHIFTREG595_MAX_ICS     4
#endif

/* ========== Type Definitions ========== */

/**
 * @brief Bit Order สำหรับการส่งข้อมูล
 */
typedef enum {
    SR595_MSBFIRST = 0, /**< MSB ส่งก่อน (default สำหรับ 74HC595) */
    SR595_LSBFIRST = 1  /**< LSB ส่งก่อน */
} ShiftReg595_BitOrder;

/**
 * @brief ShiftReg595 Instance
 */
typedef struct {
    /* Pins */
    uint8_t pin_data;   /**< DATA pin (DS, pin 14) */
    uint8_t pin_clk;    /**< CLK pin (SHCP, pin 11) */
    uint8_t pin_latch;  /**< LATCH pin (STCP, pin 12) */

    /* Configuration */
    uint8_t  num_ics;   /**< จำนวน IC ที่ cascade */
    ShiftReg595_BitOrder bit_order; /**< ลำดับ bit */

    /* Shadow buffer (output state) */
    uint8_t  buffer[SHIFTREG595_MAX_ICS]; /**< ค่าปัจจุบันของ output ทุก IC */

    uint8_t  initialized; /**< flag บอกว่า Init แล้ว */
} ShiftReg595_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น ShiftReg595
 * @param sr      ตัวแปร instance
 * @param data    GPIO pin สำหรับ DATA (DS)
 * @param clk     GPIO pin สำหรับ CLK (SHCP)
 * @param latch   GPIO pin สำหรับ LATCH (STCP)
 * @param num_ics จำนวน IC ที่ cascade (1-4)
 *
 * @example
 * ShiftReg595_Init(&sr, PC0, PC1, PC2, 1);
 */
void ShiftReg595_Init(ShiftReg595_Instance* sr,
                      uint8_t data, uint8_t clk, uint8_t latch,
                      uint8_t num_ics);

/**
 * @brief เขียน 1 byte ไปยัง IC ที่กำหนด
 * @param sr      ตัวแปร instance
 * @param ic_idx  หมายเลข IC (0 = IC แรก)
 * @param value   ค่า 8-bit ที่ต้องการ output
 *
 * @example
 * ShiftReg595_WriteByte(&sr, 0, 0xFF); // IC แรก เปิดทุก output
 * ShiftReg595_WriteByte(&sr, 1, 0x0F); // IC สอง เปิดครึ่งล่าง
 */
void ShiftReg595_WriteByte(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t value);

/**
 * @brief เขียนทุก IC พร้อมกัน
 * @param sr     ตัวแปร instance
 * @param values array ของค่า (ขนาดเท่ากับ num_ics)
 *
 * @example
 * uint8_t vals[] = {0xFF, 0x0F};
 * ShiftReg595_WriteAll(&sr, vals);
 */
void ShiftReg595_WriteAll(ShiftReg595_Instance* sr, const uint8_t* values);

/**
 * @brief ส่งออก buffer ไปยัง hardware (latch)
 * @param sr ตัวแปร instance
 * @note เรียกหลัง SetBit/ClearBit/ToggleBit เพื่ออัพเดต output
 */
void ShiftReg595_Latch(ShiftReg595_Instance* sr);

/**
 * @brief ตั้ง bit ที่กำหนดเป็น HIGH
 * @param sr      ตัวแปร instance
 * @param ic_idx  หมายเลข IC (0-based)
 * @param bit_num หมายเลข bit (0-7, 0=QA, 7=QH)
 *
 * @example
 * ShiftReg595_SetBit(&sr, 0, 3);  // ตั้ง QD (bit 3) เป็น HIGH
 * ShiftReg595_Latch(&sr);
 */
void ShiftReg595_SetBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num);

/**
 * @brief ตั้ง bit ที่กำหนดเป็น LOW
 * @param sr      ตัวแปร instance
 * @param ic_idx  หมายเลข IC (0-based)
 * @param bit_num หมายเลข bit (0-7)
 *
 * @example
 * ShiftReg595_ClearBit(&sr, 0, 3);
 * ShiftReg595_Latch(&sr);
 */
void ShiftReg595_ClearBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num);

/**
 * @brief สลับค่า bit ที่กำหนด
 * @param sr      ตัวแปร instance
 * @param ic_idx  หมายเลข IC (0-based)
 * @param bit_num หมายเลข bit (0-7)
 */
void ShiftReg595_ToggleBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num);

/**
 * @brief อ่านสถานะ bit ปัจจุบัน (จาก buffer)
 * @param sr      ตัวแปร instance
 * @param ic_idx  หมายเลข IC (0-based)
 * @param bit_num หมายเลข bit (0-7)
 * @return 1 = HIGH, 0 = LOW
 */
uint8_t ShiftReg595_GetBit(ShiftReg595_Instance* sr, uint8_t ic_idx, uint8_t bit_num);

/**
 * @brief ปิด output ทั้งหมด (เขียน 0x00 ทุก IC)
 * @param sr ตัวแปร instance
 */
void ShiftReg595_Clear(ShiftReg595_Instance* sr);

/**
 * @brief เปิด output ทั้งหมด (เขียน 0xFF ทุก IC)
 * @param sr ตัวแปร instance
 */
void ShiftReg595_SetAll(ShiftReg595_Instance* sr);

/**
 * @brief Shift buffer ทั้งหมดไปทางซ้าย 1 bit (ใช้ทำ LED chase effect)
 * @param sr      ตัวแปร instance
 * @param wrap    1=วนกลับ, 0=เติม 0
 *
 * @example
 * ShiftReg595_WriteByte(&sr, 0, 0x01);
 * while(1) { ShiftReg595_ShiftLeft(&sr, 1); Delay_Ms(100); }
 */
void ShiftReg595_ShiftLeft(ShiftReg595_Instance* sr, uint8_t wrap);

/**
 * @brief Shift buffer ทั้งหมดไปทางขวา 1 bit
 * @param sr   ตัวแปร instance
 * @param wrap 1=วนกลับ, 0=เติม 0
 */
void ShiftReg595_ShiftRight(ShiftReg595_Instance* sr, uint8_t wrap);

#ifdef __cplusplus
}
#endif

#endif /* __SHIFTREG595_H */
