# Relay Library

ไลบรารีควบคุมรีเลย์บน CH57x รองรับทั้งโมดูลแบบ Active High และ Active Low
เหมาะกับงานเปิดปิดโหลด เช่น ปั๊มน้ำ หลอดไฟ พัดลม หรือโซลินอยด์

## สารบัญ

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

Relay module ส่วนใหญ่มีอินพุตตรรกะ 1 ขา และใช้ทรานซิสเตอร์/ออปโตคัปเปลอร์บนบอร์ด
แต่ตรรกะเปิดใช้งานอาจไม่เหมือนกัน

- Active Low: เขียน LOW แล้วรีเลย์ติด (พบบ่อยที่สุด)
- Active High: เขียน HIGH แล้วรีเลย์ติด

ไลบรารีนี้จึงมีการระบุ active level ตั้งแต่ตอน init เพื่อให้ API ใช้งานตรงความหมาย

- Relay_On = เปิดรีเลย์จริง
- Relay_Off = ปิดรีเลย์จริง

---

## คุณสมบัติ

- รองรับ Active High และ Active Low
- จัดการสถานะภายใน instance
- มี Toggle และ Set แบบตรงสถานะ
- มีฟังก์ชัน Relay_IsOn สำหรับเช็คสถานะล่าสุด

---

## Hardware Setup

### Wiring พื้นฐาน

| CH57x | Relay Module |
|---|---|
| GPIO pin (เช่น PC0) | IN |
| 3.3V หรือ 5V | VCC |
| GND | GND |

ข้อควรระวัง

- ตรวจว่าโมดูลรับอินพุต 3.3V ได้หรือไม่
- โหลด AC mains ต้องต่อผ่าน terminal relay อย่างถูกต้องและปลอดภัย
- งานไฟบ้านควรมีฟิวส์และฉนวนตามมาตรฐาน

---

## หลักการทำงาน

### Active level mapping

| โหมด | Relay_On เขียนค่า | Relay_Off เขียนค่า |
|---|---|---|
| RELAY_ACTIVE_HIGH | HIGH | LOW |
| RELAY_ACTIVE_LOW | LOW | HIGH |

ดังนั้นโค้ดชั้นบนไม่ต้องจำว่าโมดูลกลับตรรกะหรือไม่

---

## การใช้งานพื้นฐาน

### 1) Init

```c
#include "SimpleHAL.h"
#include "Relay.h"

Relay_Instance relay;

int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    // โมดูลส่วนใหญ่เป็น active low
    Relay_Init(&relay, PC0, RELAY_ACTIVE_LOW);

    Relay_On(&relay);
    Delay_Ms(1000);
    Relay_Off(&relay);

    while (1) {
    }
}
```

### 2) Init แบบ Active High

```c
Relay_Instance relay_high;
Relay_Init(&relay_high, PC1, RELAY_ACTIVE_HIGH);

Relay_On(&relay_high);
Delay_Ms(500);
Relay_Off(&relay_high);
```

### 3) ควบคุมแบบตรงสถานะด้วย Relay_Set

```c
Relay_Set(&relay, 1); // ON
Delay_Ms(500);
Relay_Set(&relay, 0); // OFF
```

### 4) เปิด/ปิดตรงด้วย Relay_On และ Relay_Off

```c
Relay_On(&relay);
Delay_Ms(200);
Relay_Off(&relay);
```

### 5) เช็คสถานะด้วย Relay_IsOn

```c
if (Relay_IsOn(&relay)) {
  // relay กำลัง ON
} else {
  // relay กำลัง OFF
}
```

---

## การใช้งานขั้นสูง

### 1) Toggle เป็นจังหวะทุก 1 วินาที

```c
while (1) {
    Relay_Toggle(&relay);
    Delay_Ms(1000);
}
```

### 2) ใช้ร่วมกับเซนเซอร์

```c
if (temperature > 40.0f) {
    Relay_On(&relay);   // เปิดพัดลม
} else {
    Relay_Off(&relay);
}
```

### 3) ตรวจสถานะก่อนสั่ง

```c
if (!Relay_IsOn(&relay)) {
    Relay_On(&relay);
}
```

### 4) ควบคุม Relay หลายตัวพร้อมกัน

```c
Relay_Instance pump;
Relay_Instance fan;
Relay_Instance light;

void RelaySetup(void)
{
  Relay_Init(&pump,  PC0, RELAY_ACTIVE_LOW);
  Relay_Init(&fan,   PC1, RELAY_ACTIVE_LOW);
  Relay_Init(&light, PC2, RELAY_ACTIVE_HIGH);
}

void AllOff(void)
{
  Relay_Off(&pump);
  Relay_Off(&fan);
  Relay_Off(&light);
}

void ProcessControl(float temp, uint8_t need_light)
{
  if (temp > 45.0f) {
    Relay_On(&fan);
  } else {
    Relay_Off(&fan);
  }

  if (need_light) {
    Relay_On(&light);
  } else {
    Relay_Off(&light);
  }
}
```

