# HC-SR04 Ultrasonic Distance Sensor Library

> **Library สำหรับวัดระยะทางด้วย HC-SR04 Ultrasonic Sensor บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Troubleshooting](#troubleshooting)
8. [API Reference](#api-reference)

---

## ภาพรวม

HC-SR04 เป็น ultrasonic distance sensor ที่นิยมมากที่สุด ใช้คลื่นเสียงความถี่ 40kHz ในการวัดระยะทาง ราคาถูก ใช้งานง่าย และแม่นยำพอสำหรับโปรเจกต์ทั่วไป

### คุณสมบัติของ HC-SR04

- **ช่วงวัด**: 2-400 cm (0.8 นิ้ว ถึง 13 ฟุต)
- **ความแม่นยำ**: ±3 mm
- **Beam Angle**: 15°
- **ไฟเลี้ยง**: 5V DC, 15 mA
- **Signal Input**: 10µs TTL pulse

---

## คุณสมบัติ

- ✅ วัดระยะทาง 2-400 cm
- ✅ รองรับหน่วย cm และ inch
- ✅ วัดหลายครั้งแล้ว average เพื่อลด noise
- ✅ Timeout protection ป้องกันโปรแกรมค้าง
- ✅ รองรับหลาย sensor พร้อมกัน
- ✅ `IsObjectNear()` สำหรับตรวจจับวัตถุ
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### การต่อวงจร (3.3V Mode)

```
HC-SR04          CH57x
VCC    --------> 3.3V  (หรือ 5V ผ่าน voltage regulator)
TRIG   --------> PC3   (GPIO Output)
ECHO   --------> PC4   (GPIO Input)
GND    --------> GND
```

> ⚠️ **สำคัญ**: HC-SR04 ปกติทำงานที่ 5V  
> เมื่อใช้กับ CH57x (3.3V) มี 2 ตัวเลือก:

**ตัวเลือก 1: ใช้ 3.3V ตรง (ทำงานได้แต่ช่วงวัดลดลงเล็กน้อย)**
```
VCC → 3.3V, ECHO → PC4 โดยตรง (3.3V tolerant)
```

**ตัวเลือก 2: ใช้ 5V พร้อม Voltage Divider บน ECHO (แนะนำ)**
```
VCC → 5V
ECHO --[2kΩ]-- PC4 --[3.3kΩ]-- GND

ค่าแรงดันบน PC4 = 5V × 3.3/(2+3.3) = 3.11V ✓
```

### Pinout ของ HC-SR04

```
  +----------+
  |  HC-SR04 |
  |          |
  | VCC TRIG |  ECHO GND
  +----------+
    |    |      |    |
   5V  PC3    PC4  GND
  (out) (in)
```

---

## หลักการทำงาน

### Ultrasonic Measurement Sequence

```
TRIG:  ___|¯¯10µs¯¯|_______________________________________________

ECHO:  ________|¯¯¯¯¯¯¯¯¯¯¯echo duration¯¯¯¯¯¯¯¯¯¯¯|_____________

         ^      ^                                    ^
         |      |                                    |
       TRIG   ECHO start                           ECHO end
        pulse  (sensor ส่ง ultrasonic)           (echo กลับมา)

ระยะทาง = echo_duration_µs / 58.0 (cm)
         = echo_duration_µs / 148.0 (inch)

เหตุผล: ความเร็วเสียง ≈ 340 m/s = 0.034 cm/µs
        round-trip → /2 → 0.017 cm/µs = 1/58.8 ≈ 1/58
```

### ตัวอย่างการคำนวณ

| echo duration | ระยะทาง |
|--------------|---------|
| 116 µs | 2.0 cm |
| 580 µs | 10.0 cm |
| 2900 µs | 50.0 cm |
| 5800 µs | 100.0 cm |
| 23200 µs | 400.0 cm |

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "HCSR04.h"

HCSR04_Instance sensor;  // ประกาศ global หรือ static
```

### ขั้นตอนที่ 2: Init ใน main()

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    // Init sensor: TRIG=PC3, ECHO=PC4
    HCSR04_Init(&sensor, PC3, PC4);

    while (1) {
        float distance = HCSR04_MeasureCm(&sensor);

        if (distance != HCSR04_ERROR_VALUE) {
            printf("ระยะทาง: %.1f cm\r\n", distance);
        } else {
            printf("นอกช่วงวัด หรือไม่มีวัตถุ\r\n");
        }

        Delay_Ms(100);  // รอก่อนวัดครั้งต่อไป (≥60ms)
    }
}
```

---

## การใช้งานขั้นสูง

### วัดแบบ Average เพื่อลด Noise

```c
// วัด 5 ครั้งแล้วหาค่าเฉลี่ย (ใช้เวลา ~300ms)
float stable_distance = HCSR04_MeasureAvgCm(&sensor, 5);

if (stable_distance != HCSR04_ERROR_VALUE) {
    printf("ระยะเฉลี่ย: %.1f cm\r\n", stable_distance);
}
```

### ตรวจจับวัตถุ (Proximity Detection)

```c
#define OBSTACLE_THRESHOLD  20.0f  // 20 cm

while (1) {
    if (HCSR04_IsObjectNear(&sensor, OBSTACLE_THRESHOLD)) {
        printf("⚠️ พบสิ่งกีดขวางในระยะ %.0f cm!\r\n", OBSTACLE_THRESHOLD);
        // หยุดมอเตอร์, ส่ง alert ฯลฯ
    }
    Delay_Ms(100);
}
```

### ใช้หลาย Sensor พร้อมกัน

```c
HCSR04_Instance front;   // sensor ด้านหน้า
HCSR04_Instance left;    // sensor ด้านซ้าย
HCSR04_Instance right;   // sensor ด้านขวา

HCSR04_Init(&front, PC3, PC4);
HCSR04_Init(&left,  PC5, PC6);
HCSR04_Init(&right, PD3, PD4);

while (1) {
    float f = HCSR04_MeasureCm(&front);
    float l = HCSR04_MeasureCm(&left);
    float r = HCSR04_MeasureCm(&right);

    printf("F:%.0f L:%.0f R:%.0f\r\n", f, l, r);
    Delay_Ms(100);
}
```

### ระบบ Parking Sensor พร้อม Display ระยะทาง

```c
void parking_beep(float distance_cm) {
    if (distance_cm < 20) {
        // ใกล้มาก: beep ถี่
        printf("⚠️ ใกล้มาก! %.0f cm\r\n", distance_cm);
    } else if (distance_cm < 50) {
        printf("🟡 ระยะปานกลาง: %.0f cm\r\n", distance_cm);
    } else if (distance_cm < 100) {
        printf("🟢 ระยะดี: %.0f cm\r\n", distance_cm);
    } else {
        printf("✅ ปลอดภัย: %.0f cm\r\n", distance_cm);
    }
}

while (1) {
    float dist = HCSR04_MeasureAvgCm(&sensor, 3);
    if (dist != HCSR04_ERROR_VALUE) {
        parking_beep(dist);
    }
    Delay_Ms(200);
}
```

### วัดความเร็วการเคลื่อนที่

```c
float prev_distance = HCSR04_MeasureCm(&sensor);
uint32_t prev_time  = Get_CurrentMs();
Delay_Ms(100);

while (1) {
    float curr_distance = HCSR04_MeasureCm(&sensor);
    uint32_t curr_time  = Get_CurrentMs();

    if (prev_distance != HCSR04_ERROR_VALUE &&
        curr_distance != HCSR04_ERROR_VALUE) {

        float delta_cm  = curr_distance - prev_distance;
        float delta_sec = (curr_time - prev_time) / 1000.0f;
        float speed     = delta_cm / delta_sec;  // cm/s

        printf("Speed: %.1f cm/s\r\n", speed);
    }

    prev_distance = curr_distance;
    prev_time     = curr_time;
    Delay_Ms(100);
}
```

---

## Troubleshooting

### ปัญหา: ได้ค่า -1 (ERROR) ตลอด

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ไฟเลี้ยง sensor ไม่พอ | ใช้ 5V แทน 3.3V |
| Pin TRIG/ECHO สลับกัน | ตรวจสอบว่า TRIG=Output, ECHO=Input |
| Sensor เสีย | ใช้ multimeter วัดแรงดัน ECHO ขณะวัด ควรเห็น pulse |
| ECHO voltage สูงเกิน 3.3V | ต่อ voltage divider บน ECHO pin |

### ปัญหา: ค่าที่ได้กระเพื่อมมาก

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Noise จากมอเตอร์/PWM | ใช้ `HCSR04_MeasureAvgCm()` วัด 3-5 ครั้ง |
| พื้นผิวดูดซับเสียง | หันหัว sensor ให้ตั้งฉากกับวัตถุ |
| วัดถี่เกินไป | เพิ่ม delay เป็น ≥100ms ระหว่างการวัด |
| อุณหภูมิแวดล้อมเปลี่ยน | ความเร็วเสียงเปลี่ยนตามอุณหภูมิ (ปกติยอมรับได้) |

### ปัญหา: ค่าน้อยกว่าความเป็นจริง

HC-SR04 วัดระยะทางแบบ round-trip ของคลื่นเสียง  
ถ้าวัตถุมีพื้นผิวเอียง คลื่นจะสะท้อนออกทิศอื่น  
**วิธีแก้**: ให้หัว sensor ตั้งฉากกับวัตถุโดยตรง

---

## API Reference

### `HCSR04_Init(sensor, pin_trig, pin_echo)`
เริ่มต้น HC-SR04 sensor และตั้งค่า GPIO

| Parameter | Type | คำอธิบาย |
|-----------|------|----------|
| `sensor` | `HCSR04_Instance*` | ตัวแปร instance |
| `pin_trig` | `uint8_t` | GPIO pin สำหรับ TRIG (Output) เช่น `PC3` |
| `pin_echo` | `uint8_t` | GPIO pin สำหรับ ECHO (Input) เช่น `PC4` |

---

### `HCSR04_MeasureCm(sensor)` → `float`
วัดระยะทาง (cm) — คืน `HCSR04_ERROR_VALUE` (-1) ถ้าเกิด error

---

### `HCSR04_MeasureInch(sensor)` → `float`
วัดระยะทาง (inch) — คืน `HCSR04_ERROR_VALUE` (-1) ถ้าเกิด error

---

### `HCSR04_MeasureAvgCm(sensor, samples)` → `float`
วัดหลายครั้งแล้ว average — `samples` อยู่ในช่วง 1-10

---

### `HCSR04_GetLastDistance(sensor)` → `float`
ดึงค่าระยะทางล่าสุด โดยไม่ trigger การวัดใหม่

---

### `HCSR04_IsObjectNear(sensor, threshold)` → `bool`
คืน `true` ถ้าวัดได้ระยะน้อยกว่า `threshold` cm

---

### Constants

| ค่า | ความหมาย | Default |
|-----|----------|---------|
| `HCSR04_MAX_DISTANCE_CM` | ระยะสูงสุด | 400.0 cm |
| `HCSR04_MIN_DISTANCE_CM` | ระยะน้อยสุด | 2.0 cm |
| `HCSR04_ERROR_VALUE` | ค่า error | -1.0 |
| `HCSR04_ECHO_TIMEOUT_US` | Timeout | 30000 µs |
| `HCSR04_MIN_INTERVAL_MS` | ช่วงเวลาระหว่างวัด | 60 ms |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)