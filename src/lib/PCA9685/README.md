# PCA9685 16-Channel PWM Expander Library

> **Library สำหรับควบคุม PWM 16 ช่อง ด้วย PCA9685 ผ่าน I2C สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานกับ Servo Motor](#การใช้งานกับ-servo-motor)
4. [การใช้งานกับ LED Dimming](#การใช้งานกับ-led-dimming)
5. [API Reference](#api-reference)

---

## ภาพรวม

PCA9685 คือโมดูลขยายช่องสัญญาณ PWM (Pulse Width Modulation) ขนาด **16 ช่อง** ความละเอียด **12-bit** (4096 ระดับ) สื่อสารผ่าน I2C  
ช่วยประหยัดขา GPIO ของ CH57x และเหมาะอย่างยิ่งสำหรับโปรเจกต์หุ่นยนต์ที่ใช้ Servo หลายตัว หรือการหรี่ไฟ LED จำนวนมาก

---

## Hardware Setup

### การเชื่อมต่อ

| PCA9685 Pin | CH57x Pin | หมายเหตุ |
|-------------|--------------|----------|
| VCC         | 3.3V         | ไฟเลี้ยงส่วน Logic |
| GND         | GND          | กราวด์ร่วม |
| SCL         | PC2          | สัญญาณนาฬิกา I2C (ต้องการ Pull-up 4.7kΩ) |
| SDA         | PC1          | ข้อมูล I2C (ต้องการ Pull-up 4.7kΩ) |
| V+          | 5V - 6V      | ไฟเลี้ยงสำหรับ Servo / LED (แยกจาก MCU) |
| OE          | GND          | Output Enable (ต้องต่อ GND เพื่อให้ Output ทำงาน) |

### การตั้ง I2C Address

โมดูลมีขา A0 - A5 สำหรับเปลี่ยน Address (ลากเข้า VCC เพื่อเป็น 1)
- **Default (GND ทั้งหมด)**: Address **0x40** (Macro: `PCA9685_ADDR_DEFAULT`)

---

## การใช้งานกับ Servo Motor

สำหรับการใช้ Servo ต้องตั้งความถี่ไปที่ **50Hz**

```c
#include "main.h"
#include "Lib/PCA9685/PCA9685.h"

PCA9685_Instance pwm;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้น PCA9685 ที่ความถี่ 50Hz (สำหรับ Servo)
    PCA9685_Init(&pwm, PCA9685_ADDR_DEFAULT, 50);

    while(1) {
        // 2. สั่ง Servo ช่อง 0 ไปที่ 0 องศา
        PCA9685_SetServoAngle(&pwm, 0, 0);
        Delay_Ms(1000);

        // 3. สั่ง Servo ช่อง 0 ไปที่ 90 องศา
        PCA9685_SetServoAngle(&pwm, 0, 90);
        Delay_Ms(1000);

        // 4. สั่ง Servo ช่อง 0 ไปที่ 180 องศา
        PCA9685_SetServoAngle(&pwm, 0, 180);
        Delay_Ms(1000);
    }
}
```

---

## การใช้งานกับ LED Dimming

สำหรับการหรี่ไฟ LED แนะนำความถี่สูงขึ้น เช่น **1000Hz** เพื่อไม่ให้ตาคนเห็นการกระพริบ

```c
// เริ่มต้นที่ 1000Hz
PCA9685_Init(&pwm, PCA9685_ADDR_DEFAULT, 1000);

// หรี่ไฟช่อง 1 ไปที่ความสว่าง 50%
PCA9685_SetDuty(&pwm, 1, 50.0f);

// เปิดไฟช่อง 2 เต็ม 100%
PCA9685_FullOn(&pwm, 2);

// ปิดไฟช่อง 3
PCA9685_Off(&pwm, 3);
```

---

## API Reference

- `PCA9685_Init(pca, addr, freq)` : เริ่มต้นโมดูล (ความถี่ 24Hz - 1526Hz)
- `PCA9685_SetPWM(pca, ch, on, off)` : ตั้งค่า PWM ขั้นสูง (0-4095)
- `PCA9685_SetDuty(pca, ch, duty)` : ตั้งค่าความกว้างสัญญาณเป็นเปอร์เซ็นต์ (0-100%)
- `PCA9685_SetPulse(pca, ch, us)` : ตั้งความกว้างพัลส์เป็นไมโครวินาที (µs)
- `PCA9685_SetServoAngle(pca, ch, angle)` : ตั้งมุมเซอร์โว (0-180 องศา)
- `PCA9685_FullOn(pca, ch)` : เปิด Output ตลอดเวลา
- `PCA9685_Off(pca, ch)` : ปิด Output (หรือทุกช่องถ้า ch=255)
- `PCA9685_Sleep(pca)` : เข้าโหมดประหยัดพลังงาน
- `PCA9685_WakeUp(pca)` : ปลุกโมดูลให้กลับมาทำงาน

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)