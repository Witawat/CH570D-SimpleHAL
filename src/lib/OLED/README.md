# OLED I2C Library สำหรับ CH57x

Library ที่ครบครันสำหรับควบคุม OLED Display ผ่าน I2C บน CH57x พร้อมฟีเจอร์ตั้งแต่ขั้นพื้นฐานถึงขั้นสูง

## 📋 สารบัญ

- [ภาพรวม](#ภาพรวม)
- [คุณสมบัติ](#คุณสมบัติ)
- [การติดตั้งฮาร์ดแวร์](#การติดตั้งฮาร์ดแวร์)
- [เริ่มต้นใช้งาน](#เริ่มต้นใช้งาน)
- [API Reference](#api-reference)
- [การใช้งานขั้นพื้นฐาน](#การใช้งานขั้นพื้นฐาน)
- [การใช้งานขั้นกลาง](#การใช้งานขั้นกลาง)
- [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
- [เทคนิคต่างๆ](#เทคนิคต่างๆ)
- [การแก้ปัญหา](#การแก้ปัญหา)
- [ตัวอย่างโปรเจค](#ตัวอย่างโปรเจค)

---

## ภาพรวม

OLED I2C Library เป็น library ที่พัฒนาขึ้นสำหรับ CH57x microcontroller โดยเฉพาะ รองรับการควบคุม OLED display ที่ใช้ SSD1306 controller ผ่าน I2C interface

### OLED ที่รองรับ

| ขนาด | ความละเอียด | ขนาดจอ | I2C Address |
|------|------------|---------|-------------|
| 0.96" | 128x64 | OLED_128x64 | 0x3C, 0x3D |
| 0.91" | 128x32 | OLED_128x32 | 0x3C, 0x3D |
| 0.66" | 64x48 | OLED_64x48 | 0x3C, 0x3D |
| 1.3" | 128x64 | OLED_128x64 | 0x3C, 0x3D |

### ข้อกำหนดของระบบ

- **MCU**: CH57x series
- **I2C**: SimpleI2C library
- **Timer**: Timer library (สำหรับ delay functions)
- **RAM**: ขั้นต่ำ 1KB (สำหรับ 128x64 single buffer)
- **Flash**: ขั้นต่ำ 8KB

---

## คุณสมบัติ

### Core Features
- ✅ รองรับ SSD1306 OLED controller
- ✅ รองรับหลายขนาดหน้าจอ (128x64, 128x32, 64x48)
- ✅ Single และ Double buffering
- ✅ Partial screen update (ประหยัดเวลา)
- ✅ Hardware scrolling
- ✅ Display control (on/off, contrast, invert)

### Graphics Features
- ✅ Drawing primitives (line, rectangle, circle, triangle)
- ✅ Filled shapes
- ✅ Bitmap/image display
- ✅ Sprite system สำหรับ animations
- ✅ Progress bar และ Graph plotting
- ✅ Pattern fills

### Font & Text Features
- ✅ Built-in fonts หลายขนาด (6x8, 8x16, 12x16)
- ✅ รองรับข้อความ UTF-8 ไทย+อังกฤษ
- ✅ รองรับไทยแบบสระ/วรรณยุกต์ด้วย combining rendering
- ✅ ปรับขนาดฟอนต์แบบ integer scale (1x-4x)
- ✅ Text alignment (left, center, right)
- ✅ Text effects (inverse, underline)
- ✅ Scrolling text
- ✅ Multi-line text
- ✅ Number formatting (int, float)

### Menu System
- ✅ Interactive menu framework
- ✅ หลายสไตล์ (list, icon, full screen)
- ✅ Value editor (numeric, boolean, list)
- ✅ Submenu support
- ✅ Callback system
- ✅ Auto-scroll

---

## การติดตั้งฮาร์ดแวร์

### Pin Connections

#### Default Pins (I2C_PINS_DEFAULT)
```
CH57x        OLED Module
---------       -----------
PC1 (SDA)  <->  SDA
PC2 (SCL)  <->  SCL
GND        <->  GND
3.3V       <->  VCC
```

#### Remap Pins (I2C_PINS_REMAP)
```
CH57x        OLED Module
---------       -----------
PD1 (SDA)  <->  SDA
PD0 (SCL)  <->  SCL
GND        <->  GND
3.3V       <->  VCC
```

### Pull-up Resistors

**สำคัญ**: ต้องต่อ pull-up resistor ที่ SDA และ SCL

- **แนะนำ**: 4.7kΩ
- **ช่วงที่ใช้ได้**: 2.2kΩ - 10kΩ
- Module บางรุ่นมี pull-up resistor ติดมาแล้ว

### Power Requirements

- **แรงดัน**: 3.3V (บาง module รองรับ 5V)
- **กระแส**: ~20mA (ปกติ), ~40mA (ทุก pixel เปิด)

---

## เริ่มต้นใช้งาน

### 1. Include Headers

```c
#include "SimpleI2C.h"
#include "timer.h"
#include "oled_i2c.h"
#include "oled_fonts.h"
#include "oled_graphics.h"
```

### 2. เริ่มต้น I2C และ Timer

```c
int main(void) {
    // เริ่มต้น System
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    
    // เริ่มต้น I2C (100kHz, default pins)
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
    
    // เริ่มต้น Timer
    Timer_Init();
    
    // ... ส่วนของ OLED
}
```

### 3. เริ่มต้น OLED

```c
OLED_Handle oled;

// เริ่มต้น OLED 128x64, address 0x3C
if(!OLED_Init(&oled, OLED_128x64, 0x3C)) {
    // OLED ไม่พบ - จัดการ error
    while(1);
}
```

### 4. แสดงข้อความ

```c
// ล้างหน้าจอ
OLED_Clear(&oled);

// ตั้งค่า font
OLED_SetFont(&oled, &Font_8x16);

// แสดงข้อความ
OLED_DrawString(&oled, 0, 0, "Hello World!", OLED_COLOR_WHITE);

// อัพเดทหน้าจอ
OLED_Update(&oled);
```

---

## API Reference

### Text Rendering (UTF-8 + Scale)

#### OLED_SetTextScale
```c
void OLED_SetTextScale(OLED_Handle* oled, uint8_t scale);
```
ตั้งค่า scale เริ่มต้นของข้อความ (1..4)

#### OLED_GetTextScale
```c
uint8_t OLED_GetTextScale(OLED_Handle* oled);
```
อ่านค่า scale ปัจจุบัน

#### OLED_DrawCharScaled
```c
uint8_t OLED_DrawCharScaled(OLED_Handle* oled, uint8_t x, uint8_t y, char c, OLED_Color color, uint8_t scale);
```
วาดตัวอักษร ASCII พร้อมกำหนด scale ต่อคำสั่ง

#### OLED_DrawStringScaled
```c
uint16_t OLED_DrawStringScaled(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, uint8_t scale);
```
วาดสตริง ASCII พร้อมกำหนด scale ต่อคำสั่ง

#### OLED_DrawStringUTF8Ex
```c
uint16_t OLED_DrawStringUTF8Ex(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, const OLED_TextConfig* config);
```
วาดข้อความ UTF-8 (ไทย+อังกฤษ) ด้วยการตั้งค่าแบบละเอียด

#### OLED_MeasureStringUTF8
```c
uint16_t OLED_MeasureStringUTF8(OLED_Handle* oled, const char* str, const OLED_TextConfig* config);
```
วัดความกว้างข้อความ UTF-8 สำหรับงานจัด layout/alignment

#### OLED_TextConfig
```c
typedef struct {
    uint8_t scale;
    uint8_t letter_spacing;
    OLED_ThaiRenderMode thai_mode;
} OLED_TextConfig;
```

#### OLED_ThaiRenderMode
```c
typedef enum {
    OLED_THAI_RENDER_OFF = 0,
    OLED_THAI_RENDER_PLACEHOLDER = 1,
    OLED_THAI_RENDER_COMBINING = 2
} OLED_ThaiRenderMode;
```

ตัวอย่างใช้งาน:
```c
OLED_TextConfig cfg = {
    .scale = 2,
    .letter_spacing = 1,
    .thai_mode = OLED_THAI_RENDER_COMBINING
};

OLED_DrawStringUTF8Ex(&oled, 0, 0, "Hello สวัสดี ๑๒๓", OLED_COLOR_WHITE, &cfg);
```

### Core Functions

#### OLED_Init
```c
uint8_t OLED_Init(OLED_Handle* oled, OLED_Size size, uint8_t i2c_addr);
```
เริ่มต้นการใช้งาน OLED

**Parameters:**
- `oled`: ตัวชี้ไปยัง OLED handle
- `size`: ขนาดของ OLED (OLED_128x64, OLED_128x32, OLED_64x48)
- `i2c_addr`: ที่อยู่ I2C (ปกติ 0x3C หรือ 0x3D)

**Returns:** 1 = สำเร็จ, 0 = ล้มเหลว

#### OLED_Clear
```c
void OLED_Clear(OLED_Handle* oled);
```
ล้างหน้าจอ (เติมด้วยสีดำ)

#### OLED_Update
```c
void OLED_Update(OLED_Handle* oled);
```
อัพเดทหน้าจอทั้งหมด (ส่ง buffer ไปยัง OLED)

#### OLED_SetPixel
```c
void OLED_SetPixel(OLED_Handle* oled, uint8_t x, uint8_t y, OLED_Color color);
```
ตั้งค่า pixel

**Parameters:**
- `x`: ตำแหน่ง x (0 - width-1)
- `y`: ตำแหน่ง y (0 - height-1)
- `color`: สี (OLED_COLOR_WHITE, OLED_COLOR_BLACK, OLED_COLOR_INVERT)

### Graphics Functions

#### OLED_DrawLine
```c
void OLED_DrawLine(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, OLED_Color color);
```
วาดเส้นตรง (ใช้ Bresenham's algorithm)

#### OLED_DrawRect
```c
void OLED_DrawRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_Color color);
```
วาดสี่เหลี่ยม (เส้นขอบ)

#### OLED_FillRect
```c
void OLED_FillRect(OLED_Handle* oled, uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_Color color);
```
วาดสี่เหลี่ยมแบบเติมสี

#### OLED_DrawCircle
```c
void OLED_DrawCircle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, OLED_Color color);
```
วาดวงกลม (ใช้ Midpoint circle algorithm)

#### OLED_FillCircle
```c
void OLED_FillCircle(OLED_Handle* oled, uint8_t x0, uint8_t y0, uint8_t r, OLED_Color color);
```
วาดวงกลมแบบเติมสี

### Font & Text Functions

#### OLED_SetFont
```c
void OLED_SetFont(OLED_Handle* oled, const OLED_Font* font);
```
ตั้งค่า font ปัจจุบัน

**Built-in Fonts:**
- `Font_6x8`: ขนาด 6x8 pixels
- `Font_8x16`: ขนาด 8x16 pixels
- `Font_12x16`: ขนาด 12x16 pixels

#### OLED_DrawString
```c
uint16_t OLED_DrawString(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color);
```
แสดงข้อความ

**Returns:** ความกว้างของข้อความ (pixels)

#### OLED_DrawStringAlign
```c
void OLED_DrawStringAlign(OLED_Handle* oled, uint8_t x, uint8_t y, const char* str, OLED_Color color, OLED_TextAlign align);
```
แสดงข้อความแบบมี alignment

**Alignment:**
- `OLED_ALIGN_LEFT`: ชิดซ้าย
- `OLED_ALIGN_CENTER`: กึ่งกลาง
- `OLED_ALIGN_RIGHT`: ชิดขวา

#### OLED_DrawInt
```c
uint16_t OLED_DrawInt(OLED_Handle* oled, uint8_t x, uint8_t y, int32_t num, OLED_Color color);
```
แสดงตัวเลข integer

#### OLED_DrawFloat
```c
uint16_t OLED_DrawFloat(OLED_Handle* oled, uint8_t x, uint8_t y, float num, uint8_t decimals, OLED_Color color);
```
แสดงตัวเลข float

---

## การใช้งานขั้นพื้นฐาน

### แสดงข้อความ

```c
OLED_Clear(&oled);
OLED_SetFont(&oled, &Font_8x16);
OLED_DrawString(&oled, 0, 0, "Line 1", OLED_COLOR_WHITE);
OLED_DrawString(&oled, 0, 20, "Line 2", OLED_COLOR_WHITE);
OLED_Update(&oled);
```

### แสดงตัวเลข

```c
int temperature = 25;
float voltage = 3.3;

OLED_Clear(&oled);
OLED_DrawString(&oled, 0, 0, "Temp:", OLED_COLOR_WHITE);
OLED_DrawInt(&oled, 50, 0, temperature, OLED_COLOR_WHITE);

OLED_DrawString(&oled, 0, 20, "Volt:", OLED_COLOR_WHITE);
OLED_DrawFloat(&oled, 50, 20, voltage, 2, OLED_COLOR_WHITE);
OLED_Update(&oled);
```

### วาดรูปทรงเรขาคณิต

```c
OLED_Clear(&oled);

// วาดสี่เหลี่ยม
OLED_DrawRect(&oled, 10, 10, 40, 30, OLED_COLOR_WHITE);

// วาดวงกลม
OLED_FillCircle(&oled, 80, 25, 15, OLED_COLOR_WHITE);

// วาดเส้น
OLED_DrawLine(&oled, 0, 50, 127, 50, OLED_COLOR_WHITE);

OLED_Update(&oled);
```

---

## การใช้งานขั้นกลาง

### Double Buffering

Double buffering ช่วยให้ animation ลื่นไหล โดยวาดไปที่ back buffer แล้วค่อย swap

```c
// จัดสรร back buffer
uint8_t back_buffer[1024];  // สำหรับ 128x64

// เปิดใช้งาน double buffering
OLED_EnableDoubleBuffer(&oled, back_buffer);

while(1) {
    // วาดไปที่ back buffer
    OLED_Clear(&oled);
    OLED_DrawCircle(&oled, x, y, 10, OLED_COLOR_WHITE);
    
    // Swap buffers
    OLED_SwapBuffers(&oled);
    OLED_Update(&oled);
    
    // อัพเดทตำแหน่ง
    x++;
    if(x > 127) x = 0;
}
```

### Partial Update

อัพเดทเฉพาะส่วนที่เปลี่ยนแปลง ประหยัดเวลา

```c
// อัพเดทเฉพาะพื้นที่ x=0-63, y=0-31
OLED_UpdateArea(&oled, 0, 0, 64, 32);
```

### Progress Bar

```c
for(uint8_t i = 0; i <= 100; i += 10) {
    OLED_Clear(&oled);
    OLED_DrawProgressBar(&oled, 10, 25, 108, 15, i);
    OLED_Update(&oled);
    Delay_Ms(200);
}
```

### Graph Plotting

```c
int16_t data[20];
// ... เติมข้อมูล

OLED_Clear(&oled);
OLED_DrawGraph(&oled, 10, 10, 108, 44, data, 20, 0, 100);
OLED_Update(&oled);
```

---

## การใช้งานขั้นสูง

### Sprite Animation

```c
// กำหนด frames
const uint8_t frame1[32] = {...};
const uint8_t frame2[32] = {...};
const uint8_t frame3[32] = {...};

const uint8_t* frames[] = {frame1, frame2, frame3};

// สร้าง sprite
OLED_Sprite sprite;
OLED_CreateSprite(&sprite, 50, 20, 16, 16, frames, 3);

// Animation loop
while(1) {
    OLED_Clear(&oled);
    OLED_DrawSprite(&oled, &sprite, OLED_COLOR_WHITE);
    OLED_Update(&oled);
    
    OLED_NextSpriteFrame(&sprite);
    Delay_Ms(100);
}
```

### Menu System

```c
// Callback functions
void menu_action1(void) {
    // ทำอะไรบางอย่าง
}

// สร้างรายการเมนู
MenuItem menu_items[] = {
    OLED_MenuCreateAction("Start", menu_action1),
    OLED_MenuCreateAction("Stop", NULL),
    OLED_MenuCreateAction("Settings", NULL)
};

// เริ่มต้นเมนู
Menu main_menu;
OLED_MenuInit(&main_menu, menu_items, 3);
OLED_MenuSetTitle(&main_menu, "Main Menu");

// แสดงเมนู
OLED_Clear(&oled);
OLED_MenuDraw(&oled, &main_menu);
OLED_Update(&oled);

// Navigation (ใช้ปุ่ม)
// ปุ่มลง: OLED_MenuNext(&main_menu);
// ปุ่มขึ้น: OLED_MenuPrev(&main_menu);
// ปุ่มเลือก: OLED_MenuSelect(&main_menu);
```

### Scrolling Text

```c
uint16_t scroll_offset = 0;

while(1) {
    OLED_Clear(&oled);
    OLED_DrawScrollText(&oled, 0, 20, 128, 
                        "This is a long scrolling text message...", 
                        OLED_COLOR_WHITE, scroll_offset);
    OLED_Update(&oled);
    
    scroll_offset++;
    Delay_Ms(50);
}
```

---

## เทคนิคต่างๆ

### 1. การสร้าง Font จาก TTF

ใช้เครื่องมือ online เช่น:
- [The Dot Factory](http://www.eran.io/the-dot-factory-an-lcd-font-and-image-generator/)
- [LCD Assistant](http://en.radzio.dxp.pl/bitmap_converter/)

**ขั้นตอน:**
1. เลือก font และขนาด
2. Export เป็น C array
3. สร้าง `OLED_Font` structure

```c
const uint8_t my_font_data[] = {
    // ... font data
};

const OLED_Font MyFont = {
    .width = 8,
    .height = 16,
    .first_char = 32,
    .last_char = 126,
    .data = my_font_data
};
```

### 2. การแปลงรูปภาพเป็น Bitmap

ใช้ [LCD Image Converter](https://lcd-image-converter.riuson.com/)

**ขั้นตอน:**
1. เปิดรูปภาพ
2. ตั้งค่า: Monochrome, 1 bit per pixel
3. Export เป็น C array

```c
const uint8_t logo_data[] = {
    0xFF, 0x81, 0x81, 0x81, 0xFF, ...
};

const OLED_Bitmap logo = {
    .width = 32,
    .height = 32,
    .data = logo_data
};

// แสดงรูป
OLED_DrawBitmap(&oled, 48, 16, &logo, OLED_COLOR_WHITE);
```

### 3. Optimization เพื่อประหยัด RAM

**ใช้ Partial Update:**
```c
// แทนที่จะ OLED_Update(&oled);
OLED_UpdateArea(&oled, x, y, width, height);
```

**ใช้ Flash แทน RAM:**
```c
// เก็บ bitmap ใน Flash
const uint8_t __attribute__((section(".rodata"))) image_data[] = {...};
```

### 4. Power Saving

```c
// ปิดหน้าจอเมื่อไม่ใช้งาน
OLED_DisplayOn(&oled, 0);

// ลด contrast
OLED_SetContrast(&oled, 50);

// เปิดกลับ
OLED_DisplayOn(&oled, 1);
```

---

## การแก้ปัญหา

### ปัญหา: OLED ไม่แสดงผล

**สาเหตุที่เป็นไปได้:**
1. I2C address ไม่ถูกต้อง
2. ไม่มี pull-up resistor
3. สายเชื่อมต่อหลวม

**วิธีแก้:**
```c
// ทดสอบ I2C scan
uint8_t devices[10];
uint8_t count = I2C_Scan(devices, 10);

for(uint8_t i = 0; i < count; i++) {
    // แสดง address ที่พบ
    printf("Found: 0x%02X\n", devices[i]);
}
```

### ปัญหา: ข้อความแสดงผิดเพี้ยน

**สาเหตุ:** Font ไม่ถูกต้อง หรือ buffer ไม่ได้ clear

**วิธีแก้:**
```c
// Clear buffer ก่อนวาด
OLED_Clear(&oled);

// ตรวจสอบ font
OLED_SetFont(&oled, &Font_8x16);
```

### ปัญหา: Animation กระตุก

**สาเหตุ:** ไม่ใช้ double buffering

**วิธีแก้:**
```c
uint8_t back_buffer[1024];
OLED_EnableDoubleBuffer(&oled, back_buffer);

// ใช้ swap buffers
OLED_SwapBuffers(&oled);
OLED_Update(&oled);
```

---

## ตัวอย่างโปรเจค

### 1. Weather Station Display

```c
void display_weather(float temp, uint8_t humidity) {
    OLED_Clear(&oled);
    
    // หัวข้อ
    OLED_SetFont(&oled, &Font_8x16);
    OLED_DrawStringAlign(&oled, 64, 0, "Weather", OLED_COLOR_WHITE, OLED_ALIGN_CENTER);
    
    // อุณหภูมิ
    OLED_DrawString(&oled, 0, 20, "Temp:", OLED_COLOR_WHITE);
    OLED_DrawFloat(&oled, 50, 20, temp, 1, OLED_COLOR_WHITE);
    OLED_DrawString(&oled, 90, 20, "C", OLED_COLOR_WHITE);
    
    // ความชื้น
    OLED_DrawString(&oled, 0, 40, "Humid:", OLED_COLOR_WHITE);
    OLED_DrawInt(&oled, 50, 40, humidity, OLED_COLOR_WHITE);
    OLED_DrawString(&oled, 80, 40, "%", OLED_COLOR_WHITE);
    
    OLED_Update(&oled);
}
```

### 2. Simple Game (Pong)

```c
uint8_t ball_x = 64, ball_y = 32;
int8_t ball_dx = 2, ball_dy = 1;
uint8_t paddle_y = 24;

while(1) {
    OLED_Clear(&oled);
    
    // วาดลูกบอล
    OLED_FillCircle(&oled, ball_x, ball_y, 3, OLED_COLOR_WHITE);
    
    // วาด paddle
    OLED_FillRect(&oled, 5, paddle_y, 3, 16, OLED_COLOR_WHITE);
    
    // อัพเดทตำแหน่งลูกบอล
    ball_x += ball_dx;
    ball_y += ball_dy;
    
    // ตรวจสอบชน
    if(ball_y <= 0 || ball_y >= 63) ball_dy = -ball_dy;
    if(ball_x >= 127) ball_dx = -ball_dx;
    
    OLED_Update(&oled);
    Delay_Ms(30);
}
```

---

## License

MIT License - ใช้งานได้อย่างอิสระ

## ผู้พัฒนา

พัฒนาโดย Antigravity AI สำหรับ CH57x Community

## การสนับสนุน

หากพบปัญหาหรือต้องการความช่วยเหลือ กรุณาสร้าง Issue ใน GitHub repository

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)