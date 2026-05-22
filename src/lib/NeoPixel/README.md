# SimpleNeoPixel Library - คู่มือการใช้งาน

> **WS2812/WS2812B RGB LED Library สำหรับ CH57x series**  
> เวอร์ชัน 2.0 - พอร์ตจาก CH32V003

---

## สารบัญ

1. [บทนำ](#1-บทนำ)
2. [Hardware Setup](#2-hardware-setup)
3. [Quick Start](#3-quick-start)
4. [API Reference](#4-api-reference)
5. [Troubleshooting](#5-troubleshooting)

---

## 1. บทนำ

### 1.1 WS2812 คืออะไร?

WS2812 (หรือที่รู้จักในชื่อ NeoPixel) เป็น RGB LED แบบ addressable ที่สามารถควบคุมสีและความสว่างของแต่ละดวงได้อย่างอิสระ โดยใช้สัญญาณข้อมูลเพียงสายเดียว

**คุณสมบัติ:**
- ควบคุมด้วยสัญญาณเดียว (Single-wire protocol)
- แต่ละ LED มี IC ควบคุมในตัว
- สามารถต่อเรียงกันได้หลายดวง (Daisy-chain)
- สี RGB 24-bit (16.7 ล้านสี)
- ความสว่างปรับได้

### 1.2 หลักการทำงาน

WS2812 ใช้การส่งข้อมูลแบบ bit-banging โดยมี timing ที่เฉพาะเจาะจง:

```
Bit 0: HIGH 0.4µs, LOW 0.85µs
Bit 1: HIGH 0.8µs, LOW 0.45µs
Reset: LOW > 50µs
```

ลำดับข้อมูล: **GRB** (Green-Red-Blue) ไม่ใช่ RGB!

---

## 2. Hardware Setup

### 2.1 การต่อวงจร (สำหรับ CH57x series)

```
CH57x (CH570D)    WS2812 Strip
----------------  ------------
PA5 (DATA) -----> DIN
GND ------------> GND
3.3V/5V --------> VCC
```

### 2.2 ข้อควรระวัง

1. **แรงดันไฟ:**
   - WS2812 ทั่วไปต้องการ 5V แต่บางรุ่น (WS2812B) ทำงานที่ 3.3V ได้
   - CH570D ทำงานที่ 3.3V — สำหรับ WS2812 5V ควรใช้ level shifter (74HCT245)
   - สำหรับ WS2812B 3.3V หรือสายสั้น (<10 LEDs) ต่อตรงได้

2. **กระแสไฟ:**
   - แต่ละ LED ใช้สูงสุด ~60mA (สีขาวเต็ม)
   - 8 LEDs = 480mA สูงสุด
   - ใช้ Power Supply แยกสำหรับ LED strip ที่ยาว

3. **Capacitor:** ใส่ 1000µF capacitor ที่ขา VCC-GND ของ strip ป้องกัน voltage spike

4. **Resistor:** ใส่ 330Ω resistor ที่สาย DATA ป้องกันสัญญาณรบกวน

### 2.3 ข้อจำกัดบน CH570D

- CH570D มีเฉพาะ PORTA (PA0–PA15) — ต้องใช้ PAx แทน PCx/PDx
- timing ใช้ NOP loop — ทำงานที่ 60MHz (SystemCoreClock)
- ต้องปิด interrupt (`__disable_irq()`) ขณะส่งข้อมูลเพื่อ timing ที่แม่นยำ

---

## 3. Quick Start

### 3.1 ตัวอย่างแรก

```c
#include "SimpleHAL/SimpleHAL.h"
#include "lib/NeoPixel/NeoPixel.h"

#define NUM_LEDS 8
#define DATA_PIN GPIO_Pin_5

int main(void) {
    SimpleHAL_Init();
    
    // เริ่มต้น NeoPixel บน PA5
    NeoPixel_Init(GPIOA, DATA_PIN, NUM_LEDS);
    
    // ตั้งค่าสี LED ดวงแรกเป็นสีแดง
    NeoPixel_SetPixelColor(0, 255, 0, 0);
    NeoPixel_Show();
    
    while(1) {
        Delay_Ms(1000);
    }
}
```

---

## 4. API Reference

### 4.1 Initialization Functions

#### `NeoPixel_Init()`
```c
void NeoPixel_Init(GPIO_TypeDef* port, uint16_t pin, uint16_t num_leds);
```
- `port` = `GPIOA` (เฉพาะ PORTA)
- `pin` = `GPIO_Pin_0` ถึง `GPIO_Pin_15`

### 4.2 Basic Functions

- `NeoPixel_SetPixelColor(pixel, r, g, b)` : ตั้งค่าสีของ LED ดวงที่ระบุ
- `NeoPixel_SetPixelColor32(pixel, color)` : ตั้งค่าสีด้วย 32-bit color
- `NeoPixel_Show()` : อัพเดทผล (ต้องเรียกหลัง Set color)
- `NeoPixel_Clear()` : ดับ LEDs ทั้งหมด (ต้องเรียก Show() ด้วย)
- `NeoPixel_Fill(r, g, b)` : ตั้งค่าสีเดียวกันให้ทุก LEDs
- `NeoPixel_SetBrightness(brightness)` : ตั้งค่าความสว่างทั้งหมด (0-255)

---

## 5. Troubleshooting

### LEDs ไม่ติด
- ตรวจสอบการต่อสาย DATA pin ถูกต้อง
- ตรวจสอบ resistor 330Ω ที่ DATA
- ตรวจสอบไฟเลี้ยง (5V สำหรับ WS2812, 3.3V สำหรับ WS2812B)

### สีผิดเพี้ยน
- ตรวจสอบว่าใช้ลำดับ GRB (ไม่ใช่ RGB)
- ตรวจสอบ timing — CH570D ใช้ NOP loop ที่ 60MHz

### ไฟกระพริบ / สัญญาณรบกวน
- ใส่ capacitor 1000µF ใกล้ strip
- ลดความยาวสาย DATA
- เพิ่ม resistor เป็น 470Ω

---

**พัฒนาโดย:** CH57x Library Team (พอร์ตจาก CH32V003)
**หมายเหตุ:** ไลบรารีนี้พอร์ตผ่าน Compatibility Shim — ดู [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)