### 5) Pulse ชั่วคราว (เช่นกดปุ่มแทนคน)

```c
void Relay_Pulse(Relay_Instance* r, uint32_t on_ms)
{
  Relay_On(r);
  Delay_Ms(on_ms);
  Relay_Off(r);
}

// ใช้งาน
Relay_Pulse(&relay, 300);
```

### 6) สลับสถานะด้วยเงื่อนไขเดียวผ่าน Relay_Set

```c
uint8_t alarm = (temperature > 60.0f) ? 1 : 0;
Relay_Set(&relay, alarm);
```

### 7) ตัวอย่าง state machine แบบไม่สลับถี่

```c
typedef enum {
  RELAY_MODE_IDLE = 0,
  RELAY_MODE_RUN,
  RELAY_MODE_COOLDOWN
} RelayMode;

RelayMode mode = RELAY_MODE_IDLE;
uint32_t t_mark = 0;

void RelayTask(void)
{
  uint32_t now = Get_CurrentMs();

  switch (mode) {
  case RELAY_MODE_IDLE:
    if (start_cmd) {
      Relay_On(&relay);
      t_mark = now;
      mode = RELAY_MODE_RUN;
    }
    break;

  case RELAY_MODE_RUN:
    if ((now - t_mark) >= 5000) {
      Relay_Off(&relay);
      t_mark = now;
      mode = RELAY_MODE_COOLDOWN;
    }
    break;

  case RELAY_MODE_COOLDOWN:
    if ((now - t_mark) >= 2000) {
      mode = RELAY_MODE_IDLE;
    }
    break;
  }
}
```

### 8) ตารางตัวอย่างฟังก์ชันให้ครบทุก API

| API | ตัวอย่าง |
|---|---|
| Relay_Init | Relay_Init(&relay, PC0, RELAY_ACTIVE_LOW); |
| Relay_On | Relay_On(&relay); |
| Relay_Off | Relay_Off(&relay); |
| Relay_Toggle | Relay_Toggle(&relay); |
| Relay_Set | Relay_Set(&relay, 1); |
| Relay_IsOn | if (Relay_IsOn(&relay)) { ... } |

---

## Troubleshooting

### รีเลย์ติดตลอดหรือดับตลอด

| สาเหตุ | วิธีแก้ |
|---|---|
| เลือก active level ผิด | เปลี่ยน RELAY_ACTIVE_HIGH/LOW ให้ตรงโมดูล |
| ไฟเลี้ยงโมดูลไม่พอ | ตรวจ VCC และกระแส |

### สั่งติดแล้วมีเสียงแต่โหลดไม่ทำงาน

| สาเหตุ | วิธีแก้ |
|---|---|
| ต่อขา COM/NO/NC ผิด | ตรวจ wiring หน้าโหลด |
| โหลดเกินสเปกหน้าสัมผัส | ใช้ relay/contactor ที่รองรับ |

### MCU รีเซ็ตตอนสวิตช์โหลด

| สาเหตุ | วิธีแก้ |
|---|---|
| มีสัญญาณรบกวนจากโหลดเหนี่ยวนำ | ใส่ snubber/varistor/flyback ตามชนิดโหลด |
| กราวด์หรือไฟเลี้ยงตก | ปรับ power routing และ decoupling |

---

## API Reference

### Types

- Relay_ActiveLevel
  - RELAY_ACTIVE_HIGH
  - RELAY_ACTIVE_LOW
- Relay_Status
  - RELAY_OK
  - RELAY_ERROR_PARAM
- Relay_Instance

### Functions

- Relay_Status Relay_Init(Relay_Instance* relay, uint8_t pin, Relay_ActiveLevel level)
  - เริ่มต้น relay instance และกำหนด active level

- void Relay_On(Relay_Instance* relay)
  - เปิด relay ตามตรรกะจริงของโมดูล

- void Relay_Off(Relay_Instance* relay)
  - ปิด relay ตามตรรกะจริงของโมดูล

- void Relay_Toggle(Relay_Instance* relay)
  - สลับสถานะ ON/OFF

- void Relay_Set(Relay_Instance* relay, uint8_t state)
  - กำหนดสถานะโดยตรง (1=ON, 0=OFF)

- uint8_t Relay_IsOn(Relay_Instance* relay)
  - คืนค่าสถานะล่าสุดของ relay (1=ON, 0=OFF)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)