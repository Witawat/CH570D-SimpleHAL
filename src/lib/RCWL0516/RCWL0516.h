/**
 * @file RCWL0516.h
 * @brief RCWL-0516 Microwave Radar Motion Sensor Library
 *        สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับ RCWL-0516 Doppler microwave radar motion sensor
 * ออกแบบเฉพาะสำหรับพฤติกรรมของ RCWL-0516 โดยเฉพาะ ต่างจาก PIR
 *
 * **ความแตกต่างจาก PIR ธรรมดา:**
 * - ใช้ Doppler radar 3.18GHz ตรวจจับการเคลื่อนไหวผ่านวัตถุบางชนิด
 * - มี CDIV pin ปรับความถี่ output (ใช้เชื่อมกับ MCU ที่ไม่รองรับความถี่สูง)
 * - มี CDS pin สำหรับ light gating (หยุดการทำงานเมื่อแสงมาก)
 * - Hold time (สัญญาณ HIGH นานแค่ไหนหลังตรวจพบ) ขึ้นอยู่กับ RC ภายนอก
 * - Output HIGH = ตรวจพบ, LOW = ไม่พบ (active HIGH)
 * - สัญญาณ retrigger ได้เองตราบที่ยังมีการเคลื่อนไหว
 *
 * **Pinout RCWL-0516:**
 * ```
 *   ┌──────────────────────────────────────────────────────────┐
 *   │                     RCWL-0516                            │
 *   │                                                          │
 *   │  3V3  ●── 3.3V output (100mA max, อย่าดึงกระแสมาก)     │
 *   │  GND  ●── Ground                                         │
 *   │  OUT  ●── Digital output (HIGH=motion, 3.3V logic)       │
 *   │  VIN  ●── 4V–28V supply (แนะนำ 5V สำหรับความเสถียร)    │
 *   │  CDS  ●── Light sensor input (NC=disable, LOW=enable)    │
 *   │            เชื่อม LDR ระหว่าง 3V3–CDS เพื่อกลางคืนเท่านั้น│
 *   │  CDIV ●── Frequency divider output ไม่ค่อยได้ใช้         │
 *   └──────────────────────────────────────────────────────────┘
 * ```
 *
 * **วงจรพื้นฐาน (CH57x 3.3V):**
 * ```
 *   CH57x          RCWL-0516
 *   ─────────────────────────────
 *   PD2 (input) ←── OUT
 *   3.3V ────────── 3V3  (เป็น output ของ module! ห้ามจ่ายเข้า)
 *   GND ─────────── GND
 *   5V  ─────────── VIN  (ต้องจ่าย 4V+ เพื่อให้ radar ทำงาน)
 *
 *   ⚠️ CH57x ทำงาน 3.3V: OUT ของ RCWL-0516 ที่ใช้ไฟ 5V
 *      จะ output ~3.3V (ใช้ internal 3V3 rail) → ปลอดภัยกับ MCU
 * ```
 *
 * **วงจรพร้อม CDS (กลางคืนเท่านั้น):**
 * ```
 *   3V3 ──[LDR]── CDS ──[10kΩ]── GND
 *   (กลางวัน LDR ต้านทานต่ำ → CDS สูง → disable radar)
 *   (กลางคืน LDR ต้านทานสูง → CDS ต่ำ → enable radar)
 * ```
 *
 * **Hardware Tuning (บน PCB):**
 * - C-TM (KTIR, ตรงกลางด้านหน้า): ความไว, เพิ่ม C → ไวขึ้น
 * - R-GN (ตำแหน่ง GN): hold time, เพิ่ม R → ถือนานขึ้น
 *   - ค่า default ≈ 1-2 วินาที
 *   - เพิ่ม capacitor ที่ C2 → hold time ยาวขึ้น
 * - CDIV: เชื่อม pad เพื่อหาร output frequency ÷2
 *
 * @example
 * // การใช้งานพื้นฐาน (polling)
 * RCWL0516_Instance radar;
 * RCWL0516_Init(&radar, PD2);
 *
 * while (1) {
 *     RCWL0516_Update(&radar);
 *     if (RCWL0516_IsMotionDetected(&radar)) {
 *         // มีการเคลื่อนไหว
 *     }
 * }
 *
 * @example
 * // การใช้งานพร้อม CDS pin และ callback
 * RCWL0516_Instance radar;
 * RCWL0516_Config cfg = {
 *     .out_pin    = PD2,
 *     .cds_pin    = RCWL0516_PIN_NONE,
 *     .hold_ms    = 2000,
 *     .debounce_ms = 50,
 *     .filter_count = 3
 * };
 * RCWL0516_InitWithConfig(&radar, &cfg);
 * RCWL0516_SetCallbacks(&radar, on_motion_start, on_motion_end, NULL);
 *
 * while (1) {
 *     RCWL0516_Update(&radar);
 * }
 *
 * @author CH57x Library Team
 */

