# OH49E Library

ไลบรารีอ่านค่า Linear Hall Effect Sensor รุ่น OH49E ผ่าน ADC บน CH57x
รองรับการอ่านค่าแรงดัน, ตรวจจับสนามแม่เหล็ก, ทิศทางขั้ว และค่าความเข้มแบบ normalize

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

OH49E เป็น Hall sensor แบบเชิงเส้น (Linear)
เมื่อไม่มีสนามแม่เหล็ก ค่าเอาต์พุตจะอยู่ใกล้ VCC/2 (quiescent point)

- แรงดันสูงกว่า quiescent: ขั้วเหนือ (North)
- แรงดันต่ำกว่า quiescent: ขั้วใต้ (South)

ไลบรารีช่วยแปลงจาก ADC raw เป็นแรงดันและการตีความระดับสนามโดยตรง

---

## คุณสมบัติ

- อ่านค่า ADC raw และแรงดันจริงได้
- ตรวจว่ามีสนามแม่เหล็กหรือไม่ด้วย threshold
- บอกทิศทางสนาม (NONE/NORTH/SOUTH)
- คำนวณความเข้มสนามแบบ normalized 0.0-1.0
- ปรับ threshold runtime ได้

---

## Hardware Setup

### Wiring

| OH49E | CH57x |
|---|---|
| VCC | 3.3V |
| GND | GND |
| OUT | ADC channel ที่รองรับ |

ตัวอย่าง ADC channel ที่ใช้งานได้ให้ดูตาม SimpleHAL ของโปรเจกต์

ข้อควรระวัง

- ค่าที่แม่นขึ้นอยู่กับความนิ่งของ Vref และ noise บนสายสัญญาณ
- ควรมีการกรองค่า (moving average) เมื่อใช้งานในสภาพแวดล้อมรบกวนสูง

---

## หลักการทำงาน

### Quiescent point

ไลบรารีตั้งค่า

- quiescent_v = vcc / 2

แล้วเทียบแรงดันที่อ่านได้กับ threshold

- |Vout - quiescent_v| <= threshold: ถือว่าไม่มีสนาม
- Vout > quiescent_v + threshold: NORTH
- Vout < quiescent_v - threshold: SOUTH

### Field strength แบบ normalized

คำนวณจากระยะห่างจาก quiescent ต่อช่วงสูงสุดโดยประมาณ
แล้ว clamp ให้อยู่ในช่วง 0.0-1.0

---

## การใช้งานพื้นฐาน

### Quick start

```c
#include "SimpleHAL.h"
#include "OH49E.h"

OH49E_Instance hall;

int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    OH49E_Init(&hall, ADC_CH_PA2, 3.3f, 3.3f);

    while (1) {
        float v = OH49E_ReadVoltage(&hall);
        (void)v;
        Delay_Ms(100);
    }
}
```

### ตรวจว่ามีสนามแม่เหล็กหรือไม่

```c
if (OH49E_IsFieldDetected(&hall)) {
    // มีสนามแม่เหล็ก
}
```

---

## การใช้งานขั้นสูง

### อ่านทิศทางสนาม

```c
OH49E_FieldDir dir = OH49E_GetFieldDirection(&hall);
if (dir == OH49E_FIELD_NORTH) {
    // N-pole
} else if (dir == OH49E_FIELD_SOUTH) {
    // S-pole
}
```

### อ่านความเข้มสนาม

```c
float strength = OH49E_GetFieldStrength(&hall); // 0.0-1.0
```

### ปรับ threshold ให้เหมาะกับหน้างาน

```c
OH49E_SetThreshold(&hall, 0.10f);  // ไวขึ้น
// หรือ
OH49E_SetThreshold(&hall, 0.20f);  // ทน noise มากขึ้น
```

---

## Troubleshooting

### ค่าแกว่งแม้ไม่มีแม่เหล็ก

| สาเหตุ | วิธีแก้ |
|---|---|
| สัญญาณรบกวนสูง | เพิ่ม decoupling และทำ moving average |
| threshold ต่ำเกิน | เพิ่ม threshold เช่น 0.15 -> 0.20 |

### ทิศทางกลับด้านจากที่คาด

| สาเหตุ | วิธีแก้ |
|---|---|
| วาง sensor กลับหน้า | หมุนหัวเซนเซอร์หรือกลับด้านแม่เหล็ก |
| นิยามขั้วหน้างานต่างกัน | ปรับ mapping ในแอปพลิเคชัน |

### ค่าแรงดันดูไม่สมเหตุผล

| สาเหตุ | วิธีแก้ |
|---|---|
| vref ตั้งไม่ตรง | ใส่ค่า vref ให้ตรงระบบจริง |
| ต่อ OUT ผิดช่อง ADC | ตรวจ channel ที่ส่งเข้า Init |

---

## API Reference

### Constants

- OH49E_DEFAULT_THRESHOLD_V = 0.15f

### Types

- OH49E_Status
  - OH49E_OK
  - OH49E_ERROR_PARAM
- OH49E_FieldDir
  - OH49E_FIELD_NONE
  - OH49E_FIELD_NORTH
  - OH49E_FIELD_SOUTH
- OH49E_Instance

### Functions

- OH49E_Status OH49E_Init(OH49E_Instance* hall, ADC_Channel adc_channel, float vcc, float vref)
- uint16_t OH49E_ReadRaw(OH49E_Instance* hall)
- float OH49E_ReadVoltage(OH49E_Instance* hall)
- uint8_t OH49E_IsFieldDetected(OH49E_Instance* hall)
- OH49E_FieldDir OH49E_GetFieldDirection(OH49E_Instance* hall)
- float OH49E_GetFieldStrength(OH49E_Instance* hall)
- void OH49E_SetThreshold(OH49E_Instance* hall, float threshold_v)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)