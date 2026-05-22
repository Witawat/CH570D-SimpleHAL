# TM1637 7-Segment Display Library - คู่มือการใช้งาน

## สารบัญ

1. [บทนำและภาพรวม](#บทนำและภาพรวม)
2. [การต่อวงจรและฮาร์ดแวร์](#การต่อวงจรและฮาร์ดแวร์)
3. [API Reference](#api-reference)
4. [Tutorial: ขั้นพื้นฐาน](#tutorial-ขั้นพื้นฐาน)
5. [Tutorial: ขั้นกลาง](#tutorial-ขั้นกลาง)
6. [Tutorial: ขั้นสูง](#tutorial-ขั้นสูง)
7. [เทคนิคและ Tips](#เทคนิคและ-tips)
8. [Troubleshooting](#troubleshooting)
9. [ตัวอย่างโปรเจค](#ตัวอย่างโปรเจค)

---

## บทนำและภาพรวม

### TM1637 คืออะไร

TM1637 เป็น IC Driver สำหรับควบคุม 7-segment display ที่ใช้การสื่อสารแบบ 2-wire (CLK และ DIO) ทำให้ประหยัด GPIO pins และใช้งานง่าย

### คุณสมบัติหลัก

- ✅ **การสื่อสาร 2-wire**: ใช้เพียง 2 pins (CLK, DIO)
- ✅ **ควบคุมความสว่าง**: 8 ระดับ (0-7)
- ✅ **รองรับหลายรูปแบบ**: 4-digit, 6-digit displays
- ✅ **Built-in Key Scan**: รองรับการอ่านปุ่มกด (ไม่ได้ implement ใน library นี้)
- ✅ **ใช้งานง่าย**: API แบบ Arduino-style

### ข้อจำกัด

- ⚠️ ใช้ bit-banging (ไม่ใช่ hardware I2C/SPI)
- ⚠️ รองรับเฉพาะตัวอักษรบางตัวที่แสดงได้บน 7-segment
- ⚠️ ไม่รองรับ key scanning ใน library นี้

### การใช้งานทั่วไป

- 🕐 นาฬิกาดิจิตอล
- 🌡️ เครื่องวัดอุณหภูมิ
- 🔢 เครื่องนับ/Counter
- ⏱️ Stopwatch/Timer
- 📊 แสดงค่าเซ็นเซอร์

---

## การต่อวงจรและฮาร์ดแวร์

### Pinout ของ TM1637 Module

```
TM1637 Module
┌─────────────┐
│    ┌───┐    │
│    │888│    │  <- 7-Segment Display
│    └───┘    │
│             │
│ CLK  DIO    │
│ VCC  GND    │
└─────────────┘
```

### การต่อสายกับ CH57x

| TM1637 Pin | CH57x Pin | หมายเหตุ |
|------------|--------------|----------|
| CLK        | PC0 (หรือ GPIO อื่น) | Clock signal |
| DIO        | PC1 (หรือ GPIO อื่น) | Data I/O |
| VCC        | 3.3V หรือ 5V | ขึ้นกับ module |
| GND        | GND | Ground |

> **หมายเหตุ**: TM1637 รองรับทั้ง 3.3V และ 5V แต่ควรตรวจสอบ datasheet ของ module ที่ใช้

### Schematic ตัวอย่าง

```
CH57x                    TM1637 Module
┌─────────┐                ┌──────────────┐
│         │                │              │
│ PC0 ────┼────────────────┤ CLK          │
│         │                │              │
│ PC1 ────┼────────────────┤ DIO          │
│         │                │              │
│ 3.3V ───┼────────────────┤ VCC          │
│         │                │              │
│ GND ────┼────────────────┤ GND          │
│         │                │              │
└─────────┘                └──────────────┘
```

### ความต้องการไฟฟ้า

- **แรงดันไฟ**: 3.3V - 5V
- **กระแสไฟ**: ~80mA (ที่ความสว่างสูงสุด, 4 digits)
- **กระแสไฟ idle**: ~5mA

### รูปแบบ 7-Segment ที่รองรับ

Library นี้รองรับ:
- ✅ 4-digit display (ทั่วไป)
- ✅ 6-digit display
- ✅ Display ที่มี colon (สำหรับนาฬิกา)

---

## API Reference

### Initialization Functions

#### `TM1637_Init()`

```c
TM1637_Handle* TM1637_Init(uint8_t clk_pin, uint8_t dio_pin, uint8_t num_digits);
```

**คำอธิบาย**: เริ่มต้นการใช้งาน TM1637

**Parameters**:
- `clk_pin`: หมายเลข GPIO pin สำหรับ CLK
- `dio_pin`: หมายเลข GPIO pin สำหรับ DIO
- `num_digits`: จำนวน digits (4 หรือ 6)

**Returns**: Pointer to TM1637_Handle

**ตัวอย่าง**:
```c
TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
```

---

#### `TM1637_SetBrightness()`

```c
void TM1637_SetBrightness(TM1637_Handle* handle, uint8_t brightness);
```

**คำอธิบาย**: ตั้งค่าความสว่างของ display

**Parameters**:
- `handle`: Pointer to TM1637_Handle
- `brightness`: ระดับความสว่าง (0-7)

**ตัวอย่าง**:
```c
TM1637_SetBrightness(display, 5);  // ความสว่างปานกลาง
```

---

#### `TM1637_DisplayControl()`

```c
void TM1637_DisplayControl(TM1637_Handle* handle, bool on);
```

**คำอธิบาย**: เปิด/ปิด display

**Parameters**:
- `handle`: Pointer to TM1637_Handle
- `on`: `true` = เปิด, `false` = ปิด

**ตัวอย่าง**:
```c
TM1637_DisplayControl(display, true);   // เปิด
TM1637_DisplayControl(display, false);  // ปิด
```

---

#### `TM1637_Clear()`

```c
void TM1637_Clear(TM1637_Handle* handle);
```

**คำอธิบาย**: ล้างหน้าจอทั้งหมด

**ตัวอย่าง**:
```c
TM1637_Clear(display);
```

---

### Basic Display Functions

#### `TM1637_DisplayNumber()`

```c
void TM1637_DisplayNumber(TM1637_Handle* handle, int16_t number, bool leading_zero);
```

**คำอธิบาย**: แสดงตัวเลขจำนวนเต็ม

**Parameters**:
- `number`: ตัวเลข (-999 ถึง 9999 สำหรับ 4-digit)
- `leading_zero`: `true` = แสดง 0 นำหน้า, `false` = ไม่แสดง

**ตัวอย่าง**:
```c
TM1637_DisplayNumber(display, 42, false);    // "  42"
TM1637_DisplayNumber(display, 42, true);     // "0042"
TM1637_DisplayNumber(display, -15, false);   // " -15"
```

---

#### `TM1637_DisplayFloat()`

```c
void TM1637_DisplayFloat(TM1637_Handle* handle, float number, uint8_t decimal_places);
```

**คำอธิบาย**: แสดงตัวเลขทศนิยม

**Parameters**:
- `number`: ตัวเลขทศนิยม
- `decimal_places`: จำนวนตำแหน่งทศนิยม (0-3)

**ตัวอย่าง**:
```c
TM1637_DisplayFloat(display, 12.34, 2);  // "12.34"
TM1637_DisplayFloat(display, 98.6, 1);   // "98.6"
```

---

#### `TM1637_DisplayHex()`

```c
void TM1637_DisplayHex(TM1637_Handle* handle, uint16_t number, bool leading_zero);
```

**คำอธิบาย**: แสดงตัวเลขฐาน 16

**ตัวอย่าง**:
```c
TM1637_DisplayHex(display, 0xABCD, true);  // "ABCD"
TM1637_DisplayHex(display, 0x42, false);   // "  42"
```

---

#### `TM1637_DisplayDigit()`

```c
void TM1637_DisplayDigit(TM1637_Handle* handle, uint8_t position, uint8_t digit, bool show_dp);
```

**คำอธิบาย**: แสดงตัวเลขที่ตำแหน่งเดียว

**Parameters**:
- `position`: ตำแหน่ง (0 = ซ้ายสุด)
- `digit`: ตัวเลข (0-9)
- `show_dp`: แสดงจุดทศนิยมหรือไม่

**ตัวอย่าง**:
```c
TM1637_DisplayDigit(display, 0, 1, false);  // "1" ที่ตำแหน่งแรก
TM1637_DisplayDigit(display, 2, 5, true);   // "5." ที่ตำแหน่งที่ 3
```

---

### Character Display Functions

#### `TM1637_DisplayChar()`

```c
void TM1637_DisplayChar(TM1637_Handle* handle, uint8_t position, char ch, bool show_dp);
```

**คำอธิบาย**: แสดงตัวอักษรหรือสัญลักษณ์

**ตัวอักษรที่รองรับ**: 0-9, A-F, H, L, P, U, -, _, space

**ตัวอย่าง**:
```c
TM1637_DisplayChar(display, 0, 'H', false);
TM1637_DisplayChar(display, 1, 'E', false);
TM1637_DisplayChar(display, 2, 'L', false);
TM1637_DisplayChar(display, 3, 'P', false);  // แสดง "HELP"
```

---

#### `TM1637_DisplayString()`

```c
void TM1637_DisplayString(TM1637_Handle* handle, const char* text, uint8_t start_pos);
```

**คำอธิบาย**: แสดงข้อความ (ไม่เลื่อน)

**ตัวอย่าง**:
```c
TM1637_DisplayString(display, "COOL", 0);  // แสดง "COOL"
```

---

### Advanced Display Functions

#### `TM1637_SetBlink()`

```c
void TM1637_SetBlink(TM1637_Handle* handle, bool enable, uint16_t blink_rate);
```

**คำอธิบาย**: ตั้งค่าการกะพริบ

**Parameters**:
- `enable`: เปิด/ปิดการกะพริบ
- `blink_rate`: อัตราการกะพริบ (ms)

**ตัวอย่าง**:
```c
TM1637_SetBlink(display, true, 500);  // กะพริบทุก 500ms

// ใน main loop
while(1) {
    TM1637_UpdateBlink(display);
    Delay_Ms(10);
}
```

---

#### `TM1637_ScrollText()`

```c
void TM1637_ScrollText(TM1637_Handle* handle, const char* text, uint16_t scroll_delay);
```

**คำอธิบาย**: เลื่อนข้อความ (blocking)

**ตัวอย่าง**:
```c
TM1637_ScrollText(display, "HELLO WORLD", 300);
```

---

#### `TM1637_StartScroll()` / `TM1637_UpdateScroll()`

```c
void TM1637_StartScroll(TM1637_Handle* handle, const char* text, uint16_t scroll_delay);
bool TM1637_UpdateScroll(TM1637_Handle* handle);
```

**คำอธิบาย**: เลื่อนข้อความแบบ non-blocking

**ตัวอย่าง**:
```c
TM1637_StartScroll(display, "HELLO", 300);

while(1) {
    bool scrolling = TM1637_UpdateScroll(display);
    if (!scrolling) {
        // เลื่อนเสร็จแล้ว
        break;
    }
    Delay_Ms(10);
}
```

---

#### `TM1637_DisplayTime()`

```c
void TM1637_DisplayTime(TM1637_Handle* handle, uint8_t hours, uint8_t minutes, bool show_colon);
```

**คำอธิบาย**: แสดงเวลาในรูปแบบ HH:MM

**ตัวอย่าง**:
```c
TM1637_DisplayTime(display, 12, 34, true);  // "12:34"
```

---

## Tutorial: ขั้นพื้นฐาน

### 1. การติดตั้งและ Include Library

```c
#include "SimpleTM1637.h"
#include "SimpleHAL.h"

int main(void) {
    // Initialize system
    SystemCoreClockUpdate();
    Timer_Init();  // ต้องเรียกก่อนใช้ TM1637
    
    // Initialize display
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    
    // Your code here
}
```

### 2. แสดงตัวเลขง่ายๆ

```c
void Example_SimpleNumber(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    // แสดงตัวเลข 1234
    TM1637_DisplayNumber(display, 1234, false);
    
    while(1) {
        // Display stays on
    }
}
```

### 3. นับเลข 0-9999

```c
void Example_Counter(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    for (uint16_t i = 0; i <= 9999; i++) {
        TM1637_DisplayNumber(display, i, false);
        Delay_Ms(100);
    }
}
```

### 4. ควบคุมความสว่าง

```c
void Example_Brightness(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_DisplayNumber(display, 8888, true);
    
    while(1) {
        // Fade in
        for (uint8_t brightness = 0; brightness <= 7; brightness++) {
            TM1637_SetBrightness(display, brightness);
            Delay_Ms(200);
        }
        
        // Fade out
        for (uint8_t brightness = 7; brightness > 0; brightness--) {
            TM1637_SetBrightness(display, brightness);
            Delay_Ms(200);
        }
    }
}
```

### 5. แสดงเวลา

```c
void Example_Clock(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    uint8_t hours = 12;
    uint8_t minutes = 0;
    bool colon = false;
    
    while(1) {
        TM1637_DisplayTime(display, hours, minutes, colon);
        colon = !colon;  // กะพริบ colon
        
        Delay_Ms(500);
        
        // เพิ่มเวลา
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) hours = 0;
        }
    }
}
```

---

## Tutorial: ขั้นกลาง

### 1. แสดงทศนิยม

```c
void Example_Temperature(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    float temperature = 25.5;
    
    while(1) {
        TM1637_DisplayFloat(display, temperature, 1);  // 1 ตำแหน่งทศนิยม
        
        temperature += 0.1;
        if (temperature > 30.0) temperature = 20.0;
        
        Delay_Ms(1000);
    }
}
```

### 2. แสดงเลขฐาน 16

```c
void Example_Hexadecimal(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    for (uint16_t i = 0; i <= 0xFFFF; i++) {
        TM1637_DisplayHex(display, i, true);
        Delay_Ms(50);
    }
}
```

### 3. Custom Characters

```c
void Example_CustomChars(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    // แสดง "HELP"
    TM1637_DisplayChar(display, 0, 'H', false);
    TM1637_DisplayChar(display, 1, 'E', false);
    TM1637_DisplayChar(display, 2, 'L', false);
    TM1637_DisplayChar(display, 3, 'P', false);
    
    Delay_Ms(2000);
    
    // แสดง "COOL"
    TM1637_DisplayString(display, "COOL", 0);
}
```

### 4. Progress Bar

```c
void Example_ProgressBar(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    while(1) {
        // Fill from left to right
        for (uint8_t i = 0; i < 4; i++) {
            TM1637_Clear(display);
            for (uint8_t j = 0; j <= i; j++) {
                TM1637_DisplayRaw(display, j, 0xFF);  // All segments on
            }
            Delay_Ms(500);
        }
        
        Delay_Ms(1000);
        TM1637_Clear(display);
        Delay_Ms(1000);
    }
}
```

### 5. Blinking Effect

```c
void Example_Blink(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    TM1637_DisplayNumber(display, 1234, false);
    TM1637_SetBlink(display, true, 500);  // กะพริบทุก 500ms
    
    while(1) {
        TM1637_UpdateBlink(display);  // ต้องเรียกใน loop
        Delay_Ms(10);
    }
}
```

---

## Tutorial: ขั้นสูง

### 1. Scrolling Text (Non-Blocking)

```c
void Example_ScrollNonBlocking(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    TM1637_StartScroll(display, "HELLO WORLD", 300);
    
    while(1) {
        bool scrolling = TM1637_UpdateScroll(display);
        
        if (!scrolling) {
            // เลื่อนเสร็จแล้ว เริ่มใหม่
            Delay_Ms(1000);
            TM1637_StartScroll(display, "HELLO WORLD", 300);
        }
        
        // ทำงานอื่นๆ ได้ที่นี่
        Delay_Ms(10);
    }
}
```

### 2. Custom Animation

```c
void Example_LoadingAnimation(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    // กำหนด animation frames
    const uint8_t loading_frames[8][TM1637_MAX_DIGITS] = {
        {SEG_A, 0, 0, 0},
        {0, SEG_A, 0, 0},
        {0, 0, SEG_A, 0},
        {0, 0, 0, SEG_A},
        {0, 0, 0, SEG_D},
        {0, 0, SEG_D, 0},
        {0, SEG_D, 0, 0},
        {SEG_D, 0, 0, 0}
    };
    
    while(1) {
        TM1637_PlayAnimation(display, loading_frames, 8, 100, 3);  // เล่น 3 รอบ
        Delay_Ms(1000);
    }
}
```

### 3. State Machine สำหรับ Multi-Mode

```c
typedef enum {
    MODE_CLOCK,
    MODE_TEMP,
    MODE_COUNTER
} DisplayMode_t;

void Example_StateMachine(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    DisplayMode_t mode = MODE_CLOCK;
    uint8_t hours = 12, minutes = 0;
    float temp = 25.5;
    uint16_t counter = 0;
    
    while(1) {
        switch(mode) {
            case MODE_CLOCK:
                TM1637_DisplayTime(display, hours, minutes, true);
                // Update clock logic
                break;
                
            case MODE_TEMP:
                TM1637_DisplayFloat(display, temp, 1);
                // Update temperature
                break;
                
            case MODE_COUNTER:
                TM1637_DisplayNumber(display, counter, false);
                counter++;
                break;
        }
        
        Delay_Ms(100);
        
        // Switch mode with button (example)
        // if (button_pressed) mode = (mode + 1) % 3;
    }
}
```

---

## เทคนิคและ Tips

### 1. การประหยัดพลังงาน

```c
// ลดความสว่างเมื่อไม่ใช้งาน
TM1637_SetBrightness(display, 1);  // ความสว่างต่ำสุด

// ปิด display เมื่อไม่ใช้งาน
TM1637_DisplayControl(display, false);
```

### 2. Debouncing สำหรับ Input

```c
bool button_pressed = false;

void CheckButton(void) {
    bool current_state = (digitalRead(BUTTON_PIN) == LOW);
    
    if (current_state && !button_pressed) {
        button_pressed = true;
        // Handle button press
    } else if (!current_state) {
        button_pressed = false;
    }
}
```

### 3. การจัดการ Overflow

```c
// ใช้ unsigned arithmetic สำหรับ timer
uint32_t start_time = Get_CurrentMs();

// ตรวจสอบ timeout แบบ overflow-safe
if (ELAPSED_TIME(start_time, Get_CurrentMs()) >= 1000) {
    // 1 second passed
}
```

### 4. Performance Optimization

```c
// อัพเดท display เฉพาะเมื่อค่าเปลี่ยน
static int16_t last_value = -1;
int16_t current_value = ReadSensor();

if (current_value != last_value) {
    TM1637_DisplayNumber(display, current_value, false);
    last_value = current_value;
}
```

### 5. การแสดงเลขติดลบ

```c
// Library รองรับเลขติดลบอัตโนมัติ
TM1637_DisplayNumber(display, -42, false);  // แสดง " -42"
```

---

## Troubleshooting

### Display ไม่ติด

**อาการ**: Display ไม่แสดงอะไรเลย

**แก้ไข**:
1. ตรวจสอบการต่อสาย (CLK, DIO, VCC, GND)
2. ตรวจสอบแรงดันไฟ (3.3V หรือ 5V)
3. ตรวจสอบว่าเรียก `Timer_Init()` แล้วหรือยัง
4. ลองเพิ่มความสว่าง: `TM1637_SetBrightness(display, 7);`

```c
// Debug code
TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
TM1637_SetBrightness(display, 7);  // ความสว่างสูงสุด
TM1637_DisplayNumber(display, 8888, true);  // แสดงทุก segment
```

---

### ความสว่างไม่เท่ากัน

**อาการ**: บาง digit สว่างกว่า/มืดกว่า

**สาเหตุ**: 
- แรงดันไฟไม่เสถียร
- Module มีปัญหา

**แก้ไข**:
- ใช้ capacitor 100uF ที่ VCC
- ตรวจสอบสายไฟ

---

### ตัวเลขแสดงผิด

**อาการ**: ตัวเลขไม่ถูกต้อง หรือแสดงแปลกๆ

**สาเหตุ**:
- Timing ไม่ถูกต้อง
- สัญญาณรบกวน

**แก้ไข**:
```c
// ลองปรับ delay ใน TM1637_DelayUs()
// ใน SimpleTM1637.c
static inline void TM1637_DelayUs(void) {
    Delay_Us(10);  // เพิ่มจาก 5 เป็น 10
}
```

---

### Scrolling ไม่ smooth

**อาการ**: ข้อความเลื่อนกระตุก

**แก้ไข**:
```c
// ลด scroll delay
TM1637_StartScroll(display, "HELLO", 200);  // ลดจาก 300 เป็น 200

// หรือใช้ในโหมด non-blocking
while(1) {
    TM1637_UpdateScroll(display);
    Delay_Ms(5);  // Delay น้อยๆ
}
```

---

## ตัวอย่างโปรเจค

### 1. นาฬิกาดิจิตอล

```c
void Project_DigitalClock(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    uint8_t hours = 0, minutes = 0, seconds = 0;
    bool colon = false;
    
    Timer_t second_timer;
    Start_Timer(&second_timer, 1000, 1);
    
    Timer_t colon_timer;
    Start_Timer(&colon_timer, 500, 1);
    
    while(1) {
        if (Is_Timer_Expired(&second_timer)) {
            seconds++;
            if (seconds >= 60) {
                seconds = 0;
                minutes++;
                if (minutes >= 60) {
                    minutes = 0;
                    hours++;
                    if (hours >= 24) hours = 0;
                }
            }
        }
        
        if (Is_Timer_Expired(&colon_timer)) {
            colon = !colon;
        }
        
        TM1637_DisplayTime(display, hours, minutes, colon);
        Delay_Ms(10);
    }
}
```

---

### 2. เครื่องวัดอุณหภูมิ

```c
void Project_Thermometer(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    // Initialize ADC for temperature sensor
    // ADC_SimpleInit(...);
    
    while(1) {
        // Read temperature from sensor
        // float temp = ReadTemperatureSensor();
        float temp = 25.5;  // Example
        
        TM1637_DisplayFloat(display, temp, 1);
        Delay_Ms(1000);
    }
}
```

---

### 3. Stopwatch

```c
void Project_Stopwatch(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    pinMode(PC2, PIN_MODE_INPUT_PULLUP);  // Start/Stop button
    pinMode(PC3, PIN_MODE_INPUT_PULLUP);  // Reset button
    
    bool running = false;
    uint16_t seconds = 0;
    uint8_t centiseconds = 0;
    
    Timer_t stopwatch_timer;
    Start_Timer(&stopwatch_timer, 10, 1);  // 10ms = 1 centisecond
    
    while(1) {
        // Start/Stop button
        if (digitalRead(PC2) == LOW) {
            Delay_Ms(50);  // Debounce
            while(digitalRead(PC2) == LOW);
            running = !running;
        }
        
        // Reset button
        if (digitalRead(PC3) == LOW) {
            Delay_Ms(50);
            while(digitalRead(PC3) == LOW);
            seconds = 0;
            centiseconds = 0;
            running = false;
        }
        
        // Update stopwatch
        if (running && Is_Timer_Expired(&stopwatch_timer)) {
            centiseconds++;
            if (centiseconds >= 100) {
                centiseconds = 0;
                seconds++;
            }
        }
        
        // Display MM:SS
        uint8_t minutes = seconds / 60;
        uint8_t secs = seconds % 60;
        TM1637_DisplayTime(display, minutes, secs, true);
        
        Delay_Ms(10);
    }
}
```

---

### 4. RPM Meter

```c
void Project_RPMMeter(void) {
    TM1637_Handle* display = TM1637_Init(PC0, PC1, 4);
    TM1637_SetBrightness(display, 5);
    
    pinMode(PC2, PIN_MODE_INPUT_PULLUP);  // RPM sensor input
    
    volatile uint16_t pulse_count = 0;
    uint16_t rpm = 0;
    
    // Setup interrupt for pulse counting
    // attachInterrupt(PC2, PulseCounter_ISR, RISING);
    
    Timer_t update_timer;
    Start_Timer(&update_timer, 1000, 1);  // Update every second
    
    while(1) {
        if (Is_Timer_Expired(&update_timer)) {
            rpm = pulse_count * 60;  // Convert to RPM
            pulse_count = 0;
            
            TM1637_DisplayNumber(display, rpm, false);
        }
        
        Delay_Ms(10);
    }
}
```

---

## สรุป

Library TM1637 นี้ให้ความสามารถครบถ้วนตั้งแต่การแสดงผลพื้นฐานไปจนถึงการทำ animation ที่ซับซ้อน โดยมี API ที่ใช้งานง่ายแบบ Arduino-style

**จุดเด่น**:
- ✅ ใช้งานง่าย
- ✅ รองรับทั้ง blocking และ non-blocking
- ✅ มีตัวอย่างครบถ้วน
- ✅ เอกสารภาษาไทยละเอียด

**ข้อควรระวัง**:
- ⚠️ ต้องเรียก `Timer_Init()` ก่อนใช้งาน
- ⚠️ Non-blocking functions ต้องเรียก Update ใน main loop
- ⚠️ ตัวอักษรที่แสดงได้จำกัด

---

**เวอร์ชัน**: 1.0  
**วันที่**: 2025-12-21  
**ผู้พัฒนา**: CH57x Simple HAL Team

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)