#ifndef __RCWL0516_H
#define __RCWL0516_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief ไม่ใช้ CDS pin */
#define RCWL0516_PIN_NONE          0xFF

/**
 * @brief Hold time เริ่มต้น (ms) ของ software hold หลังสัญญาณลง
 * @note  hardware hold จาก RC บน RCWL-0516 ≈ 1-2 วินาที
 *        software hold เป็นการต่อ hold time เพิ่มใน firmware
 */
#ifndef RCWL0516_DEFAULT_HOLD_MS
#define RCWL0516_DEFAULT_HOLD_MS   2000
#endif

/** @brief Debounce เริ่มต้น (ms) */
#ifndef RCWL0516_DEFAULT_DEBOUNCE_MS
#define RCWL0516_DEFAULT_DEBOUNCE_MS   50
#endif

/**
 * @brief จำนวน consecutive sample ที่ต้องตรงกันก่อนเปลี่ยน state
 * @note  ค่า 1-8, เพิ่มเพื่อลด false positive แต่ทำให้ latency สูงขึ้น
 */
#ifndef RCWL0516_DEFAULT_FILTER_COUNT
#define RCWL0516_DEFAULT_FILTER_COUNT  3
#endif

/* ========== Types ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    RCWL0516_OK = 0,
    RCWL0516_ERROR_PARAM,
    RCWL0516_ERROR_NOT_INIT
} RCWL0516_Status;

/**
 * @brief state machine ของ output signal
 */
typedef enum {
    RCWL0516_STATE_IDLE = 0,       /**< ไม่มีการเคลื่อนไหว */
    RCWL0516_STATE_TRIGGERED,      /**< ตรวจพบ (rising edge ใหม่) */
    RCWL0516_STATE_ACTIVE,         /**< กำลังตรวจพบ (signal HIGH) */
    RCWL0516_STATE_HOLDING,        /**< signal ลงแต่ยัง hold ด้วย software */
    RCWL0516_STATE_COOLDOWN        /**< hold หมดแล้ว รอ signal ลงสมบูรณ์ */
} RCWL0516_State;

/**
 * @brief Configuration struct
 */
typedef struct {
    uint8_t  out_pin;              /**< GPIO pin ต่อกับ OUT ของ RCWL-0516 */
    uint8_t  cds_pin;              /**< GPIO pin ต่อกับ CDS (RCWL0516_PIN_NONE ถ้าไม่ใช้) */
    uint32_t hold_ms;              /**< Software hold time หลัง signal ลง (ms) */
    uint16_t debounce_ms;          /**< Debounce time (ms) */
    uint8_t  filter_count;         /**< จำนวน sample ที่ต้องตรงก่อนเปลี่ยน state (1-8) */
} RCWL0516_Config;

/**
 * @brief Instance struct — สร้าง 1 ตัวต่อ sensor
 */
typedef struct {
    /* config */
    RCWL0516_Config cfg;

    /* state machine */
    RCWL0516_State  state;
    uint8_t         raw_signal;       /**< ค่า OUT pin ปัจจุบัน */
    uint8_t         stable_signal;    /**< ค่าหลัง debounce+filter */

    /* filter buffer */
    uint8_t         filter_buf[8];
    uint8_t         filter_idx;

    /* timing */
    uint32_t        last_change_ms;   /**< เวลาที่ stable_signal เปลี่ยนล่าสุด */
    uint32_t        trigger_ms;       /**< เวลาที่ trigger ครั้งล่าสุด */
    uint32_t        hold_start_ms;    /**< เวลาที่เริ่ม software hold */
    uint32_t        last_active_ms;   /**< เวลาสุดท้ายที่ ACTIVE */

    /* statistics */
    uint32_t        total_triggers;   /**< นับจำนวนครั้งที่ trigger ทั้งหมด */
    uint32_t        last_duration_ms; /**< ระยะเวลา active ครั้งล่าสุด */

    /* callbacks */
    void (*on_triggered)(void);       /**< เรียกเมื่อ rising edge ใหม่ */
    void (*on_active)(void);          /**< เรียกซ้ำขณะ ACTIVE/HOLDING */
    void (*on_idle)(void);            /**< เรียกเมื่อกลับสู่ IDLE */

    uint8_t initialized;
} RCWL0516_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น sensor ด้วยค่า default
 * @param r      instance
 * @param out_pin GPIO pin ต่อกับ OUT ของ RCWL-0516
 * @return RCWL0516_OK หรือ error
 *
 * @note ใช้ config เริ่มต้น:
 *       hold=2000ms, debounce=50ms, filter=3, cds_pin=NONE
 *
 * @example
 * RCWL0516_Instance radar;
 * RCWL0516_Init(&radar, PD2);
 */
