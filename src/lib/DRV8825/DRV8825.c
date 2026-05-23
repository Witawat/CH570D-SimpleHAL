/**
 * @file DRV8825.c
 * @brief DRV8825 Stepper Driver Library Implementation
 */

#include "DRV8825.h"

static uint32_t _drv8825_calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0U) rpm = 1U;
    if (steps_per_rev == 0U) steps_per_rev = DRV8825_NEMA17_STEPS_PER_REV;
    return 60000000UL / (rpm * steps_per_rev);
}

static void _drv8825_set_dir(DRV8825_Instance* drv, uint8_t is_cw) {
    uint8_t dir_level = is_cw ? HIGH : LOW;
    if (drv->dir_inverted) {
        dir_level = (dir_level == HIGH) ? LOW : HIGH;
    }
    digitalWrite(drv->pin_dir, dir_level);
}

static void _drv8825_pulse_step(DRV8825_Instance* drv) {
    digitalWrite(drv->pin_step, HIGH);
    Delay_Us(2);
    digitalWrite(drv->pin_step, LOW);
    Delay_Us(drv->step_delay_us);
}

/**
 * @brief เริ่มต้น DRV8825 stepper driver
 *
 * @param drv            pointer ไปยัง DRV8825_Instance (NULL → return ทันที)
 * @param step_pin       GPIO pin สำหรับ STEP
 * @param dir_pin        GPIO pin สำหรับ DIR
 * @param en_pin         GPIO pin สำหรับ EN (0 = ไม่ใช้, pin ถูกดึง HIGH = disable)
 * @param steps_per_rev  จำนวน steps ต่อ 1 รอบ (0 = ใช้ค่า default 200)
 *
 * @note ตั้ง step_pin/dir_pin เป็น OUTPUT (LOW), en_pin เป็น OUTPUT (HIGH = disable)
 *       initial speed = 60 RPM, dir_inverted = 0, position = 0
 *       ถ้า en_pin == 0 → has_en = 0 (ไม่จัดการ enable pin)
 *       ต้องมี Timer_Init() มาก่อนเพราะ Delay_Us() ใช้งาน timer
 */
void DRV8825_Init(DRV8825_Instance* drv,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint32_t steps_per_rev) {
    if (drv == NULL) return;

    drv->pin_step = step_pin;
    drv->pin_dir = dir_pin;
    drv->pin_en = en_pin;
    drv->has_en = (en_pin != 0U) ? 1U : 0U;

    drv->dir_inverted = 0U;

    drv->steps_per_rev = (steps_per_rev > 0U) ? steps_per_rev : DRV8825_NEMA17_STEPS_PER_REV;
    drv->linear_mm_per_rev = 0.0f;
    drv->speed_rpm = 60U;
    drv->step_delay_us = _drv8825_calc_step_delay_us(drv->speed_rpm, drv->steps_per_rev);

    drv->position_steps = 0;
    drv->is_enabled = 0U;
    drv->initialized = 1U;

    pinMode(step_pin, PIN_MODE_OUTPUT);
    pinMode(dir_pin, PIN_MODE_OUTPUT);
    digitalWrite(step_pin, LOW);
    digitalWrite(dir_pin, LOW);

    if (drv->has_en) {
        pinMode(en_pin, PIN_MODE_OUTPUT);
        digitalWrite(en_pin, HIGH);  // HIGH = disable on DRV8825 modules
    }
}

/**
 * @brief เริ่มต้นมอเตอร์ NEMA17 ผ่าน DRV8825 (ค่ามาตรฐาน 200 steps/rev)
 *
 * @param drv      pointer ไปยัง DRV8825_Instance (NULL → return ทันที)
 * @param step_pin GPIO pin สำหรับ STEP
 * @param dir_pin  GPIO pin สำหรับ DIR
 * @param en_pin   GPIO pin สำหรับ EN (0 = ไม่ใช้)
 *
 * @note เป็น convenience wrapper รอบ DRV8825_Init() โดยส่ง steps_per_rev = 200
 *       ดูรายละเอียดเพิ่มเติมที่ DRV8825_Init()
 */
void DRV8825_InitNEMA17(DRV8825_Instance* drv,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin) {
    DRV8825_Init(drv, step_pin, dir_pin, en_pin, DRV8825_NEMA17_STEPS_PER_REV);
}

/**
 * @brief ตั้งค่าความเร็ว (RPM)
 *
 * @param drv pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param rpm ความเร็วหน่วย RPM (0 จะถูกปรับเป็น 1)
 *
 * @note คำนวณ step_delay_us = 60,000,000 / (rpm × steps_per_rev)
 *       และเก็บค่าไว้ใน drv->step_delay_us เพื่อใช้ใน _drv8825_pulse_step()
 *       ยิ่ง RPM สูง → step_delay_us ยิ่งน้อย → motor หมุนเร็วขึ้น
 */
