# TMC2208 TMC2209 Library

> **Library สำหรับควบคุม TMC2208/TMC2209 บน CH57x**
> โฟกัสโหมด STEP+DIR+EN และรองรับการคำนวณระยะ mm

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

TMC2208 และ TMC2209 เป็นไดรเวอร์ที่ขึ้นชื่อเรื่องการทำงานนุ่มและเงียบ
ไลบรารีนี้เน้นใช้งานแบบ STEP/DIR เพื่อให้เริ่มต้นได้ง่ายและเสถียรบน CH57x

ความสามารถที่ครอบคลุมในเวอร์ชันนี้:
- สั่งงานมอเตอร์ด้วย pulse STEP
- เลือกโมเดลไดรเวอร์ 2208 หรือ 2209
- หมุนด้วย steps/degrees/revolutions/mm
- ติดตามตำแหน่งปัจจุบัน

> หมายเหตุสำคัญ
> - เวอร์ชันนี้ยังไม่รวม UART register configuration
> - ฟีเจอร์อย่างตั้ง run current/hold current ผ่าน UART ต้องใช้ไลบรารี UART เสริมในอนาคต

---

## คุณสมบัติ

- รองรับทั้ง `TMC220X_MODEL_2208` และ `TMC220X_MODEL_2209`
- init แบบทั่วไป และ preset NEMA17
- รองรับ EN pin แบบ optional
- ควบคุมความเร็วด้วย RPM
- รองรับกลับทิศทางลอจิก DIR
- รองรับการสั่งงานระยะ mm
- position tracking และ reset home

---

## Hardware Setup

### Wiring ตัวอย่าง

| CH57x | TMC2208/TMC2209 | หมายเหตุ |
|---------|------------------|----------|
| PC0 | STEP | pulse input |
| PC1 | DIR  | direction input |
| PC2 | EN   | optional, active low |
| 3.3V | VIO | logic voltage |
| GND | GND  | กราวด์ร่วมกัน |
| PSU | VM   | motor voltage ตามบอร์ด |

### ค่าที่ต้องยืนยันก่อนใช้งาน

1. current limit บนบอร์ด
2. microstep mode (ผ่านขา/จัมเปอร์ของโมดูล)
3. `steps_per_rev` ในซอฟต์แวร์ต้องตรง microstep จริง

---

## หลักการทำงาน

### STEP DIR

- 1 pulse ที่ STEP = 1 step
- DIR กำหนดทิศทาง
- EN เปิดปิดการจ่ายกระแสให้มอเตอร์

### การคำนวณระยะ mm

```
steps = distance_mm * steps_per_rev / mm_per_rev
```

---

## การใช้งานพื้นฐาน

### Quick Start: TMC2209 + NEMA17

```c
#include "SimpleHAL.h"
#include "TMC220x.h"

TMC220x_Instance drv;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    TMC220x_InitNEMA17(&drv, TMC220X_MODEL_2209, PC0, PC1, PC2);
    TMC220x_SetSpeed(&drv, 100);

    TMC220x_Enable(&drv);
    TMC220x_MoveDegrees(&drv, 90);
    TMC220x_MoveRevolutions(&drv, -1);
    TMC220x_Disable(&drv);

    while (1) {}
}
```

### ใช้ TMC2208

```c
TMC220x_InitNEMA17(&drv, TMC220X_MODEL_2208, PC0, PC1, PC2);
```

---

## การใช้งานขั้นสูง

### 1) สั่งงานระยะ mm

```c
TMC220x_SetLinearMmPerRev(&drv, 8.0f);   // lead screw 8mm/rev
TMC220x_MoveMm(&drv, 40.0f);
TMC220x_MoveMm(&drv, -12.5f);
```

### 2) แปลงหน่วยก่อนควบคุม

```c
int32_t s = TMC220x_MmToSteps(&drv, 12.5f);
float mm = TMC220x_StepsToMm(&drv, TMC220x_GetPosition(&drv));
```

### 3) กลับทิศ DIR โดยไม่สลับสาย

```c
TMC220x_SetDirectionInverted(&drv, 1);
```

### 4) Return to Home

```c
int32_t pos = TMC220x_GetPosition(&drv);
TMC220x_Move(&drv, -pos);
TMC220x_ResetPosition(&drv);
```

---

## Troubleshooting

### ปัญหา: มอเตอร์ไม่หมุน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| EN ไม่ได้ enable | เรียก `TMC220x_Enable()` |
| STEP/DIR สลับขา | ตรวจ wiring ใหม่ |
| ไม่ได้กราวด์ร่วมกัน | ต่อ GND ร่วมกัน |

### ปัญหา: เดินไม่ครบระยะ

| สาเหตุ | วิธีแก้ |
|--------|---------|
| RPM สูงจนหลุด step | ลด RPM |
| `steps_per_rev` ไม่ตรง microstep | แก้ค่า init |
| `mm_per_rev` ไม่ตรงกลไก | วัดและตั้งค่าใหม่ |

### ปัญหา: เสียงดังหรือสั่น

| สาเหตุ | วิธีแก้ |
|--------|---------|
| current limit ไม่เหมาะ | ปรับกระแสใหม่ |
| ความเร่งกระโดดทันที | เริ่มด้วยความเร็วต่ำ แล้วไต่ขึ้น |

---

## API Reference

### Initialization

- `TMC220x_Init(drv, model, step_pin, dir_pin, en_pin, steps_per_rev)`
- `TMC220x_InitNEMA17(drv, model, step_pin, dir_pin, en_pin)`

### Basic Control

- `TMC220x_SetSpeed(drv, rpm)`
- `TMC220x_SetDirectionInverted(drv, inverted)`
- `TMC220x_Enable(drv)` / `TMC220x_Disable(drv)`
- `TMC220x_Stop(drv)`

### Motion

- `TMC220x_Move(drv, steps)`
- `TMC220x_MoveDegrees(drv, degrees)`
- `TMC220x_MoveRevolutions(drv, revolutions)`

### Linear Motion

- `TMC220x_SetLinearMmPerRev(drv, mm_per_rev)`
- `TMC220x_MmToSteps(drv, distance_mm)`
- `TMC220x_StepsToMm(drv, steps)`
- `TMC220x_MoveMm(drv, distance_mm)`

### Position

- `TMC220x_GetPosition(drv)`
- `TMC220x_ResetPosition(drv)`

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)