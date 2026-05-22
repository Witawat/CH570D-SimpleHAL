# MAX7219 LED Matrix Driver Library

**เวอร์ชัน:** 1.1  
**วันที่:** 2026-05-03  
**License:** MIT

## ภาพรวม

Library สำหรับควบคุม MAX7219 LED Matrix (8x8) บน CH57x series พร้อมฟีเจอร์ครบถ้วนตั้งแต่ขั้นพื้นฐานถึงขั้นสูง

## คุณสมบัติหลัก

- ✅ **ใช้งานง่าย** - Arduino-style API
- ✅ **Graphics Primitives** - Line, Rectangle, Circle, Triangle
- ✅ **Text Rendering** - ASCII (5x7, 8x8) + Thai UTF-8 (8x8, 44 consonants + digits + UTF-8 mapping)
- ✅ **Scrolling Text** - Horizontal + Vertical scrolling (non-blocking)
- ✅ **Animation System** - Frame-based animation
- ✅ **Sprite Support** - Transparency mask
- ✅ **Cascaded Displays** - รองรับ 1-8 matrices
- ✅ **Effects** - Fade in/out, Invert, Wipe (4 ทิศ), Blink, Sparkle, Marquee Border, Rain, Running Light
- ✅ **Buffer Utilities** - Shift (left/right/up/down), Auto-scroll, Progress Bar
- ✅ **14 ตัวอย่าง** - จากพื้นฐานถึงขั้นสูง
- ✅ **เอกสารภาษาไทย** - ครบถ้วน

## Quick Start

```c
#include "SimpleMAX7219.h"

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    
    // เริ่มต้น MAX7219
    MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);
    MAX7219_SetIntensity(display, 8);
    
    // แสดงข้อความ
    MAX7219_DrawString(display, 0, 0, "HI");
    MAX7219_Update(display);
    
    while(1) {}
}
```

## การต่อวงจร

```
CH57x          MAX7219
--------          -------
PC5 (CLK)      -> CLK
PC6 (MOSI)     -> DIN
PC4 (CS)       -> CS
GND            -> GND
5V             -> VCC
```

## ตัวอย่างโค้ด

### 1. Basic Display (01_basic_display.c)
- เริ่มต้นใช้งาน
- แสดงรูปแบบต่างๆ
- ควบคุมความสว่าง

### 2. Text Display (02_text_display.c)
- แสดงตัวอักษร ASCII
- แสดงข้อความ
- นับเลข

### 3. Graphics Primitives (03_graphics_primitives.c)
- วาดเส้น, สี่เหลี่ยม, วงกลม, สามเหลี่ยม
- Animation

### 4. Brightness Control (04_brightness_control.c)
- Fade in/out
- Pulse effect
- Breathing effect

### 5. Scrolling Text (05_scrolling_text.c)
- เลื่อนข้อความแนวนอน
- Non-blocking

### 6. Thai Text (06_thai_text.c)
- แสดงตัวอักษรไทย
- Font ภาษาไทย

### 7. Animation (07_animation.c)
- Frame-based animation
- Bouncing ball
- Rotating patterns

### 8. Sprite Display (08_sprite_display.c)
- แสดง sprite
- Transparency
- Movement animation

### 9. Multiple Matrix (09_multiple_matrix.c)
- Cascaded displays (4 matrices)
- ข้อความยาว
- Wave animation

### 10. Snake Game (10_game_snake.c)
- เกมงูกินอาหาร
- ควบคุมด้วยปุ่ม
- Collision detection

### 11. Clock Display (11_clock_display.c)
- นาฬิกาดิจิตอล
- แสดงเวลา HH:MM
- Scrolling messages

### 12. Spectrum Analyzer (12_spectrum_analyzer.c)
- แสดงระดับเสียง
- Bar graph
- ADC input

### 13. Weather Station (13_weather_station.c)
- แสดงไอคอนสภาพอากาศ
- อุณหภูมิ
- Scrolling text

### 14. Custom Animation (14_custom_animation.c)
- Plasma effect
- Ripple effect
- Matrix rain
- Starfield

## ✨ ตัวอย่างฟีเจอร์ใหม่ (v1.1)

### Thai UTF-8 Text (แนะนำ)

