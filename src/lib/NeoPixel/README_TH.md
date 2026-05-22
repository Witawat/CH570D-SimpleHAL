# SimpleNeoPixel Library - คู่มือการใช้งานฉบับสมบูรณ์

> **WS2812/WS2812B RGB LED Library สำหรับ CH57x**  
> เวอร์ชัน 2.0 - อัพเดท 2025-12-21

---

## 📑 สารบัญ

1. [บทนำ](#1-บทนำ)
2. [Hardware Setup](#2-hardware-setup)
3. [Quick Start](#3-quick-start)
4. [API Reference](#4-api-reference)
5. [Tutorials](#5-tutorials)
6. [Techniques](#6-techniques)
7. [Troubleshooting](#7-troubleshooting)
8. [Project Examples](#8-project-examples)

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

### 1.3 ข้อดีข้อเสีย

**ข้อดี:**
- ✅ ควบคุมง่าย ใช้ pin เดียว
- ✅ สีสันสดใส ปรับได้หลากหลาย
- ✅ ราคาถูก
- ✅ ต่อเรียงได้หลายดวง

**ข้อเสีย:**
- ❌ ต้องการ timing ที่แม่นยำ
- ❌ ใช้ไฟมาก (60mA/LED ที่สว่างเต็ม)
- ❌ ต้องปิด interrupt ขณะส่งข้อมูล
- ❌ ไวต่อแรงดันไฟ (ต้องใช้ 5V)

---

## 2. Hardware Setup

### 2.1 การต่อวงจร

#### การต่อพื้นฐาน

```
CH57x          WS2812 Strip
---------         ------------
PC4 (DATA) -----> DIN
GND ------------> GND
5V -------------> VCC
```

#### ข้อควรระวัง

1. **แรงดันไฟ:**
   - WS2812 ต้องการ 5V
   - CH57x ทำงานที่ 3.3V
   - ควรใช้ Level Shifter (แนะนำ 74HCT245)

2. **กระแสไฟ:**
   - แต่ละ LED ใช้สูงสุด ~60mA (สีขาวเต็ม)
   - 8 LEDs = 480mA สูงสุด
   - ใช้ Power Supply แยก สำหรับ LED strip ที่ยาว

3. **Capacitor:**
   - ใส่ 1000µF capacitor ที่ขา VCC-GND ของ strip
   - ป้องกัน voltage spike

4. **Resistor:**
   - ใส่ 330Ω resistor ที่สาย DATA
   - ป้องกันสัญญาณรบกวน

#### วงจรแบบสมบูรณ์

```
                    330Ω
CH57x  ----/\/\/\-----> Level    -----> WS2812
  PC4                      Shifter          DIN
                          (74HCT245)
  
  GND  ----------------------+-----------> GND
                             |
  5V   ---[1000µF]----------+-----------> VCC
           Cap
```

### 2.2 Level Shifting

**ทำไมต้องใช้ Level Shifter?**
- CH57x output: 3.3V
- WS2812 ต้องการ: 3.5-5V (VIH min = 0.7×VCC)
- ที่ VCC=5V → VIH = 3.5V

**วิธีแก้:**

1. **74HCT245** (แนะนำ)
   - ทำงานได้ดีที่ 3.3V input
   - ราคาถูก

2. **74AHCT125**
   - Buffer แบบ tri-state
   - เหมาะกับ single signal

3. **Diode + Resistor** (ไม่แนะนำ)
   - ใช้ได้แต่ไม่เสถียร

---

## 3. Quick Start

### 3.1 การติดตั้ง Library

1. Copy ไฟล์ทั้งหมดใน `/User/Lib/NeoPixel/` ไปยังโปรเจค
2. Include header file:

```c
#include "SimpleNeoPixel.h"
```

3. เพิ่ม Timer library (สำหรับ non-blocking effects):

```c
#include "../Timer/timer.h"
```

### 3.2 ตัวอย่างแรก

```c
#include "SimpleHAL.h"

#define NUM_LEDS 8
#define DATA_PIN GPIO_Pin_4

int main(void) {
    // เริ่มต้นระบบ
    SystemCoreClockUpdate();
    Timer_Init();
    
    // เริ่มต้น NeoPixel
    NeoPixel_Init(GPIOC, DATA_PIN, NUM_LEDS);
    
    // ตั้งค่าสี LED ดวงแรกเป็นสีแดง
    NeoPixel_SetPixelColor(0, 255, 0, 0);
    
    // แสดงผล
    NeoPixel_Show();
    
    while(1) {
        Delay_Ms(1000);
    }
}
```

### 3.3 การ Compile และ Upload

1. เพิ่มไฟล์ `.c` และ `.h` ใน project
2. Compile ด้วย MounRiver Studio
3. Upload ไปยัง CH57x
4. ตรวจสอบผลลัพธ์

---

## 4. API Reference

### 4.1 Initialization Functions

#### `NeoPixel_Init()`
```c
void NeoPixel_Init(GPIO_TypeDef* port, uint16_t pin, uint16_t num_leds);
```
เริ่มต้นการใช้งาน NeoPixel

**Parameters:**
- `port`: GPIO port (เช่น GPIOC)
- `pin`: GPIO pin (เช่น GPIO_Pin_4)
- `num_leds`: จำนวน LEDs

**Example:**
```c
NeoPixel_Init(GPIOC, GPIO_Pin_4, 8);
```

---

### 4.2 Basic Functions

#### `NeoPixel_SetPixelColor()`
```c
void NeoPixel_SetPixelColor(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b);
```
ตั้งค่าสีของ LED ดวงที่ระบุ

**Parameters:**
- `pixel`: หมายเลข LED (0-based)
- `r`, `g`, `b`: ค่าสี (0-255)

**Example:**
```c
NeoPixel_SetPixelColor(0, 255, 0, 0);  // แดง
NeoPixel_SetPixelColor(1, 0, 255, 0);  // เขียว
```

#### `NeoPixel_SetPixelColor32()`
```c
void NeoPixel_SetPixelColor32(uint16_t pixel, uint32_t color);
```
ตั้งค่าสีด้วย 32-bit color

**Example:**
```c
NeoPixel_SetPixelColor32(0, COLOR_RED);
NeoPixel_SetPixelColor32(1, 0xFF8000);  // ส้ม
```

#### `NeoPixel_Show()`
```c
void NeoPixel_Show(void);
```
อัพเดทการแสดงผล (ต้องเรียกหลัง Set color)

#### `NeoPixel_Clear()`
```c
void NeoPixel_Clear(void);
```
ดับ LEDs ทั้งหมด (ต้องเรียก Show() ด้วย)

#### `NeoPixel_Fill()`
```c
void NeoPixel_Fill(uint8_t r, uint8_t g, uint8_t b);
```
ตั้งค่าสีเดียวกันให้ทุก LEDs

#### `NeoPixel_SetBrightness()`
```c
void NeoPixel_SetBrightness(uint8_t brightness);
```
ตั้งค่าความสว่างทั้งหมด (0-255)

---

### 4.3 Color Functions

#### `NeoPixel_Color()`
```c
uint32_t NeoPixel_Color(uint8_t r, uint8_t g, uint8_t b);
```
สร้างสี 32-bit จาก RGB

#### `NeoPixel_ColorHSV()`
```c
uint32_t NeoPixel_ColorHSV(uint16_t h, uint8_t s, uint8_t v);
```
แปลง HSV เป็น RGB
- `h`: Hue (0-359)
- `s`: Saturation (0-255)
- `v`: Value (0-255)

#### `NeoPixel_Wheel()`
```c
uint32_t NeoPixel_Wheel(uint8_t pos);
```
สร้างสีจาก Color Wheel (0-255)

---

### 4.4 Effect Functions

#### `NeoPixel_Rainbow()`
แสดง Rainbow effect

#### `NeoPixel_Breathing()`
Breathing effect (หายใจเข้าออก)

#### `NeoPixel_Comet()`
Comet/Meteor effect

#### `NeoPixel_Scanner()`
KITT Scanner effect

#### `NeoPixel_RunningLights()`
Running lights with wave

#### `NeoPixel_TwinkleRandom()`
Random twinkling

#### `NeoPixel_ColorChase()`
Multi-color chase

#### `NeoPixel_RainbowCycle()`
Rainbow cycle

#### `NeoPixel_Strobe()`
Strobe effect

---

### 4.5 Advanced Functions

#### Gamma Correction
```c
uint32_t NeoPixel_GammaCorrect(uint32_t color);
```
ปรับสีด้วย Gamma Correction (ทำให้สีดูเป็นธรรมชาติ)

#### Color Blending
```c
uint32_t NeoPixel_ColorBlend(uint32_t color1, uint32_t color2, uint8_t blend);
```
ผสมสี 2 สี (blend: 0=color1, 255=color2)

#### Gradient Fill
```c
void NeoPixel_FillGradient(uint32_t start_color, uint32_t end_color);
```
เติมสีแบบ gradient

#### Rotation
```c
void NeoPixel_RotateLeft(uint16_t positions);
void NeoPixel_RotateRight(uint16_t positions);
```
หมุน pixel buffer

---

### 4.6 Non-blocking Framework

```c
NeoPixel_Effect_t effect;
NeoPixel_StartEffect(&effect, EFFECT_RAINBOW, 20, 0, 0);

while(1) {
    if(NeoPixel_UpdateEffect(&effect)) {
        NeoPixel_Show();
    }
    // ทำงานอื่นได้
}
```

**Effect Types:**
- `EFFECT_RAINBOW`
- `EFFECT_BREATHING`
- `EFFECT_COMET`
- `EFFECT_SCANNER`
- `EFFECT_TWINKLE`
- `EFFECT_COLOR_CHASE`

---

## 5. Tutorials

### Tutorial 1: พื้นฐาน - สีและความสว่าง

**เป้าหมาย:** เรียนรู้การตั้งค่าสีและความสว่าง

```c
// ตั้งค่าสีแต่ละดวง
NeoPixel_SetPixelColor(0, 255, 0, 0);    // แดง
NeoPixel_SetPixelColor(1, 0, 255, 0);    // เขียว
NeoPixel_SetPixelColor(2, 0, 0, 255);    // น้ำเงิน
NeoPixel_Show();

// ปรับความสว่าง
NeoPixel_SetBrightness(128);  // 50%
NeoPixel_Show();
```

---

### Tutorial 2: Color Spaces (RGB vs HSV)

**RGB:**
- ง่ายต่อการเข้าใจ
- ยากต่อการสร้างสีที่ต้องการ

**HSV:**
- H (Hue): สี (0-359°)
- S (Saturation): ความอิ่มตัว (0-255)
- V (Value): ความสว่าง (0-255)

```c
// RGB: ยากต่อการสร้างสีม่วงอ่อน
NeoPixel_SetPixelColor(0, 180, 100, 200);

// HSV: ง่ายกว่า!
NeoPixel_SetPixelColorHSV(0, 280, 150, 200);
```

**เมื่อไหร่ใช้ HSV:**
- สร้าง rainbow
- ปรับสีแบบ smooth
- สร้าง color wheel

---

### Tutorial 3: Built-in Effects

```c
// Rainbow
NeoPixel_Rainbow(20, 5);

// Breathing
NeoPixel_Breathing(255, 0, 0, 10, 3);

// Scanner
NeoPixel_Scanner(255, 0, 0, 3, 30, 5);
```

---

### Tutorial 4: สร้าง Custom Effects

```c
void MyCustomEffect(void) {
    for(uint16_t i = 0; i < NUM_LEDS; i++) {
        // คำนวณสี
        uint8_t hue = (i * 360 / NUM_LEDS);
        NeoPixel_SetPixelColorHSV(i, hue, 255, 255);
    }
    NeoPixel_Show();
}
```

---

### Tutorial 5: Non-blocking Programming

**ปัญหาของ Blocking:**
```c
// ไม่สามารถทำงานอื่นได้ขณะ effect ทำงาน
NeoPixel_Rainbow(20, 5);  // Block 5+ วินาที!
```

**แก้ไขด้วย Non-blocking:**
```c
NeoPixel_Effect_t effect;
NeoPixel_StartEffect(&effect, EFFECT_RAINBOW, 20, 0, 0);

while(1) {
    if(NeoPixel_UpdateEffect(&effect)) {
        NeoPixel_Show();
    }
    
    // ทำงานอื่นได้!
    CheckButton();
    ReadSensor();
}
```

---

### Tutorial 6: Memory Optimization

**เทคนิคประหยัด Memory:**

1. **ใช้ Rotation แทน Clear+Set:**
```c
// แย่: ใช้ CPU มาก
NeoPixel_Clear();
NeoPixel_SetPixelColor(pos, 255, 0, 0);

// ดี: ใช้ rotation
NeoPixel_RotateRight(1);
```

2. **ใช้ Color Presets:**
```c
// ดี
NeoPixel_SetPixelColor32(0, COLOR_RED);

// แย่
NeoPixel_SetPixelColor(0, 255, 0, 0);
```

---

### Tutorial 7: Gamma Correction

**ทำไมต้องใช้ Gamma Correction?**

ตาคนมองเห็นความสว่างแบบ logarithmic ไม่ใช่ linear

```c
// ไม่มี Gamma: ดูไม่เป็นธรรมชาติ
NeoPixel_SetPixelColor32(0, COLOR_RED);

// มี Gamma: ดูเป็นธรรมชาติ
uint32_t corrected = NeoPixel_GammaCorrect(COLOR_RED);
NeoPixel_SetPixelColor32(0, corrected);
```

---

### Tutorial 8: Multi-strip Control

สามารถควบคุมหลาย strip ได้โดยการ Init หลายครั้ง:

```c
// Strip 1
NeoPixel_Init(GPIOC, GPIO_Pin_4, 8);
// ... set colors ...
NeoPixel_Show();

// Strip 2 (ต้อง re-init)
NeoPixel_Init(GPIOD, GPIO_Pin_2, 16);
// ... set colors ...
NeoPixel_Show();
```

**หมายเหตุ:** ไม่สามารถควบคุมพร้อมกันได้ ต้องทำทีละ strip

---

## 6. Techniques

### 6.1 Color Theory

**Color Wheel:**
- 0° = แดง
- 120° = เขียว
- 240° = น้ำเงิน

**Complementary Colors:**
- แดง ↔ เขียวอมน้ำเงิน
- เหลือง ↔ น้ำเงิน
- ม่วง ↔ เหลืองอมเขียว

---

### 6.2 Smooth Animations

**เทคนิค:**

1. **ใช้ Interpolation:**
```c
uint32_t color = NeoPixel_ColorBlend(COLOR_RED, COLOR_BLUE, step);
```

2. **ใช้ Sine Wave:**
```c
uint8_t brightness = 127 + 127 * sin(step * 0.1);
```

3. **ใช้ HSV แทน RGB:**
```c
// Smooth color transition
for(uint16_t hue = 0; hue < 360; hue++) {
    NeoPixel_SetPixelColorHSV(0, hue, 255, 255);
    NeoPixel_Show();
    Delay_Ms(10);
}
```

---

### 6.3 Performance Optimization

**เทคนิค:**

1. **ลด Show() calls:**
```c
// แย่
for(int i = 0; i < 8; i++) {
    NeoPixel_SetPixelColor(i, 255, 0, 0);
    NeoPixel_Show();  // 8 ครั้ง!
}

// ดี
for(int i = 0; i < 8; i++) {
    NeoPixel_SetPixelColor(i, 255, 0, 0);
}
NeoPixel_Show();  // 1 ครั้ง
```

2. **ใช้ Non-blocking:**
   - ไม่ block main loop
   - ทำงานอื่นได้พร้อมกัน

3. **ลด Brightness:**
   - ลดกระแสไฟ
   - ลดความร้อน

---

### 6.4 Power Management

**การคำนวณกระแสไฟ:**
```
กระแสสูงสุด = จำนวน LEDs × 60mA
8 LEDs = 480mA
16 LEDs = 960mA (~1A)
```

**เทคนิคประหยัดไฟ:**

1. **ลด Brightness:**
```c
NeoPixel_SetBrightness(64);  // 25% = ลดไฟ ~75%
```

2. **ใช้สีเดียว:**
   - สีขาว = 60mA
   - สีแดง = 20mA
   - สีเขียว = 20mA
   - สีน้ำเงิน = 20mA

3. **ดับ LED ที่ไม่ใช้:**
```c
NeoPixel_Clear();
NeoPixel_Show();
```

---

### 6.5 Timing Considerations

**ข้อควรระวัง:**

1. **ปิด Interrupt:**
   - `NeoPixel_Show()` ปิด interrupt อัตโนมัติ
   - อาจทำให้พลาด interrupt อื่น

2. **Reset Time:**
   - ต้องรอ >50µs หลัง Show()
   - Library จัดการให้อัตโนมัติ

3. **Update Rate:**
   - แนะนำ: 30-60 FPS
   - เร็วเกินไป: เปลือง CPU
   - ช้าเกินไป: ดูกระตุก

---

### 6.6 Debugging Tips

**ปัญหาที่พบบ่อย:**

1. **LED ไม่ติด:**
   - ตรวจสอบ VCC, GND
   - ตรวจสอบ DATA pin
   - ลอง Show() หลัง Set

2. **สีผิด:**
   - ตรวจสอบลำดับ GRB
   - ลอง Gamma Correction

3. **กระพริบ:**
   - เพิ่ม Capacitor
   - ตรวจสอบ Power Supply

4. **ทำงานไม่เสถียร:**
   - ใช้ Level Shifter
   - ลด Brightness

---

## 7. Troubleshooting

### 7.1 ปัญหาที่พบบ่อย

#### LED ไม่ติดเลย

**สาเหตุ:**
- ไฟเลี้ยงไม่ถูกต้อง
- สาย DATA ต่อผิด
- LED เสีย

**วิธีแก้:**
1. ตรวจสอบแรงดัน VCC = 5V
2. ตรวจสอบ GND ต่อถูกต้อง
3. ลองเปลี่ยน LED ดวงแรก

---

#### สีไม่ถูกต้อง

**สาเหตุ:**
- Timing ไม่ถูกต้อง
- Level shifting ไม่ดี

**วิธีแก้:**
1. ตรวจสอบ SystemCoreClock
2. ใช้ Level Shifter
3. ลด Brightness

---

#### กระพริบ/ไม่เสถียร

**สาเหตุ:**
- Power supply ไม่เพียงพอ
- สัญญาณรบกวน

**วิธีแก้:**
1. เพิ่ม Capacitor 1000µF
2. ใช้ Power supply ที่ดี
3. เพิ่ม Resistor 330Ω

---

### 7.2 FAQ

**Q: ใช้ได้กี่ LED?**  
A: ขึ้นกับ RAM (แต่ละ LED ใช้ 3 bytes) และ Power Supply

**Q: ทำไมต้องใช้ 5V?**  
A: WS2812 ออกแบบมาสำหรับ 5V, ใช้ 3.3V อาจทำงานไม่เสถียร

**Q: ใช้กับ WS2811 ได้ไหม?**  
A: ได้ แต่ WS2811 เป็น RGB ไม่ใช่ GRB

**Q: ทำไมสีดูไม่เป็นธรรมชาติ?**  
A: ลองใช้ Gamma Correction

---

## 8. Project Examples

### 8.1 Status Indicator
ดูไฟล์: `Examples/16_Project_StatusIndicator.c`

**คุณสมบัติ:**
- แสดงสถานะระบบด้วยสีและ pattern
- IDLE, WORKING, WARNING, ERROR, SUCCESS

---

### 8.2 VU Meter
ดูไฟล์: `Examples/17_Project_VUMeter.c`

**คุณสมบัติ:**
- แสดงระดับเสียง/แรงดัน
- สีเปลี่ยนตามระดับ (เขียว-เหลือง-แดง)

---

### 8.3 Temperature Display
ดูไฟล์: `Examples/18_Project_TemperatureDisplay.c`

**คุณสมบัติ:**
- แสดงอุณหภูมิด้วยสี
- น้ำเงิน=เย็น, เขียว=ปกติ, แดง=ร้อน

---

### 8.4 Clock Display
ดูไฟล์: `Examples/19_Project_ClockDisplay.c`

**คุณสมบัติ:**
- นาฬิกาแบบ analog
- ใช้ LED แทนเข็ม

---

### 8.5 Music Visualizer
ดูไฟล์: `Examples/20_Project_MusicVisualizer.c`

**คุณสมบัติ:**
- แสดงผลเสียงแบบ real-time
- สีเปลี่ยนตามจังหวะ

---

## 📚 เอกสารเพิ่มเติม

- [WS2812 Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812.pdf)
- [CH57x Reference Manual](https://www.wch-ic.com/products/CH57x.html)
- [Examples Directory](./Examples/)

---

## 📝 License

MIT License - ใช้งานได้อย่างอิสระ

---

## 🙏 Credits

- พัฒนาโดย: SimpleHAL Team
- อ้างอิง: Adafruit NeoPixel Library
- เวอร์ชัน: 2.0 (2025-12-21)

---

**Happy Coding! 🎨✨**

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)