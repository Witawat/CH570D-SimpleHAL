# TM1650 — I2C 7-Segment Display + Key Scan Driver

Library สำหรับควบคุม 4-digit 7-segment display และอ่านปุ่มกดผ่าน IC TM1650 แบบ I2C
สำหรับ CH57x โดยเฉพาะ

---

## คุณสมบัติ

- ✅ **I2C Communication** — ใช้ฮาร์ดแวร์ I2C ของ CH57x (SCL=PC2, SDA=PC1)
- ✅ **4-Digit 7/8-Segment Display** — ควบคุมแต่ละ segment ได้อิสระ
- ✅ **ความสว่าง 8 ระดับ** — หรี่-สว่าง ปรับได้ 0-7
- ✅ **แสดงตัวเลข** — จำนวนเต็ม, เลขฐาน 16, ตัวเลขหลักเดียว
- ✅ **Raw Segment Control** — ควบคุม segment โดยตรงสำหรับตัวอักษรพิเศษ
- ✅ **Key Scan** — อ่านปุ่มกดผ่าน I2C (สูงสุด 8 ปุ่ม)
- ✅ **Debounce** — กันสัญญาณกวน ปรับค่า debounce time ได้
- ✅ **Non-blocking** — ไมโครคอนโทรลเลอร์ไม่หยุดรอ
- ✅ **Device Probe** — ตรวจสอบการเชื่อมต่อก่อนใช้งาน

---

## การต่อวงจร

### CH57x → TM1650 Module (JY-MCU)

```
   CH57x              TM1650 Module
   ─────────             ─────────────
   SCL (PC2)  ──────>    SCL
   SDA (PC1)  <────>     SDA
   3.3V       ──────>    VCC
   GND        ──────>    GND
```

> **สำคัญ:** ต้องต่อ Pull-up Resistor 4.7kΩ ที่ SDA และ SCL

### TM1650 Module Pinout

```
   ┌──────────────────────┐
   │  [8.8.:8.8.]         │  ← 4-digit 7-segment
   │                      │
   │  S1  S2  S3  S4      │  ← ปุ่มกด (บางรุ่น)
   │                      │
   │  VCC GND SCL SDA     │  ← ขั้วต่อ I2C
   └──────────────────────┘
```

---

## I2C Address

TM1650 มี I2C address ที่เลือกได้จากขา ADDR:

| ADDR Pin | Address | Define Constant |
|---|---|---|
| GND | `0x24` | `TM1650_I2C_ADDR_DEFAULT` |
| VCC | `0x34` | `TM1650_I2C_ADDR_VCC` |
| SCL | `0x35` | `TM1650_I2C_ADDR_SCL` |
| SDA | `0x36` | `TM1650_I2C_ADDR_SDA` |
| Floating (NC) | `0x37` | `TM1650_I2C_ADDR_FLOAT` |

> JY-MCU module ส่วนใหญ่ default = 0x24 (ADDR ต่อลง GND)

---

## API Reference

### Initialization

```c
TM1650_Handle disp;

// เริ่มต้น TM1650 (ต้องเรียกหลัง I2C_SimpleInit)
if (TM1650_Init(&disp, 0x24) != TM1650_OK) {
    // ไม่พบ device — ตรวจสอบการต่อวงจร
}
```

| Function | Description |
|---|---|
| `TM1650_Init(handle, i2c_addr)` | เริ่มต้นการใช้งาน, probe device |
| `TM1650_SetBrightness(handle, 0-7)` | ตั้งค่าความสว่าง |
| `TM1650_DisplayOn(handle)` | เปิด display |
| `TM1650_DisplayOff(handle)` | ปิด display |
| `TM1650_Clear(handle)` | ล้างหน้าจอ |
| `TM1650_SetMode(handle, mode)` | เลือก 8-seg หรือ 7-seg mode |

### Display Data

| Function | Description |
|---|---|
| `TM1650_DisplayNumber(handle, number, leading_zero)` | แสดงตัวเลขจำนวนเต็ม |
| `TM1650_DisplayHex(handle, number, leading_zero)` | แสดงเลขฐาน 16 |
| `TM1650_DisplayDigit(handle, pos, digit, show_dp)` | แสดงตัวเลข 1 หลัก |
| `TM1650_DisplayRaw(handle, pos, segments)` | ควบคุม segment โดยตรง |
| `TM1650_Update(handle)` | ส่ง buffer ไปยัง display |

### Key Scan

| Function | Description |
|---|---|
| `TM1650_GetKeys(handle)` | อ่านปุ่มกด (debounced, non-blocking) |
| `TM1650_GetRawKeys(handle)` | อ่านค่าปุ่มแบบ raw |
| `TM1650_IsKeyPressed(handle)` | ตรวจสอบว่ามีปุ่มกดหรือไม่ |
| `TM1650_WaitKey(handle)` | รอจนกว่ามีปุ่มกด (blocking) |

### Utility

| Function | Description |
|---|---|
| `TM1650_DigitToSegment(digit)` | แปลงตัวเลขเป็น segment pattern |

---

## ตัวอย่างการใช้งาน

### ตัวอย่างที่ 1: แสดงตัวเลข

