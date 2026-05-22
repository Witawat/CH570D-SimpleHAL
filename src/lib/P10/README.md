# P10 LED Matrix Display Library

> **Library สำหรับขับ P10 LED Matrix Panels (สีเดี่ยว / สองสี / RGB) บน CH57x series**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [📦 อุปกรณ์ที่รองรับและคำแนะนำในการซื้อ](#-อุปกรณ์ที่รองรับและคำแนะนำในการซื้อ)
3. [คุณสมบัติ](#คุณสมบัติ)
4. [หลักการทำงานของ P10 Panel](#หลักการทำงานของ-p10-panel)
5. [Hardware Setup](#hardware-setup)
6. [การเลือก Color Mode](#การเลือก-color-mode)
7. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
8. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
9. [CH57x series compatibility](#CH57x-vs-CH57x)
10. [Resource Conflicts](#resource-conflicts)
11. [Troubleshooting](#troubleshooting)
12. [API Reference](#api-reference)

---

## ภาพรวม

P10 เป็น LED matrix display ขนาดใหญ่ที่นิยมใช้ทำป้ายโฆษณา ป้ายข้อความ และ signage กลางแจ้ง แต่ละ panel มีความละเอียด 32×16 หรือ 16×16 พิกเซล ใช้หลักการ multiplexing (row scanning) เพื่อแสดงผล

### ประเภทของ P10 Panel

| ประเภท | Data Pins | สีที่แสดงได้ | การใช้งาน |
|--------|-----------|-------------|-----------|
| สีเดี่ยว (Single Color) | 1 (R) | แดง/เขียว/เหลือง/ขาว | ป้ายข้อความ, เวลาประหยัด |
| สองสี (Dual Color) | 2 (R+G) | แดง + เขียว + ผสมเหลือง | ป้ายข้อมูล, ราคาหุ้น |
| RGB Full Color | 3 (R+G+B) | 8 สี (On/Off) | ป้ายสี, ภาพกราฟิก |

> **หมายเหตุ:** Library นี้รองรับการแสดงผลแบบ On/Off (1-bit ต่อสี) เท่านั้น ไม่มี grayscale
> สำหรับ grayscale (PWM) รอใน Version ถัดไป

### CH57x series compatibility Compatibility

| Feature | CH57x series |
|---------|----------|-----------|
| สีเดี่ยว | ✅ เหมาะสม (7 pins) | ✅ เหมาะสม |
| สองสี | ✅ เหมาะสม (8 pins) | ✅ เหมาะสม |
| RGB | ✅ ได้ (9 pins) | ✅ เหมาะสม |
| GPIO Available | ~15 pins | ~30+ pins |
| RAM สำหรับ Frame Buffer | 192 Bytes (RGB) = 9.4% | < 5% |
| Flash Usage | ~70 bytes | ~70 bytes |

---

## 📦 อุปกรณ์ที่รองรับและคำแนะนำในการซื้อ

### รายชื่อ P10 Panels ที่รองรับ

Library นี้รองรับ P10 LED matrix panels ทุกยี่ห้อที่ใช้ protocol มาตรฐาน (shift register + multiplexed row scan)
โดยสามารถปรับแต่ง pin mapping, bit order, และ scan type ได้ ทำให้เข้ากันได้กับ panel ส่วนใหญ่ในท้องตลาด

| รุ่น / ยี่ห้อ | ขนาด | สี | Scan Type | หมายเหตุ |
|--------------|------|-----|-----------|----------|
| P10 Red (Single) 1/16 scan | 32×16 | แดง | 1/16 | แผงสีแดง最常见, ราคาถูก |
| P10 Red (Single) 1/4 scan | 32×8 | แดง | 1/4 | แผงสั้น, ใช้แค่ A,B pins |
| P10 Green (Single) | 32×16 | เขียว | 1/16 | เหมาะป้ายข้อความเขียว |
| P10 Yellow (Single) | 32×16 | เหลือง | 1/16 | สีเหลืองสว่าง |
| P10 White (Single) | 32×16 | ขาว | 1/16 | ป้ายขาว ดูชัด |
| P10 Dual Color (Red+Green) | 32×16 | แดง+เขียว | 1/16 | แสดง 2 สี + ผสมเหลือง |
| P10 RGB Full Color | 32×16 | RGB | 1/16 | 8 สี (R/G/B/RG/RB/GB/RGB/Off) |
| P10 RGB Full Color | 16×16 | RGB | 1/8 หรือ 1/16 | แผงเล็ก, ใช้แค่ A,B,C pins |
| P6/P8/P5 (single color) | 32×16 | ตามสี | 1/16 | P6/P8/P5 ก็ใช้ protocol เดียวกัน |
| P3.91/P4.81 (RGB) | 32×16 | RGB | 1/16 | indoor LED display |

> **หมายเหตุ:** แม้ชื่อจะขึ้นต้นด้วย P10 แต่ P6, P8, P5, P3.91, P4.81 ก็ใช้ shift register + multiplex protocol **เหมือนกัน**
> ต่างแค่ pixel pitch (ระยะห่างระหว่าง pixel) — Library นี้ใช้งานได้กับทุก panel ที่ protocol ตรงกัน

### คำค้นหาสำหรับซื้ออุปกรณ์

#### 🔍 ภาษาไทย (Thai Market — Shopee/Lazada/HomePro)

| คำค้นหา | รายละเอียด |
|---------|-----------|
| `แผง LED P10` | คำค้นหาหลัก |
| `P10 LED สีแดง` | แผงสีเดี่ยว |
| `P10 LED สองสี` | แผงสองสี |
| `P10 LED RGB` | แผง RGB |
| `ป้าย LED P10` | ป้าย LED แสดงข้อความ |
| `โมดูล LED P10 32x16` | แผง P10 ขนาดมาตรฐาน |
| `P10 1/16 scan` | แบบ 1/16 scan (16 แถว) |
| `P10 1/4 scan` | แบบ 1/4 scan (4 แถว) |
| `P10 LED display module` | โมดูล P10 สำหรับ Arduino/Raspberry Pi |
| `P10 adapter board` | แผ่นแปลงสัญญาณ P10 |
| `สายแพ P10 16 pin` | สายแพเชื่อมต่อ P10 |
| `ระดับชิฟเตอร์ 5V 3.3V P10` | Level shifter เชื่อม MCU กับ P10 |

#### 🔍 ภาษาอังกฤษ (AliExpress/eBay/Amazon)

| คำค้นหา | รายละเอียด |
|---------|-----------|
| `P10 LED panel 32x16` | คำค้นหาหลัก — 32×16 pixels |
| `P10 single color LED display` | แผงสีเดี่ยว (แดง/เขียว/เหลือง/ขาว) |
| `P10 dual color LED module` | แผงสองสี (แดง+เขียว) |
| `P10 RGB LED panel` | แผง RGB |
| `P10 1/16 scan LED module` | แบบ 1/16 scan |
| `P10 1/4 scan LED sign` | แบบ 1/4 scan |
| `P10 16x16 LED matrix` | แผงเล็ก 16×16 |
| `P10 LED display HUB08 HUB12` | P10 พร้อม interface HUB08/HUB12 |
| `indoor P3.91 LED panel` | P3.91 indoor (protocol เดียวกัน) |
| `outdoor P10 LED sign` | P10 outdoor |
| `P10 LED power supply 5V` | Power supply สำหรับ P10 |
| `74HC245 level shifter P10` | Level shifter module สำหรับ P10 |
| `Arduino P10 display library` | ตัวอย่างการค้นหาเพิ่มเติม |

#### 🔍 ผู้ขายแนะนำบน AliExpress

| ร้าน | จุดเด่น |
|------|---------|
| **Shenzhen LEDSign** | มี P10 หลายสี, ราคาถูก, ส่งไว |
| **ColorfulLED Official** | คุณภาพดี, P10 RGB มี stock |
| **TopLED Store** | มีทั้ง indoor/outdoor, ส่งฟรี |
| **DIY LED Store** | P10 module ขายแยก, เหมาะทำโปรเจค |

#### 🔍 ผู้ขายแนะนำบน Taobao (淘宝) — 搜尋關鍵詞

| ร้าน / คำค้นหา | จุดเด่น |
|---------------|---------|
| `P10 全彩 LED 模组` | คำค้นหาหลัก — P10 RGB module |
| `P10 单色 LED 模组` | P10 สีเดี่ยว (แดง/เขียว/เหลือง/ขาว) |
| `P10 双色 LED 模组` | P10 สองสี (แดง+เขียว) |
| `P10 16扫 单元板` | P10 1/16 scan unit board |
| `P10 4扫 单元板` | P10 1/4 scan unit board |
| **鼎业光电** (Dingye Opto) | ผู้ผลิต P10 รายใหญ่, คุณภาพดี, ราคาส่ง |
| **华彩光电** (Huacai Opto) | มี P10 RGB หลายรุ่น, indoor/outdoor |
| **联诚发** (LCF LED) | ผู้ผลิต LED display รายใหญ่, P10 ครบทุกแบบ |
| **强力巨彩** (Qiangli Jucai) | แบรนด์ดัง, P10 คุณภาพสูง, นิยมใช้ในจีน |
| **中航光电** (ZH Opto) | P10 ราคาถูก, เหมาะทำโปรเจคส่วนตัว |
| **LED 显示屏 控制卡** | การ์ดควบคุม LED display (ถ้าต้องการทดสอบแยก) |
| **HUB08 转接板** | Adapter board HUB08 สำหรับ P10 |
| **74HC245 电平转换**| Level shifter 3.3V→5V สำหรับ P10 |
| **5V 10A 开关电源** | Power supply 5V 10A สำหรับ P10 |

> **💡 Tips การซื้อบน Taobao:**
> 1. ใช้ **image search** (以图搜图) ค้นหารูป P10 panel ที่ต้องการ — ได้ผลลัพธ์ตรงกว่า
> 2. ดู **รีวิว** และ **จำนวนยอดขาย** (月销量) — เลือกร้านที่มียอดขายสูง
> 3. ใช้ **cainiao** (菜鸟) หรือ **forwarding service** เช่น **Chinabuy** / **Pandabuy** ส่งของมาไทย
> 4. ราคาถูกกว่า AliExpress ~30-50% แต่ค่าส่งต้องคิดเพิ่ม
> 5. สั่งหลายชิ้นรวมกัน (consolidate shipping) เพื่อประหยัดค่าส่ง

### ราคาประมาณ (อ้างอิง 2026)

| อุปกรณ์ | ราคาประมาณ |
|---------|------------|
| P10 Single Color 32×16 | 150-350 บาท |
| P10 Dual Color 32×16 | 250-500 บาท |
| P10 RGB 32×16 | 400-900 บาท |
| Power Supply 5V 10A | 200-400 บาท |
| 74HC245 Level Shifter Module | 30-80 บาท |
| สายแพ 16-pin 20cm | 20-50 บาท |

> **⚠️ คำแนะนำในการซื้อ:**
> 1. เช็คว่า panel เป็น **1/16 scan หรือ 1/4 scan** — มีผลต่อการต่อ row address pins
> 2. **HUB08 vs HUB12** — P10 บางรุ่นใช้ HUB08 (8 pins) บางรุ่นใช้ HUB12 (12 pins) — ดู pinout ให้ดี
> 3. **5V Logic vs 3.3V Logic** — P10 ใช้ 5V logic ต้องมี level shifter ถ้าใช้ CH57x (3.3V)
> 4. **ซื้อ Power Supply แยก** — P10 แต่ละแผงกินไฟ ~3-5W ต่อแผง อย่าลืม PSU
> 5. **ซื้อหลายแผง** — ถ้าต้องการป้ายใหญ่ ต่อ cascade หลายแผง (ต้องพัฒนาต่อใน v2)

### Pinout HUB08 vs HUB12 (เทียบกับ P10 Library)

P10 panel มี interface connector 2 แบบหลัก:

```
HUB08 (8-pin, พบบ่อยใน P10 32×16):
  Pin 1: R (Red Data)
  Pin 2: G (Green Data) — หรือ NC สำหรับ single color
  Pin 3: CLK
  Pin 4: STB/LAT
  Pin 5: A
  Pin 6: B
  Pin 7: C — หรือ NC ถ้า 1/4 scan
  Pin 8: D — หรือ NC ถ้า 1/4 scan
  Pin 9: OE — (บน HUB08 บางรุ่น OE จะอยู่ pin 9 หรือ pin อื่น)
  Pin 10-16: GND

HUB12 (12-pin, พบบ่อยใน P10 RGB):
  Pin 1: R
  Pin 2: G
  Pin 3: B
  Pin 4: CLK
  Pin 5: STB/LAT
  Pin 6: A
  Pin 7: B
  Pin 8: C
  Pin 9: D
  Pin 10: OE
  Pin 11: GND
  Pin 12: GND
```

**วิธีตรวจสอบ pinout ของ panel ที่ซื้อมา:**
1. ดู silkscreen บน PCB — มักมี label R, G, B, CLK, STB, OE, A, B, C, D
2. ใช้ multimeter โหมด continuity — ไล่จากขา connector ไปยัง IC shift register
3. ค้นหารุ่น panel + "pinout" ใน Google — ผู้ผลิตส่วนใหญ่มี datasheet

---

## คุณสมบัติ

### คุณสมบัติของ Library

- ✅ รองรับ P10 แบบสีเดี่ยว (1 data pin), สองสี (2 data pins), RGB (3 data pins)
- ✅ เลือก color mode ตอน compile เพื่อประหยัด RAM/Flash
- ✅ Timer interrupt scan (non-blocking, ~200Hz refresh rate)
- ✅ Configurable resolution (32×16 / 16×16 / ปรับเอง)
- ✅ Configurable bit order (MSB-first / LSB-first)
- ✅ Pin mapping ยืดหยุ่น (ใช้ GPIO pin ใดก็ได้)
- ✅ ใช้ GPIO register writes โดยตรงใน ISR เพื่อความเร็วสูง
- ✅ รองรับ CH57x series
- ✅ เอกสารภาษาไทยครบถ้วน

### ข้อมูลทางเทคนิค

- **ความละเอียดสูงสุด**: 64×32 พิกเซล (ปรับได้ใน P10.h)
- **Refresh Rate**: 200 Hz (ปรับได้ใน P10.h)
- **Frame Buffer Size** (32×16):
  - สีเดี่ยว: 64 bytes
  - สองสี: 128 bytes
  - RGB: 192 bytes
- **Scan Method**: Timer Interrupt (TIM2)
- **GPIO Pins ที่ต้องการ**: 7-9 pins (ขึ้นกับ color mode)
- **Power Supply**: 5V (P10 panel) — ต้องมี level shifter ถ้าใช้ 3.3V MCU

---

## หลักการทำงานของ P10 Panel

### P10 Protocol

P10 ใช้ shift register + multiplexing เพื่อแสดงผล:

```
สำหรับแต่ละรอบ scan (1 row):
  1. OE = HIGH       ← ปิด output (กัน ghosting)
  2. Set A/B/C/D     ← เลือกแถวที่ต้องการ scan
  3. Shift Data Out:  ← ส่งข้อมูลพิกเซลทีละ bit
     for each pixel bit:
       set DATA pin (R/G/B)
       toggle CLK (HIGH → LOW)
  4. LAT = HIGH→LOW  ← latch ข้อมูลไปยัง output register
  5. OE = LOW        ← เปิด output (LED ติด)
  6. Hold ~10µs      ← รักษาเวลาให้เห็น
  7. → Row ถัดไป
```

Timer interrupt จะวน scan ทุกแถวที่ความถี่ = `refresh_rate × rows` Hz
- 200 Hz × 16 rows = 3,200 Hz timer interrupt
- แต่ละ interrupt ใช้เวลา ~25µs → CPU usage ~8%

### Frame Buffer Layout

```
สีเดี่ยว (Single Color):
  [0] [1] ... [plane_size-1]     ← R plane (1 bit/pixel)
  
สองสี (Dual Color):
  [0] ... [plane_size-1]         ← R plane
  [plane_size] ... [2*ps-1]      ← G plane

RGB:
  [0] ... [plane_size-1]         ← R plane
  [plane_size] ... [2*ps-1]      ← G plane
  [2*ps] ... [3*ps-1]            ← B plane

ps = plane_size = width × rows / 8 bytes
```

### Signal Timing Diagram

```
Row N:
OE    ████████████________________████████████
                                               
A     __________________________________________
B     ████████________________________________
C     _______________________________████████
D     ________________████████________________

CLK   ▄▄█▄▄█▄▄█▄▄█▄▄█▄▄█▄▄█▄▄█              
DATA  █▄█▄██▄█▄██▄██▄█▄▄█▄██▄█▄              
                                            
LAT   ████████████████████████_               

      |<--- shift 32 bits --->|  |<--  OE=Low hold -->|
```

---

## Hardware Setup

### สีเดี่ยว (Single Color) — 32×16, 1/16 scan

```
P10 Single Color Panel (เช่น P10 Red):

CH57x              P10 Panel Connector
  DATA (PC0) -----> R (Red Data)        pin: พินข้อมูลสีแดง
  CLK  (PC1) -----> CLK (Shift Clock)   pin: นาฬิกา shift
  LAT  (PC2) -----> STB/LAT (Latch)     pin: latch ข้อมูล
  OE   (PC3) -----> OE (Output Enable)  pin: เปิด/ปิดแสดงผล
  A    (PC4) -----> A (Row Address LSB) pin: ที่อยู่แถว
  B    (PC5) -----> B                   pin:
  C    (PC6) -----> C                   pin: (ถ้า 8+ scan rows)
  D    (PC7) -----> D                   pin: (ถ้า 16 scan rows)

  GND  --------> GND
  5V   (จ่ายไฟ) ----> VCC (Power supply สำหรับ panel)
```

### สองสี (Dual Color) — 32×16, 1/16 scan

```
CH57x              P10 Dual Color Panel
  R    (PC0) -----> Red Data
  G    (PC1) -----> Green Data
  CLK  (PC2) -----> CLK
  LAT  (PC3) -----> STB/LAT
  OE   (PC4) -----> OE
  A    (PC5) -----> A
  B    (PC6) -----> B
  C    (PC7) -----> C
  D    (PD2) -----> D

  GND  --------> GND
  5V   --------> VCC
```

### RGB Full Color — 32×16, 1/16 scan

```
CH57x              P10 RGB Panel
  R    (PC0) -----> Red Data
  G    (PC1) -----> Green Data
  B    (PC2) -----> Blue Data
  CLK  (PC3) -----> CLK
  LAT  (PC4) -----> STB/LAT
  OE   (PC5) -----> OE
  A    (PC6) -----> A
  B    (PC7) -----> B
  C    (PD2) -----> C
  D    (PD3) -----> D

  GND  --------> GND
  5V   --------> VCC
```

> **⚠️ ข้อควรระวัง:** CH57x ใช้ 3.3V logic ส่วน P10 panel ส่วนใหญ่ใช้ 5V logic
> ควรใช้ Level Shifter (เช่น 74HC245, TXB0108, หรือ Voltage Divider) ระหว่าง MCU และ Panel
>
> **แนะนำ:** ใช้ 74HC245 (8-channel bidirectional) สำหรับ level shift สัญญาณ control ทั้งหมด

### Pin ที่ใช้ได้กับ CH57x

```
GPIOA: PA1, PA2
GPIOC: PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7
GPIOD: PD2, PD3, PD4, PD5, PD6, PD7

รวม: 15 pins (เพียงพอสำหรับ RGB = 10 pins + เหลือ 5 pins)
```

---

## การเลือก Color Mode

เลือก color mode โดยกำหนด `#define P10_COLOR_MODE` ก่อน `#include "P10.h"`:

```c
// === วิธีที่ 1: กำหนดใน main.c ก่อน include ===
#define P10_COLOR_MODE  P10_SINGLE_COLOR   // สีเดี่ยว
// #define P10_COLOR_MODE  P10_DUAL_COLOR   // สองสี
// #define P10_COLOR_MODE  P10_RGB          // RGB

#include "P10.h"
```

```c
// === วิธีที่ 2: กำหนดใน compiler flags ===
// build.bat: -DP10_COLOR_MODE=0  (single)
//            -DP10_COLOR_MODE=1  (dual)
//            -DP10_COLOR_MODE=2  (rgb)
```

```c
// === วิธีที่ 3: แก้ไขใน P10.h (default) ===
#ifndef P10_COLOR_MODE
#define P10_COLOR_MODE  P10_SINGLE_COLOR   // ← เปลี่ยนตรงนี้
#endif
```

### ผลของการเลือก Color Mode

| Color Mode | P10_NUM_PLANES | Frame Buffer (32×16) | Flash Size |
|-----------|---------------|---------------------|------------|
| SINGLE    | 1             | 64 bytes            | ~70 bytes  |
| DUAL      | 2             | 128 bytes           | ~70 bytes  |
| RGB       | 3             | 192 bytes           | ~70 bytes  |

---

## การใช้งานพื้นฐาน

### ตัวอย่าง 1: สีเดี่ยว — แสดงพิกเซล

```c
#include <main.h>
#include "Lib/P10/P10.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    // กำหนดค่า P10 panel
    P10_Config cfg = P10_CONFIG_DEFAULT(32, 16);
    cfg.data_r_pin = PC0;   // Red data
    cfg.clk_pin    = PC1;   // Shift clock
    cfg.lat_pin    = PC2;   // Latch
    cfg.oe_pin     = PC3;   // Output enable
    cfg.row_a_pin  = PC4;   // Row A
    cfg.row_b_pin  = PC5;   // Row B
    cfg.row_c_pin  = PC6;   // Row C
    cfg.row_d_pin  = PC7;   // Row D

    P10_Instance display;
    P10_Init(&display, &cfg);

    // วาดเส้นทแยงมุม
    for (uint8_t i = 0; i < 16; i++) {
        P10_SetPixel(&display, i, i, 1, 0, 0);
    }

    while (1) {
        // Timer ISR scan อัตโนมัติ — ไม่ต้องทำอะไร
    }
}
```

### ตัวอย่าง 2: RGB — วาดรูปสี่เหลี่ยม

```c
#define P10_COLOR_MODE P10_RGB
#include <main.h>
#include "Lib/P10/P10.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    P10_Config cfg = P10_CONFIG_DEFAULT(32, 16);
    cfg.data_r_pin = PC0;
    cfg.data_g_pin = PC1;
    cfg.data_b_pin = PC2;
    cfg.clk_pin    = PC3;
    cfg.lat_pin    = PC4;
    cfg.oe_pin     = PC5;
    cfg.row_a_pin  = PC6;
    cfg.row_b_pin  = PC7;
    cfg.row_c_pin  = PD2;
    cfg.row_d_pin  = PD3;

    P10_Instance display;
    P10_Init(&display, &cfg);

    P10_Clear(&display);

    // วาดแถบสีแดง
    for (uint8_t x = 0; x < 32; x++)
        P10_SetPixel(&display, x, 0, 1, 0, 0);  // R=1

    // วาดแถบสีเขียว
    for (uint8_t x = 0; x < 32; x++)
        P10_SetPixel(&display, x, 1, 0, 1, 0);  // G=1

    // วาดแถบสีน้ำเงิน
    for (uint8_t x = 0; x < 32; x++)
        P10_SetPixel(&display, x, 2, 0, 0, 1);  // B=1

    // จุดสีเหลือง (R+G)
    P10_SetPixel(&display, 15, 7, 1, 1, 0);

    // จุดสีขาว (R+G+B)
    P10_SetPixel(&display, 16, 7, 1, 1, 1);

    while (1);
}
```

### ตัวอย่าง 3: สองสี — สลับข้อความ

```c
#define P10_COLOR_MODE P10_DUAL_COLOR
#include <main.h>
#include "Lib/P10/P10.h"

// Buffer สำหรับเก็บ pattern 8x8 (ตัวอักษร A)
const uint8_t font_A[8] = {
    0x3C, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00
};

P10_Instance display;

void draw_char(uint8_t dx, uint8_t dy, const uint8_t* font, uint8_t r, uint8_t g) {
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            if (font[y] & (0x80 >> x)) {
                P10_SetPixel(&display, dx + x, dy + y, r, g, 0);
            }
        }
    }
}

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    P10_Config cfg = P10_CONFIG_DEFAULT(32, 16);
    // ... กำหนด pins ...
    P10_Init(&display, &cfg);

    P10_Clear(&display);
    draw_char(0, 0, font_A, 1, 0);  // A สีแดง
    draw_char(8, 0, font_A, 0, 1);  // A สีเขียว

    while (1) {
        // Flip frame - สลับสี
        Delay_Ms(500);
        P10_Clear(&display);
        draw_char(0, 0, font_A, 0, 1);  // A สีเขียว
        draw_char(8, 0, font_A, 1, 0);  // A สีแดง
        Delay_Ms(500);
        P10_Clear(&display);
        draw_char(0, 0, font_A, 1, 0);
        draw_char(8, 0, font_A, 0, 1);
    }
}
```

---

## การใช้งานขั้นสูง

### การปรับ Refresh Rate

```c
cfg.refresh_rate = 400;  // 400 Hz (ลด flicker แต่ใช้ CPU มากขึ้น)
```

ค่าแนะนำ:
- **200 Hz**: ค่าเริ่มต้น สมดุล CPU/Flicker
- **400 Hz**: ลด flicker แต่ CPU ~16%
- **100 Hz**: ประหยัด CPU อาจเห็น flicker

### การปรับความสว่างด้วย OE Hold Time

ปรับ `P10_OE_HOLD_US` ใน `P10.h`:

```c
#ifndef P10_OE_HOLD_US
#define P10_OE_HOLD_US   10  // 10µs = ความสว่างปกติ
// #define P10_OE_HOLD_US  5  // สว่างน้อยลง
// #define P10_OE_HOLD_US  20 // สว่างมากขึ้น
#endif
```

> **หมายเหตุ:** OE hold time จะถูก apply โดยการ delay ใน ISR
> ถ้าต้องการปรับความสว่างแบบละเอียด ต้องใช้ PWM บน OE pin (จะเพิ่มใน v2)

### การใช้กับ Panel 16×16

```c
P10_Config cfg = P10_CONFIG_DEFAULT(16, 16);
// หรือ
cfg.width  = 16;
cfg.height = 16;
cfg.rows   = 16;  // 1/16 scan
```

สำหรับ panel 1/4 scan (32×8, 4 rows):
```c
cfg.width  = 32;
cfg.height = 8;
cfg.rows   = 4;   // 1/4 scan — ใช้แค่ A, B pins
cfg.row_c_pin = 0xFF;  // ไม่ต้องใช้ C
cfg.row_d_pin = 0xFF;  // ไม่ต้องใช้ D
```

### การ Clear และ Fill อย่างรวดเร็ว

```c
// ปิดทุกพิกเซล
P10_Clear(&display);

// เปิดทุกพิกเซลสีแดง (single) / R+G+B (RGB)
P10_Fill(&display, 1, 0, 0);  // สีแดงล้วน
P10_Fill(&display, 1, 1, 1);  // สีขาว (RGB)
```

---

## CH57x series compatibility

ตารางเปรียบเทียบสำหรับ P10 Library:

| ด้าน | CH57x series |
|------|----------|-----------|
| **GPIOs** | ~15 pins | ~30+ pins |
| **RGB RGB** | 9 pins → เหลือ 6 pins | 9 pins → เหลือ ~20 pins |
| **RAM** | 2 KB (ใช้ 192B = 9.4% สำหรับ RGB) | 4-8 KB (ใช้ < 5%) |
| **Flash** | 16 KB (~70 bytes for P10 lib) | 32-64 KB |
| **Timer** | TIM1, TIM2 (TIM2 ใช้ scan) | มี timer มากกว่า |
| **Pin Mapping** | ต้องวางแผน pin usage ให้ดี | มีอิสระในการเลือก pin มากขึ้น |

### ข้อแนะนำสำหรับ CH57x

1. **วางแผนพินก่อน**: CH57x มี GPIO จำกัด (~15 pins) วางแผนให้รอบคอบ
2. **RGB ต้องเลือก pin ให้ดี**: ใช้ PC0-PC7 ครบ 8 pins + PD2-PD3 สำหรับ D lines
3. **หลีกเลี่ยง conflict**: ห้ามใช้ TIM2 สำหรับ PWM ขณะใช้ P10 scan
4. **เหลือเผื่อ**: ถ้าใช้ RGB เหลือ GPIO แค่ ~5 pins สำหรับ peripherals อื่น

---

## Resource Conflicts

| Resource | P10 Library | ห้ามใช้ร่วมกับ |
|----------|-------------|---------------|
| TIM2 | Timer scan interrupt | SimplePWM (PC0, PD3, PD4, PD7) |
| GPIOC (บาง pin) | Data/Control pins | Peripherals อื่นบน pin เดียวกัน |
| SysTick | SimpleDelay (ใช้ร่วมกันได้) | — |

---

## Troubleshooting

### 🔴 ไม่มีอะไรแสดงบน Panel

1. **ตรวจสอบไฟเลี้ยง**: P10 panel ต้องการ 5V — วัดแรงดันที่ VCC/GND
2. **Level Shifter**: ถ้า MCU 3.3V → P10 5V ต้องมี level shifter
3. **OE Polarity**: ลองกลับ OE logic (LOW enable → HIGH enable) ถ้า panel inverted
4. **Pin Mapping**: ตรวจสอบว่าต่อ pin ถูกต้องตาม config
5. **Timer ทำงานหรือไม่**: ดู LED บน CH57x board ว่า timer ISR ทำงาน

### 🔴 ภาพซ้อนหรือ Ghosting

1. **เพิ่ม OE hold time**: ค่า `P10_OE_HOLD_US` อาจต้องเพิ่ม
2. **Latency**: ตรวจสอบว่าข้อมูลถูก latch ก่อนเปิด OE
3. **Timing**: ปรับ refresh rate (ถ้า太低จะเห็น ghost)

### 🔴 ภาพ Flicker

1. **เพิ่ม Refresh Rate**: ลอง 300-400 Hz
2. **Timer Frequency**: ตรวจสอบว่า timer interrupt ทำงานถูกต้อง
3. **ISR Overrun**: ถ้า CPU โหลดมาก → ลด refresh rate

### 🔴 RAM/Flash เต็ม

1. **ลดความละเอียด**: ใช้ 16×16 แทน 32×16
2. **เปลี่ยน Color Mode**: Single color ใช้ RAM น้อยกว่า RGB
3. **ปิด printf**: ใช้ `DISABLE_PRINTF PRINTF_OFF` ใน `ch32v00x_conf.h`

---

## API Reference

### Enumerations

```c
// Color Mode (ใช้ตอน compile)
#define P10_SINGLE_COLOR  0
#define P10_DUAL_COLOR    1
#define P10_RGB           2

// Bit Order
#define P10_MSB_FIRST  0   // Most Significant Bit first
#define P10_LSB_FIRST  1   // Least Significant Bit first
```

### Configuration Macros (overrideable)

```c
#ifndef P10_COLOR_MODE
#define P10_COLOR_MODE    P10_SINGLE_COLOR   // เลือก single/dual/RGB
#endif

#ifndef P10_MAX_PANELS
#define P10_MAX_PANELS    1                  // จำนวน panels (v1 รองรับ 1)
#endif

#ifndef P10_REFRESH_RATE
#define P10_REFRESH_RATE  200                // Panel refresh rate (Hz)
#endif

#ifndef P10_OE_HOLD_US
#define P10_OE_HOLD_US    10                 // OE hold time (µs)
#endif
```

### P10_Config Structure

```c
typedef struct {
    // Data pins (ใช้ SimpleGPIO enum: PC0, PC1, ...)
    uint8_t data_r_pin;   // Red data pin (หรือสีเดียว)
    uint8_t data_g_pin;   // Green data pin (0xFF = ไม่ใช้)
    uint8_t data_b_pin;   // Blue data pin  (0xFF = ไม่ใช้)

    // Control pins
    uint8_t clk_pin;      // Shift clock
    uint8_t lat_pin;      // Latch (STB)
    uint8_t oe_pin;       // Output enable

    // Row select pins
    uint8_t row_a_pin;    // Row address A (LSB)
    uint8_t row_b_pin;    // Row address B
    uint8_t row_c_pin;    // Row address C (0xFF = ไม่ใช้)
    uint8_t row_d_pin;    // Row address D (0xFF = ไม่ใช้)

    // Panel geometry
    uint8_t width;        // พิกเซลต่อแถว (default 32)
    uint8_t height;       // ความสูงรวม (default 16)
    uint8_t rows;         // จำนวน scan rows (default = height)

    // Protocol
    uint8_t  color_mode;   // P10_SINGLE_COLOR / DUAL / RGB
    uint8_t  bit_order;    // P10_MSB_FIRST หรือ LSB_FIRST
    uint16_t refresh_rate; // Refresh rate (Hz)
} P10_Config;
```

### P10_CONFIG_DEFAULT Macro

```c
/**
 * สร้าง P10_Config ด้วยค่าเริ่มต้น
 * @param w ความกว้าง (px) เช่น 32
 * @param h ความสูงรวม (px) เช่น 16
 *
 * ยังต้องกำหนด pin mapping ก่อน Init
 */
P10_Config cfg = P10_CONFIG_DEFAULT(32, 16);
// แล้วกำหนด pin: cfg.data_r_pin = PC0; ...
```

### P10_Instance Structure

```c
typedef struct {
    P10_Config config;              // Panel configuration
    
    // Internal (pin maps, framebuffer pointer, scan state)
    // ดูใน P10.h สำหรับรายละเอียด
    
    volatile uint8_t current_row;   // Current row being scanned
    uint8_t initialized;            // Initialized flag
} P10_Instance;
```

### Functions

---

#### `P10_Init`

```c
uint8_t P10_Init(P10_Instance* inst, P10_Config* cfg);
```

เริ่มต้น P10 panel

| พารามิเตอร์ | คำอธิบาย |
|------------|----------|
| `inst` | ตัวชี้ไปยัง P10_Instance |
| `cfg` | ตัวชี้ไปยัง P10_Config (pin mapping + geometry) |

**คืนค่า:** `1` = สำเร็จ, `0` = ล้มเหลว (null param / bad config)

**สิ่งที่ทำ:**
1. ตั้งค่า GPIO pins ทั้งหมดเป็น output
2. คำนวณ frame buffer ขนาดตาม width × height × planes
3. clear frame buffer
4. เริ่ม timer interrupt scan ที่ refresh_rate × rows Hz

---

#### `P10_SetPixel`

```c
void P10_SetPixel(P10_Instance* inst, uint8_t x, uint8_t y,
                  uint8_t r, uint8_t g, uint8_t b);
```

ตั้งค่าพิกเซลใน frame buffer (On/Off)

| พารามิเตอร์ | คำอธิบาย |
|------------|----------|
| `inst` | ตัวชี้ไปยัง P10_Instance |
| `x` | พิกัด X (0 .. width-1) |
| `y` | พิกัด Y (0 .. height-1) |
| `r` | Red / single color value (0=off, 1=on) |
| `g` | Green value (0=off, 1=on) — ใช้เฉพาะ DUAL/RGB |
| `b` | Blue value  (0=off, 1=on) — ใช้เฉพาะ RGB |

> ถ้าใช้ `P10_SINGLE_COLOR`: `r` เป็นตัวกำหนด on/off (`g`, `b` ไม่มีผล)
> ถ้าใช้ `P10_DUAL_COLOR`: `r`=แดง, `g`=เขียว (`b` ไม่มีผล)
> ถ้าใช้ `P10_RGB`: `r`=แดง, `g`=เขียว, `b`=น้ำเงิน

---

#### `P10_Clear`

```c
void P10_Clear(P10_Instance* inst);
```

ลบ frame buffer ทั้งหมด (ปิดทุกพิกเซล)

---

#### `P10_Fill`

```c
void P10_Fill(P10_Instance* inst, uint8_t r, uint8_t g, uint8_t b);
```

เติม frame buffer ทั้งหมด (เปิดทุกพิกเซลด้วยสีที่กำหนด)

| พารามิเตอร์ | คำอธิบาย |
|------------|----------|
| `r` | Red / single color value (0/1) |
| `g` | Green value (0/1) |
| `b` | Blue value (0/1) |

---

#### `P10_Deinit`

```c
void P10_Deinit(P10_Instance* inst);
```

หยุดการทำงานของ P10 panel

- หยุด timer interrupt
- ปิด OE
- reset state

---

#### `P10_ScanHandler`

```c
void P10_ScanHandler(P10_Instance* inst);
```

ฟังก์ชัน scan handler — เรียกจาก timer ISR

**ปกติไม่ต้องเรียกเอง** — timer ISR จัดการให้อัตโนมัติ

---

## ใบอนุญาต

MIT License — CH57x Library Team

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)