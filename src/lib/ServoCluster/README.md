# ServoCluster — Multi-Servo Controller with Easing

> **สำหรับ CH57x series**

---

## 📦 Module: Multi-Servo Cluster Controller

ควบคุม servo หลายตัวพร้อมกันแบบ non-blocking พร้อม easing curves 10 รูปแบบ

| Property | Value |
|----------|-------|
| Backend | Dual — HW PWM (8ch) หรือ PCA9685 I2C (16ch) |
| Max Servos | 8 (HW) / 16 (PCA9685) |
| Easing | 10 curves |
| Update | Non-blocking |
| RAM Usage | ~200 bytes |

---

## 🔌 การต่อสาย

### HW PWM (ใช้ Servo.h)
```
Servo Signal → PWM pins: PD2, PA1, PC3, PC4, PD4, PD3, PC0, PD7
```

### PCA9685 I2C
```
PCA9685             CH57x
VCC  ----------->  3.3V
GND  ----------->  GND
SCL  ----------->  PC2 (SCL)
SDA  ----------->  PC1 (SDA)
PWM0-PWM15 ------> Servo signal wires
```

---

## 🚀 Quick Start

```c
#include "Lib/ServoCluster/ServoCluster.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    ServoCluster_Instance cluster;
    ServoCluster_Init(&cluster, SERVO_BACKEND_PCA9685, 4);

    /* add 4 servos */
    ServoCluster_AddServo(&cluster, 0, 500, 2500);
    ServoCluster_AddServo(&cluster, 1, 500, 2500);
    ServoCluster_AddServo(&cluster, 2, 1000, 2000);
    ServoCluster_AddServo(&cluster, 3, 1000, 2000);

    while (1) {
        /* non-blocking moves */
        ServoCluster_MoveTo(&cluster, 0, 90, 1000, EASE_QUAD_IN_OUT);
        ServoCluster_MoveTo(&cluster, 1, 45, 1500, EASE_SINE_OUT);

        /* must call update in loop */
        ServoCluster_Update(&cluster);

        if (ServoCluster_IsAllDone(&cluster)) {
            /* all movements complete — trigger next sequence */
        }

        Delay_Ms(10);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `ServoCluster_Init(cluster, backend, max)` | เริ่มต้น cluster |
| `ServoCluster_AddServo(cluster, ch, min, max)` | เพิ่ม servo เข้า cluster |
| `ServoCluster_MoveTo(cluster, id, angle, dur, ease)` | ขยับ servo ไปที่มุม (non-blocking) |
| `ServoCluster_MoveAll(cluster, angle, dur, ease)` | ขยับทุกตัวพร้อมกัน |
| `ServoCluster_SetEasing(cluster, id, ease)` | เปลี่ยน easing |
| `ServoCluster_SetSpeed(cluster, pct)` | ปรับความเร็วรวม (1-200%) |
| `ServoCluster_Update(cluster)` | อัปเดต animation (เรียกใน loop) |
| `ServoCluster_IsMoving(cluster, id)` | เช็คว่าขยับอยู่ไหม |
| `ServoCluster_IsAllDone(cluster)` | เช็คว่าทุกตัวหยุดหรือยัง |
| `ServoCluster_Stop(cluster, id)` | หยุด servo เฉพาะตัว |
| `ServoCluster_StopAll(cluster)` | หยุดทุกตัว |

---

## 🎨 Easing Curves

| Constant | Curve | Description |
|----------|-------|-------------|
| `EASE_LINEAR` | Linear | ความเร็วคงที่ |
| `EASE_QUAD_IN` | Quadratic In | เร่งจาก 0 |
| `EASE_QUAD_OUT` | Quadratic Out | เบรกตอนปลาย |
| `EASE_QUAD_IN_OUT` | Quadratic In-Out | เร่ง + เบรก |
| `EASE_CUBIC_IN` | Cubic In | เร่งแรง |
| `EASE_CUBIC_OUT` | Cubic Out | เบรกแรง |
| `EASE_CUBIC_IN_OUT` | Cubic In-Out | เร่งแรง + เบรกแรง |
| `EASE_SINE_IN` | Sine In | เร่งแบบ sine |
| `EASE_SINE_OUT` | Sine Out | เบรกแบบ sine |
| `EASE_SINE_IN_OUT` | Sine In-Out | Sine ทั้งเข้า-ออก |

---

## 💡 Use Cases

- **Robot arm:** ขยับหลาย joint พร้อมกันด้วย easing
- **Pan-tilt camera:** smooth tracking
- **Animatronics:** ทำให้หุ่นยนต์เคลื่อนไหวลื่นตา
- **Lighting rig:** ควบคุม servo หลายตัวบน DMX-style rig

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)