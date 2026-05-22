# WS2815 LED Matrix Library - คู่มือการใช้งาน

> **Library สำหรับควบคุม LED Matrix (WS2812/WS2815) บน CH57x**

## 📋 สารบัญ

1. [บทนำ](#บทนำ)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [การติดตั้งฮาร์ดแวร์](#การติดตั้งฮาร์ดแวร์)
4. [การใช้งานขั้นพื้นฐาน](#การใช้งานขั้นพื้นฐาน)
5. [การแสดงข้อความและกราฟิก](#การแสดงข้อความและกราฟิก)
6. [API Reference](#api-reference)

---

## บทนำ

WS2815 LED Matrix Library เป็น library สำหรับควบคุม LED Matrix แบบ WS2815 หรือ WS2812 บน CH57x microcontroller โดยรองรับการต่อสายแบบต่างๆ และมีฟังก์ชันครบครันตั้งแต่การวาดรูปทรงพื้นฐานไปจนถึงการแสดงผลภาษาไทย

### ความแตกต่างระหว่าง WS2812 และ WS2815

| คุณสมบัติ | WS2812 | WS2815 |
|-----------|--------|--------|
| แรงดันไฟ | 5V | 12V |
| Data Line | Single | Dual (Backup) |
| Timing | เหมือนกัน | เหมือนกัน |

---

## คุณสมบัติ

✅ **รองรับ Matrix หลายขนาด**: 8x8, 16x16, 32x8 หรือขนาดอื่นๆ ตามต้องการ
✅ **รูปแบบการต่อสาย 4 แบบ**: Zigzag Left (แนะนำ), Zigzag Right, Snake, Columns
✅ **ฟังก์ชันวาดรูปครบครัน**: จุด, เส้น, สี่เหลี่ยม, วงกลม, Bitmap
✅ **รองรับภาษาไทย**: ฟอนต์ภาษาไทย 8x8 pixels พร้อมสระและวรรณยุกต์

---

## การติดตั้งฮาร์ดแวร์

### วงจรการต่อ

| WS2815 Matrix Pin | CH57x Pin | หมายเหตุ |
|-------------------|--------------|----------|
| VCC               | 12V (External)| ไฟเลี้ยงแยกสำหรับ LED |
| GND               | GND          | กราวด์ร่วมกับ MCU |
| DIN               | PC4 (Option) | สัญญาให้ข้อมูล (ปรับเปลี่ยนได้ในโค้ด) |

> **⚠️ คำเตือน**: ห้ามต่อไฟ 12V เข้ากับขาของ CH57x โดยตรง

---

## การใช้งานขั้นพื้นฐาน

```c
#include "main.h"
#include "Lib/WS2815Matrix/WS2815Matrix.h"

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    
    // 1. เริ่มต้น Matrix 8x8 แบบ Zigzag บน pin PC4
    Matrix_Init(GPIOC, GPIO_Pin_4, 8, 8, WIRING_ZIGZAG_LEFT);
    
    // 2. ตั้งค่าความสว่าง (0-255)
    Matrix_SetBrightness(50);
    
    while(1) {
        // 3. วาดจุดสีแดงที่พิกเซล (0, 0)
        Matrix_SetPixel(0, 0, 255, 0, 0);
        
        // 4. วาดวงกลมสีเขียว
        Matrix_DrawCircle(3, 3, 3, 0, 255, 0);
        
        // 5. อัปเดตการแสดงผล
        Matrix_Show();
        
        Delay_Ms(1000);
    }
}
```

---

## การแสดงข้อความและกราฟิก

```c
// แสดงข้อความภาษาอังกฤษ
Matrix_DrawText(0, 0, "HI", COLOR_BLUE);

// แสดงข้อความภาษาไทย (ต้องการฟอนต์ไทยใน fonts.h)
Matrix_DrawThaiText(0, 0, "สวัสดี", COLOR_RED);

// เลื่อนข้อความ (Scrolling Text)
Matrix_ScrollText("WELCOME TO CH57x", COLOR_GREEN, 50); // ความเร็ว 50ms
```

---

## API Reference

- `Matrix_Init(port, pin, width, height, wiring)` : เริ่มต้นโมดูล
- `Matrix_SetBrightness(level)` : ตั้งความสว่าง (0-255)
- `Matrix_SetPixel(x, y, r, g, b)` : ระบุสีของพิกเซลรายดวง
- `Matrix_DrawLine(x1, y1, x2, y2, r, g, b)` : วาดเส้นตรง
- `Matrix_DrawRect(x, y, w, h, r, g, b)` : วาดสี่เหลี่ยม
- `Matrix_DrawCircle(x, y, radius, r, g, b)` : วาดวงกลม
- `Matrix_DrawText(x, y, text, color)` : แสดงข้อความ ASCII
- `Matrix_Show()` : ส่งข้อมูลไปแสดงผลที่ Matrix

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)