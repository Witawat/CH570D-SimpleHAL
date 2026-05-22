# ShiftReg595 Library (74HC595)

> **Library สำหรับควบคุม 74HC595 Shift Register บน CH57x**  
> ขยาย GPIO output ด้วย 3 pins รองรับ cascade หลาย IC

## 📋 สารบัญ

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

74HC595 คือ IC Shift Register ที่รับข้อมูล Serial (ทีละ bit) แล้วแปลงเป็น Parallel Output 8 bits  
ทำให้ CH57x ที่มี GPIO จำกัด สามารถ **ควบคุม output ได้สูงสุด 32 bits** โดยใช้เพียง **3 pins**

| รายการ | ค่า |
|--------|-----|
| Input | Serial (1 DATA pin) |
| Output | 8 bits parallel (QA-QH) |
| Cascade | Cascade ได้ไม่จำกัด (library รองรับ 4 IC) |
| แรงดัน | 2V–6V (ใช้ 3.3V จาก CH57x ได้) |
| กระแส | สูงสุด 35mA ต่อ pin, 70mA รวม |

---

## คุณสมบัติ

- ✅ ใช้เพียง 3 GPIO pins ควบคุม 8-32 output bits
- ✅ Cascade สูงสุด 4 IC (32 output bits)
- ✅ Shadow buffer (ไม่ต้องจำค่าปัจจุบันเอง)
- ✅ ควบคุม bit ทีละ bit: SetBit, ClearBit, ToggleBit
- ✅ เขียนทั้ง byte: WriteByte, WriteAll
- ✅ LED chase effect: ShiftLeft, ShiftRight
- ✅ รองรับ MSB first และ LSB first
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### วงจร 1 IC (8 outputs)

```
CH57x           74HC595 (DIP-16 / SOIC-16)
                   +--------+
GND  -----------> | GND  VCC| <------- 3.3V
3.3V -----------> | VCC     |
                   |         |
PC0  (DATA) -----> | DS  QA  | -------> Output 0
PC1  (CLK)  -----> | SHCP QB | -------> Output 1
PC2  (LATCH)-----> | STCP QC | -------> Output 2
3.3V -----------> | MR   QD | -------> Output 3  ← MR=HIGH (disable reset)
GND  -----------> | OE   QE | -------> Output 4  ← OE=LOW (enable output)
                   |      QF | -------> Output 5
                   |      QG | -------> Output 6
                   |      QH | -------> Output 7
                   |   QH'   | -------> IC ถัดไป (cascade)
                   +--------+
```

> ⚠️ **สำคัญ**: ต่อ MR (pin 10) → 3.3V และ OE (pin 13) → GND เสมอ  
> ถ้าปล่อย MR ลอย output จะ reset เองโดยไม่ทราบสาเหตุ

### วงจร Cascade 2 IC (16 outputs)

```
CH57x     IC1 (74HC595)      IC2 (74HC595)
              +--------+         +--------+
PC0 (DATA)-->| DS  QH'|-------->| DS  QA |-------> Output 8
PC1 (CLK) -->| SHCP   |----+--->| SHCP QB|-------> Output 9
PC2 (LATCH)->| STCP   |----+--->| STCP QC|-------> Output 10
              | QA     |-------> Output 0
              | QB     |-------> Output 1
              | ...    |
              +--------+         +--------+
```

### GPIO Pins ที่ใช้ได้

ใช้ได้กับทุก GPIO pin: PA1, PA2, PC0–PC7, PD2–PD7

---

## หลักการทำงาน

### Shift Register Protocol

```
Timing:

DATA:   ___1___0___1___1___0___0___0___1___
CLK:    _/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\_/¯\
             ↑ ข้อมูลถูกอ่านที่ rising edge
LATCH:  ___________________________/¯\___
                                    ↑ ข้อมูลทั้ง 8 bits ส่งออก output
```

1. **Clock** (SHCP): rising edge → shift bit เข้า register
2. **Data** (DS): ตั้งค่า bit ก่อน clock rising edge
3. **Latch** (STCP): rising edge → copy shift register → output register

### Shadow Buffer

Library เก็บสถานะ output ปัจจุบันใน `buffer[]` เสมอ  
ทำให้สามารถ `SetBit`/`ClearBit` ทีละ bit ได้โดยไม่ต้องจำค่าเอง

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "ShiftReg595.h"

ShiftReg595_Instance sr;
```

### ขั้นตอนที่ 2: Init ใน main()

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    // DATA=PC0, CLK=PC1, LATCH=PC2, จำนวน IC = 1
    ShiftReg595_Init(&sr, PC0, PC1, PC2, 1);
```

### ขั้นตอนที่ 3: ใช้งาน

```c
    // เปิด LED ทุกดวง
    ShiftReg595_SetAll(&sr);
    Delay_Ms(1000);

    // ปิด LED ทุกดวง
    ShiftReg595_Clear(&sr);
    Delay_Ms(500);

    // เปิดแค่ bit 0 (QA) และ bit 7 (QH)
    ShiftReg595_WriteByte(&sr, 0, 0x81);  // 0b10000001
    Delay_Ms(500);

    // เปิด bit ทีละ bit
    ShiftReg595_Clear(&sr);
    for (uint8_t i = 0; i < 8; i++) {
        ShiftReg595_SetBit(&sr, 0, i);
        Delay_Ms(200);
    }

    while (1) {}
}
```

---

## การใช้งานขั้นสูง

### LED Chase Effect (Knight Rider)

