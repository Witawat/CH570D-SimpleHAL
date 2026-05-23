/**
 * @file TMC220x.c
 * @brief TMC2208/TMC2209 Stepper Driver Library Implementation
 */

#include "TMC220x.h"

/**
 * @brief คำนวณ delay ระหว่าง step pulse (us)
 *
 * @param rpm - รอบต่อนาที (0 → ใช้ 1)
 * @param steps_per_rev - steps ต่อรอบ (0 → ใช้ default 200)
 *
 * @return delay (us)
 *
 * @note formula: 60,000,000 / (rpm × steps_per_rev)
 */
static uint32_t _tmc220x_calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0U) rpm = 1U;
    if (steps_per_rev == 0U) steps_per_rev = TMC220X_NEMA17_STEPS_PER_REV;
    return 60000000UL / (rpm * steps_per_rev);
}

/**
 * @brief ตั้งค่า DIR pin level
 *
 * @param drv - instance
 * @param is_cw - 1=CW, 0=CCW
 *
 * @note ถ้า dir_inverted=1 → กลับ logic
 */
static void _tmc220x_set_dir(TMC220x_Instance* drv, uint8_t is_cw) {
    uint8_t dir_level = is_cw ? HIGH : LOW;
    if (drv->dir_inverted) {
        dir_level = (dir_level == HIGH) ? LOW : HIGH;
    }
    digitalWrite(drv->pin_dir, dir_level);
}

/**
 * @brief ส่ง 1 step pulse (HIGH 2us → LOW)
 *
 * @param drv - instance
 *
 * @note pulse width = 2us HIGH
 *       LOW ค้างตาม step_delay_us
 *       โดยรวม delay = 2 + step_delay_us us/step
 */
static void _tmc220x_pulse_step(TMC220x_Instance* drv) {
    digitalWrite(drv->pin_step, HIGH);
    Delay_Us(2);
    digitalWrite(drv->pin_step, LOW);
    Delay_Us(drv->step_delay_us);
}

/**
 * @brief เริ่มต้น TMC220x stepper driver
 *
 * @param drv - instance (NULL → return)
 * @param model - TMC220X_MODEL_2208 หรือ TMC220X_MODEL_2209
 * @param step_pin - STEP pin number
 * @param dir_pin - DIR pin number
 * @param en_pin - ENABLE pin number (0 = ไม่มี pin enable)
 * @param steps_per_rev - จำนวน step ต่อรอบ (0 = ใช้ default 200)
 *
 * @note ตั้ง pinMode OUTPUT ทั้งหมด
 *       STEP/DIR = LOW, ENABLE = HIGH (disable)
 *       speed_rpm default = 60
 *       has_en=0 เมื่อ en_pin=0
 */
void TMC220x_Init(TMC220x_Instance* drv,
                  TMC220x_Model model,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint32_t steps_per_rev) {
    if (drv == NULL) return;

    drv->model = model;
    drv->pin_step = step_pin;
    drv->pin_dir = dir_pin;
    drv->pin_en = en_pin;
    drv->has_en = (en_pin != 0U) ? 1U : 0U;

    drv->dir_inverted = 0U;

    drv->steps_per_rev = (steps_per_rev > 0U) ? steps_per_rev : TMC220X_NEMA17_STEPS_PER_REV;
    drv->linear_mm_per_rev = 0.0f;
    drv->speed_rpm = 60U;
    drv->step_delay_us = _tmc220x_calc_step_delay_us(drv->speed_rpm, drv->steps_per_rev);

    drv->position_steps = 0;
    drv->is_enabled = 0U;
    drv->initialized = 1U;

    pinMode(step_pin, PIN_MODE_OUTPUT);
    pinMode(dir_pin, PIN_MODE_OUTPUT);
    digitalWrite(step_pin, LOW);
    digitalWrite(dir_pin, LOW);

    if (drv->has_en) {
        pinMode(en_pin, PIN_MODE_OUTPUT);
        digitalWrite(en_pin, HIGH);  // HIGH = disable on most TMC220x modules
    }
}

