# SoundSensor_KY038 — Sound Sensor Library

> **สำหรับ CH57x series**

---

## 📦 Module: KY-038 Sound Sensor

KY-038 เป็นโมดูลตรวจจับเสียงราคาถูก มี condenser microphone + LM393

| Property | Value |
|----------|-------|
| Protocol | ADC |
| VCC | 3.3V - 5V |
| Sensitivity | ปรับได้ด้วย potentiometer |
| Output | Analog (A0) + Digital (D0) |
| RAM Usage | ~16 bytes |

---

## 🔌 การต่อสาย

```
KY-038             CH57x
VCC  ----------->  3.3V
GND  ----------->  GND
A0   ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
D0   ----------->  (optional) GPIO — HIGH เมื่อเสียงเกิน threshold
```

---

## 🚀 Quick Start

```c
#include "Lib/SoundSensor_KY038/SoundSensor.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    SoundSensor_Instance sound;
    SoundSensor_Init(&sound, ADC_CH_PA1);

    while (1) {
        float level = SoundSensor_ReadLevel(&sound);

        if (SoundSensor_IsClapDetected(&sound, 0.6f)) {
            USART_Print("CLAP! Level: %.2f\r\n", level);
        }

        Delay_Ms(100);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `SoundSensor_Init(sound, adc_channel)` | เริ่มต้นเซนเซอร์ |
| `SoundSensor_ReadRaw(sound)` | อ่านค่า ADC raw (0-4095) |
| `SoundSensor_ReadLevel(sound)` | อ่านระดับเสียง 0.0-1.0 |
| `SoundSensor_IsClapDetected(sound, threshold)` | ตรวจจับเสียงดังเกิน threshold |
| `SoundSensor_GetPeak(sound)` | อ่านค่า peak สูงสุดตั้งแต่ init |
| `SoundSensor_SetThreshold(sound, val)` | ตั้ง default threshold |

---

## 💡 Use Cases

- **Clap switch:** ใช้ `SoundSensor_IsClapDetected(&sound, 0.5f)` ตรวจจับเสียงปรบมือ
- **Voice activity:** ใช้ `SoundSensor_ReadLevel()` วัดระดับเสียง
- **Noise monitor:** ใช้ `SoundSensor_GetPeak()` ติดตามเสียงดังที่สุดในช่วงเวลาหนึ่ง

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)