# WaterFlow_YFS201 — Water Flow Sensor Library

> **สำหรับ CH57x series**

---

## 📦 Module: YF-S201 Water Flow Sensor

YF-S201 วัดอัตราการไหลของน้ำด้วย Hall Effect sensor + ใบพัด

| Property | Value |
|----------|-------|
| Protocol | GPIO + EXTI Interrupt |
| VCC | 3.3V - 5V |
| K-Factor | ~450 pulses/L (default) |
| Flow Range | 1-30 L/min |
| RAM Usage | ~32 bytes |

---

## 🔌 การต่อสาย

```
YF-S201            CH57x
Red   (VCC) ---->  3.3V - 5V
Black (GND) ---->  GND
Yellow(SIG) ---->  GPIO (ต้องเป็น EXTI pin)
```

> **⚠️ สำคัญ:** SIG ต้องต่อกับ pin ที่รองรับ EXTI interrupt (PC0, PC1, PC2, PD0, PD1, ฯลฯ)

---

## 🚀 Quick Start

```c
#include "Lib/WaterFlow_YFS201/WaterFlow.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    WaterFlow_Instance flow;
    WaterFlow_Init(&flow, PC0, 450.0f);

    while (1) {
        float lpm = WaterFlow_GetFlowRate(&flow);
        float total = WaterFlow_GetTotalVolume(&flow);

        USART_Print("Flow: %.2f L/min, Total: %.2f L\r\n", lpm, total);

        Delay_Ms(1000);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `WaterFlow_Init(flow, pin, k_factor)` | เริ่มต้น flow sensor |
| `WaterFlow_GetPulseCount(flow)` | อ่านจำนวน pulse ทั้งหมด |
| `WaterFlow_GetFlowRate(flow)` | อัตราการไหล (L/min) |
| `WaterFlow_GetTotalVolume(flow)` | ปริมาณน้ำทั้งหมด (ลิตร) |
| `WaterFlow_Reset(flow)` | รีเซ็ต pulse count |
| `WaterFlow_SetKFactor(flow, k)` | ปรับ K-factor สำหรับ sensor รุ่นอื่น |

---

## ⚙️ K-Factor Guide

| Model | K-Factor (pulses/L) |
|-------|-------------------|
| YF-S201 | 450 |
| YF-B1 | 660 |
| YF-B3 | 834 |
| YF-B5 | 450 |

> K-factor จริงอาจคลาดเคลื่อน ±10% ขึ้นกับแรงดันน้ำและการติดตั้ง

---

## 💡 Use Cases

- **Smart irrigation:** วัดปริมาณน้ำที่ใช้รดต้นไม้
- **Water meter:** บันทึกการใช้น้ำรายวัน
- **Leak detection:** ตรวจจับน้ำรั่วจากการไหลต่อเนื่อง

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)