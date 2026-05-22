# WS2812 8×8 LED Matrix Library

> **Library สำหรับควบคุม WS2812 RGB LEDs ที่ต่อเป็น Matrix 8×8 (ขยายถึง 32×32) บน CH57x series**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Wiring Patterns](#wiring-patterns)
4. [Hardware Setup](#hardware-setup)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [คำแนะนำในการซื้อ](#คำแนะนำในการซื้อ)
8. [CH57x series compatibility](#CH57x-vs-CH57x)
9. [Resource Conflicts](#resource-conflicts)
10. [Troubleshooting](#troubleshooting)
11. [API Reference](#api-reference)

---

## ภาพรวม

WS2812 (NeoPixel) เป็น RGB LED ที่มี IC ควบคุมในตัว สามารถต่อแบบ Daisy Chain ได้ไม่จำกัด
เมื่อนำมาต่อเป็น Matrix 8×8 จะได้จอแสดงผล LED ราคาถูก ควบคุมด้วยพิน GPIO เส้นเดียว

### ความแตกต่างระหว่างรุ่น

| รุ่น | แรงดัน | คุณสมบัติเด่น |
|-----|--------|--------------|
| **WS2812B** | 5V | รุ่นมาตรฐาน, ราคาถูกที่สุด |
| **WS2813** | 5V | มี backup data line, ตัวหนึ่งเสียตัวอื่นยังทำงานได้ |
| **WS2815** | 12V | แรงดันสูง, ใช้ในที่ต้องเดินสายยาว |
| **SK6812** | 5V | คล้าย WS2812B, บางรุ่นมี white LED เพิ่ม |
| **APA106** | 5V | รูปทรง through-hole (5mm หรือ 8mm) |

### เปรียบเทียบกับ P10 Display

| ด้าน | WS2812 8×8 Matrix | P10 Panel |
|------|-------------------|-----------|
| พินที่ใช้ | 1 pin (data) | 7-10 pins |
| ราคา | 100-300 บาท | 150-900 บาท |
| ความละเอียด | 8×8 (ปรับได้) | 32×16 (standard) |
| สี | RGB 16 ล้านสี | สีเดี่ยว/สองสี/RGB (8 สี) |
| การใช้งาน | แสดงผลเล็ก, งาน DIY | ป้ายโฆษณาขนาดใหญ่ |

---

## คุณสมบัติ

### คุณสมบัติของ Library

- ✅ รองรับ Matrix ขนาด 8×8 (ขยายได้ถึง 32×32)
- ✅ 2 รูปแบบการต่อสาย: Zigzag (ซิกแซก) และ Snake (งูเลื้อย)
- ✅ ระบบพิกัด (x, y) ที่ใช้งานง่าย
- ✅ ฟังก์ชันวาดรูปพื้นฐาน (เส้น, สี่เหลี่ยม, วงกลม)
- ✅ Brightness control (0-255)
- ✅ ใช้ SimpleGPIO pins (PC4, PD2, ...)
- ✅ Instance struct pattern (แบบ P10/PIR/HCSR04)
- ✅ Non-blocking operation (ไม่ใช้ delay ภายใน)
- ✅ รองรับ WS2812B, WS2813, WS2815, SK6812
- ✅ รองรับ CH57x series
- ✅ เอกสารภาษาไทยครบถ้วน

### ข้อมูลทางเทคนิค

- **ความละเอียดสูงสุด**: 32×32 พิกเซล (ปรับได้)
- **Frame Buffer Size** (8×8): 192 bytes (3 bytes/LED × 64 LEDs)
- **Frame Buffer Size** (16×16): 768 bytes
- **Refresh Rate**: ขึ้นอยู่กับจำนวน LEDs (~400Hz สำหรับ 64 LEDs ที่ 8 MHz data rate)
- **Data Pin**: 1 pin (GPIO ใดก็ได้)
- **Power Supply**: 5V (สำหรับ WS2812B)
- **Current**: ~60mA ต่อ LED (full white) — 8×8 = ~5A ที่ full brightness!

---

## Wiring Patterns

### Zigzag (ซิกแซกเริ่มจากซ้าย)

รูปแบบที่พบมากที่สุดใน 8×8 Matrix modules ที่ขายทั่วไป

```
LED Index Layout:

Row 0:   0→  1→  2→  3→  4→  5→  6→  7
Row 1:  15← 14← 13← 12← 11← 10←  9←  8
Row 2:  16→ 17→ 18→ 19→ 20→ 21→ 22→ 23
Row 3:  31← 30← 29← 28← 27← 26← 25← 24
Row 4:  32→ 33→ 34→ 35→ 36→ 37→ 38→ 39
Row 5:  47← 46← 45← 44← 43← 42← 41← 40
Row 6:  48→ 49→ 50→ 51→ 52→ 53→ 54→ 55
Row 7:  63← 62← 61← 60← 59← 58← 57← 56

DIN → LED 0 → 1 → 2 → ... → 63 → DOUT
```

**วิธีการต่อ:** LED 0 คือมุมล่างซ้ายหรือบนซ้าย (ขึ้นกับรุ่น) ดูเครื่องหมายลูกศรบน PCB

### Snake (งูเลื้อยต่อเนื่อง)

ทุกแถววิ่งจากซ้ายไปขวา — ใช้กับ Matrix ที่ต่อสายแบบเรียงแถว

```
LED Index Layout:

Row 0:   0→  1→  2→  3→  4→  5→  6→  7
Row 1:   8→  9→ 10→ 11→ 12→ 13→ 14→ 15
Row 2:  16→ 17→ 18→ 19→ 20→ 21→ 22→ 23
Row 3:  24→ 25→ 26→ 27→ 28→ 29→ 30→ 31
Row 4:  32→ 33→ 34→ 35→ 36→ 37→ 38→ 39
Row 5:  40→ 41→ 42→ 43→ 44→ 45→ 46→ 47
Row 6:  48→ 49→ 50→ 51→ 52→ 53→ 54→ 55
Row 7:  56→ 57→ 58→ 59→ 60→ 61→ 62→ 63

DIN → LED 0 → 1 → 2 → ... → 63 → DOUT
```

### วิธีตรวจสอบ Wiring Pattern ของ Matrix ที่ซื้อมา

1. **เรียกใช้ฟังก์ชันทดสอบ**: สั่งให้ LED ดวงแรก (index 0) ติด แล้วดูว่าติดตรงมุมไหน
2. **วาดเส้นทแยงมุม**: ใช้ `WS2812M_DrawLine` จาก (0,0) ไป (7,7) ถ้าเส้นไม่ตรง แสดงว่า pattern ไม่ถูกต้อง
3. **ดู PCB**: ผู้ผลิตส่วนใหญ่พิมพ์หมายเลข LED หรือลูกศรไว้บน PCB

---

## Hardware Setup

### การต่อวงจรพื้นฐาน

```
WS2812 8x8 Matrix       CH57x
  DIN  -------------->  GPIO Pin (PC4)
  VCC  -------------->  5V Power Supply
  GND  -------------->  GND

Power Supply:
  5V 10A (สำหรับ Matrix 8×8 ที่ full brightness)
```

> **⚠️ ข้อควรระวัง:** WS2812 แต่ละดวงกินไฟ ~60mA เมื่อแสดงสีขาวเต็มที่
> Matrix 8×8 (64 LEDs) = 3.8A! ควรใช้ Power Supply 5V 5-10A
> **ห้ามจ่ายไฟผ่านพิน 5V ของ CH57x** — ใช้ external PSU แยก

### Level Shifter (จำเป็นสำหรับ CH57x)

CH57x ใช้ logic 3.3V แต่ WS2812 ต้องการสัญญาณ > 0.7×VDD (~3.5V ที่ 5V)
**จำเป็นต้องมี Level Shifter** เพื่อให้สัญญาณ Data ถึง 5V:

```
วิธีที่ 1: 74HC245 (แนะนำ)
  CH57x (3.3V)        74HC245            WS2812 Matrix
  PC4 (output)  -------> A1 ──→ B1 -------> DIN (5V)
  VCC (3.3V)    -------> VCC_A             VCC_B -----> 5V
  GND           -------> GND               GND   -----> GND
  DIR = HIGH (tied to VCC_A)

วิธีที่ 2: Voltage Divider (สำหรับ 1-way data)
  CH57x PC4 ──[330Ω]──+──[470Ω]── GND
                         │
                        DIN ของ WS2812

วิธีที่ 3: Diode + Pull-up (ง่ายสุด)
  CH57x PC4 ────┬─── DIN
                   │
                  [1kΩ] ─── 5V
```

> **แนะนำ:** ใช้ 74HC245 หรือ TXB0108 level shifter module (30-80 บาท)
> มีขายพร้อมสาย jumper — สะดวกและเชื่อถือได้

### การต่อ PSU

```
External 5V PSU:
  [+] ──────────────── WS2812 Matrix VCC
  [-] ──────────────── WS2812 Matrix GND
                      │
                      +──── CH57x GND (ต่อ GND ร่วม)

⚠️ อย่าลืมต่อ GND ร่วมระหว่าง PSU และ CH57x!
```

---

## การใช้งานพื้นฐาน

### ตัวอย่าง 1: 8×8 Zigzag — จุดสีแดง

```c
#include <main.h>
#include "Lib/WS2812Matrix/WS2812Matrix.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    WS2812M_Instance matrix;

    // เริ่มต้น Matrix 8×8 แบบ Zigzag บน PC4
    WS2812M_Init(&matrix, PC4, 8, 8, WIRING_ZIGZAG);

    // จุดกลางสีแดง
    WS2812M_SetPixel(&matrix, 3, 3, 255, 0, 0);
    // จุดบนซ้ายสีเขียว
    WS2812M_SetPixel(&matrix, 0, 0, 0, 255, 0);
    // จุดล่างขวาสีน้ำเงิน
    WS2812M_SetPixel(&matrix, 7, 7, 0, 0, 255);

    WS2812M_Show(&matrix);

    while (1);
}
```

### ตัวอย่าง 2: วาดรูปทรงเรขาคณิต

```c
#include <main.h>
#include "Lib/WS2812Matrix/WS2812Matrix.h"

WS2812M_Instance matrix;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    WS2812M_Init(&matrix, PC4, 8, 8, WIRING_ZIGZAG);

    // เส้นทแยงมุมสีแดง
    WS2812M_DrawLine(&matrix, 0, 0, 7, 7, 255, 0, 0);

    // สี่เหลี่ยมสีเขียว (ขอบ)
    WS2812M_DrawRect(&matrix, 1, 1, 6, 6, 0, 255, 0);

    // วงกลมสีน้ำเงิน (ขอบ)
    WS2812M_DrawCircle(&matrix, 4, 4, 3, 0, 0, 255);

    WS2812M_Show(&matrix);

    while (1);
}
```

### ตัวอย่าง 3: วนสี Rainbow อย่างง่าย

```c
#include <main.h>
#include "Lib/WS2812Matrix/WS2812Matrix.h"

WS2812M_Instance matrix;

uint32_t wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return NeoPixel_Color(255 - pos * 3, 0, pos * 3);
    }
    if (pos < 170) {
        pos -= 85;
        return NeoPixel_Color(0, pos * 3, 255 - pos * 3);
    }
    pos -= 170;
    return NeoPixel_Color(pos * 3, 255 - pos * 3, 0);
}

int main(void) {
    uint8_t hue = 0;

    SystemCoreClockUpdate();
    Timer_Init();

    WS2812M_Init(&matrix, PC4, 8, 8, WIRING_ZIGZAG);

    while (1) {
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
                uint32_t c = wheel((x + y + hue) & 0xFF);
                WS2812M_SetPixelColor(&matrix, x, y, c);
            }
        }
        WS2812M_Show(&matrix);
        Delay_Ms(50);
        hue++;
    }
}
```

### ตัวอย่าง 4: Snake — เลื่อนจุด

```c
#include <main.h>
#include "Lib/WS2812Matrix/WS2812Matrix.h"

WS2812M_Instance matrix;

int main(void) {
    int16_t x = 0, y = 0;
    int8_t dx = 1, dy = 0;

    SystemCoreClockUpdate();
    Timer_Init();

    WS2812M_Init(&matrix, PC4, 8, 8, WIRING_SNAKE);

    WS2812M_Clear(&matrix);

    while (1) {
        // ลบจุดเก่า
        WS2812M_SetPixel(&matrix, x, y, 0, 0, 0);

        // อัปเดตตำแหน่ง
        x += dx;
        if (x >= 8) { x = 7; dx = 0; dy = 1; }
        if (y >= 8) { y = 7; dy = 0; dx = -1; }
        if (x < 0)  { x = 0; dx = 0; dy = -1; }
        if (y < 0)  { y = 0; dx = 1; dy = 0; }

        // วาดจุดใหม่
        WS2812M_SetPixel(&matrix, x, y, 255, 0, 0);
        WS2812M_Show(&matrix);
        Delay_Ms(100);
    }
}
```

---

## การใช้งานขั้นสูง

### การปรับความสว่าง

```c
WS2812M_SetBrightness(&matrix, 200);  // 0-255, default 255
WS2812M_Show(&matrix);                 // brightness มีผลตอน Show
```

ค่าแนะนำ:
- **255**: สว่างเต็มที่ (กินไฟสูงสุด)
- **100**: สว่างปานกลาง
- **30**: สลัว (ประหยัดไฟ)

### การใช้ Matrix ขนาดใหญ่

```c
// 16×8 Matrix
WS2812M_Init(&matrix, PC4, 16, 8, WIRING_ZIGZAG);

// 16×16 Matrix (ใช้ RAM 768 bytes — 37.5% ของ CH57x)
WS2812M_Init(&matrix, PC4, 16, 16, WIRING_SNAKE);
```

### การเปลี่ยน Wiring Pattern หลัง Initialize

```c
// สร้าง instance ใหม่ — change wiring by re-init
WS2812M_Init(&matrix, PC4, 8, 8, WIRING_SNAKE);
```

> **หมายเหตุ:** ต้อง Clear และ SetPixel ใหม่ทุกครั้งหลังจากเปลี่ยน pattern

---

## คำแนะนำในการซื้อ

### คำค้นหา

| ตลาด | คำค้นหา |
|------|---------|
| Shopee/Lazada | `WS2812 8x8 matrix`, `NeoPixel 8x8`, `LED matrix 8x8 RGB` |
| AliExpress | `WS2812B 8x8 RGB LED matrix`, `NeoPixel 64 LED panel` |
| Taobao | `WS2812 8x8 点阵`, `RGB LED 矩阵 8x8`, `NeoPixel 柔性屏 8x8` |

### ผู้ขายแนะนำ

| ร้าน | จุดเด่น |
|------|---------|
| **HiLetGo** (AliExpress) | WS2812 8×8 matrix คุณภาพดี, ราคาถูก |
| **MakerFocus** (AliExpress) | มีทั้ง 8×8 และ 16×16, ขายพร้อม PSU |
| **淘宝 DIY 电子** (Taobao) | WS2812B 8×8 แบบแยก module, ถูกมาก |
| **Banggood** | WS2812 matrix + accessories |

### ราคาประมาณ

| อุปกรณ์ | ราคา |
|---------|-------|
| WS2812B 8×8 Matrix (64 LEDs) | 100-300 บาท |
| WS2812B 16×16 Matrix (256 LEDs) | 300-800 บาท |
| Level Shifter 74HC245 Module | 30-80 บาท |
| Power Supply 5V 10A | 200-400 บาท |
| Jumper Wires | 20-50 บาท |

---

## CH57x series compatibility

| ด้าน | CH57x series |
|------|----------|-----------|
| Max Matrix | 8×8 (64 LEDs, 192B buffer) | 32×32 (1024 LEDs) |
| RAM buffer | 9.4% (192B) | < 3% |
| Data pin | 1 pin | 1 pin |
| Level shifter | จำเป็น (3.3V→5V) | อาจจำเป็น |

> CH57x RAM 2KB → 8×8 matrix กิน 192 bytes (9.4%)
> 16×16 (768 bytes = 37.5%) อาจพอ แต่ต้องระวังไลบรารีอื่น

---

## Resource Conflicts

| Resource | WS2812Matrix | ข้อควรระวัง |
|----------|-------------|-------------|
| GPIO pin | 1 pin (data) | ยืดหยุ่น ใช้ pin ใดก็ได้ |
| NeoPixel | dependency | ใช้ NeoPixel buffer ร่วมกับ lib อื่นไม่ได้ |
| Timer | ไม่ใช้ | (NeoPixel ใช้ __disable_irq() ระหว่างส่งข้อมูล) |

---

## Troubleshooting

### 🔴 Matrix ไม่ติด

1. **ตรวจสอบไฟเลี้ยง**: WS2812 ต้องการ 5V — ต่อ external PSU หรือไม่
2. **Level Shifter**: CH57x 3.3V → WS2812 5V ต้องมี level shifter
3. **GND ร่วม**: ต้องต่อ GND ของ CH57x และ PSU เข้าด้วยกัน
4. **ปิด Interrupt**: NeoPixel ใช้ `__disable_irq()` — ห้ามมี ISR ที่ใช้เวลานาน
5. **LED แรกไม่ติด**: ตรวจสอบการต่อ DIN เข้าหรือยัง (บางรุ่น DOUT กับ DIN สลับกัน)

### 🔴 สีไม่ตรง

1. **RGB Order**: WS2812 บางรุ่นเป็น GRB (เขียว-แดง-น้ำเงิน) — ปกติ NeoPixel จัดการให้
2. **Brightness ลดลง**: ถ้า PSU ไม่พอ แรงดันตก สีจะเปลี่ยน

### 🔴 ไฟกระพริบ

1. **PSU ไม่พอ**: ใช้ PSU 5V 5A+ หรือลด brightness
2. **สาย Data ยาวเกิน**: ใช้ level shifter + สายสั้น (< 30cm)
3. **GND loop**: ต่อ GND ร่วมเท่านั้น อย่าต่อหลายจุด

### 🔴 RAM/Flash เต็ม

1. **ลด Matrix Size**: ใช้ 8×8 แทน 16×16
2. **ปิด printf**: `DISABLE_PRINTF PRINTF_OFF` ใน `ch32v00x_conf.h`

---

## API Reference

### Enumerations

```c
typedef enum {
    WIRING_ZIGZAG = 0,  // ซิกแซกเริ่มจากซ้าย (default)
    WIRING_SNAKE  = 1   // งูเลื้อยแบบต่อเนื่อง
} WS2812M_Wiring;
```

### Configuration Macros

```c
#ifndef WS2812M_MAX_WIDTH
#define WS2812M_MAX_WIDTH   32   // ความกว้างสูงสุด
#endif

#ifndef WS2812M_MAX_HEIGHT
#define WS2812M_MAX_HEIGHT  32   // ความสูงสูงสุด
#endif

#ifndef WS2812M_MAX_INSTANCES
#define WS2812M_MAX_INSTANCES  1  // จำนวน instances
#endif
```

### WS2812M_Instance Structure

```c
typedef struct {
    uint8_t          data_pin;     // SimpleGPIO data pin
    uint8_t          width;        // ความกว้าง (px)
    uint8_t          height;       // ความสูง (px)
    uint16_t         num_pixels;   // จำนวน LEDs ทั้งหมด
    WS2812M_Wiring   wiring;       // รูปแบบการต่อสาย
    uint8_t          initialized;  // Initialized flag
} WS2812M_Instance;
```

### Functions

---

#### `WS2812M_Init`

```c
uint8_t WS2812M_Init(WS2812M_Instance* inst, uint8_t data_pin,
                     uint8_t width, uint8_t height, WS2812M_Wiring wiring);
```

เริ่มต้น WS2812 LED Matrix

| พารามิเตอร์ | คำอธิบาย |
|------------|----------|
| `inst` | ตัวชี้ไปยัง WS2812M_Instance |
| `data_pin` | SimpleGPIO data pin (PC4, PD2, ...) |
| `width` | ความกว้างของ matrix (px) |
| `height` | ความสูงของ matrix (px) |
| `wiring` | รูปแบบการต่อสาย (WIRING_ZIGZAG / WIRING_SNAKE) |

**คืนค่า:** `1` = สำเร็จ, `0` = ล้มเหลว

---

#### `WS2812M_SetPixel`

```c
void WS2812M_SetPixel(WS2812M_Instance* inst, uint8_t x, uint8_t y,
                      uint8_t r, uint8_t g, uint8_t b);
```

ตั้งค่าสีของพิกเซลที่ตำแหน่ง (x, y)

| พารามิเตอร์ | คำอธิบาย |
|------------|----------|
| `x` | พิกัด X (0 = ซ้ายสุด) |
| `y` | พิกัด Y (0 = บนสุด) |
| `r` | Red (0-255) |
| `g` | Green (0-255) |
| `b` | Blue (0-255) |

> ต้องเรียก `WS2812M_Show()` เพื่ออัพเดทการแสดงผล

---

#### `WS2812M_SetPixelColor`

```c
void WS2812M_SetPixelColor(WS2812M_Instance* inst, uint8_t x, uint8_t y,
                           uint32_t color);
```

ตั้งค่าสีด้วย 32-bit color (`0xRRGGBB`)

---

#### `WS2812M_GetPixel`

```c
uint32_t WS2812M_GetPixel(WS2812M_Instance* inst, uint8_t x, uint8_t y);
```

อ่านค่าสีของพิกเซล คืนค่า `0xRRGGBB` หรือ `0` ถ้า error

---

#### `WS2812M_Clear`

```c
void WS2812M_Clear(WS2812M_Instance* inst);
```

ลบ Matrix ทั้งหมด (ดับทุกดวง)

---

#### `WS2812M_Fill`

```c
void WS2812M_Fill(WS2812M_Instance* inst, uint8_t r, uint8_t g, uint8_t b);
```

เติมสีทั้ง Matrix

---

#### `WS2812M_Show`

```c
void WS2812M_Show(WS2812M_Instance* inst);
```

อัพเดทการแสดงผล ต้องเรียกหลังจาก SetPixel/Clear/Fill

---

#### `WS2812M_SetBrightness`

```c
void WS2812M_SetBrightness(WS2812M_Instance* inst, uint8_t brightness);
```

ตั้งค่าความสว่าง (0=ดับ, 255=สว่างสุด)

---

#### `WS2812M_Deinit`

```c
void WS2812M_Deinit(WS2812M_Instance* inst);
```

หยุดการทำงานของ Matrix

---

#### `WS2812M_XYtoIndex`

```c
uint16_t WS2812M_XYtoIndex(uint8_t x, uint8_t y, uint8_t width,
                           WS2812M_Wiring wiring);
```

แปลงพิกัด (x, y) เป็น LED index ตาม wiring pattern

---

#### Drawing Primitives

```c
void WS2812M_DrawLine(inst, x0, y0, x1, y1, r, g, b);
void WS2812M_DrawLineColor(inst, x0, y0, x1, y1, color32);
void WS2812M_DrawRect(inst, x, y, w, h, r, g, b);
void WS2812M_FillRect(inst, x, y, w, h, r, g, b);
void WS2812M_DrawCircle(inst, x0, y0, radius, r, g, b);
void WS2812M_FillCircle(inst, x0, y0, radius, r, g, b);
```

---

## ใบอนุญาต

MIT License — CH57x Library Team

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)