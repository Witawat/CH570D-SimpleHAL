# DRV8825 Library

> **Library สำหรับควบคุม DRV8825 บน CH57x**
> รองรับ STEP+DIR+EN, ควบคุมด้วย steps/degrees/revolutions และระยะเชิงเส้น mm

## สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Troubleshooting](#troubleshooting)
8. [API Reference](#api-reference)

---

## ภาพรวม

DRV8825 เป็นไดรเวอร์สเต็ปเปอร์แบบ STEP/DIR ที่นิยมใช้งานกับ NEMA17/NEMA23
เหมาะกับงานที่ต้องการความเร็วสูงกว่า ULN2003 และรองรับ microstepping ผ่านขา M0/M1/M2 บนบอร์ด

ไลบรารีนี้ออกแบบให้ใช้งานแบบ Arduino-style บน CH57x โดยเน้น:
- ใช้งานง่าย (init + set speed + move)
- รองรับการคำนวณระยะ mm
- ติดตามตำแหน่งปัจจุบันของแกน (position tracking)

---

## คุณสมบัติ

- รองรับไดรเวอร์ DRV8825 แบบ STEP+DIR+EN
- init แบบทั่วไป และ preset สำหรับ NEMA17 (200 steps/rev)
- ควบคุมความเร็วด้วย RPM
- สั่งหมุนด้วย steps, degrees, revolutions
- สั่งหมุนเป็นระยะ mm
- แปลงค่า mm เป็น steps และ steps เป็น mm
- ติดตามตำแหน่งปัจจุบัน และ reset home
- รองรับกลับทิศทางลอจิกด้วย `DRV8825_SetDirectionInverted`

---

## Hardware Setup

### Wiring ตัวอย่าง

| CH57x | DRV8825 | หมายเหตุ |
|---------|---------|----------|
| PC0 | STEP | pulse step |
| PC1 | DIR  | direction |
| PC2 | EN   | optional, active low |
| GND | GND  | ต้องกราวด์ร่วมกัน |
| 3.3V | VDD | logic power |
| PSU 12V | VMOT | motor power |

> ข้อควรระวัง
> - ควรมี capacitor ใกล้ VMOT ตามคำแนะนำบอร์ด
> - ต้องตั้ง current limit ก่อนใช้งานจริง
> - MCU และไดรเวอร์ต้องใช้ GND ร่วมกันเสมอ

### ตาราง microstep (ตั้งที่ฮาร์ดแวร์)

| โหมด | steps/rev ของ NEMA17 |
|------|-----------------------|
| Full (1/1) | 200 |
| 1/2 | 400 |
| 1/4 | 800 |
| 1/8 | 1600 |
| 1/16 | 3200 |
| 1/32 | 6400 |

> เมื่อเปลี่ยน microstep ต้องเปลี่ยน `steps_per_rev` ให้ตรง เพื่อให้มุมและระยะไม่เพี้ยน

---

## หลักการทำงาน

### STEP DIR EN

- DIR กำหนดทิศทาง
- STEP รับ pulse แต่ละลูก = 1 step
- EN เป็น active low ในบอร์ดส่วนใหญ่

```
STEP:  _|‾|_|‾|_|‾|_
          1   2   3   ... (จำนวน pulse = จำนวน step)
```

### สูตรคำนวณระยะเชิงเส้น

```
steps = distance_mm * steps_per_rev / mm_per_rev
distance_mm = steps * mm_per_rev / steps_per_rev
```

---

## การใช้งานพื้นฐาน

### Quick Start: NEMA17

```c
#include "SimpleHAL.h"
#include "DRV8825.h"

DRV8825_Instance drv;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    DRV8825_InitNEMA17(&drv, PC0, PC1, PC2);  // 200 steps/rev
    DRV8825_SetSpeed(&drv, 120);              // 120 RPM

    DRV8825_Enable(&drv);
    DRV8825_MoveRevolutions(&drv, 2);
    DRV8825_MoveDegrees(&drv, -90);
    DRV8825_Disable(&drv);

    while (1) {}
}
```

### Quick Start: กำหนด microstep เอง

```c
// NEMA17 + microstep 1/8 => 1600 steps/rev
DRV8825_Init(&drv, PC0, PC1, PC2, 1600);
DRV8825_SetSpeed(&drv, 90);
```

---

## การใช้งานขั้นสูง

### 1) สั่งงานระยะ mm

```c
DRV8825_SetLinearMmPerRev(&drv, 8.0f);   // lead screw 8 mm/rev

DRV8825_Enable(&drv);
DRV8825_MoveMm(&drv, 25.0f);             // ไปข้างหน้า 25mm
DRV8825_MoveMm(&drv, -3.5f);             // ถอย 3.5mm
DRV8825_Disable(&drv);
```

### 2) แปลงหน่วยก่อนขยับจริง

```c
int32_t s = DRV8825_MmToSteps(&drv, 12.5f);
DRV8825_Move(&drv, s);
```

### 3) ติดตามตำแหน่งและกลับ Home

```c
int32_t pos = DRV8825_GetPosition(&drv);
DRV8825_Move(&drv, -pos);
DRV8825_ResetPosition(&drv);
```

### 4) กลับทิศ DIR โดยไม่สลับสาย

```c
DRV8825_SetDirectionInverted(&drv, 1);
```

---

## Troubleshooting

### ปัญหา: มอเตอร์ไม่หมุน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| EN ไม่ได้ถูกดึง LOW | เรียก `DRV8825_Enable()` ก่อน move |
| ไม่ได้กราวด์ร่วมกัน | ต่อ GND ร่วม MCU และไดรเวอร์ |
| VMOT ไม่เข้า | ตรวจไฟหลักและ fuse/สาย |

### ปัญหา: สั่นแต่ไม่ไป

| สาเหตุ | วิธีแก้ |
|--------|---------|
| RPM สูงเกินช่วงแรงบิด | ลด RPM แล้วไต่ขึ้น |
| current limit ต่ำเกิน | ปรับ Vref ให้พอดี |
| steps_per_rev ไม่ตรง microstep | แก้ค่า init ให้ถูกต้อง |

### ปัญหา: ระยะ mm คลาดเคลื่อน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ตั้ง `mm_per_rev` ผิด | วัดกลไกจริงแล้วตั้งใหม่ |
| microstep ไม่ตรงกับค่าในโค้ด | ตรวจ M0/M1/M2 และ steps/rev |

---

## API Reference

### Initialization

- `DRV8825_Init(drv, step_pin, dir_pin, en_pin, steps_per_rev)`
- `DRV8825_InitNEMA17(drv, step_pin, dir_pin, en_pin)`

### Basic Control

- `DRV8825_SetSpeed(drv, rpm)`
- `DRV8825_SetDirectionInverted(drv, inverted)`
- `DRV8825_Enable(drv)` / `DRV8825_Disable(drv)`
- `DRV8825_Stop(drv)`

### Motion

- `DRV8825_Move(drv, steps)`
- `DRV8825_MoveDegrees(drv, degrees)`
- `DRV8825_MoveRevolutions(drv, revolutions)`

### Linear Motion

- `DRV8825_SetLinearMmPerRev(drv, mm_per_rev)`
- `DRV8825_MmToSteps(drv, distance_mm)`
- `DRV8825_StepsToMm(drv, steps)`
- `DRV8825_MoveMm(drv, distance_mm)`

### Position

- `DRV8825_GetPosition(drv)`
- `DRV8825_ResetPosition(drv)`

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)