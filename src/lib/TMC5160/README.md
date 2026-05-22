# TMC5160 Library

> **Library สำหรับควบคุม TMC5160 บน CH57x**
> รองรับทั้ง STEP+DIR motion และ SPI register access

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

TMC5160 เป็นไดรเวอร์ระดับสูงที่รองรับทั้งการขับแบบ STEP/DIR และการตั้งค่าผ่าน SPI register
ไลบรารีนี้ออกแบบให้เริ่มง่ายด้วย STEP/DIR ก่อน แล้วค่อยเปิด SPI เมื่อต้องการตั้งค่าเชิงลึก

เหมาะกับงานที่ต้องการ:
- แรงบิดสูง
- งาน precision
- มีการอ่าน/เขียน register บางตัวจาก MCU

---

## คุณสมบัติ

- init แบบทั่วไป และ preset NEMA17
- รองรับ STEP/DIR/EN/CS
- ควบคุมความเร็วด้วย RPM
- หมุนด้วย steps/degrees/revolutions/mm
- ติดตามตำแหน่งปัจจุบัน
- รองรับ SPI begin แบบกำหนด mode/speed/pin mapping
- รองรับฟังก์ชัน `WriteReg` และ `ReadReg`

---

## Hardware Setup

### Wiring ฝั่ง Motion

| CH57x | TMC5160 | หมายเหตุ |
|---------|---------|----------|
| PC0 | STEP | pulse input |
| PC1 | DIR  | direction input |
| PC2 | EN   | optional |
| PC3 | CS   | chip select สำหรับ SPI |
| GND | GND  | กราวด์ร่วมกัน |

### Wiring ฝั่ง SPI (SimpleSPI default)

| SPI Signal | CH57x (default) |
|-----------|----------------------|
| SCK  | PC5 |
| MOSI | PC6 |
| MISO | PC7 |

> หมายเหตุ
> - ไลบรารีใช้ CS จาก `pin_cs` ใน instance ไม่ได้ใช้ NSS ฮาร์ดแวร์อัตโนมัติ
> - ควรเริ่ม SPI ที่ 500kHz ถึง 1MHz ก่อน

---

## หลักการทำงาน

### 1) Motion Plane

ไลบรารีส่ง pulse ที่ STEP และใช้ DIR กำหนดทิศทางเหมือนไดรเวอร์ STEP/DIR ทั่วไป

### 2) Register Plane ผ่าน SPI

- `TMC5160_WriteReg` เขียนรีจิสเตอร์ 32-bit
- `TMC5160_ReadReg` อ่านรีจิสเตอร์แบบ pipeline

การอ่านแบบ pipeline จะมี
1. dummy read transaction
2. actual read transaction

### 3) คำนวณระยะ mm

```
steps = distance_mm * steps_per_rev / mm_per_rev
```

---

## การใช้งานพื้นฐาน

### Quick Start แบบ STEP/DIR

```c
#include "SimpleHAL.h"
#include "TMC5160.h"

TMC5160_Instance drv;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    TMC5160_InitNEMA17(&drv, PC0, PC1, PC2, PC3);
    TMC5160_SetSpeed(&drv, 80);

    TMC5160_Enable(&drv);
    TMC5160_Move(&drv, 400);
    TMC5160_MoveDegrees(&drv, -180);
    TMC5160_Disable(&drv);

    while (1) {}
}
```

### เปิด SPI และทดสอบ register

```c
TMC5160_SPIBegin(&drv, SPI_MODE3, SPI_1MHZ, SPI_PINS_DEFAULT);

TMC5160_WriteReg(&drv, 0x00, 0x00000000);      // GCONF
uint32_t ifcnt = TMC5160_ReadReg(&drv, 0x02);  // IFCNT
```

---

## การใช้งานขั้นสูง

### 1) สั่งงานระยะ mm

```c
TMC5160_SetLinearMmPerRev(&drv, 8.0f);
TMC5160_MoveMm(&drv, 15.0f);
TMC5160_MoveMm(&drv, -2.5f);
```

### 2) แปลงหน่วยก่อนตัดสินใจ

```c
int32_t need_steps = TMC5160_MmToSteps(&drv, 32.0f);
float current_mm = TMC5160_StepsToMm(&drv, TMC5160_GetPosition(&drv));
```

### 3) Return to Home

```c
int32_t pos = TMC5160_GetPosition(&drv);
TMC5160_Move(&drv, -pos);
TMC5160_ResetPosition(&drv);
```

### 4) กลับทิศ DIR

```c
TMC5160_SetDirectionInverted(&drv, 1);
```

---

## Troubleshooting

### ปัญหา: SPI อ่านค่าไม่ได้

| สาเหตุ | วิธีแก้ |
|--------|---------|
| SPI mode ไม่ตรง | ใช้ `SPI_MODE3` เป็นค่าเริ่มต้น |
| speed สูงเกิน | ลดเป็น 500kHz หรือ 1MHz |
| CS pin ผิด | ตรวจ `pin_cs` และ wiring |
| MISO/MOSI สลับ | ตรวจสายใหม่ |

### ปัญหา: มอเตอร์หมุนไม่เสถียร

| สาเหตุ | วิธีแก้ |
|--------|---------|
| current limit ไม่เหมาะ | ปรับตามสเปคมอเตอร์ |
| RPM สูงเกินช่วงแรงบิด | ลดความเร็ว |
| ระบบไฟกระชาก | ตรวจ supply และ decoupling |

### ปัญหา: ระยะ mm คลาดเคลื่อน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| `mm_per_rev` ผิด | วัดกลไกจริงแล้วตั้งใหม่ |
| `steps_per_rev` ไม่ตรง microstep | แก้ค่า init ให้ตรง |

---

## API Reference

### Initialization

- `TMC5160_Init(drv, step_pin, dir_pin, en_pin, cs_pin, steps_per_rev)`
- `TMC5160_InitNEMA17(drv, step_pin, dir_pin, en_pin, cs_pin)`
- `TMC5160_SPIBegin(drv, mode, speed, pin_cfg)`

### Basic Control

- `TMC5160_SetSpeed(drv, rpm)`
- `TMC5160_SetDirectionInverted(drv, inverted)`
- `TMC5160_Enable(drv)` / `TMC5160_Disable(drv)`
- `TMC5160_Stop(drv)`

### Motion

- `TMC5160_Move(drv, steps)`
- `TMC5160_MoveDegrees(drv, degrees)`
- `TMC5160_MoveRevolutions(drv, revolutions)`

### Linear Motion

- `TMC5160_SetLinearMmPerRev(drv, mm_per_rev)`
- `TMC5160_MmToSteps(drv, distance_mm)`
- `TMC5160_StepsToMm(drv, steps)`
- `TMC5160_MoveMm(drv, distance_mm)`

### Position

- `TMC5160_GetPosition(drv)`
- `TMC5160_ResetPosition(drv)`

### SPI Register Access

- `TMC5160_WriteReg(drv, reg_addr, value)`
- `TMC5160_ReadReg(drv, reg_addr)`

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)