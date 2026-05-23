/**
 * @file TMC5160.c
 * @brief TMC5160 Stepper Driver Library Implementation
 */

#include "TMC5160.h"

#define TMC5160_WRITE_BIT 0x80U

/**
 * @brief คำนวณ step delay (µs) จาก RPM
 *
 * @param rpm - รอบต่อนาที
 * @param steps_per_rev - step ต่อรอบ
 *
 * @return delay (µs) ระหว่างแต่ละ step
 *
 * @note สูตร: 60,000,000 / (rpm × steps_per_rev)
 */
static uint32_t _tmc5160_calc_step_delay_us(uint32_t rpm, uint32_t steps_per_rev) {
    if (rpm == 0U) rpm = 1U;
    if (steps_per_rev == 0U) steps_per_rev = TMC5160_NEMA17_STEPS_PER_REV;
    return 60000000UL / (rpm * steps_per_rev);
}

/**
 * @brief ตั้งทิศทางการหมุน (CW/CCW)
 *
 * @param drv - instance
 * @param is_cw - 1 = ตามเข็ม, 0 = ทวนเข็ม
 *
 * @note ถ้า dir_inverted == 1 จะกลับทิศ
 */
static void _tmc5160_set_dir(TMC5160_Instance* drv, uint8_t is_cw) {
    uint8_t dir_level = is_cw ? HIGH : LOW;
    if (drv->dir_inverted) {
        dir_level = (dir_level == HIGH) ? LOW : HIGH;
    }
    digitalWrite(drv->pin_dir, dir_level);
}

/**
 * @brief สร้าง step pulse (HIGH 2µs → LOW)
 *
 * @param drv - instance
 *
 * @note pulse HIGH = 2µs จากนั้น LOW ตาม step_delay_us
 */
static void _tmc5160_pulse_step(TMC5160_Instance* drv) {
    digitalWrite(drv->pin_step, HIGH);
    Delay_Us(2);
    digitalWrite(drv->pin_step, LOW);
    Delay_Us(drv->step_delay_us);
}

/**
 * @brief set CS pin = LOW (start SPI transaction)
 *
 * @param drv - instance
 */
static void _tmc5160_cs_low(TMC5160_Instance* drv) {
    digitalWrite(drv->pin_cs, LOW);
}

/**
 * @brief set CS pin = HIGH (end SPI transaction)
 *
 * @param drv - instance
 */
static void _tmc5160_cs_high(TMC5160_Instance* drv) {
    digitalWrite(drv->pin_cs, HIGH);
}

/**
 * @brief เริ่มต้น TMC5160 stepper driver
 *
 * @param drv - instance ที่จะ init
 * @param step_pin - GPIO pin สำหรับ step pulse
 * @param dir_pin - GPIO pin สำหรับ direction
 * @param en_pin - GPIO pin สำหรับ enable (0 = no enable pin)
 * @param cs_pin - GPIO pin สำหรับ SPI chip select
 * @param steps_per_rev - จำนวน step ต่อรอบ (เช่น 200 สำหรับ NEMA17)
 *
 * @note ตั้งค่า GPIO pins (step, dir, cs เป็น output)
 *       ตั้ง speed default = 60 RPM
 *       enable pin เริ่มต้นเป็น HIGH (disable)
 *       SPI ยังไม่ init — ต้องเรียก TMC5160_SPIBegin() แยก
 */