void DRV8825_SetSpeed(DRV8825_Instance* drv, uint32_t rpm) {
    if (drv == NULL || !drv->initialized) return;
    if (rpm < 1U) rpm = 1U;
    drv->speed_rpm = rpm;
    drv->step_delay_us = _drv8825_calc_step_delay_us(rpm, drv->steps_per_rev);
}

/**
 * @brief กลับทิศทาง DIR (สำหรับกรณีต่อสาย DIR กลับด้าน)
 *
 * @param drv     pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param inverted 1 = กลับทิศทาง, 0 = ปกติ
 *
 * @note ถ้า inverted = 1, การขอ CW จะส่ง LOW ไปที่ DIR pin แทน HIGH
 *       มีประโยชน์เมื่อต่อสาย DIR สลับกัน หรือต้องการกลับทิศทางโดยไม่แก้ hardware
 */
void DRV8825_SetDirectionInverted(DRV8825_Instance* drv, uint8_t inverted) {
    if (drv == NULL || !drv->initialized) return;
    drv->dir_inverted = inverted ? 1U : 0U;
}

/**
 * @brief Enable motor (เปิด coil)
 *
 * @param drv pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ถ้า has_en == 1 → ดึง EN pin ลง LOW (DRV8825 module, LOW = enable)
 *       ต้อง Enable ก่อนเรียก Move() ไม่งั้น motor จะไม่ขยับ
 *       ตั้ง is_enabled = 1
 */
void DRV8825_Enable(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, LOW);
    }
    drv->is_enabled = 1U;
}

/**
 * @brief Disable motor (ปิด coil ลดความร้อน)
 *
 * @param drv pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ถ้า has_en == 1 → ดึง EN pin ขึ้น HIGH (disable)
 *       ควรเรียกหลังหมุนเสร็จเพื่อลด current draw และความร้อน
 *       ตั้ง is_enabled = 0
 */
void DRV8825_Disable(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, HIGH);
    }
    drv->is_enabled = 0U;
}

/**
 * @brief หยุด motor ทันที (หยุดส่ง STEP pulse)
 *
 * @param drv pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ดึง STEP pin ลง LOW
 *       motor จะหยุด ณ ตำแหน่งปัจจุบัน (แต่ coil ยังมีแรง)
 *       ถ้าต้องการปล่อย coil ให้เรียก DRV8825_Disable() เพิ่ม
 */
void DRV8825_Stop(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    digitalWrite(drv->pin_step, LOW);
}

/**
 * @brief หมุน motor ตามจำนวน steps (blocking)
 *
 * @param drv   pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param steps จำนวน steps (บวก = CW, ลบ = CCW, 0 = return ทันที)
 *
 * @note เป็น blocking function — ส่ง STEP pulse ตามจำนวนที่กำหนด
 *       แต่ละ pulse ใช้ _drv8825_pulse_step() ซึ่งมี HIGH 2 µs + LOW step_delay_us
 *       อัปเดต drv->position_steps ตามทิศทาง
 *       ต้อง Enable() ก่อน call นี้
 */
void DRV8825_Move(DRV8825_Instance* drv, int32_t steps) {
    int32_t n;

    if (drv == NULL || !drv->initialized) return;
    if (steps == 0) return;

    if (steps > 0) {
        _drv8825_set_dir(drv, 1U);
        n = steps;
        while (n-- > 0) {
            _drv8825_pulse_step(drv);
            drv->position_steps++;
        }
    } else {
        _drv8825_set_dir(drv, 0U);
        n = -steps;
        while (n-- > 0) {
            _drv8825_pulse_step(drv);
            drv->position_steps--;
        }
    }
}

/**
 * @brief หมุน motor ตามมุม (degrees, blocking)
 *
 * @param drv    pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param degrees มุมที่ต้องการหมุน (บวก = CW, ลบ = CCW)
 *
 * @note แปลงมุมเป็น steps: steps = degrees × steps_per_rev / 360
 *       แล้ว delegate ไปที่ DRV8825_Move()
 *       ค่าอาจมีการ truncation จาก integer division
 */
void DRV8825_MoveDegrees(DRV8825_Instance* drv, int32_t degrees) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)degrees * (int64_t)drv->steps_per_rev / 360LL);
    DRV8825_Move(drv, steps);
}

/**
 * @brief หมุน motor จำนวนรอบที่กำหนด (blocking)
 *
 * @param drv         pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param revolutions จำนวนรอบ (บวก = CW, ลบ = CCW)
 *
 * @note แปลงรอบเป็น steps: steps = revolutions × steps_per_rev
 *       แล้ว delegate ไปที่ DRV8825_Move()
 */
void DRV8825_MoveRevolutions(DRV8825_Instance* drv, int32_t revolutions) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)revolutions * (int64_t)drv->steps_per_rev);
    DRV8825_Move(drv, steps);
}

