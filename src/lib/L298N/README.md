# L298N Library

ไลบรารีควบคุม DC Motor ผ่าน L298N บน CH57x
รองรับควบคุมทิศทางด้วย IN1/IN2 และควบคุมความเร็วด้วย PWM (ขา EN)

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

L298N เป็น H-Bridge driver สำหรับขับมอเตอร์ DC โดยใช้สัญญาณควบคุมหลัก 3 ขา

- IN1, IN2: กำหนดทิศทาง/โหมดหยุด
- EN: สัญญาณ PWM สำหรับกำหนดความเร็ว

ไลบรารีนี้ออกแบบให้ใช้งานง่ายแบบ instance-based

- เริ่มต้นด้วย L298N_Init
- สั่งหมุนด้วย L298N_Run
- ปรับสปีดแยกได้ด้วย L298N_SetSpeed
- เลือกหยุดแบบ coast หรือ brake ได้

---

## คุณสมบัติ

- รองรับทิศทาง Forward / Backward
- รองรับโหมดหยุด 2 แบบ: Stop (coast) และ Brake (short brake)
- ตั้งความเร็ว 0-100 เปอร์เซ็นต์
- ปรับทิศทางและความเร็วแยกกันได้
- มีโครงสร้างสถานะปัจจุบันใน L298N_Instance

---

## Hardware Setup

### Wiring

| CH57x | L298N | หมายเหตุ |
|---|---|---|
| GPIO (เช่น PC1) | IN1 | ทิศทาง |
| GPIO (เช่น PC2) | IN2 | ทิศทาง |
| PWM pin (เช่น PC0) | ENA/EN | ความเร็ว |
| GND | GND | ต้องกราวด์ร่วม |

### Power Motor

| L298N | แหล่งจ่าย |
|---|---|
| +12V / VIN | ไฟมอเตอร์ (เช่น 6-12V ตามมอเตอร์) |
| GND | GND แหล่งจ่ายมอเตอร์ |
| 5V logic | ตามบอร์ด L298N ที่ใช้ |

ข้อควรระวัง

- ต้องต่อ GND ร่วมกันระหว่าง CH57x, L298N และแหล่งจ่ายมอเตอร์
- อย่าจ่ายไฟมอเตอร์จากบอร์ด MCU โดยตรง

### ตาราง PWM channel ที่รองรับ

| Pin | PWM channel |
|---|---|
| PA1 | PWM1_CH2 |
| PC0 | PWM2_CH3 |
| PC3 | PWM1_CH3 |
| PC4 | PWM1_CH4 |
| PD2 | PWM1_CH1 |
| PD3 | PWM2_CH2 |
| PD4 | PWM2_CH1 |
| PD7 | PWM2_CH4 |

---

## หลักการทำงาน

### Truth table ของ IN1/IN2

| IN1 | IN2 | โหมด |
|---|---|---|
| 1 | 0 | Forward |
| 0 | 1 | Backward |
| 0 | 0 | Stop (coast) |
| 1 | 1 | Brake |

### ความเร็วด้วย PWM

ไลบรารีใช้ duty cycle 0-100%

- 0% หมุนช้ามากหรือหยุด (ขึ้นกับโหลด)
- 100% แรงดันเฉลี่ยสูงสุดไปที่มอเตอร์

---

## การใช้งานพื้นฐาน

### 1) Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "L298N.h"

L298N_Instance motor;
```

### 2) Init และสั่งหมุน

```c
int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    // IN1=PC1, IN2=PC2, EN ใช้ PWM2_CH3 (PC0)
    L298N_Init(&motor, PC1, PC2, PWM2_CH3);

    L298N_Run(&motor, L298N_FORWARD, 70);
    Delay_Ms(2000);

    L298N_Stop(&motor);

    while (1) {
    }
}
```

---

## การใช้งานขั้นสูง

### ปรับความเร็วระหว่างวิ่ง

```c
L298N_Run(&motor, L298N_FORWARD, 30);
Delay_Ms(1000);

L298N_SetSpeed(&motor, 60);
Delay_Ms(1000);

L298N_SetSpeed(&motor, 90);
Delay_Ms(1000);
```

### สลับทิศทางแบบปลอดภัย

```c
L298N_Stop(&motor);
Delay_Ms(150);
L298N_SetDirection(&motor, L298N_BACKWARD);
L298N_SetSpeed(&motor, 60);
```

### ใช้ Brake ตอนต้องหยุดเร็ว

```c
L298N_Run(&motor, L298N_FORWARD, 80);
Delay_Ms(500);
L298N_Brake(&motor);   // หยุดเร็วกว่า Stop
```

---

## Troubleshooting

### มอเตอร์ไม่หมุน

| สาเหตุ | วิธีแก้ |
|---|---|
| ไม่ได้ต่อไฟมอเตอร์ | ตรวจ VIN/12V และ GND ของ L298N |
| เลือก PWM channel ไม่ตรง pin จริง | ตรวจคู่ pin กับ PWM channel |
| GND ไม่ร่วมกัน | ต่อ GND ของทุกวงจรร่วมกัน |

### หมุนได้ทิศเดียว

| สาเหตุ | วิธีแก้ |
|---|---|
| IN1/IN2 ต่อสลับหรือหลวม | ตรวจสาย IN1/IN2 |
| โค้ดไม่เปลี่ยน direction | ใช้ L298N_SetDirection หรือ L298N_Run ใหม่ |

### มอเตอร์ร้อนหรือแรงบิดตก

| สาเหตุ | วิธีแก้ |
|---|---|
| PWM สูงต่อเนื่องนาน | ลด duty หรือพักรอบ |
| มอเตอร์โหลดเกิน | ใช้มอเตอร์/driver ที่เหมาะสม |
| แหล่งจ่ายกระแสไม่พอ | เปลี่ยน PSU ที่กระแสสูงพอ |

---

## API Reference

### Types

- L298N_Status
  - L298N_OK
  - L298N_ERROR_PARAM
- L298N_Direction
  - L298N_FORWARD
  - L298N_BACKWARD
  - L298N_STOP
  - L298N_BRAKE
- L298N_Instance

### Functions

- L298N_Status L298N_Init(L298N_Instance* motor, uint8_t pin_in1, uint8_t pin_in2, PWM_Channel pwm_channel)
  - เริ่มต้นพิน IN1/IN2 และ PWM channel

- void L298N_Run(L298N_Instance* motor, L298N_Direction dir, uint8_t speed)
  - สั่งทิศทางพร้อมความเร็วในคำสั่งเดียว

- void L298N_Stop(L298N_Instance* motor)
  - หยุดแบบ coast (IN1=L, IN2=L)

- void L298N_Brake(L298N_Instance* motor)
  - เบรกแบบ short brake (IN1=H, IN2=H)

- void L298N_SetSpeed(L298N_Instance* motor, uint8_t speed)
  - ปรับความเร็วคงทิศทางเดิม

- void L298N_SetDirection(L298N_Instance* motor, L298N_Direction dir)
  - ปรับทิศทางคงความเร็วเดิม

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)