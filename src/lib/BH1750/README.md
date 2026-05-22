# BH1750 Light Sensor Library

> **Library สำหรับอ่านค่าความสว่าง (Lux) ด้วย BH1750 Digital Light Sensor ผ่าน I2C สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
5. [โหมดการวัด](#โหมดการวัด)
6. [API Reference](#api-reference)

---

## ภาพรวม

BH1750 คือเซนเซอร์วัดความเข้มแสงแบบดิจิทัล (Ambient Light Sensor) ที่ให้ผลลัพธ์เป็นหน่วย **Lux** โดยตรง ไม่ต้องคำนวณซับซ้อนเหมือนการใช้ LDR มีความแม่นยำสูงและตอบสนองต่อแสงคล้ายกับดวงตามนุษย์

---

## คุณสมบัติ

- ✅ ช่วงการวัดกว้าง: 1 - 65535 Lux
- ✅ ความละเอียดสูงสุด 0.5 Lux (ในโหมด High-Res 2)
- ✅ สื่อสารผ่าน I2C (รองรับ 2 Address)
- ✅ มีโหมดประหยัดพลังงาน (Power Down) กินกระแสเพียง 0.01µA
- ✅ ไม่ขึ้นกับแหล่งกำเนิดแสง (แสงอาทิตย์, หลอดไฟฟลูออเรสเซนต์, หลอด LED)

---

## Hardware Setup

### การเชื่อมต่อ

| BH1750 Pin | CH57x Pin | หมายเหตุ |
|------------|--------------|----------|
| VCC        | 3.3V         | แรงดันไฟเลี้ยง (2.4V - 3.6V) |
| GND        | GND          | กราวด์ร่วม |
| SCL        | PC2          | สัญญาณนาฬิกา I2C (ต้องการ Pull-up 4.7kΩ) |
| SDA        | PC1          | ข้อมูล I2C (ต้องการ Pull-up 4.7kΩ) |
| ADDR       | GND / VCC    | กำหนด I2C Address (ดูด้านล่าง) |

### การตั้ง I2C Address

- **ADDR = GND** (หรือลอยไว้): Address **0x23** (Macro: `BH1750_ADDR_LOW`)
- **ADDR = VCC**: Address **0x5C** (Macro: `BH1750_ADDR_HIGH`)

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/BH1750/BH1750.h"

BH1750_Instance lightSensor;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้น BH1750 (Address 0x23)
    BH1750_Init(&lightSensor, BH1750_ADDR_LOW);

    while(1) {
        // 2. อ่านค่าความสว่างเป็น Lux
        float lux = BH1750_ReadLux(&lightSensor);
        
        if (lux >= 0) {
            printf("Light Intensity: %.1f Lux\r\n", lux);
        } else {
            printf("Error reading sensor!\r\n");
        }
        
        Delay_Ms(500);
    }
}
```

---

## โหมดการวัด

Library รองรับการตั้งค่าโหมดการวัดเพื่อความเหมาะสมของงาน:

| โหมด (Mode) | ความละเอียด | เวลาในการวัด | คำอธิบาย |
|-------------|------------|--------------|----------|
| `BH1750_CONT_H_RES` | 1.0 Lux | 120ms | วัดต่อเนื่อง ความละเอียดสูง (Default) |
| `BH1750_CONT_H_RES2`| 0.5 Lux | 120ms | วัดต่อเนื่อง ความละเอียดสูงพิเศษ |
| `BH1750_CONT_L_RES` | 4.0 Lux | 16ms | วัดต่อเนื่อง ความเร็วสูง (ความละเอียดต่ำ) |
| `BH1750_ONE_H_RES`  | 1.0 Lux | 120ms | วัดครั้งเดียวแล้วเข้าโหมดประหยัดไฟ |

---

## API Reference

- `BH1750_Init(bh, addr)` : เริ่มต้นเซนเซอร์
- `BH1750_SetMode(bh, mode)` : ตั้งโหมดการวัด
- `BH1750_ReadLux(bh)` : อ่านค่าความสว่าง (หน่วย Lux)
- `BH1750_PowerDown(bh)` : เข้าโหมดประหยัดพลังงาน
- `BH1750_PowerUp(bh)` : ปลุกเซนเซอร์ให้กลับมาทำงาน

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)