# WS2815 LED Matrix Library - คู่มือการใช้งาน

## 📋 สารบัญ

1. [บทนำ](#บทนำ)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [การติดตั้งฮาร์ดแวร์](#การติดตั้งฮาร์ดแวร์)
4. [การใช้งานขั้นพื้นฐาน](#การใช้งานขั้นพื้นฐาน)
5. [เทคนิคขั้นกลาง](#เทคนิคขั้นกลาง)
6. [เทคนิคขั้นสูง](#เทคนิคขั้นสูง)
7. [API Reference](#api-reference)
8. [แก้ปัญหา](#แก้ปัญหา)

---

## บทนำ

WS2815 LED Matrix Library เป็น library สำหรับควบคุม LED Matrix แบบ WS2815 บน CH57x microcontroller โดยรองรับการต่อสายแบบต่างๆ และมีฟังก์ชันครบครันตั้งแต่ขั้นพื้นฐานถึงขั้นสูง

### ความแตกต่างระหว่าง WS2812 และ WS2815

| คุณสมบัติ | WS2812 | WS2815 |
|-----------|--------|--------|
| แรงดันไฟ | 5V | 12V |
| Data Line | Single | Dual (Backup) |
| ความเสถียร | ปานกลาง | สูง |
| ระยะทาง | สั้น (< 3m) | ยาว (< 10m) |
| ราคา | ถูกกว่า | แพงกว่า |
| Timing | เหมือนกัน | เหมือนกัน |

> **หมายเหตุ**: Library นี้ใช้งานได้กับทั้ง WS2812 และ WS2815 เนื่องจาก timing protocol เหมือนกัน

---

## คุณสมบัติ

### คุณสมบัติหลัก

✅ **รองรับ Matrix หลายขนาด**
- 8x8 (64 LEDs) - แนะนำ
- 16x16 (256 LEDs)
- 32x8 (256 LEDs)
- ขนาดอื่นๆ ได้ถึง 32x32

✅ **รูปแบบการต่อสาย 4 แบบ**
- Zigzag Left (ซิกแซกเริ่มซ้าย) - ใช้บ่อยที่สุด
- Zigzag Right (ซิกแซกเริ่มขวา)
- Snake (งูเลื้อยต่อเนื่อง)
- Columns (เรียงตามคอลัมน์)

✅ **ฟังก์ชันวาดรูปครบครัน**
- จุด, เส้น, สี่เหลี่ยม, วงกลม
- Bitmap และ Sprite
- Text (ASCII และภาษาไทย)

✅ **เอฟเฟกต์และ Animation**
- Fade In/Out
- Wipe, Slide Transition
- Scrolling Text
- Pattern Generation

✅ **รองรับภาษาไทย**
- ฟอนต์ภาษาไทย 8x8 pixels
- UTF-8 encoding
- พยัญชนะ, สระ, เลขไทย

---

## การติดตั้งฮาร์ดแวร์

### วงจรการต่อ

```
CH57x          WS2815 Matrix
---------         -------------
PC4 (หรือ pin อื่น) → DIN
GND              → GND
12V Power Supply → VCC (WS2815)
                 → GND
```

> **⚠️ คำเตือน**: WS2815 ใช้ไฟ 12V แต่ CH57x ใช้ 3.3V/5V
> - ต้องแยกไฟเลี้ยง LED กับ MCU
> - ต่อ GND ร่วมกัน
> - Data line (DIN) สามารถต่อตรงจาก 3.3V GPIO ได้

### การคำนวณกำลังไฟ

LED แต่ละดวงใช้กระแสสูงสุด:
- แดง: ~20mA
- เขียว: ~20mA  
- น้ำเงิน: ~20mA
- **รวม (สีขาว): ~60mA**

**ตัวอย่าง Matrix 8x8:**
- จำนวน LED: 64 ดวง
- กระแสสูงสุด: 64 × 60mA = **3.84A**
- แนะนำ Power Supply: **12V 5A**

> **💡 เคล็ดลับ**: ใช้ `Matrix_SetBrightness(50)` เพื่อลดการใช้ไฟ

### รูปแบบการต่อสาย

#### 1. Zigzag Left (แนะนำ)

```
แถว 0:  0→ 1→ 2→ 3→ 4→ 5→ 6→ 7
แถว 1: 15←14←13←12←11←10← 9← 8
แถว 2: 16→17→18→19→20→21→22→23
แถว 3: 31←30←29←28←27←26←25←24
```

**ข้อดี**: ใช้สายสั้นที่สุด, เป็นมาตรฐาน
**ข้อเสีย**: ต้องระวังทิศทาง

#### 2. Snake (งูเลื้อย)

```
แถว 0:  0→ 1→ 2→ 3→ 4→ 5→ 6→ 7
แถว 1:  8→ 9→10→11→12→13→14→15
แถว 2: 16→17→18→19→20→21→22→23
แถว 3: 24→25→26→27→28→29→30→31
```

**ข้อดี**: ง่ายต่อการเข้าใจ
**ข้อเสีย**: ใช้สายยาวกว่า

---

## การใช้งานขั้นพื้นฐาน

### 1. การเริ่มต้นใช้งาน

```c
#include "SimpleWS2815Matrix.h"

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    
    // เริ่มต้น Matrix 8x8 แบบ Zigzag บน pin PC4
    Matrix_Init(GPIOC, GPIO_Pin_4, 8, 8, WIRING_ZIGZAG_LEFT);
    
    // ตั้งค่าความสว่าง (0-255)
    Matrix_SetBrightness(50);
    
    // ทดสอบ: เติมสีแดง
    Matrix_Fill(255, 0, 0);
    Matrix_Show();
    
    while(1) {
        // โค้ดของคุณ
    }
}
```

### 2. การตั้งค่าพิกเซล

```c
// ตั้งค่าพิกเซลที่ (x, y) = (3, 3) เป็นสีแดง
Matrix_SetPixel(3, 3, 255, 0, 0);
Matrix_Show();  // ต้องเรียกเพื่ออัพเดทการแสดงผล

// ใช้สีที่กำหนดไว้
Matrix_SetPixelColor(4, 4, COLOR_GREEN);
Matrix_Show();

// อ่านค่าสีของพิกเซล
uint32_t color = Matrix_GetPixel(3, 3);
```

### 3. การวาดรูปทรงพื้นฐาน

```c
// วาดเส้นตรง
Matrix_DrawLine(0, 0, 7, 7, 255, 0, 0);  // เส้นทแยงสีแดง

// วาดสี่เหลี่ยม (ขอบ)
Matrix_DrawRect(1, 1, 6, 6, 0, 255, 0);  // กรอบสีเขียว

// วาดสี่เหลี่ยม (เติมสี)
Matrix_FillRect(2, 2, 4, 4, 0, 0, 255);  // สี่เหลี่ยมสีน้ำเงิน

// วาดวงกลม (ขอบ)
Matrix_DrawCircle(3, 3, 3, 255, 255, 0);  // วงกลมสีเหลือง

// วาดวงกลม (เติมสี)
Matrix_FillCircle(3, 3, 2, 255, 0, 255);  // วงกลมสีม่วง

Matrix_Show();
```

### 4. การแสดงข้อความ

```c
// ข้อความภาษาอังกฤษ
Matrix_Clear();
Matrix_DrawText(0, 0, "Hi", COLOR_RED);
Matrix_Show();

// ตัวอักษรภาษาไทย
Matrix_Clear();
Matrix_DrawCharThai(0, 0, "ก", COLOR_GREEN);
Matrix_Show();

// ข้อความภาษาไทย
Matrix_Clear();
Matrix_DrawTextThai(0, 0, "สวัสดี", COLOR_BLUE);
Matrix_Show();
```

---

## เทคนิคขั้นกลาง

### 1. การเลื่อนข้อความ

```c
ScrollText_t scroll;

// เริ่มต้นการเลื่อนข้อความ
Matrix_ScrollTextInit(&scroll, "Hello World", COLOR_RED, 100, false);

// ใน main loop
while(1) {
    if(Matrix_ScrollTextUpdate(&scroll, 0)) {
        Matrix_Show();
    }
    Delay_Ms(10);
}
```

**พารามิเตอร์:**
- `text`: ข้อความที่จะเลื่อน
- `color`: สีของข้อความ
- `speed`: ความเร็ว (ms per step)
- `vertical`: `true` = เลื่อนแนวตั้ง, `false` = เลื่อนแนวนอน

### 2. การใช้ Sprite

```c
// สร้างข้อมูล Sprite (รูปหัวใจ 5x5)
const uint32_t heart_data[] = {
    0x000000, 0xFF0000, 0x000000, 0xFF0000, 0x000000,
    0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000,
    0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000,
    0x000000, 0xFF0000, 0xFF0000, 0xFF0000, 0x000000,
    0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000
};

Sprite_t heart = {
    .width = 5,
    .height = 5,
    .data = heart_data,
    .has_transparency = false,
    .transparent_color = 0
};

// วาด Sprite
Matrix_DrawSprite(1, 1, &heart);
Matrix_Show();
```

### 3. การใช้ Bitmap

```c
// สร้าง Bitmap (รูปยิ้ม 8x8)
const uint8_t smile[] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100
};

Matrix_DrawBitmap(0, 0, smile, 8, 8, COLOR_YELLOW);
Matrix_Show();
```

### 4. การสร้าง Pattern

```c
// Checkerboard
Matrix_PatternCheckerboard(COLOR_RED, COLOR_BLACK);
Matrix_Show();

// Gradient แนวนอน
Matrix_PatternGradientH(COLOR_RED, COLOR_BLUE);
Matrix_Show();

// Gradient แนวตั้ง
Matrix_PatternGradientV(COLOR_GREEN, COLOR_YELLOW);
Matrix_Show();

// Random
Matrix_PatternRandom(50, COLOR_WHITE);  // 50% density
Matrix_Show();
```

---

## เทคนิคขั้นสูง

### 1. Transition Effects

```c
// Fade In
Matrix_Fill(255, 0, 0);
Matrix_FadeIn(1000, 50);  // 1 วินาที, 50 steps

// Fade Out
Matrix_FadeOut(1000, 50);

// Wipe Transition (เช็ดจากซ้ายไปขวา)
Matrix_WipeTransition(COLOR_GREEN, 50);

// Slide Transition (เลื่อนจากล่างขึ้นบน)
Matrix_SlideTransition(COLOR_BLUE, 50);
```

### 2. การหมุนและกลับด้าน

```c
// หมุน 90 องศาตามเข็มนาฬิกา
Matrix_RotateBuffer90CW();
Matrix_Show();

// หมุน 90 องศาทวนเข็มนาฬิกา
Matrix_RotateBuffer90CCW();
Matrix_Show();

// กลับด้านแนวนอน
Matrix_MirrorH();
Matrix_Show();

// กลับด้านแนวตั้ง
Matrix_MirrorV();
Matrix_Show();
```

> **⚠️ หมายเหตุ**: การหมุนใช้ได้เฉพาะ Matrix สี่เหลี่ยมจัตุรัส (8x8, 16x16)

### 3. การใช้ NeoPixel Effects ร่วมกัน

```c
// Rainbow Effect
NeoPixel_Rainbow(20, 5);

// Theater Chase
NeoPixel_TheaterChase(255, 0, 0, 50, 10);

// Breathing Effect
NeoPixel_Breathing(0, 255, 0, 20, 5);
```

### 4. การเพิ่มประสิทธิภาพ

**เทคนิค 1: ลดการเรียก `Matrix_Show()`**
```c
// ❌ ไม่ดี - เรียก Show() บ่อยเกินไป
for(int i = 0; i < 64; i++) {
    Matrix_SetPixel(i % 8, i / 8, 255, 0, 0);
    Matrix_Show();  // ช้า!
}

// ✅ ดี - เรียก Show() ครั้งเดียว
for(int i = 0; i < 64; i++) {
    Matrix_SetPixel(i % 8, i / 8, 255, 0, 0);
}
Matrix_Show();  // เร็ว!
```

**เทคนิค 2: ลดความสว่าง**
```c
// ลดการใช้ไฟและความร้อน
Matrix_SetBrightness(50);  // 50% brightness
```

**เทคนิค 3: ใช้ Timer แทน Delay**
```c
Timer_t update_timer;
Start_Timer(&update_timer, 100, 1);  // 100ms, repeat

while(1) {
    if(Is_Timer_Expired(&update_timer)) {
        // อัพเดท animation
        Matrix_Show();
    }
    // ทำงานอื่นได้
}
```

---

## API Reference

### Initialization

#### `Matrix_Init()`
```c
void Matrix_Init(GPIO_TypeDef* port, uint16_t pin, uint8_t width, uint8_t height, WiringPattern_e wiring);
```
เริ่มต้นการใช้งาน LED Matrix

**พารามิเตอร์:**
- `port`: GPIO port (เช่น `GPIOC`)
- `pin`: GPIO pin (เช่น `GPIO_Pin_4`)
- `width`: ความกว้างของ Matrix
- `height`: ความสูงของ Matrix
- `wiring`: รูปแบบการต่อสาย

**ตัวอย่าง:**
```c
Matrix_Init(GPIOC, GPIO_Pin_4, 8, 8, WIRING_ZIGZAG_LEFT);
```

### Basic Drawing

#### `Matrix_SetPixel()`
```c
void Matrix_SetPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
```
ตั้งค่าสีของพิกเซลที่ตำแหน่ง (x, y)

#### `Matrix_Clear()`
```c
void Matrix_Clear(void);
```
ล้าง Matrix ทั้งหมด (ดับทุกดวง)

#### `Matrix_Show()`
```c
void Matrix_Show(void);
```
อัพเดทการแสดงผล (ต้องเรียกหลังจากเปลี่ยนแปลงพิกเซล)

### Shape Drawing

#### `Matrix_DrawLine()`
```c
void Matrix_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t r, uint8_t g, uint8_t b);
```
วาดเส้นตรงจาก (x0, y0) ไป (x1, y1)

#### `Matrix_DrawRect()`
```c
void Matrix_DrawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b);
```
วาดสี่เหลี่ยม (เส้นขอบ)

#### `Matrix_FillRect()`
```c
void Matrix_FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t g, uint8_t b);
```
วาดสี่เหลี่ยม (เติมสี)

#### `Matrix_DrawCircle()`
```c
void Matrix_DrawCircle(int16_t x0, int16_t y0, uint8_t radius, uint8_t r, uint8_t g, uint8_t b);
```
วาดวงกลม (เส้นขอบ)

### Text Drawing

#### `Matrix_DrawText()`
```c
uint16_t Matrix_DrawText(int16_t x, int16_t y, const char* text, uint32_t color);
```
วาดข้อความ ASCII

**Return**: ความกว้างของข้อความ (pixels)

#### `Matrix_DrawTextThai()`
```c
uint16_t Matrix_DrawTextThai(int16_t x, int16_t y, const char* text, uint32_t color);
```
วาดข้อความภาษาไทย (UTF-8)

---

## แก้ปัญหา

### ปัญหา: LEDs ไม่ติด

**สาเหตุที่เป็นไปได้:**
1. ไฟเลี้ยงไม่เพียงพอ
2. สาย Data ต่อผิด
3. GND ไม่ได้ต่อร่วมกัน

**วิธีแก้:**
```c
// ทดสอบด้วยโค้ดง่ายๆ
Matrix_Init(GPIOC, GPIO_Pin_4, 8, 8, WIRING_ZIGZAG_LEFT);
Matrix_Fill(255, 0, 0);  // เติมสีแดง
Matrix_Show();
```

### ปัญหา: สีไม่ตรงตำแหน่ง

**สาเหตุ**: รูปแบบการต่อสายไม่ตรงกับที่ตั้งค่า

**วิธีแก้:**
```c
// ลองเปลี่ยนรูปแบบการต่อสาย
Matrix_SetWiringPattern(WIRING_SNAKE);
// หรือ
Matrix_SetWiringPattern(WIRING_ZIGZAG_RIGHT);
```

### ปัญหา: ภาษาไทยแสดงไม่ถูกต้อง

**สาเหตุ**: ไฟล์ไม่ได้ save เป็น UTF-8

**วิธีแก้:**
1. ใช้ editor ที่รองรับ UTF-8 (VS Code, Notepad++)
2. Save file เป็น UTF-8 encoding
3. ตรวจสอบว่าใช้ `Matrix_DrawTextThai()` ไม่ใช่ `Matrix_DrawText()`

### ปัญหา: LEDs กระพริบ

**สาเหตุ**: เรียก `Matrix_Show()` บ่อยเกินไป

**วิธีแก้:**
```c
// ใช้ Timer เพื่อควบคุมอัตราการอัพเดท
Timer_t refresh_timer;
Start_Timer(&refresh_timer, 16, 1);  // 60 FPS

while(1) {
    // อัพเดทข้อมูล
    Matrix_SetPixel(...);
    
    // แสดงผลเมื่อถึงเวลา
    if(Is_Timer_Expired(&refresh_timer)) {
        Matrix_Show();
    }
}
```

### ปัญหา: ใช้ไฟมาก/ร้อน

**วิธีแก้:**
```c
// ลดความสว่าง
Matrix_SetBrightness(30);  // 30% brightness

// หรือใช้สีที่ใช้ไฟน้อยกว่า
// สีขาว (255,255,255) ใช้ไฟมากที่สุด
// สีเดี่ยว (255,0,0) ใช้ไฟน้อยกว่า
```

---

## ตัวอย่างโปรเจกต์

### 1. นาฬิกาดิจิตอล

```c
// ดูไฟล์: Examples/11_Project_Clock.c
```

### 2. แสดงอุณหภูมิ

```c
// ดูไฟล์: Examples/12_Project_Weather.c
```

### 3. เกม Snake

```c
// ดูไฟล์: Examples/07_Intermediate_Games.c
```

---

## เอกสารเพิ่มเติม

- [NeoPixel Library Documentation](../NeoPixel/README_TH.md)
- [SimpleHAL Documentation](../../SimpleHAL/README.md)
- [CH57x Datasheet](https://www.wch.cn/products/CH57x.html)

---

## License

MIT License - ใช้งานได้อย่างอิสระ

## ผู้พัฒนา

CH57x Simple HAL Team

**เวอร์ชัน**: 1.0.0  
**วันที่**: 2025-12-21

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)