RCWL0516_Status RCWL0516_Init(RCWL0516_Instance* r, uint8_t out_pin);

/**
 * @brief เริ่มต้น sensor พร้อม config ที่กำหนดเอง
 * @param r   instance
 * @param cfg config struct
 * @return RCWL0516_OK หรือ error
 *
 * @example
 * RCWL0516_Config cfg = {
 *     .out_pin     = PD2,
 *     .cds_pin     = RCWL0516_PIN_NONE,
 *     .hold_ms     = 5000,
 *     .debounce_ms = 80,
 *     .filter_count = 4
 * };
 * RCWL0516_InitWithConfig(&radar, &cfg);
 */
RCWL0516_Status RCWL0516_InitWithConfig(RCWL0516_Instance* r,
                                         const RCWL0516_Config* cfg);

/**
 * @brief ตั้ง callback functions
 * @param r            instance
 * @param on_triggered เรียกเมื่อตรวจพบการเคลื่อนไหวครั้งใหม่ (rising edge)
 * @param on_active    เรียกซ้ำขณะ ACTIVE/HOLDING (ทุกรอบ Update)
 * @param on_idle      เรียกเมื่อกลับสู่ IDLE (motion สิ้นสุด)
 *
 * @note NULL = ไม่ใช้ callback นั้น
 */
RCWL0516_Status RCWL0516_SetCallbacks(RCWL0516_Instance* r,
                                       void (*on_triggered)(void),
                                       void (*on_active)(void),
                                       void (*on_idle)(void));

/**
 * @brief อัปเดต state machine — เรียกใน main loop ทุกรอบ
 * @param r instance
 *
 * @note ต้องเรียกสม่ำเสมอ (ทุก loop iteration)
 * @note ไม่ blocking
 */
void RCWL0516_Update(RCWL0516_Instance* r);

/**
 * @brief ตรวจสอบว่า active อยู่ไหม (TRIGGERED + ACTIVE + HOLDING)
 * @return 1 = มีการเคลื่อนไหวหรืออยู่ใน hold, 0 = ไม่มี
 */
uint8_t RCWL0516_IsMotionDetected(RCWL0516_Instance* r);

/**
 * @brief อ่าน signal raw (ไม่ผ่าน debounce/filter)
 * @return 1 = HIGH (motion), 0 = LOW
 */
uint8_t RCWL0516_ReadRaw(RCWL0516_Instance* r);

/**
 * @brief อ่านสถานะ state machine
 */
RCWL0516_State RCWL0516_GetState(RCWL0516_Instance* r);

/**
 * @brief อ่านจำนวนครั้ง trigger ทั้งหมดนับแต่ init
 */
uint32_t RCWL0516_GetTotalTriggers(RCWL0516_Instance* r);

/**
 * @brief อ่านระยะเวลาที่ ACTIVE ในครั้งล่าสุด (ms)
 */
uint32_t RCWL0516_GetLastDurationMs(RCWL0516_Instance* r);

/**
 * @brief อ่านเวลาที่ผ่านมาหลังจาก trigger ครั้งล่าสุด (ms)
 */
uint32_t RCWL0516_GetMsSinceLastTrigger(RCWL0516_Instance* r);

/**
 * @brief ปรับ software hold time
 * @param hold_ms ระยะเวลา hold (ms)
 *
 * @note ค่านี้เป็น software hold ต่อท้าย hardware hold ของ module
 *       ถ้าต้องการลด hold ใน hardware ต้องแก้ RC บน PCB
 */
RCWL0516_Status RCWL0516_SetHoldTime(RCWL0516_Instance* r, uint32_t hold_ms);

/**
 * @brief รีเซ็ต state กลับสู่ IDLE ทันที (ใช้สำหรับ test หรือ force clear)
 */
RCWL0516_Status RCWL0516_Reset(RCWL0516_Instance* r);

#ifdef __cplusplus
}
#endif

#endif /* __RCWL0516_H */
