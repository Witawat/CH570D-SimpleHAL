# PWM

## Overview

โมดูล PWM5 แยกต่างหาก — 5 ช่องสัญญาณ (PWM1–PWM5) รองรับ 8-bit และ 16-bit mode พร้อม DMA, interrupt, และ synchronous output

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_pwm_init(ch, freq)` | เริ่มต้น 8-bit mode ความถี่ Hz (duty 0%) |
| `hal_pwm_init_16bit(ch, freq)` | เริ่มต้น 16-bit mode ความถี่ Hz (duty 0%) |
| `hal_pwm_set_duty(ch, pct)` | ตั้ง duty cycle 0–100% (8-bit) |
| `hal_pwm_set_duty_16bit(ch, val)` | ตั้ง duty cycle 0–period (16-bit) |
| `hal_pwm_set_freq(ch, freq)` | เปลี่ยนความถี่ (กรณี stop อยู่) |
| `hal_pwm_start(ch)` | เริ่ม output |
| `hal_pwm_stop(ch)` | หยุด output |

**Channel mapping (`PWM_Channel`):**

| ชื่อใน SimpleHAL | PWM module | ขา (fix) |
|---|---|---|
| `PWM1_CH1` | PWM1 | PA12 |
| `PWM1_CH2` | PWM2 | PA13 |
| `PWM1_CH3` | PWM3 | PA14 |
| `PWM1_CH4` | PWM4 | PA15 |
| `PWM2_CH1` | → HAL_PWM_CH5 | PA4 |

`PWM2_CH1` ถูก map เป็น channel 5 ภายใน (`_map_pwm_channel`) — ใช้คำสั่ง PWM เดียวกันกับช่องอื่น

## Circuit Guide

### PWM → LED (fade)

```
PA12 (PWM1_CH1) ──── R(220Ω) ──── LED(+) ──── LED(-) ──── GND
```

### PWM → Servo Motor

```
PA12 ──── Servo Signal (orange/white)
VCC_5V ──── Servo VCC (red) — ***ใช้แหล่งจ่าย 5V แยกต่างหาก***
GND ──── Servo GND (brown/black)
```

Servo ทั่วไปต้องการ pulse กว้าง 1–2ms ที่ 50Hz:
- `PWM_Init(PWM1_CH1, 50)` → period = 20ms
- `PWM_SetDuty(PWM1_CH1, 5)` → pulse ~1ms (0°)
- `PWM_SetDuty(PWM1_CH1, 10)` → pulse ~1.5ms (90°)
- `PWM_SetDuty(PWM1_CH1, 15)` → pulse ~2ms (180°)

### PWM → ESC (Electronic Speed Controller)

```
PA13 ──── ESC Signal
GND ──── ESC GND
※ ESC ส่วนใหญ่ต้องการ 50Hz เช่นกัน — ใช้ค่า duty 5–10%
```

## CH57x Specifics

- **8-bit mode:** period = 64–256 ticks (configurable via `PWMX_CycleCfg`)
- **16-bit mode:** period = 0–65535 (ตั้งผ่าน `hal_pwm_init_16bit`)
- **Polarity:** ตั้งได้ `High_Level` หรือ `Low_Level` (default = active high)
- **Synchronous mode:** PWM ทุกช่องเริ่ม/หยุดพร้อมกัน (sync start)
- **Interrupt:** แต่ละช่องมี interrupt แยกเมื่อ period ครบ

**Period vs Duty (8-bit, freq = 1kHz):**

| Duty % | Duty value | Output |
|---|---|---|
| 0% | 0 | LOW ตลอด |
| 25% | 64 | HIGH 25% |
| 50% | 128 | HIGH 50% |
| 100% | 255 | HIGH ตลอด |

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| `PWM_Init()` ซ้ำไม่ free | เรียก `PWM_Init` ซ้ำจะทับ handle โดยไม่ free ของเก่า → ควรเรียกครั้งเดียว |
| 16-bit period ดึงค่าจาก internal struct | `hal_pwm_obj.cycle_16bit` — ใช้ direct struct access |
| ความถี่ไม่ต่อเนื่อง | ความถี่ = sysclock / divider — ได้เฉพาะค่าที่หารลงตัว |
| PWM2_CH1 → CH5 | Map ผ่าน internal function — จำกัดที่ 5 ช่องสูงสุด |
| ไม่มี complementary output | ไม่มี dead-time insertion หรือ complimentary PWM |
| 8-bit duty 100% = HIGH ตลอด | ช่วง 99% → 100% ไม่มีการปรับ granularity |
| ขายึดตายตัว | PA12–PA15 + PA4 — ไม่สามารถ remap PWM output |

## Code สั้น

```c
// LED Fade
PWM_Init(PWM1_CH1, 1000);  // 1kHz
PWM_Start(PWM1_CH1);

for (int duty = 0; duty <= 100; duty++) {
    PWM_SetDuty(PWM1_CH1, duty);
    Delay_Ms(10);
}

// หรือใช้ HAL โดยตรง
hal_pwm_init(PWM1_CH1, 1000);
hal_pwm_set_duty(PWM1_CH1, 50);
hal_pwm_start(PWM1_CH1);

// 16-bit mode
hal_pwm_init_16bit(PWM1_CH2, 100);  // 100Hz, period = 100000 ticks
hal_pwm_set_duty_16bit(PWM1_CH2, 50000);  // 50%
hal_pwm_start(PWM1_CH2);
```