/**
 * @brief ตั้งค่าระยะเชิงเส้นที่มอเตอร์เคลื่อนที่ต่อ 1 รอบ
 *
 * @param drv        pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param mm_per_rev ระยะทางต่อ 1 รอบ (mm) เช่น lead screw 8.0 mm/rev
 *
 * @note ต้องตั้งค่านี้ก่อนใช้ MoveMm / MmToSteps / StepsToMm
 *       ถ้า mm_per_rev <= 0 → ตั้งค่าเป็น 0.0 (disabled)
 *       linear_mm_per_rev ใช้สำหรับแปลง steps ↔ mm
 */
void DRV8825_SetLinearMmPerRev(DRV8825_Instance* drv, float mm_per_rev) {
    if (drv == NULL || !drv->initialized) return;
    if (mm_per_rev <= 0.0f) {
        drv->linear_mm_per_rev = 0.0f;
        return;
    }
    drv->linear_mm_per_rev = mm_per_rev;
}

/**
 * @brief แปลงระยะทาง (mm) เป็นจำนวน steps
 *
 * @param drv         pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return 0)
 * @param distance_mm ระยะทาง (mm), บวก = CW, ลบ = CCW
 *
 * @return จำนวน steps ที่คำนวณได้ (ปัดเศษ), 0 ถ้า linear_mm_per_rev ยังไม่ถูกตั้ง
 *
 * @note สูตร: steps = distance_mm × steps_per_rev / linear_mm_per_rev
 *       ปัดเศษแบบ bankers' rounding: +0.5 สำหรับบวก, -0.5 สำหรับลบ
 *       ต้องเรียก SetLinearMmPerRev() ก่อน
 */
int32_t DRV8825_MmToSteps(DRV8825_Instance* drv, float distance_mm) {
    float steps_f;
    if (drv == NULL || !drv->initialized) return 0;
    if (drv->linear_mm_per_rev <= 0.0f) return 0;

    steps_f = (distance_mm * (float)drv->steps_per_rev) / drv->linear_mm_per_rev;
    if (steps_f >= 0.0f) return (int32_t)(steps_f + 0.5f);
    return (int32_t)(steps_f - 0.5f);
}

/**
 * @brief แปลงจำนวน steps เป็นระยะทาง (mm)
 *
 * @param drv   pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return 0.0f)
 * @param steps จำนวน steps
 *
 * @return ระยะทาง (mm), 0.0f ถ้า linear_mm_per_rev ยังไม่ถูกตั้ง
 *
 * @note สูตร: mm = steps × linear_mm_per_rev / steps_per_rev
 *       ต้องเรียก SetLinearMmPerRev() ก่อน
 */
float DRV8825_StepsToMm(DRV8825_Instance* drv, int32_t steps) {
    if (drv == NULL || !drv->initialized) return 0.0f;
    if (drv->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * drv->linear_mm_per_rev) / (float)drv->steps_per_rev;
}

/**
 * @brief หมุนตามระยะทางเชิงเส้น (mm, blocking)
 *
 * @param drv         pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 * @param distance_mm ระยะทางที่ต้องการ (mm), บวก = CW, ลบ = CCW
 *
 * @note แปลง mm → steps ด้วย DRV8825_MmToSteps() แล้ว delegate ไปที่ DRV8825_Move()
 *       ต้องเรียก SetLinearMmPerRev() ก่อน
 */
void DRV8825_MoveMm(DRV8825_Instance* drv, float distance_mm) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = DRV8825_MmToSteps(drv, distance_mm);
    DRV8825_Move(drv, steps);
}

/**
 * @brief ดูตำแหน่งปัจจุบัน (steps จาก home)
 *
 * @param drv pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return 0)
 *
 * @return ตำแหน่งปัจจุบัน (steps) — ค่าสะสมตั้งแต่ init หรือ reset ครั้งล่าสุด
 *
 * @note ตำแหน่งถูกอัปเดตใน DRV8825_Move() (+ สำหรับ CW, - สำหรับ CCW)
 *       ไม่ใช่ absolute encoder — เป็น virtual position counter
 */
int32_t DRV8825_GetPosition(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return 0;
    return drv->position_steps;
}

/**
 * @brief รีเซ็ตตำแหน่งเป็น 0 (home)
 *
 * @param drv pointer ไปยัง DRV8825_Instance (NULL หรือไม่ initialized → return ทันที)
 *
 * @note ตั้ง drv->position_steps = 0
 *       ไม่ได้ทำให้ motor หมุน — แค่รีเซ็ต counter
 *       ใช้หลังจากทำ homing sequence ด้วยตัวเอง
 */
void DRV8825_ResetPosition(DRV8825_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    drv->position_steps = 0;
}
