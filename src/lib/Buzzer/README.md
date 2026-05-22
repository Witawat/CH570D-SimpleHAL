# Buzzer Library สำหรับ CH57x

> **Library สำหรับควบคุม Buzzer แบบครบวงจร** - รองรับทั้ง Active High และ Active Low พร้อมฟีเจอร์ควบคุมโทนเสียง เล่นทำนอง สร้างจังหวะ Beep Pattern ที่กำหนดเองได้ และเสียงแจ้งเตือนสำเร็จรูป

## 📋 สารบัญ

- [ภาพรวม](#ภาพรวม)
- [ทฤษฎี Buzzer](#ทฤษฎี-buzzer)
- [Hardware Setup](#hardware-setup)
- [การติดตั้ง](#การติดตั้ง)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [ตัวอย่างการใช้งาน](#ตัวอย่างการใช้งาน)
- [เทคนิคขั้นสูง](#เทคนิคขั้นสูง)
- [Musical Notes Reference](#musical-notes-reference)
- [Troubleshooting](#troubleshooting)
- [Changelog](#changelog)

---

## ภาพรวม

Buzzer Library เป็น library ที่ออกแบบมาเพื่อควบคุม Buzzer บน CH57x อย่างง่ายดาย โดยใช้ PWM สำหรับควบคุมความถี่และระดับเสียงอย่างแม่นยำ

### คุณสมบัติหลัก

- ✅ รองรับ **Active High** และ **Active Low** buzzer
- ✅ ควบคุมความถี่ **20Hz - 20kHz**
- ✅ เล่นทำนอง (melody) พร้อม **musical notes**
- ✅ สร้างจังหวะ (rhythm patterns)
- ✅ โหมด **blocking** และ **non-blocking**
- ✅ ปรับระดับเสียง (volume control)
- ✅ ใช้ **PWM** สำหรับความแม่นยำสูง
- ✅ Pre-defined patterns (SOS, Alarm, Success, Error)
- ✅ **Beep Pattern** กำหนด on/off ต่อ step ได้อิสระ
- ✅ **Alert** เสียงแจ้งเตือนสำเร็จรูป 6 แบบ (Long, Pulse, Tremolo, Double, Triple, Urgent)

### รองรับ PWM Pins

| Pin  | Timer Channel | หมายเหตุ |
|------|---------------|----------|
| PA1  | PWM1_CH2      | ✓        |
| PC0  | PWM2_CH3      | ✓        |
| PC3  | PWM1_CH3      | ✓        |
| PC4  | PWM1_CH4      | ✓        |
| PD2  | PWM1_CH1      | ✓        |
| PD3  | PWM2_CH2      | ✓        |
| PD4  | PWM2_CH1      | ✓        |
| PD7  | PWM2_CH4      | ✓        |

---

## ทฤษฎี Buzzer

### ชนิดของ Buzzer

#### 1. **Passive Buzzer (Piezo Buzzer)**
- ต้องการสัญญาณ PWM เพื่อสร้างเสียง
- ควบคุมความถี่ได้ (สามารถเล่นโน้ตเพลง)
- Library นี้รองรับ Passive Buzzer

#### 2. **Active Buzzer**
- มี oscillator ในตัว
- เสียงความถี่เดียว (ไม่สามารถเปลี่ยนโทนได้)
- ใช้ได้แต่ไม่ได้ประโยชน์เต็มที่

### Active High vs Active Low

#### **Active High**
- เปิดด้วยสัญญาณ **HIGH** (3.3V)
- ปิดด้วยสัญญาณ **LOW** (0V)
- ใช้กับ buzzer ทั่วไป

```
MCU Pin (HIGH) --> Buzzer --> GND
```

#### **Active Low**
- เปิดด้วยสัญญาณ **LOW** (0V)
- ปิดด้วยสัญญาณ **HIGH** (3.3V)
- ใช้กับ buzzer ที่ต่อกับ VCC

```
VCC --> Buzzer --> MCU Pin (LOW)
```

### การทำงานของ PWM

Library นี้ใช้ PWM สำหรับสร้างโทนเสียง:

- **ความถี่ (Frequency)** = โทนเสียง (pitch)
  - 262 Hz = โน้ต C4 (โด)
  - 440 Hz = โน้ต A4 (ลา - standard tuning)
  - 1000 Hz = เสียง beep ทั่วไป

- **Duty Cycle** = ระดับเสียง (volume)
  - 0% = ไม่มีเสียง
  - 50% = เสียงดังปานกลาง
  - 100% = เสียงดังสุด (แต่อาจผิดเพี้ยน)

---

## Hardware Setup

### วงจรพื้นฐาน

#### Active High Buzzer

```
CH57x Pin (PC0)
    |
    ├─── R1 (100Ω - 1kΩ)
    |
    ├─── Q1 (NPN Transistor, เช่น 2N2222)
    |         Base
    |
    ├─── Emitter --> GND
    |
    └─── Collector --> Buzzer (+)
                        |
                    Buzzer (-)
                        |
                       GND
```

#### Active Low Buzzer

```
VCC (3.3V)
    |
Buzzer (+)
    |
Buzzer (-)
    |
    ├─── Q1 (NPN Transistor)
    |         Collector
    |
    ├─── Emitter --> GND
    |
CH57x Pin (PC0) --> R1 (1kΩ) --> Base
```

### รายการอุปกรณ์

| อุปกรณ์ | รายละเอียด | หมายเหตุ |
|---------|-----------|----------|
| Buzzer | Passive Buzzer 3-5V | แนะนำ piezo buzzer |
| Transistor | NPN (2N2222, BC547) | สำหรับขับกระแส |
| Resistor | 100Ω - 1kΩ | Base resistor |
| (Optional) Diode | 1N4148 | Flyback protection |

### ข้อควรระวัง

> [!WARNING]
> **อย่าต่อ Buzzer โดยตรงกับ MCU pin!**
> - กระแสของ buzzer อาจสูงกว่า 20mA (ขีดจำกัดของ GPIO)
> - ใช้ transistor เพื่อขับกระแส

> [!CAUTION]
> **ระวังความถี่สูงเกินไป**
> - ความถี่เกิน 20kHz อาจทำให้ buzzer เสียหาย
> - Library จำกัดที่ 20kHz

---

## การติดตั้ง

### 1. คัดลอกไฟล์

คัดลอกไฟล์ทั้งหมดใน `/User/Lib/Buzzer/` ไปยังโปรเจกต์:

```
/User/Lib/Buzzer/
├── Buzzer.h
├── Buzzer.c
└── Examples/
    ├── 01_BasicBeep.c
    ├── 02_ToneControl.c
    ├── 03_MelodyPlayer.c
    ├── 04_RhythmPatterns.c
    ├── 05_NonBlocking.c
    ├── 06_AlarmSystem.c
    └── 07_MusicNotes.c
```

### 2. Include Header

```c
#include "SimpleHAL.h"
#include "Buzzer.h"
```

### 3. เริ่มใช้งาน

```c
int main(void) {
    SystemCoreClockUpdate();
    
    // Init buzzer
    Buzzer_Init(PC0, BUZZER_ACTIVE_HIGH);
    
    // Beep!
    Buzzer_Beep(200);
    
    while(1) {
        // Your code
    }
}
```

---

## Quick Start

### ตัวอย่างที่ 1: Beep ง่ายๆ

```c
#include "SimpleHAL.h"
#include "Buzzer.h"

int main(void) {
    SystemCoreClockUpdate();
    
    Buzzer_Init(PC0, BUZZER_ACTIVE_HIGH);
    
    while(1) {
        Buzzer_Beep(200);  // Beep 200ms
        Delay_Ms(1000);    // Wait 1 second
    }
}
```

### ตัวอย่างที่ 2: เล่นโทนเสียง

```c
// เล่นโน้ต C4 (โด) เป็นเวลา 500ms
Buzzer_Tone(NOTE_C4, 500);

// เล่นโน้ต A4 (ลา - 440Hz)
Buzzer_Tone(NOTE_A4, 500);
```

### ตัวอย่างที่ 3: เล่นทำนอง

```c
// สร้างทำนองง่ายๆ
Note melody[] = {
    {NOTE_C4, 250},  // โด
    {NOTE_E4, 250},  // มี
    {NOTE_G4, 250},  // ซอล
    {NOTE_C5, 500}   // โด (สูงกว่า)
};

// เล่นทำนอง
Buzzer_PlayMelody(melody, 4);
```

---

## API Reference

### Initialization

#### `Buzzer_Init()`
```c
void Buzzer_Init(uint8_t pin, BuzzerType type);
```

เริ่มต้นการใช้งาน Buzzer

**Parameters:**
- `pin` - หมายเลข pin (ต้องเป็น PWM pin)
- `type` - `BUZZER_ACTIVE_HIGH` หรือ `BUZZER_ACTIVE_LOW`

**Example:**
```c
Buzzer_Init(PC0, BUZZER_ACTIVE_HIGH);
```

---

#### `Buzzer_SetVolume()`
```c
void Buzzer_SetVolume(uint8_t volume);
```

ตั้งค่าระดับเสียง

**Parameters:**
- `volume` - ระดับเสียง 0-100 (%)

**Example:**
```c
Buzzer_SetVolume(75);  // 75% volume
```

---

### Basic Control

#### `Buzzer_On()` / `Buzzer_Off()`
```c
void Buzzer_On(void);
void Buzzer_Off(void);
```

เปิด/ปิด buzzer

**Example:**
```c
Buzzer_On();
Delay_Ms(1000);
Buzzer_Off();
```

---

#### `Buzzer_Beep()`
```c
void Buzzer_Beep(uint16_t duration_ms);
```

Beep ครั้งเดียว (blocking)

**Parameters:**
- `duration_ms` - ระยะเวลา (milliseconds)

**Example:**
```c
Buzzer_Beep(200);  // Beep 200ms
```

---

#### `Buzzer_BeepMultiple()`
```c
void Buzzer_BeepMultiple(uint8_t count, uint16_t on_time, uint16_t off_time);
```

Beep หลายครั้ง

**Parameters:**
- `count` - จำนวนครั้ง
- `on_time` - ระยะเวลาเปิด (ms)
- `off_time` - ระยะเวลาปิด (ms)

**Example:**
```c
Buzzer_BeepMultiple(3, 100, 100);  // Beep 3 ครั้ง
```

---

### Tone Control

#### `Buzzer_Tone()`
```c
void Buzzer_Tone(uint16_t frequency, uint16_t duration_ms);
```

เล่นโทนเสียง (blocking)

**Parameters:**
- `frequency` - ความถี่ (Hz), 20-20000
- `duration_ms` - ระยะเวลา (ms), 0 = continuous

**Example:**
```c
Buzzer_Tone(1000, 500);  // 1kHz for 500ms
Buzzer_Tone(440, 0);     // A4 continuous
Buzzer_NoTone();         // Stop
```

---

#### `Buzzer_ToneAsync()`
```c
void Buzzer_ToneAsync(uint16_t frequency, uint16_t duration_ms);
```

เล่นโทนเสียงแบบ non-blocking

**Example:**
```c
Buzzer_ToneAsync(1000, 500);
while(Buzzer_IsBusy()) {
    // Do other work
}
```

---

### Melody & Pattern

#### `Buzzer_PlayMelody()`
```c
void Buzzer_PlayMelody(const Note* melody, uint8_t length);
```

เล่นทำนอง (blocking)

**Parameters:**
- `melody` - array ของ Note structures
- `length` - จำนวนโน้ต

**Example:**
```c
Note melody[] = {
    {NOTE_C4, 250},
    {NOTE_E4, 250},
    {NOTE_G4, 500}
};
Buzzer_PlayMelody(melody, 3);
```

---

#### `Buzzer_PlayMelodyAsync()`
```c
void Buzzer_PlayMelodyAsync(const Note* melody, uint8_t length, uint8_t repeat);
```

เล่นทำนองแบบ non-blocking

**Parameters:**
- `melody` - array ของ Note structures
- `length` - จำนวนโน้ต
- `repeat` - จำนวนครั้งที่เล่นซ้ำ (0 = infinite)

**Example:**
```c
Note melody[] = {{NOTE_C4, 250}, {NOTE_E4, 250}};
Buzzer_PlayMelodyAsync(melody, 2, 1);

while(1) {
    Buzzer_Update();  // ต้องเรียกใน loop!
    // Do other work
}
```

---

### Advanced Functions

#### `Buzzer_FrequencySweep()`
```c
void Buzzer_FrequencySweep(uint16_t start_freq, uint16_t end_freq, 
                           uint16_t duration_ms, uint16_t step_ms);
```

Sweep ความถี่ (เสียงไซเรน)

**Example:**
```c
// Siren effect
Buzzer_FrequencySweep(500, 2000, 1000, 10);
```

---

#### `Buzzer_IsBusy()`
```c
uint8_t Buzzer_IsBusy(void);
```

ตรวจสอบว่า buzzer กำลังทำงานอยู่หรือไม่

**Returns:** `1` = กำลังทำงาน, `0` = ว่าง

---

### Beep Pattern & Alert

#### `BeepStep` struct

หนึ่ง step กำหนดได้ 2 ค่า:
- `on_ms` — ระยะเวลาส่งเสียง (ms)
- `off_ms` — ระยะเวลาเงียบ (ms), `0` = ข้ามช่วงเงียบ ต่อเสียงไปยัง step ถัดไปทันที

```c
typedef struct {
    uint16_t on_ms;
    uint16_t off_ms;
} BeepStep;
```

---

#### `BuzzerAlertType` enum

| ค่า | เสียง | Pattern ภายใน |
|-----|-------|---------------|
| `BUZZER_ALERT_LONG`    | เสียงยาวต่อเนื่อง   | `{2000, 0}` |
| `BUZZER_ALERT_PULSE`   | บี๊บ-เว้นสลับกัน   | `{500, 500}` |
| `BUZZER_ALERT_TREMOLO` | เสียงสั่นถี่         | `{50, 50}` |
| `BUZZER_ALERT_DOUBLE`  | บี๊บๆ แล้วหยุด      | `{100,100}, {100,500}` |
| `BUZZER_ALERT_TRIPLE`  | บี๊บๆๆ แล้วหยุด     | `{100,100}, {100,100}, {100,500}` |
| `BUZZER_ALERT_URGENT`  | จังหวะเร่งด่วน       | `{80,40}, {80,40}, {80,300}` |

---

#### `Buzzer_PlayPattern()`
```c
void Buzzer_PlayPattern(const BeepStep* steps, uint8_t length, uint8_t repeat);
```

เล่น pattern ที่กำหนดเอง (blocking)

**Parameters:**
- `steps`  — array ของ BeepStep
- `length` — จำนวน steps
- `repeat` — จำนวนรอบ (0 = infinite)

**Example:**
```c
// บี๊บสั้น 50ms เว้น 20ms แล้วบี๊บยาว 150ms  เล่น 3 รอบ
BeepStep pat[] = {{50, 20}, {150, 0}};
Buzzer_PlayPattern(pat, 2, 3);

// บี๊บยาว 100ms เว้น 20ms แล้วบี๊บสั่น 30ms x3  เล่น 1 รอบ
BeepStep pat2[] = {{100, 20}, {30,30}, {30,30}, {30,0}};
Buzzer_PlayPattern(pat2, 4, 1);
```

---

#### `Buzzer_PlayPatternAsync()`
```c
void Buzzer_PlayPatternAsync(const BeepStep* steps, uint8_t length, uint8_t repeat);
```

เล่น pattern ที่กำหนดเองแบบ non-blocking

**Example:**
```c
BeepStep pat[] = {{80,80}, {80,80}, {80,80}, {400,0}};
Buzzer_PlayPatternAsync(pat, 4, 0);  // infinite

while(1) {
    Buzzer_Update();   // ต้องเรียกใน loop!
    if(done) Buzzer_Stop();
}
```

---

#### `Buzzer_Alert()`
```c
void Buzzer_Alert(BuzzerAlertType type, uint16_t duration_ms);
```

เล่นเสียงแจ้งเตือนสำเร็จรูป (blocking)

**Parameters:**
- `type`        — ชนิดเสียง (BuzzerAlertType)
- `duration_ms` — ระยะเวลาทั้งหมด (ms), `0` = เล่นครบ 1 รอบ

**Example:**
```c
Buzzer_Alert(BUZZER_ALERT_TREMOLO, 2000);  // เสียงสั่น 2 วิ
Buzzer_Alert(BUZZER_ALERT_TRIPLE, 0);      // บี๊บๆๆ 1 รอบ
```

---

#### `Buzzer_AlertAsync()`
```c
void Buzzer_AlertAsync(BuzzerAlertType type);
```

เล่นเสียงแจ้งเตือนสำเร็จรูปแบบ non-blocking (infinite ต้อง `Buzzer_Stop()` เพื่อหยุด)

**Example:**
```c
Buzzer_AlertAsync(BUZZER_ALERT_URGENT);
while(1) {
    Buzzer_Update();
    if(alarm_cleared) Buzzer_Stop();
}
```

---

### Pre-defined Patterns

```c
void Buzzer_PlaySOS(void);           // SOS pattern
void Buzzer_PlayAlarm(uint16_t ms);  // Alarm sound
void Buzzer_PlaySuccess(void);       // Success sound
void Buzzer_PlayError(void);         // Error sound
void Buzzer_PlayStartup(void);       // Startup sound
```

**Example:**
```c
Buzzer_PlaySOS();      // ... --- ...
Buzzer_PlaySuccess();  // C-E-G-C
```

---

## ตัวอย่างการใช้งาน

### 1. Basic Beep

```c
// Beep ง่ายๆ
Buzzer_Beep(200);

// Beep หลายครั้ง
Buzzer_BeepMultiple(3, 100, 100);

// Toggle
for(uint8_t i = 0; i < 10; i++) {
    Buzzer_Toggle();
    Delay_Ms(200);
}
```

### 2. Tone Control

```c
// เล่นโน้ต C Major Scale
Buzzer_Tone(NOTE_C4, 300);  // โด
Buzzer_Tone(NOTE_D4, 300);  // เร
Buzzer_Tone(NOTE_E4, 300);  // มี
Buzzer_Tone(NOTE_F4, 300);  // ฟา
Buzzer_Tone(NOTE_G4, 300);  // ซอล
Buzzer_Tone(NOTE_A4, 300);  // ลา
Buzzer_Tone(NOTE_B4, 300);  // ที
Buzzer_Tone(NOTE_C5, 500);  // โด

// Frequency sweep (siren)
Buzzer_FrequencySweep(800, 1200, 500, 10);
```

### 3. Melody Player

```c
// Happy Birthday
const Note happy_birthday[] = {
    {NOTE_C4, 250}, {NOTE_C4, 125}, {NOTE_D4, 375},
    {NOTE_C4, 375}, {NOTE_F4, 375}, {NOTE_E4, 750},
    // ... (ดูใน Examples/03_MelodyPlayer.c)
};

Buzzer_PlayMelody(happy_birthday, 
                  sizeof(happy_birthday)/sizeof(Note));
```

### 4. Non-blocking Operation

```c
// เล่นเพลงในพื้นหลัง
const Note bg_music[] = {
    {NOTE_C4, 250}, {NOTE_E4, 250}, {NOTE_G4, 500}
};

Buzzer_PlayMelodyAsync(bg_music, 3, 0);  // เล่นวนซ้ำ

while(1) {
    Buzzer_Update();  // ต้องเรียกใน loop!
    
    // ทำงานอื่นได้
    digitalToggle(LED_PIN);
    Delay_Ms(250);
}
```

### 5. Alarm System

```c
// Fire alarm
const Note fire_alarm[] = {
    {NOTE_A5, 200}, {NOTE_REST, 100},
    {NOTE_A5, 200}, {NOTE_REST, 100},
    {NOTE_A5, 200}, {NOTE_REST, 500}
};

while(sensor_triggered) {
    Buzzer_PlayMelody(fire_alarm, 
                      sizeof(fire_alarm)/sizeof(Note));
}
```

### 6. Beep Pattern — กำหนดจังหวะเอง

```c
// บี๊บสั้น 50ms เว้น 20ms แล้วบี๊บยาว 150ms  เล่น 3 รอบ
BeepStep pat1[] = {{50, 20}, {150, 0}};
Buzzer_PlayPattern(pat1, 2, 3);

// บี๊บ 3 ครั้งสั้นแล้วบี๊บยาว 1 ครั้ง (SOS style)
BeepStep pat2[] = {{80,80}, {80,80}, {80,80}, {400,0}};
Buzzer_PlayPattern(pat2, 4, 1);

// บี๊บยาว 100ms เว้น 20ms แล้วบี๊บสั่นๆ 30ms x3
BeepStep pat3[] = {{100, 20}, {30,30}, {30,30}, {30,0}};
Buzzer_PlayPattern(pat3, 4, 1);

// เล่น pattern ต่อเนื่องใน background (non-blocking)
BeepStep pat4[] = {{60, 60}, {60, 60}, {60, 300}};
Buzzer_PlayPatternAsync(pat4, 3, 0);   // 0 = infinite
while(1) {
    Buzzer_Update();
    if(condition_met) Buzzer_Stop();
}
```

### 7. Alert สำเร็จรูป

```c
// เสียงสั่นนาน 2 วิ (blocking)
Buzzer_Alert(BUZZER_ALERT_TREMOLO, 2000);

// บี๊บๆๆ 1 รอบ แล้วกลับ
Buzzer_Alert(BUZZER_ALERT_TRIPLE, 0);

// เสียง urgent ต่อเนื่อง (non-blocking) จนกว่า sensor จะหมด
Buzzer_AlertAsync(BUZZER_ALERT_URGENT);
while(sensor_on) {
    Buzzer_Update();
    // ทำงานอื่นได้
}
Buzzer_Stop();
```

---

## เทคนิคขั้นสูง

### 1. สร้าง Melody ของตัวเอง

```c
// กำหนดโน้ตและระยะเวลา
const Note my_melody[] = {
    {NOTE_C4, 200},      // โด - 200ms
    {NOTE_REST, 100},    // พัก - 100ms
    {NOTE_E4, 200},      // มี - 200ms
    {NOTE_G4, 400},      // ซอล - 400ms
    {NOTE_C5, 600}       // โด (สูง) - 600ms
};

Buzzer_PlayMelody(my_melody, 5);
```

### 2. ปรับ Tempo

```c
// Slow tempo (120 BPM)
#define TEMPO_SLOW 500  // Quarter note = 500ms

// Fast tempo (180 BPM)
#define TEMPO_FAST 333  // Quarter note = 333ms

const Note melody_slow[] = {
    {NOTE_C4, TEMPO_SLOW},
    {NOTE_E4, TEMPO_SLOW},
    {NOTE_G4, TEMPO_SLOW * 2}  // Half note
};
```

### 3. สร้าง Chord (คอร์ด)

```c
// จำลองคอร์ดด้วยการเล่นโน้ตเร็วมาก
void PlayChord(uint16_t n1, uint16_t n2, uint16_t n3, uint16_t duration) {
    for(uint16_t i = 0; i < duration; i += 30) {
        Buzzer_Tone(n1, 10);
        Buzzer_Tone(n2, 10);
        Buzzer_Tone(n3, 10);
    }
}

// C Major Chord (C-E-G)
PlayChord(NOTE_C4, NOTE_E4, NOTE_G4, 500);
```

### 4. Pattern Generator

```c
// สร้าง pattern แบบ dynamic
void GeneratePattern(uint16_t base_freq, uint8_t steps) {
    for(uint8_t i = 0; i < steps; i++) {
        uint16_t freq = base_freq + (i * 100);
        Buzzer_Tone(freq, 100);
        Delay_Ms(50);
    }
}

GeneratePattern(500, 10);  // 500Hz ถึง 1400Hz
```

### 5. Volume Fade

```c
// Fade in
for(uint8_t vol = 0; vol <= 100; vol += 10) {
    Buzzer_SetVolume(vol);
    Delay_Ms(50);
}

// Fade out
for(uint8_t vol = 100; vol > 0; vol -= 10) {
    Buzzer_SetVolume(vol);
    Delay_Ms(50);
}
```

### 6. Memory Optimization

```c
// ใช้ PROGMEM สำหรับ melody ขนาดใหญ่ (ถ้ารองรับ)
const Note PROGMEM long_melody[] = {
    // ... many notes ...
};

// หรือใช้ macro สำหรับลดขนาด
#define C4_Q {NOTE_C4, DURATION_QUARTER}
#define E4_Q {NOTE_E4, DURATION_QUARTER}

const Note melody[] = {C4_Q, E4_Q, C4_Q, E4_Q};
```

---

## Musical Notes Reference

### Note Frequencies

| Note | Octave 3 | Octave 4 | Octave 5 | Octave 6 |
|------|----------|----------|----------|----------|
| C    | 131 Hz   | 262 Hz   | 523 Hz   | 1047 Hz  |
| C#/Db| 139 Hz   | 277 Hz   | 554 Hz   | 1109 Hz  |
| D    | 147 Hz   | 294 Hz   | 587 Hz   | 1175 Hz  |
| D#/Eb| 156 Hz   | 311 Hz   | 622 Hz   | 1245 Hz  |
| E    | 165 Hz   | 330 Hz   | 659 Hz   | 1319 Hz  |
| F    | 175 Hz   | 349 Hz   | 698 Hz   | 1397 Hz  |
| F#/Gb| 185 Hz   | 370 Hz   | 740 Hz   | 1480 Hz  |
| G    | 196 Hz   | 392 Hz   | 784 Hz   | 1568 Hz  |
| G#/Ab| 208 Hz   | 415 Hz   | 831 Hz   | 1661 Hz  |
| A    | 220 Hz   | **440 Hz** | 880 Hz   | 1760 Hz  |
| A#/Bb| 233 Hz   | 466 Hz   | 932 Hz   | 1865 Hz  |
| B    | 247 Hz   | 494 Hz   | 988 Hz   | 1976 Hz  |

> **Note:** A4 = 440 Hz เป็น standard tuning

### Note Durations (120 BPM)

| Duration | Time (ms) | Constant |
|----------|-----------|----------|
| Whole    | 2000 ms   | `DURATION_WHOLE` |
| Half     | 1000 ms   | `DURATION_HALF` |
| Quarter  | 500 ms    | `DURATION_QUARTER` |
| Eighth   | 250 ms    | `DURATION_EIGHTH` |
| Sixteenth| 125 ms    | `DURATION_SIXTEENTH` |

---

## Troubleshooting

### ไม่มีเสียง

**สาเหตุที่เป็นไปได้:**

1. **ต่อวงจรผิด**
   - ตรวจสอบ Active High/Low
   - ตรวจสอบ transistor ต่อถูกต้อง

2. **Pin ไม่รองรับ PWM**
   ```c
   // ✗ Wrong - PD5 ไม่รองรับ PWM
   Buzzer_Init(PD5, BUZZER_ACTIVE_HIGH);
   
   // ✓ Correct - PC0 รองรับ PWM
   Buzzer_Init(PC0, BUZZER_ACTIVE_HIGH);
   ```

3. **Volume ต่ำเกินไป**
   ```c
   Buzzer_SetVolume(75);  // เพิ่ม volume
   ```

### เสียงผิดเพี้ยน

**สาเหตุ:**

1. **Volume สูงเกินไป**
   ```c
   Buzzer_SetVolume(50);  // ลด volume
   ```

2. **ความถี่ไม่เหมาะสม**
   ```c
   // ใช้ความถี่ในช่วง 200-5000 Hz สำหรับ buzzer ทั่วไป
   Buzzer_Tone(1000, 500);  // ✓ Good
   ```

### เสียงไม่ต่อเนื่อง (Non-blocking)

**สาเหตุ:** ลืมเรียก `Buzzer_Update()`

```c
// ✗ Wrong
Buzzer_PlayMelodyAsync(melody, length, 1);
Buzzer_PlayPatternAsync(pat, pat_len, 0);
Buzzer_AlertAsync(BUZZER_ALERT_PULSE);
while(1) {
    // ไม่มี Buzzer_Update()!  → เสียงหยุด
}

// ✓ Correct
Buzzer_PlayPatternAsync(pat, pat_len, 0);
while(1) {
    Buzzer_Update();  // ต้องเรียกใน loop!
    Delay_Ms(10);
}
```

### Timing ไม่แม่นยำ

**สาเหตุ:** ใช้ blocking functions ใน non-blocking mode

```c
// ✗ Wrong
Buzzer_PlayMelodyAsync(melody, length, 1);
while(Buzzer_IsBusy()) {
    Delay_Ms(1000);  // Delay ยาวเกินไป!
}

// ✓ Correct
Buzzer_PlayMelodyAsync(melody, length, 1);
while(Buzzer_IsBusy()) {
    Buzzer_Update();
    Delay_Ms(10);  // Delay สั้นๆ
}
```

### กระแสสูงเกินไป

**แก้ไข:** ใช้ transistor ขับกระแส

```c
// อย่าต่อ buzzer โดยตรงกับ MCU!
// ใช้ transistor (2N2222, BC547) เสมอ
```

---

## License

MIT License - ใช้งานได้ฟรี

## ผู้พัฒนา

CH57x Library Team

## Version

2.0.0 (2026-05-01)

---

## ดูเพิ่มเติม

- [Examples/](Examples/) - ตัวอย่างการใช้งานทั้งหมด
- [SimpleHAL Documentation](../SimpleHAL/README.md)
- [SimplePWM Documentation](../SimpleHAL/README.md#simplepwm)

---

---

## Changelog

### v2.0.0 (2026-05-01)
- เพิ่ม `BeepStep` struct สำหรับกำหนด on/off ต่อ step อย่างอิสระ
- เพิ่ม `BuzzerAlertType` enum พร้อม 6 แบบสำเร็จรูป
- เพิ่ม `Buzzer_PlayPattern()` — blocking pattern player
- เพิ่ม `Buzzer_PlayPatternAsync()` — non-blocking pattern player
- เพิ่ม `Buzzer_Alert()` — blocking alert พร้อม duration
- เพิ่ม `Buzzer_AlertAsync()` — non-blocking infinite alert
- ขยาย `Buzzer_Update()` รองรับ pattern state machine
- อัปเดต `Buzzer_Stop()` ให้ clear pattern state ด้วย

### v1.0.0 (2025-12-22)
- Initial release

---

**Happy Buzzing! 🎵**

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)