void TMC5160_Init(TMC5160_Instance* drv,
                  uint8_t step_pin,
                  uint8_t dir_pin,
                  uint8_t en_pin,
                  uint8_t cs_pin,
                  uint32_t steps_per_rev) {
    if (drv == NULL) return;

    drv->pin_step = step_pin;
    drv->pin_dir = dir_pin;
    drv->pin_en = en_pin;
    drv->pin_cs = cs_pin;
    drv->has_en = (en_pin != 0U) ? 1U : 0U;

    drv->dir_inverted = 0U;

    drv->steps_per_rev = (steps_per_rev > 0U) ? steps_per_rev : TMC5160_NEMA17_STEPS_PER_REV;
    drv->linear_mm_per_rev = 0.0f;
    drv->speed_rpm = 60U;
    drv->step_delay_us = _tmc5160_calc_step_delay_us(drv->speed_rpm, drv->steps_per_rev);

    drv->position_steps = 0;
    drv->is_enabled = 0U;
    drv->spi_ready = 0U;
    drv->initialized = 1U;

    pinMode(step_pin, PIN_MODE_OUTPUT);
    pinMode(dir_pin, PIN_MODE_OUTPUT);
    pinMode(cs_pin, PIN_MODE_OUTPUT);

    digitalWrite(step_pin, LOW);
    digitalWrite(dir_pin, LOW);
    _tmc5160_cs_high(drv);

    if (drv->has_en) {
        pinMode(en_pin, PIN_MODE_OUTPUT);
        digitalWrite(en_pin, HIGH);  // HIGH = disable
    }
}

/**
 * @brief เริ่มต้น TMC5160 สำหรับ NEMA17 motor (200 steps/rev)
 *
 * @param drv - instance
 * @param step_pin, dir_pin, en_pin, cs_pin - GPIO pins
 *
 * @note convenience wrapper — 200 steps/rev
 */
void TMC5160_InitNEMA17(TMC5160_Instance* drv,
                        uint8_t step_pin,
                        uint8_t dir_pin,
                        uint8_t en_pin,
                        uint8_t cs_pin) {
    TMC5160_Init(drv, step_pin, dir_pin, en_pin, cs_pin, TMC5160_NEMA17_STEPS_PER_REV);
}

/**
 * @brief เริ่มต้น SPI สำหรับ TMC5160 register access
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param mode - SPI mode (SPI_MODE0, etc.)
 * @param speed - SPI speed (SPI_1MHZ, SPI_2MHZ, etc.)
 * @param pin_cfg - SPI pin config
 *
 * @note เรียก SPI_SimpleInit() และ set spi_ready flag
 *       driver ต้องมี spi_ready = 1 ก่อนเรียก WriteReg/ReadReg
 */
void TMC5160_SPIBegin(TMC5160_Instance* drv, SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg) {
    if (drv == NULL || !drv->initialized) return;
    SPI_SimpleInit(mode, speed, pin_cfg);
    drv->spi_ready = 1U;
}

/**
 * @brief กำหนดความเร็วรอบ motor
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param rpm - ความเร็ว (รอบต่อนาที, minimum = 1)
 *
 * @note คำนวณ step_delay_us = 60,000,000 / (rpm × steps_per_rev)
 *       ค่า RPM ต่ำ = หน่วง step มากขึ้น
 */
void TMC5160_SetSpeed(TMC5160_Instance* drv, uint32_t rpm) {
    if (drv == NULL || !drv->initialized) return;
    if (rpm < 1U) rpm = 1U;
    drv->speed_rpm = rpm;
    drv->step_delay_us = _tmc5160_calc_step_delay_us(rpm, drv->steps_per_rev);
}

/**
 * @brief กลับทิศทางการหมุน
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param inverted - 1 = กลับทิศ, 0 = ปกติ
 *
 * @note สลับ HIGH/LOW ที่ dir pin ตามค่า inverted
 */
void TMC5160_SetDirectionInverted(TMC5160_Instance* drv, uint8_t inverted) {
    if (drv == NULL || !drv->initialized) return;
    drv->dir_inverted = inverted ? 1U : 0U;
}

/**
 * @brief เปิดการทำงานของ motor (enable)
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 *
 * @note set enable pin = LOW (active low)
 *       ต่อให้ step pulse ได้
 */
void TMC5160_Enable(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, LOW);
    }
    drv->is_enabled = 1U;
}

/**
 * @brief ปิดการทำงานของ motor (disable)
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 *
 * @note set enable pin = HIGH (disable)
 *       motor คลายแรงบิด
 */
void TMC5160_Disable(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    if (drv->has_en) {
        digitalWrite(drv->pin_en, HIGH);
    }
    drv->is_enabled = 0U;
}

/**
 * @brief หยุด motor ทันที
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 *
 * @note set step pin = LOW
 *       หยุดส่ง step pulse
 */
