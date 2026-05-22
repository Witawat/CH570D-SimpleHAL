# MAX7219 LED Matrix Driver Library - คู่มือการใช้งานฉบับสมบูรณ์

## สารบัญ

1. [บทนำ](#บทนำ)
2. [ฮาร์ดแวร์และการต่อวงจร](#ฮาร์ดแวร์และการต่อวงจร)
3. [การติดตั้งและเริ่มต้นใช้งาน](#การติดตั้งและเริ่มต้นใช้งาน)
4. [การใช้งานขั้นพื้นฐาน](#การใช้งานขั้นพื้นฐาน)
5. [การแสดงข้อความ](#การแสดงข้อความ)
6. [กราฟิกขั้นสูง](#กราฟิกขั้นสูง)
7. [Animation และ Sprite](#animation-และ-sprite)
8. [เทคนิคขั้นสูง](#เทคนิคขั้นสูง)
9. [🆕 ฟีเจอร์ใหม่ (v1.1)](#-ฟีเจอร์ใหม่-v11)
10. [API Reference](#api-reference)
11. [การแก้ปัญหา](#การแก้ปัญหา)
12. [ตัวอย่างโปรเจกต์](#ตัวอย่างโปรเจกต์)

---

## บทนำ

### MAX7219 คืออะไร?

MAX7219 เป็น IC Driver สำหรับควบคุม LED Matrix หรือ 7-Segment Display ผ่าน SPI interface มีคุณสมบัติ:

- **ควบคุม LED ได้ 64 ดวง** (8x8 matrix)
- **SPI Interface** - ใช้สาย 3 เส้น (CLK, MOSI, CS)
- **Cascading** - ต่อหลายตัวได้ (สูงสุด 8 ตัว)
- **ควบคุมความสว่าง** - 16 ระดับ (0-15)
- **ประหยัดพลังงาน** - มี shutdown mode

### คุณสมบัติของ Library

Library นี้ให้ฟังก์ชันครบถ้วนสำหรับการใช้งาน MAX7219:

**พื้นฐาน:**
- เริ่มต้นใช้งานง่าย 1 บรรทัด
- ควบคุมความสว่าง, เปิด/ปิด display
- Clear screen, update display

**กราฟิก:**
- Pixel manipulation
- Line drawing (Bresenham's algorithm)
- Rectangle (filled/outline)
- Circle (Bresenham's circle)
- Triangle
- Bitmap/Sprite rendering

**ข้อความ:**
- รองรับ ASCII font (5x7 และ 8x8)
- รองรับตัวอักษรไทย UTF-8 (8x8, 44 พยัญชนะ + เลขไทย) **[v1.1]**
- รองรับตัวอักษรไทย (5x7 — ดั้งเดิม)
- Scrolling text (horizontal + vertical) **[v1.1]**
- Multiple fonts

**ขั้นสูง:**
- Frame-based animation
- Sprite system พร้อม transparency
- Non-blocking updates
- Cascaded displays (1-8 matrices)
- Fade in/out, Invert effects
- Wipe effects (4 ทิศทาง) **[v1.1]**
- Blink, Sparkle, Marquee, Rain, Running Light effects **[v1.1]**
- Buffer utilities (Shift, Scroll, Progress Bar) **[v1.1]**
- Config macros สำหรับประหยัด Flash **[v1.1]**

---

## ฮาร์ดแวร์และการต่อวงจร

### อุปกรณ์ที่ต้องใช้

1. **CH57x Microcontroller**
2. **MAX7219 LED Matrix Module** (8x8)
3. **สายจั๊มเปอร์** 5 เส้น
4. **แหล่งจ่ายไฟ** 5V (สำหรับ LED Matrix)

### Pin Connections

```
CH57x          MAX7219
--------          -------
PC5 (SPI CLK)  -> CLK
PC6 (SPI MOSI) -> DIN
PC4 (GPIO)     -> CS
GND            -> GND
5V             -> VCC
```

### การต่อแบบ Cascaded (หลาย Matrix)

```
CH57x -> Matrix 1 -> Matrix 2 -> Matrix 3 -> Matrix 4
            CLK        CLK         CLK         CLK
            DIN        DOUT->DIN   DOUT->DIN   DOUT->DIN
            CS         CS          CS          CS
```

**หมายเหตุ:**
- ต่อ DOUT ของ matrix แรกเข้า DIN ของ matrix ถัดไป
- CS ต่อร่วมกันทุกตัว
- แต่ละ matrix ต้องมีไฟเลี้ยงแยก (5V)

### ข้อควรระวัง

1. **แรงดันไฟ:** MAX7219 ใช้ 5V, CH57x ใช้ 3.3V logic (แต่ compatible)
2. **กระแสไฟ:** LED Matrix 8x8 ใช้กระแสสูงสุด ~320mA (ทุก LED ติด)
3. **Decoupling Capacitor:** ใส่ 100nF ที่ VCC-GND ของ MAX7219
4. **ความยาวสาย:** ไม่ควรเกิน 30cm สำหรับ SPI

---

## การติดตั้งและเริ่มต้นใช้งาน

### การติดตั้ง Library

1. คัดลอกไฟล์ทั้งหมดใน `/User/Lib/MAX7219/` ไปยังโปรเจกต์
2. เพิ่ม include path ใน project settings
3. Include header file:

```c
#include "SimpleMAX7219.h"
```

### ตัวอย่างโค้ดเริ่มต้น

```c
#include "debug.h"
#include "SimpleMAX7219.h"

int main(void) {
    // เริ่มต้นระบบ
    SystemCoreClockUpdate();
    Delay_Init();
    
    // เริ่มต้น MAX7219
    // (CLK=PC5, MOSI=PC6, CS=PC4, จำนวน matrix=1)
    MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);
    
    // ตั้งความสว่าง (0-15)
    MAX7219_SetIntensity(display, 8);
    
    // แสดงข้อความ "HI"
    MAX7219_DrawString(display, 0, 0, "HI");
    MAX7219_Update(display);
    
    while(1) {
        // Main loop
    }
}
```

---

## การใช้งานขั้นพื้นฐาน

### 1. การเริ่มต้น Display

```c
// เริ่มต้น 1 matrix
MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);

// เริ่มต้น 4 matrices (cascaded)
MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 4);
```

### 2. การควบคุมความสว่าง

```c
// ตั้งความสว่าง (0=มืดที่สุด, 15=สว่างที่สุด)
MAX7219_SetIntensity(display, 8);

// Fade in (ค่อยๆ สว่างขึ้น)
MAX7219_FadeIn(display, 1000);  // 1 วินาที

// Fade out (ค่อยๆ มืดลง)
MAX7219_FadeOut(display, 1000);
```

### 3. การเปิด/ปิด Display

```c
// เปิด display
MAX7219_DisplayControl(display, true);

// ปิด display (ประหยัดพลังงาน)
MAX7219_DisplayControl(display, false);
```

### 4. การล้างหน้าจอ

```c
// ล้างหน้าจอและอัพเดททันที
MAX7219_Clear(display, true);

// ล้าง buffer อย่างเดียว (ไม่อัพเดท hardware)
MAX7219_Clear(display, false);
```

### 5. การวาด Pixel

```c
// เปิด pixel ที่ตำแหน่ง (3, 4)
MAX7219_SetPixel(display, 3, 4, true);

// ปิด pixel
MAX7219_SetPixel(display, 3, 4, false);

// อัพเดทหน้าจอ
MAX7219_Update(display);
```

### 6. การทดสอบ Display

```c
// เปิด test mode (ไฟติดทั้งหมด)
MAX7219_DisplayTest(display, true);
Delay_Ms(1000);

// ปิด test mode
MAX7219_DisplayTest(display, false);
```

---

## การแสดงข้อความ

### 1. การเลือก Font

Library มี font 3 แบบ:

```c
// Font 5x7 (ประหยัดพื้นที่, อ่านง่าย)
MAX7219_SetFont(display, &font_5x7);

// Font 8x8 (ใช้พื้นที่เต็ม matrix)
MAX7219_SetFont(display, &font_8x8);

// Font ภาษาไทย 5x7
MAX7219_SetFont(display, &font_thai_5x7);
```

### 2. การแสดงตัวอักษร

```c
// แสดงตัวอักษร 'A' ที่ตำแหน่ง (0, 0)
MAX7219_DrawChar(display, 0, 0, 'A');
MAX7219_Update(display);
```

### 3. การแสดงข้อความ

```c
// แสดงข้อความ "HELLO"
MAX7219_Clear(display, false);
MAX7219_DrawString(display, 0, 0, "HELLO");
MAX7219_Update(display);
```

### 4. การคำนวณความกว้างข้อความ

```c
// คำนวณความกว้าง (pixels)
uint16_t width = MAX7219_GetStringWidth(display, "HELLO");
// width = 29 pixels (สำหรับ font 5x7)
```

### 5. การเลื่อนข้อความ (Scrolling)

```c
// เริ่มการเลื่อนข้อความ (non-blocking)
MAX7219_StartScrollText(display, "HELLO WORLD", 100);  // 100ms delay

// อัพเดทใน main loop
while(1) {
    if (!MAX7219_UpdateScroll(display)) {
        // เลื่อนเสร็จแล้ว
        break;
    }
}

// หยุดการเลื่อน
MAX7219_StopScroll(display);
```

### 6. การแสดงข้อความภาษาไทย

```c
// ตั้งค่าใช้ font ภาษาไทย
MAX7219_SetFont(display, &font_thai_5x7);

// หมายเหตุ: ต้องแปลง UTF-8 เป็น Unicode code point ก่อน
// ดูตัวอย่างใน Examples/06_thai_text.c
```

---

## กราฟิกขั้นสูง

### 1. การวาดเส้นตรง

```c
// วาดเส้นจาก (0,0) ไป (7,7)
MAX7219_DrawLine(display, 0, 0, 7, 7, true);
MAX7219_Update(display);
```

### 2. การวาดสี่เหลี่ยม

```c
// วาดกรอบสี่เหลี่ยม
MAX7219_DrawRect(display, 1, 1, 6, 6, false, true);

// วาดสี่เหลี่ยมเติมสี
MAX7219_DrawRect(display, 2, 2, 4, 4, true, true);

MAX7219_Update(display);
```

### 3. การวาดวงกลม

```c
// วาดวงกลมกรอบ (ศูนย์กลาง 3,3 รัศมี 3)
MAX7219_DrawCircle(display, 3, 3, 3, false, true);

// วาดวงกลมเติมสี
MAX7219_DrawCircle(display, 3, 3, 3, true, true);

MAX7219_Update(display);
```

### 4. การวาดสามเหลี่ยม

```c
// วาดสามเหลี่ยมกรอบ
MAX7219_DrawTriangle(display, 3, 0, 0, 7, 6, 7, false, true);

// วาดสามเหลี่ยมเติมสี
MAX7219_DrawTriangle(display, 3, 0, 0, 7, 6, 7, true, true);

MAX7219_Update(display);
```

### 5. การแสดง Bitmap

```c
// กำหนด bitmap 8x8 (รูปหัวใจ)
const uint8_t heart[] = {
    0x00,  // ........
    0x66,  // .##..##.
    0xFF,  // ########
    0xFF,  // ########
    0xFF,  // ########
    0x7E,  // .######.
    0x3C,  // ..####..
    0x18   // ...##...
};

// แสดง bitmap
MAX7219_DrawBitmap(display, 0, 0, heart, 8, 8);
MAX7219_Update(display);
```

---

## Animation และ Sprite

### 1. Frame-based Animation

```c
// กำหนด animation frames
const uint8_t frame1[] = {0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t frame2[] = {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00};
const uint8_t frame3[] = {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00};

const uint8_t* frames[] = {frame1, frame2, frame3};

// เริ่ม animation (3 frames, 100ms delay, loop=true)
MAX7219_StartAnimation(display, frames, 3, 100, true);

// อัพเดทใน main loop
while(1) {
    MAX7219_UpdateAnimation(display);
}

// หยุด animation
MAX7219_StopAnimation(display);
```

### 2. Sprite System

```c
// กำหนด sprite
const uint8_t sprite[] = {
    0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C
};

// กำหนด mask (transparency)
const uint8_t mask[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// วาด sprite ที่ตำแหน่ง (0, 0)
MAX7219_DrawSprite(display, 0, 0, sprite, mask, 8, 8);
MAX7219_Update(display);
```

### 3. Sprite Animation (Moving)

```c
// เคลื่อนที่ sprite จากซ้ายไปขวา
for (int x = 0; x < 8; x++) {
    MAX7219_Clear(display, false);
    MAX7219_DrawSprite(display, x, 0, sprite, mask, 8, 8);
    MAX7219_Update(display);
    Delay_Ms(100);
}
```

---

## เทคนิคขั้นสูง

### 1. Double Buffering

Library ใช้ double buffering อัตโนมัติ:

```c
// วาดกราฟิกใน buffer (ไม่แสดงผลทันที)
MAX7219_Clear(display, false);
MAX7219_DrawCircle(display, 3, 3, 3, true, true);
MAX7219_DrawRect(display, 0, 0, 8, 8, false, true);

// อัพเดทหน้าจอพร้อมกัน (ไม่มี flicker)
MAX7219_Update(display);
```

### 2. Smooth Scrolling

```c
// Scrolling ที่ smooth (ใช้ delay น้อย)
MAX7219_StartScrollText(display, "SMOOTH SCROLL", 50);

while(MAX7219_UpdateScroll(display)) {
    // Non-blocking - สามารถทำงานอื่นได้
}
```

### 3. Custom Fonts

สร้าง font เอง:

```c
// กำหนด font data
static const uint8_t my_font_data[] = {
    // Character data (5 bytes per character)
    0x7E, 0x11, 0x11, 0x11, 0x7E,  // A
    0x7F, 0x49, 0x49, 0x49, 0x36,  // B
    // ... more characters
};

// สร้าง font structure
const MAX7219_Font my_font = {
    .width = 5,
    .height = 7,
    .first_char = 'A',
    .last_char = 'Z',
    .data = my_font_data
};

// ใช้งาน
MAX7219_SetFont(display, &my_font);
```

### 4. Performance Optimization

```c
// ❌ ไม่ดี - อัพเดททุก pixel
for (int i = 0; i < 64; i++) {
    MAX7219_SetPixel(display, i % 8, i / 8, true);
    MAX7219_Update(display);  // อัพเดทบ่อยเกินไป
}

// ✅ ดี - อัพเดทครั้งเดียว
for (int i = 0; i < 64; i++) {
    MAX7219_SetPixel(display, i % 8, i / 8, true);
}
MAX7219_Update(display);  // อัพเดทครั้งเดียว
```

### 5. Cascaded Displays

```c
// เริ่มต้น 4 matrices (32x8 pixels)
MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 4);

// วาดเส้นยาวข้าม 4 matrices
MAX7219_DrawLine(display, 0, 3, 31, 3, true);

// แสดงข้อความยาว
MAX7219_DrawString(display, 0, 0, "LONG TEXT");

MAX7219_Update(display);
```

---

## 🆕 ฟีเจอร์ใหม่ (v1.1)

### การแสดงข้อความภาษาไทยด้วย UTF-8

ฟังก์ชัน `MAX7219_DrawCharThai()` และ `MAX7219_DrawStringThai()` รองรับการแสดงข้อความภาษาไทย
โดยใช้ฟอนต์ 8x8 ที่อ่านง่ายกว่ารุ่นเดิม (5x7)

**ข้อแตกต่างจากรุ่นเก่า:**
- ของเก่า `DrawChar()` + `font_thai_5x7` — ไม่รองรับ UTF-8 จริง (Char 8-bit ไม่พอ)
- ของใหม่ `DrawStringThai()` — รองรับ UTF-8 3-byte, ผสมไทย-อังกฤษได้

```c
#include "max7219_fonts_thai.h"  // ต้อง include

// ข้อความภาษาไทย
MAX7219_DrawStringThai(display, 0, 0, "สวัสดี", 8);
MAX7219_Update(display);

// ผสมไทย-อังกฤษ
MAX7219_DrawStringThai(display, 0, 0, "Hello สวัสดี", 8);
MAX7219_Update(display);
```

### Wipe Effects

```c
// เติมคอลัมน์จากซ้ายไปขวา
MAX7219_WipeLeft(display, 30);   // 30ms ต่อคอลัมน์

// เติมคอลัมน์จากขวาไปซ้าย
MAX7219_WipeRight(display, 30);

// เติมแถวจากบนลงล่าง
MAX7219_WipeUp(display, 30);

// เติมแถวจากล่างขึ้นบน
MAX7219_WipeDown(display, 30);
```

### Other Effects

```c
// กระพริบ 3 ครั้ง
MAX7219_Blink(display, 3, 200, 200);

// Sparkle (เปิดปิด pixels สุ่ม) 2 วินาที ที่ density 10%
MAX7219_Sparkle(display, 2000, 10);

// แสงวิ่งรอบขอบ 2 รอบ
MAX7219_MarqueeBorder(display, 50, 2);

// ฝนตก 3 วินาที
MAX7219_RainEffect(display, 3000, 100);

// จุดวิ่งรอบขอบ (non-blocking, เรียกใน main loop)
// MAX7219_RunningLight(display, 100); // infinite loop
```

### Vertical Scrolling

```c
MAX7219_StartScrollTextVertical(display, "ข้อความ", 100);
while(MAX7219_UpdateScroll(display)) {
    // Non-blocking — ทำอย่างอื่นได้
}
```

### Buffer Utilities

```c
// Shift buffer ซ้าย 1 pixel
MAX7219_ShiftLeft(display, 1);
MAX7219_Update(display);

// Progress bar 75% (แนวนอน)
MAX7219_ProgressBar(display, 75, false);
MAX7219_Update(display);

// Progress bar แนวตั้ง
MAX7219_ProgressBar(display, 50, true);
MAX7219_Update(display);
```

### Config Macros (ประหยัด Flash)

```c
// ปิด Thai 8x8 font — ประหยัด ~600B Flash
#define MAX7219_ENABLE_THAI_FULL  0

// ปิด Effects — ประหยัด ~600B Flash
#define MAX7219_ENABLE_EFFECTS    0

#include "MAX7219.h"
```

---

### Initialization Functions

#### `MAX7219_Init()`
```c
MAX7219_Handle* MAX7219_Init(uint8_t clk_pin, uint8_t mosi_pin, 
                             uint8_t cs_pin, uint8_t num_devices);
```
- **Parameters:**
  - `clk_pin`: SPI CLK pin
  - `mosi_pin`: SPI MOSI pin
  - `cs_pin`: CS pin
  - `num_devices`: จำนวน cascaded devices (1-8)
- **Returns:** Pointer to MAX7219_Handle
- **Example:** `MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);`

#### `MAX7219_SetIntensity()`
```c
void MAX7219_SetIntensity(MAX7219_Handle* handle, uint8_t intensity);
```
- **Parameters:**
  - `handle`: Display handle
  - `intensity`: ความสว่าง (0-15)

#### `MAX7219_DisplayControl()`
```c
void MAX7219_DisplayControl(MAX7219_Handle* handle, bool on);
```
- **Parameters:**
  - `handle`: Display handle
  - `on`: true=เปิด, false=ปิด

#### `MAX7219_Clear()`
```c
void MAX7219_Clear(MAX7219_Handle* handle, bool update);
```
- **Parameters:**
  - `handle`: Display handle
  - `update`: true=อัพเดท hardware, false=ล้าง buffer อย่างเดียว

#### `MAX7219_Update()`
```c
void MAX7219_Update(MAX7219_Handle* handle);
```
- **Parameters:**
  - `handle`: Display handle
- **Description:** อัพเดท buffer ไปยัง hardware

### Graphics Functions

#### `MAX7219_SetPixel()`
```c
void MAX7219_SetPixel(MAX7219_Handle* handle, int16_t x, int16_t y, bool on);
```

#### `MAX7219_GetPixel()`
```c
bool MAX7219_GetPixel(MAX7219_Handle* handle, int16_t x, int16_t y);
```

#### `MAX7219_DrawLine()`
```c
void MAX7219_DrawLine(MAX7219_Handle* handle, int16_t x0, int16_t y0, 
                      int16_t x1, int16_t y1, bool on);
```

#### `MAX7219_DrawRect()`
```c
void MAX7219_DrawRect(MAX7219_Handle* handle, int16_t x, int16_t y, 
                      int16_t w, int16_t h, bool filled, bool on);
```

#### `MAX7219_DrawCircle()`
```c
void MAX7219_DrawCircle(MAX7219_Handle* handle, int16_t x0, int16_t y0, 
                        int16_t r, bool filled, bool on);
```

#### `MAX7219_DrawTriangle()`
```c
void MAX7219_DrawTriangle(MAX7219_Handle* handle, int16_t x0, int16_t y0, 
                          int16_t x1, int16_t y1, int16_t x2, int16_t y2, 
                          bool filled, bool on);
```

#### `MAX7219_DrawBitmap()`
```c
void MAX7219_DrawBitmap(MAX7219_Handle* handle, int16_t x, int16_t y, 
                        const uint8_t* bitmap, uint8_t width, uint8_t height);
```

### Text Functions

#### `MAX7219_SetFont()`
```c
void MAX7219_SetFont(MAX7219_Handle* handle, const MAX7219_Font* font);
```

#### `MAX7219_DrawChar()`
```c
uint8_t MAX7219_DrawChar(MAX7219_Handle* handle, int16_t x, int16_t y, char ch);
```

#### `MAX7219_DrawString()`
```c
uint16_t MAX7219_DrawString(MAX7219_Handle* handle, int16_t x, int16_t y, 
                            const char* text);
```

#### `MAX7219_GetStringWidth()`
```c
uint16_t MAX7219_GetStringWidth(MAX7219_Handle* handle, const char* text);
```

### Scrolling Functions

#### `MAX7219_StartScrollText()`
```c
void MAX7219_StartScrollText(MAX7219_Handle* handle, const char* text, 
                             uint16_t scroll_delay);
```

#### `MAX7219_UpdateScroll()`
```c
bool MAX7219_UpdateScroll(MAX7219_Handle* handle);
```

#### `MAX7219_StopScroll()`
```c
void MAX7219_StopScroll(MAX7219_Handle* handle);
```

### Animation Functions

#### `MAX7219_StartAnimation()`
```c
void MAX7219_StartAnimation(MAX7219_Handle* handle, const uint8_t** frames, 
                           uint8_t num_frames, uint16_t frame_delay, bool loop);
```

#### `MAX7219_UpdateAnimation()`
```c
bool MAX7219_UpdateAnimation(MAX7219_Handle* handle);
```

#### `MAX7219_StopAnimation()`
```c
void MAX7219_StopAnimation(MAX7219_Handle* handle);
```

### Sprite Functions

#### `MAX7219_DrawSprite()`
```c
void MAX7219_DrawSprite(MAX7219_Handle* handle, int16_t x, int16_t y, 
                       const uint8_t* sprite, const uint8_t* mask, 
                       uint8_t width, uint8_t height);
```

### Utility Functions

#### `MAX7219_FadeIn()`
```c
void MAX7219_FadeIn(MAX7219_Handle* handle, uint16_t duration);
```

#### `MAX7219_FadeOut()`
```c
void MAX7219_FadeOut(MAX7219_Handle* handle, uint16_t duration);
```

#### `MAX7219_Invert()`
```c
void MAX7219_Invert(MAX7219_Handle* handle);
```

---

### 🆕 Thai Text Functions (UTF-8) **[v1.1]**

#### `MAX7219_DrawCharThai()`
```c
uint8_t MAX7219_DrawCharThai(MAX7219_Handle* handle, int16_t x, int16_t y,
                             const char* thai_char, uint8_t intensity);
```
วาดตัวอักษรไทย 1 ตัว (UTF-8, 3 bytes) ด้วย intensity 0-15

#### `MAX7219_DrawStringThai()`
```c
uint16_t MAX7219_DrawStringThai(MAX7219_Handle* handle, int16_t x, int16_t y,
                                const char* text, uint8_t intensity);
```
วาดข้อความภาษาไทย (UTF-8, รองรับผสมภาษาอังกฤษ) — ต้อง `#include "max7219_fonts_thai.h"`

---

### 🆕 Effects (เพิ่มจาก v1.1)

#### `MAX7219_WipeLeft()` / `WipeRight()` / `WipeUp()` / `WipeDown()`
```c
void MAX7219_WipeLeft(MAX7219_Handle* handle, uint16_t delay_ms);
void MAX7219_WipeRight(MAX7219_Handle* handle, uint16_t delay_ms);
void MAX7219_WipeUp(MAX7219_Handle* handle, uint16_t delay_ms);
void MAX7219_WipeDown(MAX7219_Handle* handle, uint16_t delay_ms);
```
Wipe effect — ค่อยๆ เติม pixels จากทิศทางที่กำหนด (blocking)

#### `MAX7219_Blink()`
```c
void MAX7219_Blink(MAX7219_Handle* handle, uint16_t count,
                   uint16_t on_ms, uint16_t off_ms);
```
กระพริบ display `count` ครั้ง (blocking)

#### `MAX7219_Sparkle()`
```c
void MAX7219_Sparkle(MAX7219_Handle* handle, uint16_t duration_ms,
                     uint8_t density);
```
เปิด/ปิด pixels แบบสุ่มตาม density (0-100%) (blocking)

#### `MAX7219_MarqueeBorder()`
```c
void MAX7219_MarqueeBorder(MAX7219_Handle* handle, uint16_t speed_ms,
                           uint8_t rounds);
```
จุดวิ่งรอบขอบ display `rounds` รอบ (0 = ไม่มีที่สิ้นสุด) (blocking)

#### `MAX7219_RainEffect()`
```c
void MAX7219_RainEffect(MAX7219_Handle* handle, uint16_t duration_ms,
                        uint16_t speed_ms);
```
ฝนตกจากบนลงล่าง (blocking)

#### `MAX7219_RunningLight()`
```c
void MAX7219_RunningLight(MAX7219_Handle* handle, uint16_t speed_ms);
```
จุดวิ่งรอบขอบตลอด (infinite loop — ใช้ด้วยความระมัดระวัง)

---

### 🆕 Vertical Scrolling **[v1.1]**

#### `MAX7219_StartScrollTextVertical()`
```c
void MAX7219_StartScrollTextVertical(MAX7219_Handle* handle, const char* text,
                                     uint16_t scroll_delay);
```
เริ่มเลื่อนข้อความแนวตั้ง (ล่างขึ้นบน) — ใช้ `MAX7219_UpdateScroll()` เช่นเดียวกับ horizontal

---

### 🆕 Buffer Utilities **[v1.1]**

#### `MAX7219_ShiftLeft()` / `ShiftRight()` / `ShiftUp()` / `ShiftDown()`
```c
void MAX7219_ShiftLeft(MAX7219_Handle* handle, uint8_t pixels);
void MAX7219_ShiftRight(MAX7219_Handle* handle, uint8_t pixels);
void MAX7219_ShiftUp(MAX7219_Handle* handle, uint8_t pixels);
void MAX7219_ShiftDown(MAX7219_Handle* handle, uint8_t pixels);
```
เลื่อน buffer content ตามทิศทาง

#### `MAX7219_ScrollBufferLeft()` / `ScrollBufferRight()`
```c
void MAX7219_ScrollBufferLeft(MAX7219_Handle* handle, uint16_t speed_ms);
void MAX7219_ScrollBufferRight(MAX7219_Handle* handle, uint16_t speed_ms);
```
Auto-scroll buffer (non-blocking — เรียกใน main loop)

#### `MAX7219_ProgressBar()`
```c
void MAX7219_ProgressBar(MAX7219_Handle* handle, uint8_t percent, bool vertical);
```
วาด progress bar 0-100% (แนวนอนแนวตั้ง)

---

## การแก้ปัญหา

### ปัญหา: Display ไม่ติด

**สาเหตุที่เป็นไปได้:**
1. ต่อสายผิด
2. ไฟเลี้ยงไม่พอ
3. MAX7219 เสีย

**วิธีแก้:**
```c
// ทดสอบ display test mode
MAX7219_DisplayTest(display, true);
Delay_Ms(2000);
MAX7219_DisplayTest(display, false);

// ถ้าไฟไม่ติด = ปัญหาฮาร์ดแวร์
// ถ้าไฟติด = ปัญหาโค้ด
```

### ปัญหา: Display กะพริบ (Flicker)

**สาเหตุ:** อัพเดทบ่อยเกินไป

**วิธีแก้:**
```c
// ❌ ไม่ดี
for (int i = 0; i < 100; i++) {
    MAX7219_SetPixel(display, i % 8, i / 8, true);
    MAX7219_Update(display);  // อัพเดทบ่อยเกินไป
}

// ✅ ดี
for (int i = 0; i < 100; i++) {
    MAX7219_SetPixel(display, i % 8, i / 8, true);
}
MAX7219_Update(display);  // อัพเดทครั้งเดียว
```

### ปัญหา: Cascaded Displays ไม่ทำงาน

**สาเหตุ:** ต่อสายผิด

**วิธีแก้:**
1. ตรวจสอบ DOUT -> DIN ระหว่าง matrices
2. ตรวจสอบ CS ต่อร่วมกันทุกตัว
3. ตรวจสอบไฟเลี้ยงแต่ละตัว

### ปัญหา: ข้อความแสดงผิด

**สาเหตุ:** Font ไม่รองรับตัวอักษร

**วิธีแก้:**
```c
// ตรวจสอบว่า font รองรับตัวอักษรหรือไม่
// font_5x7 รองรับ ASCII 32-127
// font_thai_5x7 รองรับตัวอักษรไทยบางตัว
```

### ปัญหา: Memory Overflow

**สาเหตุ:** ใช้ cascaded displays มากเกินไป

**วิธีแก้:**
- ลดจำนวน devices
- ใช้ dynamic memory allocation
- เพิ่ม RAM

---

## ตัวอย่างโปรเจกต์

### 1. นาฬิกาดิจิตอล

ดูตัวอย่างใน `Examples/11_clock_display.c`

**คุณสมบัติ:**
- แสดงเวลา HH:MM
- Colon กะพริบทุกวินาที
- เลื่อนข้อความทุก 10 วินาที

### 2. เกม Snake

ดูตัวอย่างใน `Examples/10_game_snake.c`

**คุณสมบัติ:**
- ควบคุมด้วยปุ่ม 4 ทิศทาง
- ตรวจจับการชน
- นับคะแนน

### 3. Spectrum Analyzer

ดูตัวอย่างใน `Examples/12_spectrum_analyzer.c`

**คุณสมบัติ:**
- อ่านค่าจาก ADC
- แสดงเป็น bar graph
- Real-time update

### 4. Weather Station

ดูตัวอย่างใน `Examples/13_weather_station.c`

**คุณสมบัติ:**
- แสดงไอคอนสภาพอากาศ
- แสดงอุณหภูมิ
- เลื่อนข้อความสภาพอากาศ

---

## สรุป

MAX7219 LED Matrix Driver Library นี้ให้ฟังก์ชันครบถ้วนสำหรับการใช้งาน LED Matrix ตั้งแต่ขั้นพื้นฐานถึงขั้นสูง พร้อมตัวอย่างโค้ด 14 ตัวอย่าง และเอกสารภาษาไทยฉบับสมบูรณ์

**ข้อดีของ Library:**
- ✅ ใช้งานง่าย Arduino-style API
- ✅ รองรับ cascaded displays
- ✅ Graphics primitives ครบถ้วน
- ✅ รองรับภาษาไทย
- ✅ Animation และ sprite system
- ✅ Non-blocking operations
- ✅ เอกสารภาษาไทยครบถ้วน

**เริ่มต้นใช้งาน:**
1. ดูตัวอย่างใน `Examples/01_basic_display.c`
2. ศึกษา API Reference
3. ทดลองสร้างโปรเจกต์ของคุณเอง

**ติดต่อและสนับสนุน:**
- GitHub: [CH57x-main]
- Documentation: [README.md]

---

**เวอร์ชัน:** 1.0  
**วันที่:** 2025-12-21  
**ผู้พัฒนา:** CH57x SimpleHAL Team  
**License:** MIT License

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)