```c
#include "MAX7219.h"
#include "max7219_fonts_thai.h"

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();

    MAX7219_Handle* display = MAX7219_Init(PC5, PC6, PC4, 1);
    MAX7219_SetIntensity(display, 8);

    // แสดงข้อความภาษาไทยด้วย UTF-8
    MAX7219_DrawStringThai(display, 0, 0, "สวัสดี", 8);
    MAX7219_Update(display);

    // แสดงผสมไทย-อังกฤษ
    MAX7219_DrawStringThai(display, 0, 0, "Hello สวัสดี", 8);
    MAX7219_Update(display);

    while(1);
}
```

### Wipe Effects

```c
// เติมสีจากซ้ายไปขวา
MAX7219_WipeLeft(display, 30);

// เติมสีจากขวาไปซ้าย
MAX7219_WipeRight(display, 30);

// เติมจากบนลงล่าง / ล่างขึ้นบน
MAX7219_WipeUp(display, 30);
MAX7219_WipeDown(display, 30);

// กระพริบ 3 ครั้ง
MAX7219_Blink(display, 3, 200, 200);

// Sparkle 2 วินาที
MAX7219_Sparkle(display, 2000, 10);

// ฝนตก
MAX7219_RainEffect(display, 3000, 100);

// จุดวิ่งรอบขอบ
MAX7219_RunningLight(display, 100);
```

### Vertical Scrolling

```c
// เลื่อนข้อความแนวตั้ง (ล่างขึ้นบน)
MAX7219_StartScrollTextVertical(display, "Hello", 100);
while(MAX7219_UpdateScroll(display)) {
    // Non-blocking
}
```

### Buffer Utilities

```c
// เลื่อน buffer ซ้าย 1 pixel
MAX7219_ShiftLeft(display, 1);
MAX7219_Update(display);

// เลื่อน buffer ลง
MAX7219_ShiftDown(display, 1);
MAX7219_Update(display);

// Auto-scroll (เรียกใน main loop)
while(1) {
    MAX7219_ScrollBufferLeft(display, 100);
    // ไม่ต้องเรียก Update — ScrollBufferLeft เรียกให้แล้ว
}

// Progress Bar 75%
MAX7219_ProgressBar(display, 75, false);  // แนวนอน
MAX7219_Update(display);
```

### Config Macros (ประหยัด Flash)

```c
// ปิด Thai 8x8 font (ใช้ 5x7 แทน) — ประหยัด ~600B
#define MAX7219_ENABLE_THAI_FULL  0

// ปิด Effects — ประหยัด ~600B
#define MAX7219_ENABLE_EFFECTS    0

#include "MAX7219.h"
```

## API Reference

### Initialization
- `MAX7219_Init()` - เริ่มต้น display
- `MAX7219_SetIntensity()` - ตั้งความสว่าง (0-15)
- `MAX7219_DisplayControl()` - เปิด/ปิด display
- `MAX7219_Clear()` - ล้างหน้าจอ
- `MAX7219_Update()` - อัพเดท hardware

### Graphics
- `MAX7219_SetPixel()` - ตั้งค่า pixel
- `MAX7219_DrawLine()` - วาดเส้น
- `MAX7219_DrawRect()` - วาดสี่เหลี่ยม
- `MAX7219_DrawCircle()` - วาดวงกลม
- `MAX7219_DrawTriangle()` - วาดสามเหลี่ยม
- `MAX7219_DrawBitmap()` - แสดง bitmap

### Text
- `MAX7219_SetFont()` - เลือก font
- `MAX7219_DrawChar()` - แสดงตัวอักษร
- `MAX7219_DrawString()` - แสดงข้อความ
- `MAX7219_GetStringWidth()` - คำนวณความกว้าง

### Scrolling
- `MAX7219_StartScrollText()` - เริ่มเลื่อนข้อความแนวนอน
- `MAX7219_StartScrollTextVertical()` - เริ่มเลื่อนข้อความแนวตั้ง **[v1.1]**
- `MAX7219_UpdateScroll()` - อัพเดทการเลื่อน (รองรับทั้งแนวนอนและแนวตั้ง)
- `MAX7219_StopScroll()` - หยุดการเลื่อน

