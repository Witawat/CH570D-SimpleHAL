# FlameSensor_KY026 — Flame Sensor Library

> **สำหรับ CH57x series**

---

## 📦 Module: KY-026 Flame Sensor

KY-026 ตรวจจับเปลวไฟผ่าน Infrared (760nm-1100nm) มีทั้ง analog และ digital output

| Property | Value |
|----------|-------|
| Protocol | ADC + GPIO |
| VCC | 3.3V - 5V |
| Detection Range | 20cm - 100cm |
| Output | Analog (A0) + Digital (D0) |
| RAM Usage | ~16 bytes |

---

## 🔌 การต่อสาย

```
KY-026             CH57x
VCC  ----------->  3.3V
GND  ----------->  GND
A0   ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
D0   ----------->  GPIO (optional) — HIGH เมื่อมีเปลวไฟ
```

---

## 🚀 Quick Start

```c
#include "Lib/FlameSensor_KY026/FlameSensor.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    FlameSensor_Instance flame;
    FlameSensor_Init(&flame, ADC_CH_PA1, PD0);

    while (1) {
        float intensity = FlameSensor_ReadIntensity(&flame);

        if (FlameSensor_IsFlameDetected(&flame)) {
            USART_Print("FIRE DETECTED! Intensity: %.2f\r\n", intensity);
        }

        Delay_Ms(200);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `FlameSensor_Init(flame, adc, digi_pin)` | เริ่มต้นเซนเซอร์ (ใช้ `0xFF` ถ้าไม่ต่อ digital pin) |
| `FlameSensor_ReadRaw(flame)` | อ่านค่า ADC raw (0-4095) |
| `FlameSensor_ReadIntensity(flame)` | อ่านความเข้มเปลวไฟ 0.0-1.0 |
| `FlameSensor_IsFlameDetected(flame)` | ตรวจพบเปลวไฟหรือไม่ |
| `FlameSensor_SetThreshold(flame, val)` | ตั้ง software threshold (0.0-1.0) |

---

## ⚙️ การตรวจจับ 2 โหมด

### โหมด 1: Digital (D0) — แนะนำ
- ต่อ D0 เข้ากับ GPIO
- ปรับ threshold ด้วย potentiometer บนโมดูล
- `FlameSensor_IsFlameDetected()` ใช้ hardware detection

### โหมด 2: Analog (A0) only
- ไม่ต้องต่อ D0 (ส่ง `0xFF` แทน pin)
- ใช้ ADC เทียบกับ software threshold
- `FlameSensor_SetThreshold()` ปรับค่าได้เอง

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)