# SHT3x Temperature & Humidity Sensor Library

> **Library สำหรับอ่านอุณหภูมิและความชื้นด้วยเซนเซอร์ตระกูล SHT3x (SHT30, SHT31, SHT35) สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [ความแม่นยำและ CRC](#ความแม่นยำและ-crc)
5. [API Reference](#api-reference)

---

## ภาพรวม

เซนเซอร์ตระกูล SHT3x จาก Sensirion เป็นหนึ่งในเซนเซอร์วัดอุณหภูมิและความชื้นที่ดีที่สุดในตลาด ให้ความแม่นยำสูง เสถียร และสื่อสารผ่านดิจิทัล (I2C) เต็มรูปแบบ
- **SHT30**: รุ่นประหยัด (±0.3°C, ±3%RH)
- **SHT31**: รุ่นมาตรฐาน (±0.2°C, ±2%RH)
- **SHT35**: รุ่นความแม่นยำสูงพิเศษ (±0.1°C, ±1.5%RH)

---

## Hardware Setup

### การเชื่อมต่อ

| SHT3x Pin | CH57x Pin | หมายเหตุ |
|-----------|--------------|----------|
| VDD       | 3.3V / 5V    | แรงดันไฟเลี้ยง (2.15V - 5.5V) |
| GND       | GND          | กราวด์ร่วม |
| SCL       | PC2          | สัญญาณนาฬิกา I2C (ต้องการ Pull-up 4.7kΩ) |
| SDA       | PC1          | ข้อมูล I2C (ต้องการ Pull-up 4.7kΩ) |
| ADDR      | GND / VCC    | กำหนด I2C Address (ดูด้านล่าง) |

### การตั้ง I2C Address (ADDR Pin)

- **ADDR = GND**: Address **0x44** (Macro: `SHT3X_ADDR_LOW`)
- **ADDR = VCC**: Address **0x45** (Macro: `SHT3X_ADDR_HIGH`)

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/SHT3x/SHT3x.h"

SHT3x_Instance sht;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้น SHT3x
    if (SHT3x_Init(&sht, SHT3X_ADDR_LOW) == SHT3X_OK) {
        printf("SHT3x Initialized\r\n");
    }

    float temp, hum;

    while(1) {
        // 2. อ่านค่าอุณหภูมิและความชื้น
        if (SHT3x_Read(&sht, &temp, &hum) == SHT3X_OK) {
            printf("Temp: %.2f C, Hum: %.1f %%\r\n", temp, hum);
        } else {
            printf("Sensor Error!\r\n");
        }
        
        Delay_Ms(2000);
    }
}
```

---

## ความแม่นยำและ CRC

Library นี้ใช้ระบบตรวจสอบความถูกต้องของข้อมูลด้วย **CRC-8** (Cyclic Redundancy Check) หากข้อมูลระหว่างการสื่อสารเกิดการรบกวน ฟังก์ชัน `SHT3x_Read` จะคืนค่า `SHT3X_ERROR_CRC` เพื่อให้มั่นใจว่าค่าที่นำไปใช้งานนั้นถูกต้อง 100%

นอกจากนี้ยังสามารถตั้งค่า **Repeatability** ได้:
- `SHT3X_REP_HIGH`: แม่นยำสูงสุด (ใช้เวลาวัดนานสุด ~15ms)
- `SHT3X_REP_MEDIUM`: ระดับปานกลาง
- `SHT3X_REP_LOW`: เร็วที่สุด (ความแม่นยำลดลงเล็กน้อย)

---

## API Reference

- `SHT3x_Init(sht, addr)` : เริ่มต้นเซนเซอร์
- `SHT3x_Read(sht, &temp, &hum)` : อ่านค่าอุณหภูมิ (°C) และความชื้น (%RH)
- `SHT3x_SetRepeatability(sht, rep)` : ตั้งค่าความแม่นยำในการวัด
- `SHT3x_Reset(sht)` : สั่ง Soft Reset เซนเซอร์
- `SHT3x_GetStatus(sht, &status)` : อ่านค่าสถานะภายในของเซนเซอร์

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)