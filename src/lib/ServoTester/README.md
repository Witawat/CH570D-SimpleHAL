# ServoTester — Servo Calibration & Diagnostic Tool

> **สำหรับ CH57x series**

---

## 📦 Module: Servo Tester

เครื่องมือสำหรับทดสอบและ calibrate servo motor — ใช้หา pulse range ที่เหมาะสม

| Property | Value |
|----------|-------|
| Protocol | PWM 50Hz |
| Channels | 1 |
| Functions | Sweep, FindCenter, FindPulseRange, Manual Pulse |
| RAM Usage | ~32 bytes |

---

## 🔌 การต่อสาย

```
Servo              CH57x
Signal (White/Orange) → PWM pin
VCC  (Red)           → External 5V (ห้ามใช้ 3.3V จาก MCU!)
GND  (Black/Brown)   → GND
```

> **⚠️ ใช้ไฟ 5V แยกสำหรับ servo — CH57x จ่ายกระแสไม่พอ!**

---

## 🚀 Quick Start

```c
#include "Lib/ServoTester/ServoTester.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    ServoTester_Instance tester;
    ServoTester_Init(&tester, PWM1_CH1);

    /* 1. Auto-sweep: ดูการเคลื่อนที่ของ servo */
    ServoTester_Sweep(&tester, 500, 2500, 100, 300);

    /* 2. หาค่า center (midpoint ของช่วงที่ใช้) */
    uint16_t center = ServoTester_FindCenter(&tester, 1000, 2000);
    // center = 1500µs

    /* 3. หาค่า min/max */
    uint16_t min_pulse, max_pulse;
    ServoTester_FindPulseRange(&tester, 1000, 2000, &min_pulse, &max_pulse);

    /* 4. ใช้ค่า min/max กับ Servo library */
    Servo_Instance servo;
    Servo_Init(&servo, PWM1_CH1);
    Servo_SetPulseRange(&servo, min_pulse, max_pulse);
    Servo_Attach(&servo);

    while (1) {
        Servo_Write(&servo, 90);
        Delay_Ms(1000);
        Servo_Write(&servo, 0);
        Delay_Ms(1000);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `ServoTester_Init(tester, channel)` | เริ่มต้น tester |
| `ServoTester_Sweep(tester, start, end, step, delay)` | Auto-sweep pulse |
| `ServoTester_FindCenter(tester, start, end)` | หา pulse กลาง |
| `ServoTester_FindPulseRange(tester, start, end, min, max)` | หา range min/max |
| `ServoTester_SetPulse(tester, us)` | ส่ง pulse เอง |
| `ServoTester_GetCurrentPulse(tester)` | อ่าน pulse ปัจจุบัน |

---

## 🔧 Workflow การ Calibrate Servo

1. **Sweep:** `ServoTester_Sweep(&tester, 500, 2500, 50, 200)` → สังเกต servo ขยับ
2. **หา min:** pulse ต่ำสุดที่ servo เริ่มขยับจาก 0°
3. **หา max:** pulse สูงสุดที่ servo ถึง 180° (ก่อนจะกระตุก)
4. **หา center:** `ServoTester_FindCenter(&tester, min, max)` → ตำแหน่ง 90°
5. **บันทึกค่า:** ใช้ min/max กับ `Servo_SetPulseRange()`

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)