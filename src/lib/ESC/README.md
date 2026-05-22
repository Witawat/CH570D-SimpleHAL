# ESC — Electronic Speed Controller Library

> **สำหรับ CH57x series**

---

## 📦 Module: BLDC ESC (Brushless Motor)

ควบคุมมอเตอร์ BLDC ผ่าน ESC มาตรฐานด้วยสัญญาณ PWM 50Hz

| Property | Value |
|----------|-------|
| Protocol | PWM 50Hz (1000-2000µs) |
| Max ESCs | 4 |
| Safety | Arm/Disarm |
| RAM Usage | ~32 bytes |

---

## 🔌 การต่อสาย

```
ESC                 CH57x
Signal (White/Yellow) → PWM pin
GND   (Black/Brown)  → GND
Power (Red)          → Battery (LiPo 2S-6S)
Motor (3 wires)      → Brushless Motor
```

---

## 🚀 Quick Start

```c
#include "Lib/ESC/ESC.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    ESC_Instance esc;
    ESC_Init(&esc, PWM1_CH1, 1000, 2000);

    /* Arm ESC (ต้องทำก่อน!) */
    ESC_Arm(&esc);
    Delay_Ms(3000);  // รอ ESC beep

    while (1) {
        /* ramp up slowly */
        for (uint8_t t = 0; t <= 100; t += 5) {
            ESC_SetThrottle(&esc, t);
            Delay_Ms(200);
        }

        /* ramp down */
        for (uint8_t t = 100; t > 0; t -= 5) {
            ESC_SetThrottle(&esc, t);
            Delay_Ms(200);
        }

        ESC_Stop(&esc);
        Delay_Ms(5000);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `ESC_Init(esc, ch, min, max)` | เริ่มต้น ESC |
| `ESC_Arm(esc)` | Arm ESC (ต้องเรียกก่อนใช้) |
| `ESC_SetThrottle(esc, pct)` | ตั้ง throttle 0-100% |
| `ESC_SetThrottleMicroseconds(esc, us)` | ตั้ง throttle ด้วย pulse (µs) |
| `ESC_Calibrate(esc)` | Auto-calibrate ESC |
| `ESC_Stop(esc)` | หยุดมอเตอร์ (0%) |
| `ESC_Disarm(esc)` | Disarm (กลับสู่ปลอดภัย) |
| `ESC_IsArmed(esc)` | เช็คว่า armed หรือยัง |

---

## ⚙️ Calibration

1. เรียก `ESC_Calibrate(&esc)`
2. ESC จะส่ง max throttle → **เสียบแบตเตอรี่** → รอ beep
3. ESC จะลดเป็น min throttle → รอ beep ยืนยัน
4. Calibration เสร็จ — ESC พร้อมใช้งาน

> **⚠️ ถอด propeller ออกก่อน calibrate ทุกครั้ง!**

---

## 💡 Use Cases

- **Quadcopter / Drone:** ควบคุมมอเตอร์ 4 ตัว
- **RC Plane:** throttle control
- **Robotics:** BLDC motor for high-torque applications
- **Pump / Fan:** variable speed control

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)