/**
 * @brief เริ่มต้น TMC220x สำหรับ NEMA17 (steps_per_rev=200)
 *
 * @param drv - instance
 * @param model - TMC220X_MODEL_2208 หรือ TMC220X_MODEL_2209
 * @param step_pin - STEP pin
 * @param dir_pin - DIR pin
 * @param en_pin - ENABLE pin (0 = ไม่มี)
 *
 * @note wrapper ของ TMC220x_Init() ใช้ 200 steps/rev
 */
void TMC220x_InitNEMA17(TMC220x_Instance* drv,
                        TMC220x_Model model,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin) {
    TMC220x_Init(drv, model, step_pin, dir_pin, en_pin, TMC220X_NEMA17_STEPS_PER_REV);
}

/**
 * @brief ตั้งค่าความเร็วรอบ
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param rpm - รอบต่อนาที (ถ้า < 1 → ใช้ 1)
 *
 * @note คำนวณ step_delay_us = 60,000,000 / (rpm × steps_per_rev)
 *       ไม่จำกัดค่าสูงสุด — ขึ้นกับ hardware limit
 */
void TMC220x_SetSpeed(TMC220x_Instance* drv, uint32_t rpm) {
    if (drv == NULL || !drv->initialized) return;
    if (rpm < 1U) rpm = 1U;
    drv->speed_rpm = rpm;
    drv->step_delay_us = _tmc220x_calc_step_delay_us(rpm, drv->steps_per_rev);
}

/**
 * @brief กลับทิศทางมอเตอร์ (invert DIR logic)
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param inverted - 1=กลับทิศ, 0=ปกติ
 *
 * @note ส่งผลต่อ TMC220x_Move() และฟังก์ชันอื่นที่เรียก _tmc220x_set_dir()
 */
void TMC220x_SetDirectionInverted(TMC220x_Instance* drv, uint8_t inverted) {
    if (drv == NULL || !drv->initialized) return;
    drv->dir_inverted = inverted ? 1U : 0U;
}

/**
 * @brief เปิดการทำงานของมอเตอร์ (ENABLE = LOW)
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 *
 * @note ถ้า has_en=0 → แค่ตั้ง flag is_enabled
 *       TMC220x ปกติ active LOW enable
 */
void TMC220x_Enable(TMC220x_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, LOW);
    }
    drv->is_enabled = 1U;
}

/**
 * @brief ปิดการทำงานของมอเตอร์ (ENABLE = HIGH)
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 *
 * @note มอเตอร์จะหยุดจับ torque (coast)
 *       ต้องเรียก TMC220x_Enable() เพื่อเริ่มใหม่
 */
void TMC220x_Disable(TMC220x_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, HIGH);
    }
    drv->is_enabled = 0U;
}

/**
 * @brief หยุด STEP pulse ทันที (STEP = LOW)
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 *
 * @note ใช้ stop แบบฉุกเฉิน — ไม่ต้องรอให้ step cycle จบ
 *       ไม่ได้เปลี่ยน DIR, ENABLE หรือ position_steps
 */
void TMC220x_Stop(TMC220x_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    digitalWrite(drv->pin_step, LOW);
}
    if (drv == NULL || !drv->initialized) return;
    digitalWrite(drv->pin_step, LOW);
}

/**
 * @brief เคลื่อนที่ตามจำนวน step (blocking)
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param steps - จำนวน step (+ = CW, - = CCW, 0 = return)
 *
 * @note blocking — รอจนครบทุก step pulse
 *       อัปเดต position_steps ตามทิศทาง
 *       delay ระหว่าง pulse ขึ้นกับ speed ที่ตั้งไว้
 */
void TMC220x_Move(TMC220x_Instance* drv, int32_t steps) {
    int32_t n;

    if (drv == NULL || !drv->initialized) return;
    if (steps == 0) return;

    if (steps > 0) {
        _tmc220x_set_dir(drv, 1U);
        n = steps;
        while (n-- > 0) {
            _tmc220x_pulse_step(drv);
            drv->position_steps++;
        }
    } else {
        _tmc220x_set_dir(drv, 0U);
        n = -steps;
        while (n-- > 0) {
            _tmc220x_pulse_step(drv);
            drv->position_steps--;
        }
    }
}

/**
 * @brief เคลื่อนที่ตามองศา
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param degrees - (+ = CW, - = CCW)
 *
 * @note แปลงองศา → steps: steps = degrees × steps_per_rev / 360
 *       blocking (เรียก TMC220x_Move)
 */
