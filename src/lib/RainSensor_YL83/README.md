# RainSensor_YL83 — Rain Sensor Library

> **สำหรับ CH57x series**

---

## 📦 Module: YL-83 Rain Sensor

YL-83 ตรวจจับฝน/หยดน้ำด้วยแผ่น PCB ตัวนำไฟฟ้า

| Property | Value |
|----------|-------|
| Protocol | ADC |
| VCC | 3.3V - 5V |
| Output | Analog (A0) + Digital (D0) |
| RAM Usage | ~16 bytes |

---

## 🔌 การต่อสาย

```
YL-83              CH57x
VCC  ----------->  3.3V
GND  ----------->  GND
A0   ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
D0   ----------->  (optional) GPIO — HIGH เมื่อ有水
```

---

## 🚀 Quick Start

```c
#include "Lib/RainSensor_YL83/RainSensor.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    RainSensor_Instance rain;
    RainSensor_Init(&rain, ADC_CH_PA1);

    while (1) {
        float level = RainSensor_ReadLevel(&rain);
        RainSensor_Intensity intensity = RainSensor_GetIntensity(&rain);

        if (RainSensor_IsRaining(&rain, 0.3f)) {
            USART_Print("RAIN! Level: %.2f, Intensity: %d\r\n", level, intensity);
        }

        Delay_Ms(1000);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `RainSensor_Init(rain, adc_channel)` | เริ่มต้นเซนเซอร์ |
| `RainSensor_ReadRaw(rain)` | อ่านค่า ADC raw (0-4095) |
| `RainSensor_ReadLevel(rain)` | อ่านระดับน้ำ 0.0 (แห้ง) - 1.0 (เปียกโชก) |
| `RainSensor_IsRaining(rain, threshold)` | เช็คว่าฝนตกหรือไม่ |
| `RainSensor_GetIntensity(rain)` | ระดับความแรงฝน (NONE/LIGHT/MODERATE/HEAVY) |
| `RainSensor_SetThreshold(rain, val)` | ตั้ง threshold |

---

## 🌧️ Rain Intensity Levels

| Level | Value | Description | ADC Range |
|-------|-------|-------------|-----------|
| `RAIN_NONE` | 0 | ไม่มีฝน | < 1% |
| `RAIN_LIGHT` | 1 | ฝนปรอยๆ | 1-33% |
| `RAIN_MODERATE` | 2 | ฝนปานกลาง | 34-66% |
| `RAIN_HEAVY` | 3 | ฝนหนัก | 67-100% |

---

## 💡 Use Cases

- **Smart home:** ตรวจจับฝนเพื่อปิดหน้าต่างอัตโนมัติ
- **Agriculture:** แจ้งเตือนเมื่อฝนตกในแปลงเกษตร
- **Weather station:** บันทึกข้อมูลฝนร่วมกับเซนเซอร์อื่น

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)