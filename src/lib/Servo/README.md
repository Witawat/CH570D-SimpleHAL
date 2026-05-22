# Servo Motor Control Library

> **Library สำหรับควบคุม RC Servo Motor บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [หลักการทำงาน](#หลักการทำงาน)
4. [Hardware Setup](#hardware-setup)
5. [PWM Channels และ Pins](#pwm-channels-และ-pins)
6. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
7. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
8. [Calibration](#calibration)
9. [Troubleshooting](#troubleshooting)
10. [API Reference](#api-reference)

---

## ภาพรวม

Servo Motor เป็น actuator ที่ควบคุมมุมหมุนได้แม่นยำผ่าน PWM signal ใช้กันแพร่หลายในงาน robotics, RC car, camera gimbal, และโปรเจกต์ทั่วไป

### ประเภทของ Servo

| ประเภท | มุมหมุน | ตัวอย่าง | ใช้งาน |
|--------|---------|---------|--------|
| Standard | 0-180° | SG90, MG996R | แขนกล, ประตู |
| 360° / Continuous | หมุนต่อเนื่อง | FS90R | ล้อหุ่นยนต์ |
| Digital | 0-180° | Savöx | ความแม่นยำสูง |

---

## คุณสมบัติ

- ✅ ควบคุมมุม 0-180° ด้วยความละเอียดสูง (1µs)
- ✅ ปรับ pulse width โดยตรง (`WriteMicroseconds`)
- ✅ Smooth movement ด้วย `Servo_SweepTo()`
- ✅ Calibration ช่วง pulse min/max ต่อ instance
- ✅ Detach เพื่อประหยัดพลังงาน
- ✅ รองรับสูงสุด 8 servo (ขึ้นกับ PWM channels)
- ✅ Compatible กับ Continuous Rotation Servo
- ✅ เอกสารภาษาไทยครบถ้วน

---

## หลักการทำงาน

### PWM Servo Control

Servo ควบคุมด้วย PWM ความถี่ 50Hz (period = 20ms):

```
PWM Signal (50Hz = 20ms period):

  |←————————— 20ms ——————————→|
  |                           |
  ¯¯¯|_________________________|  ← 0°   (pulse 1000µs)
  ¯¯¯¯¯|_______________________|  ← 90°  (pulse 1500µs)
  ¯¯¯¯¯¯¯|_____________________|  ← 180° (pulse 2000µs)

  pulse 1000µs = 0°
  pulse 1500µs = 90° (กลาง)
  pulse 2000µs = 180°
```

### การแปลงมุม → Pulse Width

```
pulse_us = pulse_min + (angle / 180) × (pulse_max - pulse_min)

ตัวอย่าง (default: min=1000, max=2000):
  angle=0    → 1000 + (0/180)×1000   = 1000µs
  angle=90   → 1000 + (90/180)×1000  = 1500µs
  angle=180  → 1000 + (180/180)×1000 = 2000µs
```

---

## Hardware Setup

### การต่อวงจร

Servo มี 3 สาย (สีอาจต่างกันตามรุ่น):

```
Servo Wire Colors:
  Signal (White/Yellow/Orange) -----> PWM Pin (เช่น PC4)
  Power  (Red)                 -----> 5V (สำคัญ! ไม่ใช่ 3.3V)
  Ground (Brown/Black)         -----> GND

Circuit:
  5V ——————————————————————> Servo VCC (Red)
  GND —————————————————————> Servo GND (Brown/Black)
  PC4 —[ตรงๆ]——————————————> Servo Signal (White/Yellow)
```

> ⚠️ **สำคัญ**: Servo ต้องใช้ 5V (ไม่ใช่ 3.3V)  
> แต่ signal pin 3.3V ของ CH57x ทำงานได้กับ servo ส่วนใหญ่  
> สำหรับ servo หลายตัว หรือ servo กำลังสูง แนะนำใช้ power supply แยก

### Decoupling Capacitor (แนะนำ)

```
5V ——[100µF + 100nF]——> GND  (วางใกล้ servo power connector)
```

ช่วยลด voltage spike ที่เกิดจาก motor ขณะหมุน

---

## PWM Channels และ Pins

| Channel | Pin (Default) | หมายเหตุ |
|---------|--------------|---------|
| `PWM1_CH1` | PD2 | |
| `PWM1_CH2` | PA1 | |
| `PWM1_CH3` | PC3 | |
| `PWM1_CH4` | PC4 | แนะนำสำหรับ servo |
| `PWM2_CH1` | PD4 | |
| `PWM2_CH2` | PD3 | |
| `PWM2_CH3` | PC0 | |
| `PWM2_CH4` | PD7 | |

> ⚠️ **ข้อควรระวัง**: TIM1 และ TIM2 แชร์กันระหว่าง Servo และ SimplePWM  
> ห้ามใช้ servo กับ TIM เดียวกับ PWM ที่ใช้ความถี่อื่น

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "Servo.h"

Servo_Instance servo;  // ประกาศ global หรือ static
```

### ขั้นตอนที่ 2: Init และ Attach

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    // Init servo บน pin PC4 (PWM1_CH4)
    Servo_Init(&servo, PWM1_CH4);
    Servo_Attach(&servo);  // เริ่ม PWM output

    while (1) {
        Servo_Write(&servo, 0);    // หมุนไป 0°
        Delay_Ms(1000);
        Servo_Write(&servo, 90);   // หมุนไปตรงกลาง
        Delay_Ms(1000);
        Servo_Write(&servo, 180);  // หมุนไป 180°
        Delay_Ms(1000);
    }
}
```

---

## การใช้งานขั้นสูง

### Smooth Sweep Movement

```c
// หมุนจาก 0° ไป 180° อย่างช้าๆ (ทุก 1° ใช้เวลา 15ms = ทั้งหมด 2.7 วินาที)
Servo_SweepTo(&servo, 180, 15);

// หมุนกลับจาก 180° ไป 0°
Servo_SweepTo(&servo, 0, 15);
```

### ควบคุมหลาย Servo พร้อมกัน

```c
Servo_Instance servo_x;  // แกน X
Servo_Instance servo_y;  // แกน Y

Servo_Init(&servo_x, PWM1_CH3);   // PC3
Servo_Init(&servo_y, PWM1_CH4);   // PC4

Servo_Attach(&servo_x);
Servo_Attach(&servo_y);

// ควบคุมพร้อมกัน
Servo_Write(&servo_x, 90);
Servo_Write(&servo_y, 45);
```

### Camera Gimbal / Pan-Tilt

```c
// Pan-Tilt system ด้วย 2 servo
Servo_Instance pan;   // หมุนซ้าย-ขวา
Servo_Instance tilt;  // ก้ม-เงย

Servo_Init(&pan,  PWM1_CH3);
Servo_Init(&tilt, PWM1_CH4);
Servo_Attach(&pan);
Servo_Attach(&tilt);

// Scan รูปแบบ S-pattern
for (uint8_t row = 0; row <= 6; row++) {
    uint8_t tilt_angle = row * 30;
    Servo_Write(&tilt, tilt_angle);

    if (row % 2 == 0) {
        // sweep ซ้าย → ขวา
        Servo_SweepTo(&pan, 180, 10);
    } else {
        // sweep ขวา → ซ้าย
        Servo_SweepTo(&pan, 0, 10);
    }
    Delay_Ms(500);
}
```

### Continuous Rotation Servo (360°)

```c
// สำหรับ Continuous Rotation Servo
// 1500µs = หยุด
// <1500µs = หมุนทวนเข็ม (ความเร็วตามระยะห่างจาก 1500)
// >1500µs = หมุนตามเข็ม

Servo_Init(&servo, PWM1_CH4);
Servo_Attach(&servo);

// หมุนไปข้างหน้าเต็มกำลัง
Servo_WriteMicroseconds(&servo, 2000);
Delay_Ms(2000);

// หยุด
Servo_WriteMicroseconds(&servo, 1500);
Delay_Ms(1000);

// หมุนถอยหลังครึ่งกำลัง
Servo_WriteMicroseconds(&servo, 1250);
Delay_Ms(2000);

// หยุด
Servo_WriteMicroseconds(&servo, 1500);
```

### ประหยัดพลังงาน (Detach เมื่อไม่ใช้งาน)

```c
// เคลื่อน servo ไปตำแหน่งต้องการ
Servo_Write(&servo, 45);
Delay_Ms(500);  // รอให้ servo หมุนไปถึง

// ปิด PWM เพื่อประหยัดพลังงาน (servo จะไม่ค้างตำแหน่ง)
Servo_Detach(&servo);

Delay_Ms(5000);  // รอ 5 วินาที

// เปิดใหม่เมื่อต้องการใช้งาน
Servo_Attach(&servo);
```

---

## Calibration

### ทำไมต้อง Calibrate?

Servo แต่ละรุ่นมี pulse range ต่างกัน:
- Standard: 1000-2000µs  
- Wide-range: 500-2500µs  
- บางรุ่นอาจไม่หมุนถึง 0° หรือ 180° ด้วย default settings

### วิธี Calibrate

```c
// ขั้นตอนที่ 1: หา pulse ที่ทำให้หมุนถึง 0° จริงๆ
Servo_WriteMicroseconds(&servo, 800);   // ลองค่าต่างๆ
Servo_WriteMicroseconds(&servo, 900);
Servo_WriteMicroseconds(&servo, 1000);  // → บันทึกค่านี้เป็น min_us

// ขั้นตอนที่ 2: หา pulse สำหรับ 180°
Servo_WriteMicroseconds(&servo, 2000);  // ลองค่าต่างๆ
Servo_WriteMicroseconds(&servo, 2100);
Servo_WriteMicroseconds(&servo, 2200);  // → บันทึกค่านี้เป็น max_us

// ขั้นตอนที่ 3: ตั้งค่า range
Servo_SetPulseRange(&servo, 900, 2100);  // ใส่ค่าที่หาได้
```

### Pulse Range ของ Servo ทั่วไป

| รุ่น | Min (µs) | Max (µs) |
|------|----------|----------|
| SG90 | 500 | 2400 |
| MG996R | 600 | 2400 |
| MG90S | 500 | 2500 |
| Tower Pro Standard | 1000 | 2000 |

---

## Troubleshooting

### ปัญหา: Servo สั่นตลอดเวลา

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ไฟเลี้ยงไม่พอ | ใช้ 5V power supply แยก ≥500mA ต่อ servo |
| Noise บน signal | ต่อ capacitor 100nF บน power |
| PWM ไม่เสถียร | ตรวจสอบว่าไม่มี interrupt รบกวน |

### ปัญหา: หมุนไม่ครบมุม

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Pulse range ไม่ตรง | ทำ calibration ด้วย `Servo_SetPulseRange()` |
| Servo ผิดรุ่น | ตรวจสอบ datasheet |
| กลไกติด | ตรวจสอบส่วน mechanical |

### ปัญหา: Servo ไม่ขยับเลย

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ลืมเรียก `Servo_Attach()` | เรียก Attach ก่อน Write |
| PWM channel ผิด | ตรวจสอบ channel กับ pin ใน datasheet |
| ไฟ servo ไม่ต่อ | ตรวจสอบสาย VCC ของ servo |

---

## API Reference

### `Servo_Init(servo, channel)`
เริ่มต้น instance ด้วย PWM channel ที่เลือก

### `Servo_Attach(servo)`
เปิด PWM output ที่ 50Hz, servo พร้อมรับคำสั่ง

### `Servo_Detach(servo)`
ปิด PWM output, servo ไม่รับคำสั่งและไม่ค้างตำแหน่ง

### `Servo_Write(servo, angle)`
หมุนไปมุม 0-180°

### `Servo_WriteMicroseconds(servo, pulse_us)`
ตั้ง pulse width โดยตรง สำหรับ calibration หรือ continuous rotation

### `Servo_Read(servo)` → `uint8_t`
อ่านมุมปัจจุบัน

### `Servo_ReadMicroseconds(servo)` → `uint16_t`
อ่าน pulse width ปัจจุบัน (µs)

### `Servo_SetPulseRange(servo, min_us, max_us)`
กำหนด pulse range สำหรับ servo รุ่นนี้ (calibration)

### `Servo_SweepTo(servo, target, step_ms)`
หมุนไปยังมุมอย่างช้าๆ — blocking จนถึง target

### `Servo_IsAttached(servo)` → `bool`
ตรวจสอบว่า PWM กำลังทำงานอยู่หรือไม่

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)