```c
#include "SimpleHAL.h"
#include "TM1650.h"

int main(void) {
    SystemCoreClockUpdate();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    TM1650_Handle disp;
    if (TM1650_Init(&disp, 0x24) != TM1650_OK) {
        // Error: device not found
        while(1);
    }

    TM1650_SetBrightness(&disp, 5);
    TM1650_DisplayNumber(&disp, 1234, false);

    while(1);
}
```

### ตัวอย่างที่ 2: นับเลข 0-9999

```c
#include "SimpleHAL.h"
#include "TM1650.h"

int main(void) {
    SystemCoreClockUpdate();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    TM1650_Handle disp;
    TM1650_Init(&disp, 0x24);
    TM1650_SetBrightness(&disp, 4);

    int16_t count = 0;
    while(1) {
        TM1650_DisplayNumber(&disp, count, true);
        Delay_Ms(100);
        count++;
        if (count > 9999) count = 0;
    }
}
```

### ตัวอย่างที่ 3: อ่านปุ่มกด

```c
#include "SimpleHAL.h"
#include "TM1650.h"

int main(void) {
    SystemCoreClockUpdate();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    TM1650_Handle disp;
    TM1650_Init(&disp, 0x24);
    TM1650_DisplayNumber(&disp, 0, true);

    while(1) {
        uint8_t keys = TM1650_GetKeys(&disp);

        if (keys & TM1650_KEY_S1) {
            TM1650_DisplayDigit(&disp, 0, 1, false);
        }
        if (keys & TM1650_KEY_S2) {
            TM1650_DisplayDigit(&disp, 0, 2, false);
        }
        if (keys & TM1650_KEY_S3) {
            TM1650_DisplayDigit(&disp, 0, 3, false);
        }

        Delay_Ms(20);  // loop delay
    }
}
```

### ตัวอย่างที่ 4: Display + Key Counter

```c
#include "SimpleHAL.h"
#include "TM1650.h"

int main(void) {
    SystemCoreClockUpdate();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    TM1650_Handle disp;
    TM1650_Init(&disp, 0x24);
    TM1650_SetBrightness(&disp, 6);

    int16_t value = 0;
    TM1650_DisplayNumber(&disp, value, true);

    while(1) {
        uint8_t keys = TM1650_GetKeys(&disp);

        if (keys & TM1650_KEY_S1) {  // S1 = increment
            value++;
            if (value > 9999) value = 0;
            TM1650_DisplayNumber(&disp, value, true);
        }
        if (keys & TM1650_KEY_S5) {  // S5 = decrement
            value--;
            if (value < 0) value = 9999;
            TM1650_DisplayNumber(&disp, value, true);
        }

        Delay_Ms(20);
    }
}
```

### ตัวอย่างที่ 5: แสดงตัวอักษรด้วย Raw Segment

```c
#include "SimpleHAL.h"
#include "TM1650.h"

int main(void) {
    SystemCoreClockUpdate();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    TM1650_Handle disp;
    TM1650_Init(&disp, 0x24);

    // แสดง "HELP"
    // H: SEG_B|SEG_C|SEG_E|SEG_F|SEG_G
    TM1650_DisplayRaw(&disp, 0, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G);

    // E: SEG_A|SEG_D|SEG_E|SEG_F|SEG_G
    TM1650_DisplayRaw(&disp, 1, SEG_A | SEG_D | SEG_E | SEG_F | SEG_G);

    // L: SEG_D|SEG_E|SEG_F
    TM1650_DisplayRaw(&disp, 2, SEG_D | SEG_E | SEG_F);

    // P: SEG_A|SEG_B|SEG_E|SEG_F|SEG_G
    TM1650_DisplayRaw(&disp, 3, SEG_A | SEG_B | SEG_E | SEG_F | SEG_G);

    while(1);
}
```

---

## การปรับแต่งค่า Config

สามารถ override config macros ก่อน `#include "TM1650.h"`:

```c
#define TM1650_KEY_DEBOUNCE_MS   50   // เพิ่ม debounce time
#define TM1650_KEY_LONG_PRESS_MS 1000 // เพิ่มเวลา long-press
#include "TM1650.h"
```

| Macro | Default | Description |
|---|---|---|
| `TM1650_I2C_ADDR_DEFAULT` | `0x24` | I2C address เริ่มต้น |
| `TM1650_NUM_DIGITS` | `4` | จำนวน digit (ปกติ = 4) |
| `TM1650_KEY_DEBOUNCE_MS` | `30` | เวลา debounce ปุ่ม (ms) |
| `TM1650_KEY_LONG_PRESS_MS` | `800` | เวลากดค้าง (ms) |

---

## หมายเหตุ

- **ต้องเรียก `I2C_SimpleInit()` ก่อน** `TM1650_Init()` ทุกครั้ง
- TM1650 ใช้ I2C address แบบ 7-bit (ไม่ต้อง shift)
- การเปลี่ยน display mode (8-seg ↔ 7-seg) จะล้าง buffer ทั้งหมด
- Key scan อ่านค่าผ่าน I2C Read โดยตรง — ไม่ต้องต่อ GPIO เพิ่ม
- `TM1650_GetKeys()` คืนค่าปุ่มเพียงครั้งเดียวต่อการกด (ไม่ repeat)
- รองรับการกดหลายปุ่มพร้อมกัน (multi-key)

---

## Version History

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-05-15 | Initial release: display control, key scan, debounce |

---

@author CH57x Library Team
@copyright MIT License

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)