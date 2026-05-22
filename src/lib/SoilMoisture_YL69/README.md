# SoilMoisture_YL69 — Soil Moisture Sensor Library

> **สำหรับ CH57x series**

---

## 📦 Module: YL-69 Soil Moisture Sensor

YL-69 เป็นเซนเซอร์วัดความชื้นในดินราคาถูก ใช้งานง่ายผ่าน ADC

| Property | Value |
|----------|-------|
| Protocol | ADC |
| VCC | 3.3V - 5V |
| Output | Analog (AOUT) + Digital (DOUT) |
| RAM Usage | ~16 bytes |

---

## 🔌 การต่อสาย

```
YL-69              CH57x
VCC  ----------->  3.3V
GND  ----------->  GND
AOUT ----------->  ADC pin (PA1, PA2, PC4, PD2-PD7)
DOUT ----------->  (optional) GPIO — digital threshold output
```

---

## 🚀 Quick Start

```c
#include "Lib/SoilMoisture_YL69/SoilMoisture.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    SoilMoisture_Instance soil;
    SoilMoisture_Init(&soil, ADC_CH_PA1);

    /* Calibrate: อ่าน raw ตอนแห้ง (ในอากาศ) และตอนเปียก (จุ่มน้ำ) */
    SoilMoisture_Calibrate(&soil, 3200, 1800);

    while (1) {
        uint8_t moisture = SoilMoisture_Read(&soil);
        USART_Print("Moisture: %d%%\r\n", moisture);

        if (SoilMoisture_IsDry(&soil, 30)) {
            USART_Print("Soil is DRY! Water needed.\r\n");
        }

        Delay_Ms(1000);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `SoilMoisture_Init(soil, adc_channel)` | เริ่มต้นเซนเซอร์ |
| `SoilMoisture_Calibrate(soil, dry, wet)` | ตั้งค่า calibrate (dry=ในอากาศ, wet=ในน้ำ) |
| `SoilMoisture_ReadRaw(soil)` | อ่านค่า ADC raw (0-4095) |
| `SoilMoisture_Read(soil)` | อ่านความชื้น 0-100% |
| `SoilMoisture_IsDry(soil, threshold)` | เช็คว่าดินแห้งเกิน threshold หรือไม่ |

---

## ⚙️ Calibration

1. ปัก probe ในอากาศ (ดินแห้งสนิท) → อ่านค่า raw → `dry_value`
2. จุ่ม probe ในน้ำสะอาด → อ่านค่า raw → `wet_value`
3. เรียก `SoilMoisture_Calibrate(&soil, dry_value, wet_value)`

> **หมายเหตุ:** ค่า `dry_value` จะสูงกว่า `wet_value` เสมอ เพราะดินแห้งมีความต้านทานสูง

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)