```c
ShiftReg595_Init(&sr, PC0, PC1, PC2, 1);

// เริ่มต้นที่ bit 0
ShiftReg595_WriteByte(&sr, 0, 0x01);

while (1) {
    // วิ่งไปทางซ้าย
    for (uint8_t i = 0; i < 7; i++) {
        ShiftReg595_ShiftLeft(&sr, 0);
        Delay_Ms(80);
    }
    // วิ่งกลับทางขวา
    for (uint8_t i = 0; i < 7; i++) {
        ShiftReg595_ShiftRight(&sr, 0);
        Delay_Ms(80);
    }
}
```

### Cascade 2 IC — ควบคุม 16 outputs

```c
ShiftReg595_Init(&sr, PC0, PC1, PC2, 2);  // 2 IC

// เปิด IC แรกทั้งหมด
ShiftReg595_WriteByte(&sr, 0, 0xFF);

// เปิด IC สองครึ่งล่าง
ShiftReg595_WriteByte(&sr, 1, 0x0F);

// เขียนทั้งสองพร้อมกัน (มี latch เพียงครั้งเดียว — แนะนำ)
uint8_t vals[] = {0xAA, 0x55};
ShiftReg595_WriteAll(&sr, vals);
```

### Toggle bit โดยไม่เสีย state

```c
ShiftReg595_Init(&sr, PC0, PC1, PC2, 1);
ShiftReg595_WriteByte(&sr, 0, 0x0F);  // เปิด 4 bits แรก

// สลับ bit 2 โดยไม่กระทบ bit อื่น
ShiftReg595_ToggleBit(&sr, 0, 2);

// ตรวจสอบสถานะปัจจุบัน
uint8_t state = ShiftReg595_GetBit(&sr, 0, 2);
printf("Bit 2 = %d\r\n", state);
```

### Chase Effect วนรอบ (wrap)

```c
ShiftReg595_WriteByte(&sr, 0, 0x01);

while (1) {
    ShiftReg595_ShiftLeft(&sr, 1);  // wrap=1: บิตวนกลับ
    Delay_Ms(100);
}
```

### Batch Update (ปรับหลาย bit แล้ว Latch ครั้งเดียว)

```c
// แก้ไข buffer โดยตรงแล้วค่อย Latch
ShiftReg595_SetBit(&sr, 0, 0);   // Latch อัตโนมัติ
ShiftReg595_SetBit(&sr, 0, 3);   // Latch อีกครั้ง
ShiftReg595_SetBit(&sr, 0, 7);   // Latch อีกครั้ง

// ถ้าต้องการ Latch ครั้งเดียว ใช้ WriteByte แทน:
ShiftReg595_WriteByte(&sr, 0, 0x89);  // 0b10001001 = bits 0, 3, 7
```

---

## Troubleshooting

### ปัญหา: output ไม่เปลี่ยน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| MR (pin 10) ไม่ได้ต่อ HIGH | ต่อ MR → 3.3V |
| OE (pin 13) ไม่ได้ต่อ LOW | ต่อ OE → GND |
| Wiring ผิด | ตรวจ DATA→DS, CLK→SHCP, LATCH→STCP |

### ปัญหา: output กระพริบหรือ glitch

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ไม่มี decoupling capacitor | ต่อ 100nF ระหว่าง VCC-GND ใกล้ IC |
| CLK เร็วเกินไป | เพิ่ม Delay_Us ใน `_clk_pulse()` |

### ปัญหา: Cascade ไม่ทำงาน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ไม่ได้ต่อ QH' (pin 9) ของ IC แรก → DS (pin 14) IC สอง | ตรวจ cascade wiring |
| num_ics ผิด | ตรวจ argument ใน `ShiftReg595_Init()` |

---

## API Reference

### `ShiftReg595_Init(sr, data, clk, latch, num_ics)`
เริ่มต้น ShiftReg595, clear output ทั้งหมด

| Parameter | Type | คำอธิบาย |
|-----------|------|----------|
| `sr` | `ShiftReg595_Instance*` | ตัวแปร instance |
| `data` | `uint8_t` | GPIO pin สำหรับ DATA (DS) |
| `clk` | `uint8_t` | GPIO pin สำหรับ CLK (SHCP) |
| `latch` | `uint8_t` | GPIO pin สำหรับ LATCH (STCP) |
| `num_ics` | `uint8_t` | จำนวน IC ที่ cascade (1-4) |

---

### `ShiftReg595_WriteByte(sr, ic_idx, value)`
เขียน 1 byte ไปยัง IC ที่กำหนด แล้ว latch ทันที

---

### `ShiftReg595_WriteAll(sr, values[])`
เขียนทุก IC พร้อมกัน (latch ครั้งเดียว) — แนะนำใช้เมื่อต้อง update หลาย IC

---

### `ShiftReg595_Latch(sr)`
ส่ง buffer ปัจจุบันไปยัง hardware

---

### `ShiftReg595_SetBit(sr, ic_idx, bit_num)` / `ClearBit` / `ToggleBit`
ควบคุม 1 bit — latch อัตโนมัติ

---

### `ShiftReg595_GetBit(sr, ic_idx, bit_num)` → `uint8_t`
อ่านสถานะ bit จาก buffer (0 หรือ 1)

---

### `ShiftReg595_Clear(sr)` / `SetAll(sr)`
ปิด/เปิด output ทั้งหมด

---

### `ShiftReg595_ShiftLeft(sr, wrap)` / `ShiftRight(sr, wrap)`
Shift buffer ทีละ bit — `wrap=1` วนกลับ, `wrap=0` เติม 0

---

### Constants

| Constant | ค่า default | ความหมาย |
|----------|------------|----------|
| `SHIFTREG595_MAX_ICS` | 4 | จำนวน IC สูงสุด (override ด้วย `#define` ก่อน include) |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)