### Thai Text (UTF-8) **[v1.1]**
- `MAX7219_DrawCharThai()` - วาดตัวอักษรไทย 1 ตัว (UTF-8)
- `MAX7219_DrawStringThai()` - วาดข้อความภาษาไทย (UTF-8, ผสมอังกฤษได้)

### Effects (เพิ่มจาก v1.1)
- `MAX7219_FadeIn()` - Fade in effect
- `MAX7219_FadeOut()` - Fade out effect
- `MAX7219_Invert()` - สลับ on/off ทุก pixel
- `MAX7219_WipeLeft()` / `WipeRight()` / `WipeUp()` / `WipeDown()` - Wipe effect **[v1.1]**
- `MAX7219_Blink()` - กระพริบ display **[v1.1]**
- `MAX7219_Sparkle()` - เปิด/ปิด pixels แบบสุ่ม **[v1.1]**
- `MAX7219_MarqueeBorder()` - แสงวิ่งรอบขอบ **[v1.1]**
- `MAX7219_RainEffect()` - ฝนตก **[v1.1]**
- `MAX7219_RunningLight()` - จุดวิ่งรอบขอบ **[v1.1]**

### Buffer Utilities **[v1.1]**
- `MAX7219_ShiftLeft/Right/Up/Down()` - เลื่อน buffer
- `MAX7219_ScrollBufferLeft/Right()` - Auto-scroll (non-blocking)
- `MAX7219_ProgressBar()` - วาด progress bar 0-100%

## Fonts

Library มีฟอนต์ทั้งหมด:
- **font_5x7** - ASCII 5x7 (ประหยัดพื้นที่)
- **font_8x8** - ASCII 8x8 (ใช้พื้นที่เต็ม)
- **font_thai_5x7** - ตัวอักษรไทย 5x7 (เดิม)
- **font_thai_8x8** - ตัวอักษรไทย 8x8 (44 พยัญชนะ + เลขไทย) **[v1.1]**
- **font_thai_numbers_8x8** - เลขไทย 8x8 (๐-๙) **[v1.1]**

> ต้องการ `#include "max7219_fonts_thai.h"` สำหรับ Thai 8x8 font

## Config Macros **[v1.1]**

```c
// ปิด Thai 8x8 font (ใช้ 5x7 แทน — ประหยัด ~600B Flash)
#define MAX7219_ENABLE_THAI_FULL  0

// ปิด Effects (wipe/blink/sparkle/rain — ประหยัด ~600B Flash)
#define MAX7219_ENABLE_EFFECTS    0

#include "MAX7219.h"
```

## เอกสารเพิ่มเติม

📖 **[MAX7219_Documentation_TH.md](MAX7219_Documentation_TH.md)** - เอกสารภาษาไทยฉบับสมบูรณ์ (1000+ บรรทัด)

เนื้อหาครอบคลุม:
- บทนำและฮาร์ดแวร์
- การติดตั้งและเริ่มต้นใช้งาน
- การใช้งานขั้นพื้นฐาน
- การแสดงข้อความ
- กราฟิกขั้นสูง
- Animation และ Sprite
- เทคนิคขั้นสูง
- API Reference ครบถ้วน
- การแก้ปัญหา
- ตัวอย่างโปรเจกต์

## การติดตั้ง

1. คัดลอกโฟลเดอร์ `MAX7219` ไปยัง `/User/Lib/`
2. Include ใน project:
```c
#include "SimpleMAX7219.h"
```

## ข้อกำหนดระบบ

- **MCU:** CH57x
- **Dependencies:** SimpleSPI, SimpleGPIO, Timer
- **RAM:** ~1KB (สำหรับ 1 matrix)
- **Flash:** ~8KB

## License

MIT License - ใช้งานได้อย่างอิสระ

## ผู้พัฒนา

CH57x SimpleHAL Team

## การสนับสนุน

หากพบปัญหาหรือมีคำถาม:
1. ดูเอกสาร [MAX7219_Documentation_TH.md](MAX7219_Documentation_TH.md)
2. ดูตัวอย่างใน `Examples/`
3. ตรวจสอบ [การแก้ปัญหา](MAX7219_Documentation_TH.md#การแก้ปัญหา)

---

**Happy Coding! 🚀**

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)