void TMC5160_Stop(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    digitalWrite(drv->pin_step, LOW);
}

/**
 * @brief หมุน motor ตามจำนวน step
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param steps - จำนวน step (บวก = CW, ลบ = CCW)
 *
 * @note ตรวจสอบทิศทางจาก sign ของ steps
 *       ใช้ blocking delay ระหว่างแต่ละ step
 *       ตำแหน่งถูกติดตามใน position_steps
 */
void TMC5160_Move(TMC5160_Instance* drv, int32_t steps) {
    int32_t n;

    if (drv == NULL || !drv->initialized) return;
    if (steps == 0) return;

    if (steps > 0) {
        _tmc5160_set_dir(drv, 1U);
        n = steps;
        while (n-- > 0) {
            _tmc5160_pulse_step(drv);
            drv->position_steps++;
        }
    } else {
        _tmc5160_set_dir(drv, 0U);
        n = -steps;
        while (n-- > 0) {
            _tmc5160_pulse_step(drv);
            drv->position_steps--;
        }
    }
}

/**
 * @brief หมุน motor ตามจำนวนองศา
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param degrees - องศาที่ต้องการหมุน (+ = CW, - = CCW)
 *
 * @note แปลงองศา → steps โดย steps_per_rev / 360
 *       แล้วเรียก TMC5160_Move()
 */
void TMC5160_MoveDegrees(TMC5160_Instance* drv, int32_t degrees) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)degrees * (int64_t)drv->steps_per_rev / 360LL);
    TMC5160_Move(drv, steps);
}

/**
 * @brief หมุน motor ตามจำนวนรอบ
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param revolutions - จำนวนรอบ (+ = CW, - = CCW)
 *
 * @note steps = revolutions × steps_per_rev
 */
void TMC5160_MoveRevolutions(TMC5160_Instance* drv, int32_t revolutions) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = (int32_t)((int64_t)revolutions * (int64_t)drv->steps_per_rev);
    TMC5160_Move(drv, steps);
}

/**
 * @brief ตั้งค่าระยะทางเชิงเส้นต่อรอบ (สำหรับ linear motion)
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param mm_per_rev - ระยะทาง mm ต่อ 1 รอบ (เช่น lead screw pitch)
 *
 * @note ใช้สำหรับแปลง steps ↔ mm
 *       ถ้า ≤ 0 จะ disable linear mode
 */
void TMC5160_SetLinearMmPerRev(TMC5160_Instance* drv, float mm_per_rev) {
    if (drv == NULL || !drv->initialized) return;
    if (mm_per_rev <= 0.0f) {
        drv->linear_mm_per_rev = 0.0f;
        return;
    }
    drv->linear_mm_per_rev = mm_per_rev;
}

/**
 * @brief แปลงระยะทาง mm เป็น steps
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param distance_mm - ระยะทาง (mm)
 *
 * @return จำนวน steps (ปัดเศษ), 0 ถ้า linear_mm_per_rev == 0
 *
 * @note ใช้ linear_mm_per_rev และ steps_per_rev ในการคำนวณ
 */
int32_t TMC5160_MmToSteps(TMC5160_Instance* drv, float distance_mm) {
    float steps_f;
    if (drv == NULL || !drv->initialized) return 0;
    if (drv->linear_mm_per_rev <= 0.0f) return 0;

    steps_f = (distance_mm * (float)drv->steps_per_rev) / drv->linear_mm_per_rev;
    if (steps_f >= 0.0f) return (int32_t)(steps_f + 0.5f);
    return (int32_t)(steps_f - 0.5f);
}

/**
 * @brief แปลง steps เป็นระยะทาง mm
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param steps - จำนวน steps
 *
 * @return ระยะทาง (mm), 0.0 ถ้า linear_mm_per_rev == 0
 */
float TMC5160_StepsToMm(TMC5160_Instance* drv, int32_t steps) {
    if (drv == NULL || !drv->initialized) return 0.0f;
    if (drv->linear_mm_per_rev <= 0.0f) return 0.0f;
    return ((float)steps * drv->linear_mm_per_rev) / (float)drv->steps_per_rev;
}

