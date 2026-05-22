# Rotary Encoder KY-040 Library สำหรับ CH57x

> **Library สำหรับควบคุม Rotary Encoder KY-040 แบบครบวงจร** - รองรับการหมุน (quadrature decoding), การกดปุ่ม, acceleration mode, และ callback system

## 📋 สารบัญ

- [ภาพรวม](#ภาพรวม)
- [ทฤษฎี Rotary Encoder](#ทฤษฎี-rotary-encoder)
- [Hardware Setup](#hardware-setup)
- [การติดตั้ง](#การติดตั้ง)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [ตัวอย่างการใช้งาน](#ตัวอย่างการใช้งาน)
- [เทคนิคขั้นสูง](#เทคนิคขั้นสูง)
- [Troubleshooting](#troubleshooting)

---

## ภาพรวม

Rotary Encoder Library เป็น library ที่ออกแบบมาเพื่อควบคุม KY-040 Rotary Encoder บน CH57x อย่างง่ายดาย โดยใช้ interrupt-based quadrature decoding สำหรับความแม่นยำสูง

### คุณสมบัติหลัก

- ✅ **Interrupt-based Quadrature Decoding** - ตรวจจับการหมุนแม่นยำ 100%
- ✅ **Direction Detection** - ตรวจจับทิศทาง CW/CCW
- ✅ **Position Counter** - นับตำแหน่งแบบ unlimited range
- ✅ **Button Support** - รองรับการกดปุ่ม (press, long press, double click)
- ✅ **Acceleration Mode** - หมุนเร็ว = ค่าเปลี่ยนเร็วขึ้น
- ✅ **Debouncing** - กำจัด noise อัตโนมัติ
- ✅ **Callback System** - Event-driven programming
- ✅ **Min/Max Limits** - จำกัดช่วงค่าได้
- ✅ **Arduino-style API** - ใช้งานง่าย

### การใช้งาน

Rotary Encoder สามารถใช้แทน:
- **ปุ่มกด** - เลือกเมนู, ปรับค่า
- **Potentiometer** - ควบคุม volume, brightness
- **เครื่องวัดระยะทาง** - วัดระยะทาง, มุม
- **Input Device** - รับ input จากผู้ใช้

### รองรับ Pins

KY-040 ใช้ 3 pins:
- **CLK (A)** - Encoder signal A (ต้องรองรับ EXTI)
- **DT (B)** - Encoder signal B (ต้องรองรับ EXTI)
- **SW** - Button switch (pin ใดก็ได้)

**Pins ที่รองรับ EXTI บน CH57x:**
- GPIOA: PA1, PA2
- GPIOC: PC0-PC7
- GPIOD: PD2-PD7

---

## ทฤษฎี Rotary Encoder

### หลักการทำงาน

Rotary Encoder KY-040 เป็น **Incremental Quadrature Encoder** ที่ใช้หลักการ:

#### 1. Quadrature Encoding

Encoder มี 2 output signals (A และ B) ที่มี phase shift 90°:

```
        ┌───┐   ┌───┐   ┌───┐
CLK (A) │   │   │   │   │   │
      ──┘   └───┘   └───┘   └──
    ┌───┐   ┌───┐   ┌───┐
DT (B) │   │   │   │   │   │
    ───┘   └───┘   └───┘   └───

    CW Rotation (Clockwise) →
```

```
    ┌───┐   ┌───┐   ┌───┐
CLK (A) │   │   │   │   │   │
    ────┘   └───┘   └───┘   └──
        ┌───┐   ┌───┐   ┌───┐
DT (B)  │   │   │   │   │   │
      ──┘   └───┘   └───┘   └──

    ← CCW Rotation (Counter-Clockwise)
```

#### 2. State Machine

Encoder มี 4 states (2-bit):
- State 0: A=0, B=0
- State 1: A=0, B=1
- State 2: A=1, B=0
- State 3: A=1, B=1

**Clockwise (CW):**
```
0 → 2 → 3 → 1 → 0 (repeat)
```

**Counter-Clockwise (CCW):**
```
0 → 1 → 3 → 2 → 0 (repeat)
```

#### 3. Gray Code

State transitions ใช้ Gray code (เปลี่ยน 1 bit ต่อครั้ง):
```
State 0: 00
State 1: 01  ← เปลี่ยน bit 0
State 3: 11  ← เปลี่ยน bit 1
State 2: 10  ← เปลี่ยน bit 0
```

### Quadrature Decoding Table

Library ใช้ lookup table สำหรับ decode:

| Previous | Current | Direction |
|----------|---------|-----------|
| 00       | 01      | CCW (-1)  |
| 00       | 10      | CW (+1)   |
| 01       | 00      | CW (+1)   |
| 01       | 11      | CCW (-1)  |
| 10       | 00      | CCW (-1)  |
| 10       | 11      | CW (+1)   |
| 11       | 01      | CW (+1)   |
| 11       | 10      | CCW (-1)  |

### Specifications

**KY-040 Typical Specs:**
- **Pulses per rotation:** 20 (detents)
- **Voltage:** 3.3V - 5V
- **Current:** < 10mA
- **Pull-up resistors:** Built-in (10kΩ)
- **Button:** Active LOW
- **Mechanical life:** > 30,000 rotations

---

## Hardware Setup

### วงจรต่อพื้นฐาน

#### แบบง่าย (Direct Connection)

```
KY-040 Module          CH57x
┌──────────┐          ┌─────────┐
│   CLK    │─────────→│  PC1    │
│   DT     │─────────→│  PC2    │
│   SW     │─────────→│  PC3    │
│   VCC    │←─────────│  3.3V   │
│   GND    │←─────────│  GND    │
└──────────┘          └─────────┘
```

> [!NOTE]
> KY-040 มี pull-up resistor ในตัวแล้ว ไม่ต้องต่อ resistor เพิ่ม

#### แบบมี LED Indicator (Optional)

```
KY-040 Module          CH57x
┌──────────┐          ┌─────────┐
│   CLK    │─────────→│  PC1    │
│   DT     │─────────→│  PC2    │
│   SW     │─────────→│  PC3    │
│   VCC    │←─────────│  3.3V   │
│   GND    │←─────────│  GND    │
└──────────┘          └─────────┘
                           │
                           │  PC0
                           ├──[R 220Ω]──→ LED+ ──→ GND
                           │
                           │  PD4
                           └──[R 220Ω]──→ LED+ ──→ GND
```

### รายการอุปกรณ์

| อุปกรณ์ | รายละเอียด | หมายเหตุ |
|---------|-----------|----------|
| KY-040 Module | Rotary Encoder | มี pull-up ในตัว |
| CH57x | Microcontroller | - |
| LED (Optional) | Status indicator | 3mm หรือ 5mm |
| Resistor (Optional) | 220Ω | สำหรับ LED |
| Jumper Wires | Female-Female | สำหรับต่อ |

### ข้อควรระวัง

> [!WARNING]
> **Pin Requirements**
> - CLK และ DT ต้องเป็น pins ที่รองรับ EXTI (external interrupt)
> - SW สามารถใช้ pin ใดก็ได้
> - ห้ามใช้ pins เดียวกันกับ PWM/Timer ที่กำลังใช้งาน

> [!CAUTION]
> **Noise Issues**
> - ใช้สาย jumper สั้นที่สุด (< 20cm)
> - หลีกเลี่ยงการวางสายใกล้แหล่งกำเนิด noise (motor, relay)
> - ถ้ามีปัญหา bouncing ให้เพิ่ม debounce time

---

## การติดตั้ง

### 1. คัดลอกไฟล์

คัดลอกไฟล์ทั้งหมดใน `/User/Lib/RotaryEncoder/` ไปยังโปรเจกต์:

```
/User/Lib/RotaryEncoder/
├── RotaryEncoder.h
├── RotaryEncoder.c
├── README.md
└── Examples/
    ├── 01_BasicRotation.c
    ├── 02_ButtonControl.c
    ├── 03_MenuNavigation.c
    ├── 04_VolumeControl.c
    ├── 05_DistanceMeasure.c
    ├── 06_AdvancedCallbacks.c
    └── 07_Acceleration.c
```

### 2. Include Header

```c
#include "SimpleHAL.h"
#include "RotaryEncoder.h"
```

### 3. เริ่มใช้งาน

```c
#include "SimpleHAL.h"
#include "RotaryEncoder.h"

RotaryEncoder encoder;

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    
    // Initialize encoder (CLK=PC1, DT=PC2, SW=PC3)
    Rotary_Init(&encoder, PC1, PC2, PC3);
    
    while(1) {
        int32_t pos = Rotary_GetPosition(&encoder);
        printf("Position: %ld\n", pos);
        Delay_Ms(100);
    }
}
```

---

## Quick Start

### ตัวอย่างที่ 1: อ่านตำแหน่ง

```c
#include "SimpleHAL.h"
#include "RotaryEncoder.h"

RotaryEncoder encoder;

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    
    // Initialize encoder
    Rotary_Init(&encoder, PC1, PC2, PC3);
    
    int32_t last_pos = 0;
    
    while(1) {
        int32_t pos = Rotary_GetPosition(&encoder);
        
        if (pos != last_pos) {
            printf("Position: %ld\n", pos);
            last_pos = pos;
        }
        
        Delay_Ms(10);
    }
}
```

### ตัวอย่างที่ 2: ตรวจจับทิศทาง

```c
// Get direction
RotaryDirection dir = Rotary_GetDirection(&encoder);

if (dir == ROTARY_DIR_CW) {
    printf("Clockwise\n");
} else if (dir == ROTARY_DIR_CCW) {
    printf("Counter-Clockwise\n");
}
```

### ตัวอย่างที่ 3: ใช้งานปุ่ม

```c
// Update button state (call in main loop)
Rotary_UpdateButton(&encoder);

// Check button press
if (Rotary_IsButtonPressed(&encoder)) {
    printf("Button pressed!\n");
}
```

### ตัวอย่างที่ 4: จำกัดช่วงค่า

```c
// Set limits (0-100)
Rotary_SetLimits(&encoder, 0, 100);
Rotary_SetPosition(&encoder, 50);

// Position จะถูกจำกัดอยู่ในช่วง 0-100
```

### ตัวอย่างที่ 5: Acceleration Mode

```c
// Enable acceleration
Rotary_SetAcceleration(&encoder, true);

// หมุนเร็ว = ค่าเปลี่ยนเร็วขึ้น (x2, x4, x8)
```

---

## API Reference

### Initialization Functions

#### `Rotary_Init()`
```c
void Rotary_Init(RotaryEncoder* encoder, uint8_t pin_clk, 
                 uint8_t pin_dt, uint8_t pin_sw);
```

เริ่มต้น Rotary Encoder

**Parameters:**
- `encoder` - Pointer to RotaryEncoder structure
- `pin_clk` - CLK pin (A) - ต้องรองรับ EXTI
- `pin_dt` - DT pin (B) - ต้องรองรับ EXTI
- `pin_sw` - SW pin (Button) - pin ใดก็ได้

**Example:**
```c
RotaryEncoder encoder;
Rotary_Init(&encoder, PC1, PC2, PC3);
```

---

#### `Rotary_Reset()`
```c
void Rotary_Reset(RotaryEncoder* encoder);
```

รีเซ็ต encoder state (ตำแหน่ง = 0, ล้าง state ทั้งหมด)

**Example:**
```c
Rotary_Reset(&encoder);
```

---

### Position Control Functions

#### `Rotary_GetPosition()`
```c
int32_t Rotary_GetPosition(RotaryEncoder* encoder);
```

อ่านตำแหน่งปัจจุบัน

**Returns:** ตำแหน่งปัจจุบัน (signed integer, unlimited range)

**Example:**
```c
int32_t pos = Rotary_GetPosition(&encoder);
printf("Position: %ld\n", pos);
```

---

#### `Rotary_SetPosition()`
```c
void Rotary_SetPosition(RotaryEncoder* encoder, int32_t position);
```

ตั้งค่าตำแหน่ง

**Parameters:**
- `position` - ตำแหน่งที่ต้องการ

**Example:**
```c
Rotary_SetPosition(&encoder, 0);   // Reset to 0
Rotary_SetPosition(&encoder, 50);  // Set to 50
```

---

#### `Rotary_GetDirection()`
```c
RotaryDirection Rotary_GetDirection(RotaryEncoder* encoder);
```

อ่านทิศทางการหมุนล่าสุด

**Returns:** 
- `ROTARY_DIR_CW` - Clockwise
- `ROTARY_DIR_CCW` - Counter-Clockwise
- `ROTARY_DIR_NONE` - No rotation

**Example:**
```c
RotaryDirection dir = Rotary_GetDirection(&encoder);
if (dir == ROTARY_DIR_CW) {
    printf("Rotating clockwise\n");
}
```

---

#### `Rotary_HasChanged()`
```c
bool Rotary_HasChanged(RotaryEncoder* encoder);
```

ตรวจสอบว่าตำแหน่งเปลี่ยนจากครั้งล่าสุดหรือไม่

**Returns:** `true` ถ้าตำแหน่งเปลี่ยน

**Example:**
```c
if (Rotary_HasChanged(&encoder)) {
    int32_t pos = Rotary_GetPosition(&encoder);
    printf("New position: %ld\n", pos);
}
```

---

#### `Rotary_SetLimits()`
```c
void Rotary_SetLimits(RotaryEncoder* encoder, int32_t min, int32_t max);
```

ตั้งค่าขอบเขตตำแหน่ง (min/max)

**Parameters:**
- `min` - ตำแหน่งต่ำสุด
- `max` - ตำแหน่งสูงสุด

**Note:** ตำแหน่งจะถูกจำกัดอยู่ในช่วง [min, max]

**Example:**
```c
// จำกัดตำแหน่งระหว่าง 0-100
Rotary_SetLimits(&encoder, 0, 100);
```

---

#### `Rotary_ClearLimits()`
```c
void Rotary_ClearLimits(RotaryEncoder* encoder);
```

ยกเลิกการจำกัดตำแหน่ง (unlimited range)

**Example:**
```c
Rotary_ClearLimits(&encoder);
```

---

### Button Control Functions

#### `Rotary_IsButtonPressed()`
```c
bool Rotary_IsButtonPressed(RotaryEncoder* encoder);
```

ตรวจสอบว่าปุ่มถูกกดอยู่หรือไม่

**Returns:** `true` ถ้าปุ่มถูกกด

**Example:**
```c
if (Rotary_IsButtonPressed(&encoder)) {
    printf("Button is pressed!\n");
}
```

---

#### `Rotary_WaitForButton()`
```c
void Rotary_WaitForButton(RotaryEncoder* encoder);
```

รอจนกว่าปุ่มจะถูกกดและปล่อย (blocking)

**Example:**
```c
printf("Press button to continue...\n");
Rotary_WaitForButton(&encoder);
printf("Button pressed!\n");
```

---

#### `Rotary_UpdateButton()`
```c
void Rotary_UpdateButton(RotaryEncoder* encoder);
```

อัปเดต button state (ต้องเรียกใน main loop)

**Note:** 
- ต้องเรียกฟังก์ชันนี้ใน main loop เพื่อตรวจจับ button events
- ควรเรียกบ่อยๆ (ทุก 10-50ms) สำหรับ debouncing ที่ดี
- จำเป็นสำหรับ long press และ double click detection

**Example:**
```c
while(1) {
    Rotary_UpdateButton(&encoder);
    // Your code here
    Delay_Ms(10);
}
```

---

### Advanced Settings Functions

#### `Rotary_SetDebounceTime()`
```c
void Rotary_SetDebounceTime(RotaryEncoder* encoder, uint16_t debounce_ms);
```

ตั้งค่าเวลา debounce

**Parameters:**
- `debounce_ms` - เวลา debounce (milliseconds)

**Note:** 
- ค่าเริ่มต้น: 5ms
- เพิ่มค่าถ้ามีปัญหา bouncing
- ลดค่าถ้าต้องการ response เร็วขึ้น

**Example:**
```c
Rotary_SetDebounceTime(&encoder, 10);  // 10ms debounce
```

---

#### `Rotary_SetAcceleration()`
```c
void Rotary_SetAcceleration(RotaryEncoder* encoder, bool enabled);
```

เปิด/ปิด acceleration mode

**Parameters:**
- `enabled` - `true` = เปิด, `false` = ปิด

**Note:**
- Acceleration mode: หมุนเร็ว = ค่าเปลี่ยนเร็วขึ้น (x2, x4, x8)
- เหมาะสำหรับการปรับค่าในช่วงกว้าง (เช่น 0-1000)

**Example:**
```c
Rotary_SetAcceleration(&encoder, true);  // เปิด acceleration
```

---

### Callback Functions

#### `Rotary_OnRotate()`
```c
void Rotary_OnRotate(RotaryEncoder* encoder, 
                     void (*callback)(int32_t position, RotaryDirection direction));
```

ตั้งค่า callback เมื่อหมุน encoder

**Parameters:**
- `callback` - ฟังก์ชันที่จะถูกเรียก (รับ position และ direction)

**Example:**
```c
void on_rotate(int32_t pos, RotaryDirection dir) {
    printf("Position: %ld, Direction: %d\n", pos, dir);
}

Rotary_OnRotate(&encoder, on_rotate);
```

---

#### `Rotary_OnButtonPress()`
```c
void Rotary_OnButtonPress(RotaryEncoder* encoder, void (*callback)(void));
```

ตั้งค่า callback เมื่อกดปุ่ม

**Example:**
```c
void on_press(void) {
    printf("Button pressed!\n");
}

Rotary_OnButtonPress(&encoder, on_press);
```

---

#### `Rotary_OnButtonRelease()`
```c
void Rotary_OnButtonRelease(RotaryEncoder* encoder, void (*callback)(void));
```

ตั้งค่า callback เมื่อปล่อยปุ่ม

**Example:**
```c
void on_release(void) {
    printf("Button released!\n");
}

Rotary_OnButtonRelease(&encoder, on_release);
```

---

#### `Rotary_OnButtonLongPress()`
```c
void Rotary_OnButtonLongPress(RotaryEncoder* encoder, void (*callback)(void));
```

ตั้งค่า callback เมื่อกดปุ่มค้าง (> 500ms)

**Example:**
```c
void on_long_press(void) {
    printf("Long press detected!\n");
    Rotary_SetPosition(&encoder, 0);  // Reset
}

Rotary_OnButtonLongPress(&encoder, on_long_press);
```

---

#### `Rotary_OnButtonDoubleClick()`
```c
void Rotary_OnButtonDoubleClick(RotaryEncoder* encoder, void (*callback)(void));
```

ตั้งค่า callback เมื่อดับเบิลคลิก (< 300ms)

**Example:**
```c
void on_double_click(void) {
    printf("Double click!\n");
}

Rotary_OnButtonDoubleClick(&encoder, on_double_click);
```

---

## ตัวอย่างการใช้งาน

### 1. Basic Rotation

```c
#include "SimpleHAL.h"
#include "RotaryEncoder.h"

RotaryEncoder encoder;

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Init(USART1, 115200);
    
    Rotary_Init(&encoder, PC1, PC2, PC3);
    
    int32_t last_pos = 0;
    
    while(1) {
        int32_t pos = Rotary_GetPosition(&encoder);
        
        if (pos != last_pos) {
            RotaryDirection dir = Rotary_GetDirection(&encoder);
            printf("Position: %ld [%s]\n", pos, 
                   dir == ROTARY_DIR_CW ? "CW" : "CCW");
            last_pos = pos;
        }
        
        Delay_Ms(10);
    }
}
```

### 2. Menu Navigation

```c
const char* menu[] = {"Settings", "Display", "Network", "About"};
#define MENU_COUNT 4

Rotary_Init(&encoder, PC1, PC2, PC3);
Rotary_SetLimits(&encoder, 0, MENU_COUNT - 1);

while(1) {
    Rotary_UpdateButton(&encoder);
    
    int8_t selection = Rotary_GetPosition(&encoder);
    
    if (Rotary_HasChanged(&encoder)) {
        printf("Selected: %s\n", menu[selection]);
    }
    
    if (Rotary_IsButtonPressed(&encoder)) {
        printf("Confirmed: %s\n", menu[selection]);
        Delay_Ms(500);  // Debounce
    }
    
    Delay_Ms(10);
}
```

### 3. Volume Control with PWM

```c
#define LED_PIN PC0

Rotary_Init(&encoder, PC1, PC2, PC3);
Rotary_SetLimits(&encoder, 0, 100);
Rotary_SetPosition(&encoder, 50);

PWM_Init(PWM2_CH3, 1000);  // 1kHz
PWM_Start(PWM2_CH3);

while(1) {
    int32_t volume = Rotary_GetPosition(&encoder);
    
    if (Rotary_HasChanged(&encoder)) {
        // Map 0-100 to 0-255 (PWM duty)
        uint16_t duty = (volume * 255) / 100;
        PWM_SetDuty(PWM2_CH3, duty);
        
        printf("Volume: %ld%%\n", volume);
    }
    
    Delay_Ms(10);
}
```

### 4. Distance Measurement

```c
#define PULSES_PER_ROTATION 20
#define WHEEL_DIAMETER_MM 50.0f
#define MM_PER_PULSE ((3.14159f * WHEEL_DIAMETER_MM) / PULSES_PER_ROTATION)

Rotary_Init(&encoder, PC1, PC2, PC3);
Rotary_ClearLimits(&encoder);  // Unlimited

while(1) {
    int32_t pulses = Rotary_GetPosition(&encoder);
    float distance_mm = pulses * MM_PER_PULSE;
    float distance_cm = distance_mm / 10.0f;
    
    if (Rotary_HasChanged(&encoder)) {
        printf("Distance: %.2f cm\n", distance_cm);
    }
    
    Delay_Ms(10);
}
```

### 5. Event-Driven with Callbacks

```c
void on_rotate(int32_t pos, RotaryDirection dir) {
    printf("Position: %ld [%s]\n", pos, 
           dir == ROTARY_DIR_CW ? "CW" : "CCW");
}

void on_button_press(void) {
    printf("Button pressed!\n");
}

void on_long_press(void) {
    printf("Long press - Reset!\n");
    Rotary_SetPosition(&encoder, 0);
}

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    
    Rotary_Init(&encoder, PC1, PC2, PC3);
    
    // Register callbacks
    Rotary_OnRotate(&encoder, on_rotate);
    Rotary_OnButtonPress(&encoder, on_button_press);
    Rotary_OnButtonLongPress(&encoder, on_long_press);
    
    while(1) {
        Rotary_UpdateButton(&encoder);  // Required!
        Delay_Ms(10);
    }
}
```

---

## เทคนิคขั้นสูง

### 1. Multi-Encoder Support

```c
// รองรับหลาย encoders พร้อมกัน
RotaryEncoder encoder1, encoder2;

Rotary_Init(&encoder1, PC1, PC2, PC3);
Rotary_Init(&encoder2, PD2, PD3, PD4);

while(1) {
    int32_t pos1 = Rotary_GetPosition(&encoder1);
    int32_t pos2 = Rotary_GetPosition(&encoder2);
    
    printf("Encoder1: %ld, Encoder2: %ld\n", pos1, pos2);
    
    Delay_Ms(100);
}
```

### 2. Custom Acceleration Tuning

```c
// ปรับ acceleration threshold ใน RotaryEncoder.h:
#define ROTARY_ACCEL_THRESHOLD_RPM  60   // เริ่ม accel ที่ 60 RPM

// หรือใช้ acceleration factor ที่กำหนดเอง:
if (encoder.acceleration_enabled) {
    uint32_t rpm = calculate_rpm();
    
    if (rpm > 200) {
        encoder.acceleration_factor = 16;  // Very fast
    } else if (rpm > 100) {
        encoder.acceleration_factor = 8;
    } else if (rpm > 50) {
        encoder.acceleration_factor = 4;
    } else {
        encoder.acceleration_factor = 2;
    }
}
```

### 3. Debounce Time Optimization

```c
// สำหรับ encoder คุณภาพดี - ลด debounce
Rotary_SetDebounceTime(&encoder, 2);  // 2ms (faster response)

// สำหรับ encoder คุณภาพต่ำ - เพิ่ม debounce
Rotary_SetDebounceTime(&encoder, 15);  // 15ms (more stable)

// Dynamic debounce based on speed
if (rotation_speed > threshold) {
    Rotary_SetDebounceTime(&encoder, 2);  // Fast
} else {
    Rotary_SetDebounceTime(&encoder, 5);  // Normal
}
```

### 4. State Persistence (EEPROM)

```c
#include "SimpleFlash.h"

// Save position to flash
void save_position(void) {
    int32_t pos = Rotary_GetPosition(&encoder);
    Flash_WriteWord(FLASH_USER_START, pos);
}

// Load position from flash
void load_position(void) {
    int32_t pos = Flash_ReadWord(FLASH_USER_START);
    Rotary_SetPosition(&encoder, pos);
}

// Auto-save every 5 seconds
uint32_t last_save = 0;
while(1) {
    if ((Get_CurrentMs() - last_save) >= 5000) {
        save_position();
        last_save = Get_CurrentMs();
    }
    Delay_Ms(10);
}
```

### 5. Interrupt Optimization

```c
// ลด interrupt overhead ด้วย debouncing ใน ISR
void Rotary_ProcessRotation(RotaryEncoder* encoder) {
    uint32_t current_time = Get_CurrentMs();
    
    // Quick debounce check
    if ((current_time - encoder->last_change_time) < encoder->debounce_ms) {
        return;  // Ignore bouncing
    }
    
    // Process rotation...
    encoder->last_change_time = current_time;
}
```

### 6. Memory Optimization

```c
// ใช้ global encoder แทน local (ประหยัด stack)
RotaryEncoder g_encoder;

// ใช้ uint8_t สำหรับค่าเล็กๆ
typedef struct {
    uint8_t min;
    uint8_t max;
    uint8_t current;
} SmallRange;

SmallRange range = {0, 100, 50};

// Map encoder position to small range
uint8_t value = (Rotary_GetPosition(&encoder) % 101);
```

### 7. Advanced Button Patterns

```c
// Triple click detection
static uint8_t click_count = 0;
static uint32_t last_click_time = 0;

if (button_pressed && !last_button_state) {
    uint32_t now = Get_CurrentMs();
    
    if ((now - last_click_time) < 300) {
        click_count++;
    } else {
        click_count = 1;
    }
    
    last_click_time = now;
    
    if (click_count == 3) {
        printf("Triple click!\n");
        click_count = 0;
    }
}
```

### 8. Velocity Calculation

```c
// คำนวณความเร็วการหมุน (RPM)
typedef struct {
    int32_t last_position;
    uint32_t last_time;
    float rpm;
} VelocityTracker;

void update_velocity(VelocityTracker* tracker) {
    int32_t pos = Rotary_GetPosition(&encoder);
    uint32_t now = Get_CurrentMs();
    
    int32_t delta_pos = pos - tracker->last_position;
    uint32_t delta_time = now - tracker->last_time;
    
    if (delta_time > 0) {
        // RPM = (pulses/sec) * (60 sec/min) / (pulses/rotation)
        float pulses_per_sec = (delta_pos * 1000.0f) / delta_time;
        tracker->rpm = (pulses_per_sec * 60.0f) / 20.0f;
    }
    
    tracker->last_position = pos;
    tracker->last_time = now;
}
```

---

## Troubleshooting

### การหมุนไม่ตรง / กระตุก

**สาเหตุที่เป็นไปได้:**

1. **Bouncing**
   ```c
   // เพิ่ม debounce time
   Rotary_SetDebounceTime(&encoder, 10);  // 10ms
   ```

2. **Noise จากสายสัญญาณ**
   - ใช้สาย jumper สั้นที่สุด
   - ห้ามวางสายใกล้ motor/relay
   - เพิ่ม capacitor 100nF ระหว่าง CLK/DT กับ GND

3. **Interrupt conflicts**
   ```c
   // ตรวจสอบว่าไม่มี interrupt อื่นใช้ pins เดียวกัน
   // ห้ามใช้ PC1, PC2 กับ EXTI อื่น
   ```

### ปุ่มไม่ทำงาน

**สาเหตุ:**

1. **ลืมเรียก UpdateButton()**
   ```c
   // ✗ Wrong
   while(1) {
       // ไม่มี Rotary_UpdateButton()!
   }
   
   // ✓ Correct
   while(1) {
       Rotary_UpdateButton(&encoder);
       Delay_Ms(10);
   }
   ```

2. **Debounce ไม่เพียงพอ**
   ```c
   // เพิ่ม delay หลังตรวจจับปุ่ม
   if (Rotary_IsButtonPressed(&encoder)) {
       Delay_Ms(50);  // Debounce
       // Handle button
   }
   ```

### Long Press / Double Click ไม่ทำงาน

**สาเหตุ:** ไม่ได้เรียก `Rotary_UpdateButton()` บ่อยพอ

```c
// ✗ Wrong - เรียกช้าเกินไป
while(1) {
    Rotary_UpdateButton(&encoder);
    Delay_Ms(100);  // ช้าเกินไป!
}

// ✓ Correct - เรียกบ่อยๆ
while(1) {
    Rotary_UpdateButton(&encoder);
    Delay_Ms(10);  // ดี!
}
```

### Acceleration ไม่ทำงาน

**สาเหตุ:**

1. **ไม่ได้เปิด acceleration**
   ```c
   Rotary_SetAcceleration(&encoder, true);  // ต้องเปิด!
   ```

2. **หมุนช้าเกินไป**
   - Acceleration เริ่มทำงานที่ > 60 RPM
   - ลองหมุนเร็วขึ้น

3. **ช่วงค่าแคบเกินไป**
   ```c
   // Acceleration เหมาะกับช่วงกว้าง
   Rotary_SetLimits(&encoder, 0, 1000);  // ดี!
   // ไม่เหมาะกับช่วงแคบ
   Rotary_SetLimits(&encoder, 0, 10);    // ไม่เห็นผล
   ```

### ตำแหน่งเพิ่ม/ลดผิด

**สาเหตุ:** ต่อสาย CLK/DT สลับกัน

```c
// ถ้าทิศทางกลับกัน ให้สลับ CLK กับ DT
// ✗ Wrong direction
Rotary_Init(&encoder, PC1, PC2, PC3);

// ✓ Swap CLK and DT
Rotary_Init(&encoder, PC2, PC1, PC3);  // สลับ!
```

### Callback ไม่ถูกเรียก

**สาเหตุ:**

1. **ลืมเรียก UpdateButton() สำหรับ button callbacks**
   ```c
   while(1) {
       Rotary_UpdateButton(&encoder);  // จำเป็น!
       Delay_Ms(10);
   }
   ```

2. **Callback function signature ผิด**
   ```c
   // ✗ Wrong
   void on_rotate(void) { }
   
   // ✓ Correct
   void on_rotate(int32_t pos, RotaryDirection dir) { }
   ```

### หน่วยความจำไม่พอ

**แก้ไข:**

```c
// ลดจำนวน encoder instances
static RotaryEncoder* g_encoders[4];  // ลดจาก 8 เป็น 4

// ใช้ global encoder
RotaryEncoder g_encoder;  // แทน local variable

// ปิด features ที่ไม่ใช้
encoder.acceleration_enabled = false;
```

---

## License

MIT License - ใช้งานได้ฟรี

## ผู้พัฒนา

CH57x Library Team

## Version

**v1.0** - 2025-12-22
- Initial release
- Quadrature decoding
- Button support
- Acceleration mode
- Callback system

---

## ดูเพิ่มเติม

- [Examples](Examples/) - ตัวอย่างการใช้งาน 7 แบบ
- [SimpleHAL Documentation](../SimpleHAL/README.md)
- [CH57x Datasheet](https://www.wch-ic.com/products/CH57x.html)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)