void TMC220x_MoveDegrees(TMC220x_Instance* drv, int32_t degrees) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)degrees * (int64_t)drv->steps_per_rev / 360LL);
    TMC220x_Move(drv, steps);
}

/**
 * @brief เคลื่อนที่ตามจำนวนรอบ
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param revolutions - (+ = CW, - = CCW)
 *
 * @note แปลงรอบ → steps: steps = revolutions × steps_per_rev
 *       blocking (เรียก TMC220x_Move)
 */
void TMC220x_MoveRevolutions(TMC220x_Instance* drv, int32_t revolutions) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)revolutions * (int64_t)drv->steps_per_rev);
    TMC220x_Move(drv, steps);
}

/**
 * @brief ตั้งค่า linear distance ต่อรอบ (mm/rev)
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param mm_per_rev - mm ต่อรอบ (≤0 = ไม่ใช้ linear mode)
 *
 * @note จำเป็นสำหรับ TMC220x_MoveMm()
 *       เช่น lead screw pitch 8mm/rev
 */
void TMC220x_SetLinearMmPerRev(TMC220x_Instance* drv, float mm_per_rev) {
    if (drv == NULL || !drv->initialized) return;
    if (mm_per_rev <= 0.0f) {
        drv->linear_mm_per_rev = 0.0f;
        return;
    }
    drv->linear_mm_per_rev = mm_per_rev;
}

/**
 * @brief แปลงระยะทาง mm → steps
 *
 * @param drv - instance (NULL หรือ !initialized → return 0)
 * @param distance_mm - ระยะทาง mm (+ = CW, - = CCW)
 *
 * @return จำนวน steps หรือ 0 ถ้า linear_mm_per_rev ไม่ได้ตั้ง
 *
 * @note ต้องการ TMC220x_SetLinearMmPerRev() ก่อน
 *       ปัดเศษ half-up
 */
int32_t TMC220x_MmToSteps(TMC220x_Instance* drv, float distance_mm) {
    float steps_f;
    if (drv == NULL || !drv->initialized) return 0;
    if (drv->linear_mm_per_rev <= 0.0f) return 0;

    steps_f = (distance_mm * (float)drv->steps_per_rev) / drv->linear_mm_per_rev;
    if (steps_f >= 0.0f) return (int32_t)(steps_f + 0.5f);
    return (int32_t)(steps_f - 0.5f);
}

/**
 * @brief แปลง steps → ระยะทาง mm
 *
 * @param drv - instance (NULL หรือ !initialized → return 0)
 * @param steps - จำนวน step
 *
 * @return ระยะทาง mm หรือ 0 ถ้า linear_mm_per_rev ไม่ได้ตั้ง
 */
float TMC220x_StepsToMm(TMC220x_Instance* drv, int32_t steps) {
    if (drv == NULL || !drv->initialized) return 0.0f;
    if (drv->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * drv->linear_mm_per_rev) / (float)drv->steps_per_rev;
}

/**
 * @brief เคลื่อนที่ตามระยะทาง mm
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 * @param distance_mm - ระยะทาง mm (+ = CW, - = CCW)
 *
 * @note เรียก TMC220x_MmToSteps() → TMC220x_Move()
 *       blocking
 */
void TMC220x_MoveMm(TMC220x_Instance* drv, float distance_mm) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = TMC220x_MmToSteps(drv, distance_mm);
    TMC220x_Move(drv, steps);
}

/**
 * @brief อ่านตำแหน่งปัจจุบัน (step counter)
 *
 * @param drv - instance (NULL หรือ !initialized → return 0)
 *
 * @return ตำแหน่งสะสม (steps)
 *
 * @note position_steps เพิ่ม/ลดตาม TMC220x_Move()
 *       ไม่ได้สัมพันธ์กับตำแหน่งจริงของ rotor
 */
int32_t TMC220x_GetPosition(TMC220x_Instance* drv) {
    if (drv == NULL || !drv->initialized) return 0;
    return drv->position_steps;
}

/**
 * @brief รีเซ็ต position_steps เป็น 0
 *
 * @param drv - instance (NULL หรือ !initialized → return)
 *
 * @note ไม่ได้ส่งผลต่อ hardware
 */
void TMC220x_ResetPosition(TMC220x_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    drv->position_steps = 0;
}