/**
 * @brief หมุน motor ตามระยะทาง mm
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param distance_mm - ระยะทาง (mm, + = forward, - = backward)
 *
 * @note แปลง mm → steps แล้วเรียก TMC5160_Move()
 *       ต้องตั้งค่า linear_mm_per_rev ก่อน
 */
void TMC5160_MoveMm(TMC5160_Instance* drv, float distance_mm) {
    int32_t steps;
    if (drv == NULL || !drv->initialized) return;
    steps = TMC5160_MmToSteps(drv, distance_mm);
    TMC5160_Move(drv, steps);
}

/**
 * @brief อ่านตำแหน่งปัจจุบัน (steps)
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 *
 * @return ตำแหน่ง (steps), 0 ถ้า drv == NULL
 *
 * @note track โดย increment/decrement ใน TMC5160_Move()
 *       ไม่ได้อ่านจาก hardware register
 */
int32_t TMC5160_GetPosition(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return 0;
    return drv->position_steps;
}

/**
 * @brief รีเซ็ตตำแหน่งเป็น 0
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 *
 * @note ใช้สำหรับตั้งค่า home position
 *       ไม่ได้เขียนถึง hardware register
 */
void TMC5160_ResetPosition(TMC5160_Instance* drv) {
    if (drv == NULL || !drv->initialized) return;
    drv->position_steps = 0;
}

/**
 * @brief เขียน register ผ่าน SPI
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param reg_addr - register address (7-bit)
 * @param value - 32-bit value ที่จะเขียน
 *
 * @note ใช้ datagram: [W/R bit | reg_addr : 1 byte] + [data : 4 bytes]
 *       W/R bit = 1 สำหรับ write (0x80 | reg_addr)
 *       ต้องเรียก TMC5160_SPIBegin() ก่อน
 */
void TMC5160_WriteReg(TMC5160_Instance* drv, uint8_t reg_addr, uint32_t value) {
    if (drv == NULL || !drv->initialized) return;
    if (!drv->spi_ready) return;

    _tmc5160_cs_low(drv);
    SPI_Transfer((uint8_t)(reg_addr | TMC5160_WRITE_BIT));
    SPI_Transfer((uint8_t)((value >> 24) & 0xFFU));
    SPI_Transfer((uint8_t)((value >> 16) & 0xFFU));
    SPI_Transfer((uint8_t)((value >> 8) & 0xFFU));
    SPI_Transfer((uint8_t)(value & 0xFFU));
    _tmc5160_cs_high(drv);
}

/**
 * @brief อ่าน register ผ่าน SPI
 *
 * @param drv - instance ที่ผ่านการ init แล้ว
 * @param reg_addr - register address (7-bit)
 *
 * @return 32-bit register value, 0 ถ้า drv == NULL หรือ SPI ไม่พร้อม
 *
 * @note ใช้ pipeline read: dummy read phase + actual read phase
 *       W/R bit = 0 สำหรับ read
 *       ต้องเรียก TMC5160_SPIBegin() ก่อน
 */
uint32_t TMC5160_ReadReg(TMC5160_Instance* drv, uint8_t reg_addr) {
    uint32_t value;

    if (drv == NULL || !drv->initialized) return 0U;
    if (!drv->spi_ready) return 0U;

    // Dummy read phase (pipeline)
    _tmc5160_cs_low(drv);
    SPI_Transfer((uint8_t)(reg_addr & 0x7FU));
    SPI_Transfer(0x00U);
    SPI_Transfer(0x00U);
    SPI_Transfer(0x00U);
    SPI_Transfer(0x00U);
    _tmc5160_cs_high(drv);

    // Actual read phase
    _tmc5160_cs_low(drv);
    SPI_Transfer((uint8_t)(reg_addr & 0x7FU));
    value  = ((uint32_t)SPI_Transfer(0x00U) << 24);
    value |= ((uint32_t)SPI_Transfer(0x00U) << 16);
    value |= ((uint32_t)SPI_Transfer(0x00U) << 8);
    value |= ((uint32_t)SPI_Transfer(0x00U));
    _tmc5160_cs_high(drv);

    return value;
}
