# SimplePIR Library - คู่มือการใช้งานฉบับสมบูรณ์

**Version:** 1.0  
**Date:** 2025-12-21  
**Author:** CH57x SimplePIR Team

---

## สารบัญ

1. [ภาพรวม PIR Sensors](#1-ภาพรวม-pir-sensors)
2. [การต่อวงจร](#2-การต่อวงจร)
3. [การใช้งานขั้นพื้นฐาน](#3-การใช้งานขั้นพื้นฐาน)
4. [เทคนิคขั้นกลาง](#4-เทคนิคขั้นกลาง)
5. [เทคนิคขั้นสูง](#5-เทคนิคขั้นสูง)
6. [LED Interference Prevention](#6-led-interference-prevention)
7. [Continuous Motion Detection](#7-continuous-motion-detection)
8. [API Reference](#8-api-reference)
9. [Troubleshooting](#9-troubleshooting)
10. [โปรเจกต์ตัวอย่าง](#10-โปรเจกต์ตัวอย่าง)

---

## 1. ภาพรวม PIR Sensors

### 1.1 PIR คืออะไร?

**PIR (Passive Infrared)** เป็นเซ็นเซอร์ตรวจจับการเคลื่อนไหวโดยการรับรังสีอินฟราเรดจากวัตถุที่มีความร้อน (เช่น คน สัตว์)

**หลักการทำงาน:**
- ตรวจจับการเปลี่ยนแปลงของรังสี IR ในบริเวณที่ตรวจจับ
- เมื่อมีวัตถุที่มีความร้อนเคลื่อนที่ผ่าน จะเกิดการเปลี่ยนแปลงของสัญญาณ
- Output เป็น digital signal (HIGH/LOW)

### 1.2 NS312 vs RCWL-0516

#### NS312 (PIR Sensor)
- **เทคโนโลยี:** Passive Infrared
- **แรงดัน:** 3.3V - 5V
- **ระยะตรวจจับ:** ~7 เมตร
- **มุมตรวจจับ:** ~120 องศา
- **เวลา delay:** ปรับได้ (2-200 วินาที)
- **ข้อดี:** ราคาถูก, ใช้พลังงานน้อย, ไม่รบกวน RF
- **ข้อเสีย:** ตรวจจับผ่านกระจกไม่ได้, ไวต่ออุณหภูมิ

#### RCWL-0516 (Microwave Radar)
- **เทคโนโลยี:** Microwave Doppler Radar (3.18 GHz)
- **แรงดัน:** 4-28V (แนะนำ 5V)
- **ระยะตรวจจับ:** ~5-7 เมตร
- **มุมตรวจจับ:** ~360 องศา
- **เวลา delay:** ~2 วินาที (fixed)
- **ข้อดี:** ตรวจจับผ่านกระจก/พลาสติกได้, ไม่ไวต่ออุณหภูมิ
- **ข้อเสีย:** ไวต่อ EMI, อาจตรวจจับผิดพลาดจาก LED/relay

### 1.3 การเลือกใช้

| Application | NS312 | RCWL-0516 |
|-------------|-------|-----------|
| Auto-light (indoor) | ✅ แนะนำ | ⚠️ อาจมี false trigger |
| Security (outdoor) | ✅ | ✅ |
| ตรวจจับผ่านกระจก | ❌ | ✅ |
| ประหยัดพลังงาน | ✅ | ⚠️ ใช้พลังงานมากกว่า |
| ใช้กับ LED/relay | ✅ | ⚠️ ต้องใช้ blanking |

---

## 2. การต่อวงจร

### 2.1 NS312 Connection

```
NS312          CH57x
─────          ────────
VCC    ───────  3.3V/5V
GND    ───────  GND
OUT    ───────  PC4 (หรือ pin อื่นๆ)
```

**หมายเหตุ:**
- NS312 มี trimmer 2 ตัว: Sensitivity (Sx) และ Time Delay (Tx)
- ปรับ Sx เพื่อเพิ่ม/ลดระยะตรวจจับ
- ปรับ Tx เพื่อเพิ่ม/ลดเวลา delay หลังตรวจพบ

### 2.2 RCWL-0516 Connection

```
RCWL-0516      CH57x
─────────      ────────
VIN    ───────  5V (แนะนำ)
GND    ───────  GND
OUT    ───────  PC4 (หรือ pin อื่นๆ)
3V3    ───────  (ไม่ต่อ - เป็น output 3.3V)
```

**หมายเหตุ:**
- RCWL-0516 มี pads สำหรับปรับแต่ง:
  - `C-TM`: ต่อ GND เพื่อเปลี่ยนเป็น repeat trigger mode
  - `R-GN`: ปรับ detection range (ต่อ resistor)
  - `R-CDS`: ต่อ LDR เพื่อทำงานเฉพาะตอนมืด

### 2.3 การต่อวงจรสมบูรณ์ (Auto-Light System)

```
                    CH57x
                    ────────
PIR OUT  ──────────  PC4 (Input)
LED      ──────────  PC0 (PWM Output)
GND      ──────────  GND
VCC      ──────────  3.3V/5V
```

**Schematic:**
```
         +5V
          │
          ├─── PIR VCC
          │
          ├─── LED (ผ่าน resistor)
          │
         GND
```

---

## 3. การใช้งานขั้นพื้นฐาน

### 3.1 Initialization

```c
#include "SimpleHAL.h"
#include "SimplePIR.h"

int main(void) {
    // เริ่มต้นระบบ
    SystemCoreClockUpdate();
    Timer_Init();  // ต้องเรียกก่อนใช้ PIR
    
    // เริ่มต้น PIR
    PIR_Instance* pir = PIR_Init(PC4);
    if (pir == NULL) {
        // Error handling
        while(1);
    }
    
    while(1) {
        PIR_Update(pir);  // ต้องเรียกใน main loop
        Delay_Ms(10);
    }
}
```

### 3.2 Basic Detection

```c
// อ่านค่า PIR
bool motion = PIR_Read(pir);
if (motion) {
    printf("Motion detected!\r\n");
}

// หรือใช้ callback
void on_motion(void) {
    printf("Motion detected!\r\n");
}

PIR_SetCallback(pir, on_motion, NULL);
```

### 3.3 Configuration

```c
// ตั้งค่า debounce
PIR_SetDebounce(pir, 200);  // 200ms

// ตั้งค่า timeout
PIR_SetTimeout(pir, 10000);  // 10 วินาที

// ตั้งค่า filter level
PIR_SetFilterLevel(pir, PIR_FILTER_MEDIUM);
```

---

## 4. เทคนิคขั้นกลาง

### 4.1 Debouncing

**ปัญหา:** สัญญาณ PIR อาจกระเด้ง (bounce) ทำให้ตรวจจับผิดพลาด

**วิธีแก้:** ใช้ debounce เพื่อรอให้สัญญาณคงที่

```c
// ตั้งค่า debounce 150ms
PIR_SetDebounce(pir, 150);

// สัญญาณต้องคงที่เป็นเวลา 150ms ก่อนจะยอมรับ
```

**การเลือก Debounce Time:**
- **50-100ms:** ตอบสนองเร็ว แต่อาจมี false trigger
- **150-200ms:** แนะนำสำหรับการใช้งานทั่วไป
- **300-500ms:** ความแม่นยำสูง แต่ตอบสนองช้า

### 4.2 Callback System

```c
void on_motion_start(void) {
    printf("Motion started!\r\n");
    // เปิดไฟ
}

void on_motion_end(void) {
    printf("Motion ended\r\n");
    // ปิดไฟ
}

void on_motion_active(void) {
    printf("Motion active\r\n");
    // ต่ออายุไฟ
}

PIR_SetCallback(pir, on_motion_start, on_motion_end);
PIR_SetActiveCallback(pir, on_motion_active);
```

### 4.3 Multiple Sensors

```c
// สร้าง PIR หลายตัว
PIR_Instance* pir1 = PIR_Init(PC4);
PIR_Instance* pir2 = PIR_Init(PC5);
PIR_Instance* pir3 = PIR_Init(PC6);

// อัพเดททั้งหมดพร้อมกัน
while(1) {
    PIR_UpdateAll();  // อัพเดททุกตัว
    Delay_Ms(10);
}

// หรืออัพเดทแยก
while(1) {
    PIR_Update(pir1);
    PIR_Update(pir2);
    PIR_Update(pir3);
    Delay_Ms(10);
}
```

---

## 5. เทคนิคขั้นสูง

### 5.1 Advanced Filtering

Library รองรับ 4 ระดับของ filter:

```c
typedef enum {
    PIR_FILTER_NONE = 0,     // ไม่มีการกรอง
    PIR_FILTER_LOW,          // 2 samples
    PIR_FILTER_MEDIUM,       // 4 samples (แนะนำ)
    PIR_FILTER_HIGH          // 8 samples
} PIR_FilterLevel;
```

**การทำงาน:**
- ใช้ **Moving Average Filter**
- เก็บค่าล่าสุด N ตัว (ตาม filter level)
- คำนวณค่าเฉลี่ย
- ถ้าค่าเฉลี่ย >= 50% → HIGH

**ตัวอย่าง:**

```c
// ตั้งค่า filter level
PIR_SetFilterLevel(pir, PIR_FILTER_HIGH);

// อ่านค่าที่กรองแล้ว
uint8_t filtered = PIR_GetFilteredValue(pir);
printf("Filter value: %d/255\r\n", filtered);
```

**การเลือก Filter Level:**

| Level | Samples | Response Time | Accuracy | Use Case |
|-------|---------|---------------|----------|----------|
| NONE | 1 | เร็วที่สุด | ต่ำ | Testing, debugging |
| LOW | 2 | เร็ว | ปานกลาง | Gaming, interactive |
| MEDIUM | 4 | ปานกลาง | ดี | **แนะนำทั่วไป** |
| HIGH | 8 | ช้า | สูงสุด | Security, critical |

### 5.2 State Machine

PIR library ใช้ state machine ภายใน:

```
IDLE ──────────────────────────────────┐
  │                                    │
  │ Motion detected                    │
  ▼                                    │
MOTION_DETECTED                        │
  │                                    │
  │ Still moving                       │
  ▼                                    │
MOTION_ACTIVE ◄────────┐               │
  │                    │               │
  │ Stopped            │ Motion again  │
  ▼                    │               │
MOTION_TIMEOUT ────────┘               │
  │                                    │
  │ Timeout expired                    │
  ▼                                    │
MOTION_END ────────────────────────────┘
```

**การใช้งาน:**

```c
PIR_State state = PIR_GetState(pir);

switch(state) {
    case PIR_STATE_IDLE:
        // ไม่มีการเคลื่อนไหว
        break;
        
    case PIR_STATE_MOTION_DETECTED:
        // ตรวจพบการเคลื่อนไหวครั้งแรก
        break;
        
    case PIR_STATE_MOTION_ACTIVE:
        // กำลังมีการเคลื่อนไหว
        break;
        
    case PIR_STATE_MOTION_TIMEOUT:
        // รอ timeout
        break;
        
    case PIR_STATE_MOTION_END:
        // การเคลื่อนไหวสิ้นสุด
        break;
}
```

### 5.3 Advanced Configuration

```c
PIR_Config config = {
    .pin = PC4,
    .debounce_ms = 150,
    .blanking_window_ms = 250,
    .timeout_ms = 10000,
    .filter_level = PIR_FILTER_MEDIUM,
    .mode = PIR_MODE_CONTINUOUS,
    .led_protection_enabled = true
};

PIR_Instance* pir = PIR_InitWithConfig(&config);
```

---

## 6. LED Interference Prevention

### 6.1 ปัญหา

เมื่อ LED หรืออุปกรณ์ไฟฟ้าเปิด/ปิด จะเกิด **EMI (Electromagnetic Interference)** ที่อาจทำให้ PIR (โดยเฉพาะ RCWL-0516) ตรวจจับผิดพลาด

**สาเหตุ:**
- การเปลี่ยนแปลงกระแสไฟฟ้าอย่างรวดเร็ว
- สนามแม่เหล็กไฟฟ้าจากการสวิตช์
- PWM switching noise
- Relay coil switching

### 6.2 วิธีแก้: Blanking Window

**Blanking Window** คือช่วงเวลาที่ PIR จะไม่อ่านค่า หลังจาก LED เปิด/ปิด

```c
// เปิดใช้งาน LED protection
PIR_EnableLEDProtection(pir, true);

// ตั้งค่า blanking window
PIR_SetBlankingWindow(pir, 250);  // 250ms

// เมื่อ LED เปิด/ปิด
digitalWrite(LED_PIN, HIGH);
PIR_TriggerBlankingWindow(pir);  // เริ่ม blanking
```

### 6.3 การทำงาน

```
Time ──────────────────────────────────────►
      │         │                    │
      │ LED ON  │  Blanking Window   │ Normal
      ▼         ▼                    ▼
PIR:  ████████  ░░░░░░░░░░░░░░░░░░  ████████
      Reading   Ignored              Reading
```

### 6.4 การเลือก Blanking Time

| Device | Recommended Blanking |
|--------|---------------------|
| LED ธรรมดา | 100-200ms |
| LED PWM | 200-300ms |
| LED Strip (WS2812) | 300-500ms |
| Relay | 200-400ms |
| Motor | 300-500ms |

### 6.5 ตัวอย่างการใช้งาน

```c
void turn_on_led(void) {
    digitalWrite(LED_PIN, HIGH);
    PIR_TriggerBlankingWindow(pir);
    printf("LED ON (blanking started)\r\n");
}

void turn_off_led(void) {
    digitalWrite(LED_PIN, LOW);
    PIR_TriggerBlankingWindow(pir);
    printf("LED OFF (blanking started)\r\n");
}

// ตรวจสอบว่าอยู่ใน blanking หรือไม่
if (PIR_IsInBlankingWindow(pir)) {
    printf("PIR is blanked\r\n");
}
```

### 6.6 เทคนิคเพิ่มเติม

**1. Hardware Filtering:**
```c
// เพิ่ม capacitor ที่ PIR output
// 100nF - 1uF ceramic capacitor
```

**2. Software Filtering:**
```c
// ใช้ filter level สูง
PIR_SetFilterLevel(pir, PIR_FILTER_HIGH);

// เพิ่ม debounce time
PIR_SetDebounce(pir, 300);
```

**3. Shielding:**
- ใช้สาย shielded cable สำหรับ PIR
- แยก ground plane ระหว่าง digital และ analog
- ใช้ ferrite bead ที่สาย power

---

## 7. Continuous Motion Detection

### 7.1 ภาพรวม

**Continuous Motion Detection** คือการตรวจจับการเคลื่อนไหวอย่างต่อเนื่อง พร้อมติดตามระยะเวลา

**Use Cases:**
- Auto-light system: เปิดไฟเมื่อมีคน ปิดเมื่อไม่มีคน
- Presence detection: ตรวจจับว่ามีคนอยู่ในห้อง
- Security system: บันทึกระยะเวลาการเคลื่อนไหว
- Energy saving: ปิดอุปกรณ์เมื่อไม่มีคน

### 7.2 การทำงาน

```c
// ตั้งค่าเป็น continuous mode
PIR_SetMode(pir, PIR_MODE_CONTINUOUS);

// ตั้งค่า timeout
PIR_SetTimeout(pir, 5000);  // 5 วินาที
```

**State Flow:**
```
1. IDLE: รอการเคลื่อนไหว
   ↓ (ตรวจพบการเคลื่อนไหว)
2. MOTION_DETECTED: ตรวจพบครั้งแรก
   ↓ (ยังมีการเคลื่อนไหว)
3. MOTION_ACTIVE: กำลังเคลื่อนไหว
   ↓ (หยุดเคลื่อนไหว)
4. MOTION_TIMEOUT: รอ timeout
   ↓ (timeout หมด)
5. MOTION_END: สิ้นสุด
   ↓
6. กลับไป IDLE
```

### 7.3 Callbacks

```c
void on_motion_start(void) {
    printf("Motion started!\r\n");
    // เปิดไฟ
}

void on_motion_active(void) {
    printf("Motion active\r\n");
    // ต่ออายุไฟ (reset timeout)
}

void on_motion_end(void) {
    uint32_t duration = PIR_GetMotionDuration(pir);
    printf("Motion ended (duration: %lu ms)\r\n", duration);
    // ปิดไฟ
}

PIR_SetCallback(pir, on_motion_start, on_motion_end);
PIR_SetActiveCallback(pir, on_motion_active);
```

### 7.4 Tracking Motion Duration

```c
// อ่านระยะเวลาการเคลื่อนไหว
uint32_t duration = PIR_GetMotionDuration(pir);
printf("Motion duration: %lu ms\r\n", duration);

// อ่านเวลาที่ผ่านไปนับจากการเคลื่อนไหวครั้งล่าสุด
uint32_t since_last = PIR_GetTimeSinceLastMotion(pir);
printf("Time since last motion: %lu ms\r\n", since_last);
```

### 7.5 Timeout Configuration

**การเลือก Timeout:**

| Application | Timeout | เหตุผล |
|-------------|---------|--------|
| Auto-light (bathroom) | 1-2 min | ใช้เวลาสั้น |
| Auto-light (living room) | 5-10 min | อยู่นานกว่า |
| Presence detection | 30-60 min | ตรวจจับการอยู่ในห้อง |
| Security | 10-30 sec | ต้องการความไว |

```c
// ตัวอย่าง: Auto-light สำหรับห้องน้ำ
PIR_SetTimeout(pir, 120000);  // 2 นาที

// ตัวอย่าง: Presence detection
PIR_SetTimeout(pir, 1800000);  // 30 นาที
```

---

## 8. API Reference

### 8.1 Initialization

#### `PIR_Init()`
```c
PIR_Instance* PIR_Init(uint8_t pin);
```
เริ่มต้น PIR sensor ด้วยค่า default

**Parameters:**
- `pin`: GPIO pin (PC0-PC7, PD2-PD7)

**Returns:**
- ตัวชี้ไปยัง PIR instance หรือ NULL ถ้าเต็ม

**Example:**
```c
PIR_Instance* pir = PIR_Init(PC4);
```

#### `PIR_InitWithConfig()`
```c
PIR_Instance* PIR_InitWithConfig(PIR_Config* config);
```
เริ่มต้น PIR พร้อม configuration

**Parameters:**
- `config`: ตัวชี้ไปยัง configuration structure

**Returns:**
- ตัวชี้ไปยัง PIR instance หรือ NULL ถ้าเต็ม

**Example:**
```c
PIR_Config cfg = {
    .pin = PC4,
    .debounce_ms = 150,
    .blanking_window_ms = 200,
    .timeout_ms = 5000,
    .filter_level = PIR_FILTER_MEDIUM,
    .mode = PIR_MODE_CONTINUOUS,
    .led_protection_enabled = true
};
PIR_Instance* pir = PIR_InitWithConfig(&cfg);
```

### 8.2 Configuration Functions

#### `PIR_SetDebounce()`
```c
void PIR_SetDebounce(PIR_Instance* pir, uint16_t ms);
```
ตั้งค่า debounce time

**Parameters:**
- `pir`: PIR instance
- `ms`: เวลา debounce (milliseconds)

**Default:** 150ms

#### `PIR_SetBlankingWindow()`
```c
void PIR_SetBlankingWindow(PIR_Instance* pir, uint16_t ms);
```
ตั้งค่า blanking window

**Parameters:**
- `pir`: PIR instance
- `ms`: เวลา blanking (milliseconds)

**Default:** 200ms

#### `PIR_SetTimeout()`
```c
void PIR_SetTimeout(PIR_Instance* pir, uint16_t ms);
```
ตั้งค่า timeout สำหรับ continuous detection

**Parameters:**
- `pir`: PIR instance
- `ms`: เวลา timeout (milliseconds)

**Default:** 5000ms

#### `PIR_SetFilterLevel()`
```c
void PIR_SetFilterLevel(PIR_Instance* pir, PIR_FilterLevel level);
```
ตั้งค่า filter level

**Parameters:**
- `pir`: PIR instance
- `level`: `PIR_FILTER_NONE`, `PIR_FILTER_LOW`, `PIR_FILTER_MEDIUM`, `PIR_FILTER_HIGH`

**Default:** `PIR_FILTER_MEDIUM`

#### `PIR_SetMode()`
```c
void PIR_SetMode(PIR_Instance* pir, PIR_Mode mode);
```
ตั้งค่า detection mode

**Parameters:**
- `pir`: PIR instance
- `mode`: `PIR_MODE_SINGLE`, `PIR_MODE_CONTINUOUS`

**Default:** `PIR_MODE_CONTINUOUS`

#### `PIR_EnableLEDProtection()`
```c
void PIR_EnableLEDProtection(PIR_Instance* pir, bool enabled);
```
เปิด/ปิด LED interference protection

**Parameters:**
- `pir`: PIR instance
- `enabled`: `true` = เปิด, `false` = ปิด

**Default:** `false`

### 8.3 Callback Functions

#### `PIR_SetCallback()`
```c
void PIR_SetCallback(PIR_Instance* pir, 
                     void (*on_start)(void), 
                     void (*on_end)(void));
```
ตั้งค่า callbacks สำหรับ motion start/end

**Parameters:**
- `pir`: PIR instance
- `on_start`: Callback เมื่อเริ่มตรวจพบ
- `on_end`: Callback เมื่อสิ้นสุด

#### `PIR_SetActiveCallback()`
```c
void PIR_SetActiveCallback(PIR_Instance* pir, 
                           void (*on_active)(void));
```
ตั้งค่า callback สำหรับ motion active

**Parameters:**
- `pir`: PIR instance
- `on_active`: Callback ขณะมีการเคลื่อนไหว

### 8.4 Core Functions

#### `PIR_Update()`
```c
void PIR_Update(PIR_Instance* pir);
```
อัพเดทสถานะ PIR (ต้องเรียกใน main loop)

**Parameters:**
- `pir`: PIR instance

**Note:** ต้องเรียกฟังก์ชันนี้ใน main loop เพื่อให้ library ทำงาน

#### `PIR_Read()`
```c
bool PIR_Read(PIR_Instance* pir);
```
อ่านค่า PIR (หลังผ่าน debounce และ filter)

**Parameters:**
- `pir`: PIR instance

**Returns:**
- `true` = ตรวจพบการเคลื่อนไหว
- `false` = ไม่พบ

#### `PIR_ReadRaw()`
```c
bool PIR_ReadRaw(PIR_Instance* pir);
```
อ่านค่า PIR แบบ raw (ไม่ผ่าน debounce/filter)

**Parameters:**
- `pir`: PIR instance

**Returns:**
- `true` = HIGH
- `false` = LOW

### 8.5 State Query Functions

#### `PIR_GetState()`
```c
PIR_State PIR_GetState(PIR_Instance* pir);
```
อ่านสถานะปัจจุบัน

**Returns:**
- `PIR_STATE_IDLE`
- `PIR_STATE_MOTION_DETECTED`
- `PIR_STATE_MOTION_ACTIVE`
- `PIR_STATE_MOTION_TIMEOUT`
- `PIR_STATE_MOTION_END`

#### `PIR_IsMotionDetected()`
```c
bool PIR_IsMotionDetected(PIR_Instance* pir);
```
ตรวจสอบว่ามีการเคลื่อนไหวหรือไม่

**Returns:**
- `true` = มีการเคลื่อนไหว
- `false` = ไม่มี

#### `PIR_GetMotionDuration()`
```c
uint32_t PIR_GetMotionDuration(PIR_Instance* pir);
```
อ่านระยะเวลาการเคลื่อนไหว

**Returns:**
- ระยะเวลา (milliseconds)

#### `PIR_GetTimeSinceLastMotion()`
```c
uint32_t PIR_GetTimeSinceLastMotion(PIR_Instance* pir);
```
อ่านเวลาที่ผ่านไปนับจากการเคลื่อนไหวครั้งล่าสุด

**Returns:**
- เวลาที่ผ่านไป (milliseconds)

### 8.6 LED Interference Functions

#### `PIR_TriggerBlankingWindow()`
```c
void PIR_TriggerBlankingWindow(PIR_Instance* pir);
```
เรียกใช้ blanking window (เมื่อ LED เปิด/ปิด)

**Note:** เรียกทันทีหลัง LED เปลี่ยนสถานะ

#### `PIR_IsInBlankingWindow()`
```c
bool PIR_IsInBlankingWindow(PIR_Instance* pir);
```
ตรวจสอบว่าอยู่ใน blanking window หรือไม่

**Returns:**
- `true` = อยู่ใน blanking
- `false` = ไม่อยู่

### 8.7 Utility Functions

#### `PIR_Reset()`
```c
void PIR_Reset(PIR_Instance* pir);
```
รีเซ็ตสถานะ PIR

#### `PIR_GetFilteredValue()`
```c
uint8_t PIR_GetFilteredValue(PIR_Instance* pir);
```
อ่านค่าจาก filter buffer (สำหรับ debugging)

**Returns:**
- ค่าที่กรองแล้ว (0-255)

#### `PIR_UpdateAll()`
```c
void PIR_UpdateAll(void);
```
อัพเดท PIR instances ทั้งหมด

#### `PIR_GetInstanceByPin()`
```c
PIR_Instance* PIR_GetInstanceByPin(uint8_t pin);
```
หา PIR instance จาก pin

**Parameters:**
- `pin`: GPIO pin

**Returns:**
- ตัวชี้ไปยัง PIR instance หรือ NULL

---

## 9. Troubleshooting

### 9.1 ปัญหาที่พบบ่อย

#### ปัญหา: PIR ไม่ตรวจจับการเคลื่อนไหว

**สาเหตุที่เป็นไปได้:**
1. ต่อวงจรผิด
2. PIR ยังไม่พร้อม (warm-up time)
3. Sensitivity ต่ำเกินไป
4. Filter level สูงเกินไป

**วิธีแก้:**
```c
// 1. ตรวจสอบการต่อวงจร
// 2. รอ PIR warm-up (30-60 วินาที)
Delay_Ms(60000);

// 3. ลด filter level
PIR_SetFilterLevel(pir, PIR_FILTER_LOW);

// 4. ลด debounce time
PIR_SetDebounce(pir, 50);

// 5. ทดสอบด้วย raw value
bool raw = PIR_ReadRaw(pir);
printf("Raw: %d\r\n", raw);
```

#### ปัญหา: False Positive (ตรวจจับผิดพลาด)

**สาเหตุ:**
- EMI จาก LED/relay
- Vibration
- Temperature changes
- Air flow

**วิธีแก้:**
```c
// 1. เปิดใช้งาน LED protection
PIR_EnableLEDProtection(pir, true);
PIR_SetBlankingWindow(pir, 300);

// 2. เพิ่ม filter level
PIR_SetFilterLevel(pir, PIR_FILTER_HIGH);

// 3. เพิ่ม debounce
PIR_SetDebounce(pir, 300);

// 4. ใช้ hardware filtering
// เพิ่ม capacitor 100nF-1uF ที่ PIR output
```

#### ปัญหา: ตอบสนองช้า

**สาเหตุ:**
- Filter level สูงเกินไป
- Debounce time มากเกินไป

**วิธีแก้:**
```c
// ลด filter level
PIR_SetFilterLevel(pir, PIR_FILTER_LOW);

// ลด debounce
PIR_SetDebounce(pir, 100);
```

#### ปัญหา: Timeout ไม่ทำงาน

**สาเหตุ:**
- ไม่ได้เรียก `PIR_Update()`
- Timeout time สั้นเกินไป

**วิธีแก้:**
```c
// ตรวจสอบว่าเรียก PIR_Update() ใน main loop
while(1) {
    PIR_Update(pir);  // ต้องมี!
    Delay_Ms(10);
}

// เพิ่ม timeout time
PIR_SetTimeout(pir, 10000);  // 10 วินาที
```

### 9.2 Debugging Tips

```c
// 1. แสดงสถานะทุกอย่าง
void debug_pir(PIR_Instance* pir) {
    printf("\r\n=== PIR Debug ===\r\n");
    printf("Raw: %d\r\n", PIR_ReadRaw(pir));
    printf("Filtered: %d\r\n", PIR_Read(pir));
    printf("Filter Value: %d/255\r\n", PIR_GetFilteredValue(pir));
    printf("State: %d\r\n", PIR_GetState(pir));
    printf("Motion: %d\r\n", PIR_IsMotionDetected(pir));
    printf("In Blanking: %d\r\n", PIR_IsInBlankingWindow(pir));
    printf("Duration: %lu ms\r\n", PIR_GetMotionDuration(pir));
    printf("================\r\n\r\n");
}

// เรียกทุก 1 วินาที
if (ELAPSED_TIME(last_debug, Get_CurrentMs()) >= 1000) {
    debug_pir(pir);
    last_debug = Get_CurrentMs();
}
```

---

## 10. โปรเจกต์ตัวอย่าง

### 10.1 Auto-Light System

ดูตัวอย่างสมบูรณ์ที่ [07_Project_AutoLight.c](Examples/07_Project_AutoLight.c)

**Features:**
- Smooth LED fade in/out
- LED interference prevention
- Continuous motion detection
- Auto timeout
- Status reporting

### 10.2 Security Alarm

```c
// ระบบแจ้งเตือนเมื่อตรวจพบการเคลื่อนไหว
void on_motion_detected(void) {
    // เปิดเสียงสัญญาณเตือน
    digitalWrite(BUZZER_PIN, HIGH);
    
    // ส่งการแจ้งเตือน
    send_notification();
    
    // บันทึก log
    log_event("Motion detected");
}
```

### 10.3 Presence Detection

```c
// ตรวจจับว่ามีคนอยู่ในห้องหรือไม่
bool is_room_occupied(void) {
    PIR_State state = PIR_GetState(pir);
    return (state == PIR_STATE_MOTION_ACTIVE || 
            state == PIR_STATE_MOTION_TIMEOUT);
}

// ปรับ HVAC ตามการมีคน
if (is_room_occupied()) {
    set_temperature(22);  // Comfort mode
} else {
    set_temperature(26);  // Energy saving mode
}
```

### 10.4 Energy Saving System

```c
// ปิดอุปกรณ์เมื่อไม่มีคนใช้งาน
uint32_t idle_time = PIR_GetTimeSinceLastMotion(pir);

if (idle_time > 1800000) {  // 30 นาที
    // ปิดจอ
    turn_off_display();
    
    // ปิดเครื่องปรับอากาศ
    turn_off_hvac();
    
    // เข้า sleep mode
    enter_sleep_mode();
}
```

---

## สรุป

SimplePIR Library ให้ความสามารถครบถ้วนสำหรับการควบคุม PIR sensors:

✅ รองรับ NS312 และ RCWL-0516  
✅ Debounce และ filtering ขั้นสูง  
✅ LED interference prevention  
✅ Continuous motion detection  
✅ State machine สำหรับการควบคุมที่ซับซ้อน  
✅ Callback system ที่ยืดหยุ่น  
✅ ตัวอย่างโค้ดครบถ้วน  

**Next Steps:**
1. ทดลองใช้ตัวอย่างพื้นฐาน
2. ปรับแต่ง configuration ตามความต้องการ
3. สร้างโปรเจกต์ของคุณเอง
4. แชร์ผลงานกับชุมชน!

---

**License:** MIT  
**Support:** https://github.com/your-repo/SimplePIR  
**Version:** 1.0 (2025-12-21)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)