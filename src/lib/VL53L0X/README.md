# VL53L0X Time-of-Flight Distance Sensor Library

> **Library สำหรับวัดระยะทางด้วยเลเซอร์ (ToF) ด้วย VL53L0X สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [เปรียบเทียบกับ Ultrasonic](#เปรียบเทียบกับ-ultrasonic)
5. [API Reference](#api-reference)

---

## ภาพรวม

VL53L0X คือเซนเซอร์วัดระยะทางเทคโนโลยี **Time-of-Flight (ToF)** จาก STMicroelectronics โดยการยิงเลเซอร์ IR ที่ไม่เป็นอันตรายออกไปและจับเวลาที่สะท้อนกลับมา ทำให้วัดระยะได้แม่นยำมากโดยไม่ขึ้นกับสีหรือพื้นผิวของวัตถุ

- **ช่วงการวัด**: 30mm - 2000mm (2 เมตร)
- **ความแม่นยำ**: ±3% ในสภาวะปกติ

---

## Hardware Setup

### การเชื่อมต่อ

| VL53L0X Pin | CH57x Pin | หมายเหตุ |
|-------------|--------------|----------|
| VCC         | 3.3V         | แรงดันไฟเลี้ยง (2.6V - 3.5V) |
| GND         | GND          | กราวด์ร่วม |
| SCL         | PC2          | สัญญาณนาฬิกา I2C |
| SDA         | PC1          | ข้อมูล I2C |
| XSHUT       | GPIO / 3.3V  | Shutdown (ต้องเป็น HIGH เพื่อทำงาน) |

> ⚠️ **ข้อควรระวัง**: โมดูลส่วนใหญ่ทำงานที่ 3.3V หากใช้กับบอร์ด 5V ต้องมี Voltage Regulator บนโมดูล

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/VL53L0X/VL53L0X.h"

VL53L0X_Instance tof;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้นเซนเซอร์
    if (VL53L0X_Init(&tof, VL53L0X_ADDR_DEFAULT) == VL53L0X_OK) {
        printf("VL53L0X Ready\r\n");
    }

    while(1) {
        // 2. อ่านค่าระยะทางเป็นมิลลิเมตร (mm)
        uint16_t distance = VL53L0X_ReadRangeMM(&tof);
        
        if (distance < 8000) {
            printf("Distance: %d mm\r\n", distance);
        } else {
            printf("Out of range!\r\n");
        }
        
        Delay_Ms(100);
    }
}
```

---

## เปรียบเทียบกับ Ultrasonic

| คุณสมบัติ | VL53L0X (ToF) | HC-SR04 (Ultrasonic) |
|-----------|---------------|----------------------|
| ขนาด      | เล็กมาก        | ใหญ่                 |
| ความแม่นยำ | สูง (±3%)      | ปานกลาง              |
| มุมการวัด  | แคบ (25 องศา)  | กว้าง                |
| ผลกระทบจากสี | ไม่มี          | ไม่มี                |
| ผลกระทบจากเสียง | ไม่มี        | มี                   |

---

## API Reference

- `VL53L0X_Init(tof, addr)` : เริ่มต้นเซนเซอร์และโหลดค่า Calibration
- `VL53L0X_ReadRangeMM(tof)` : วัดระยะทางครั้งเดียว (Single Shot) หน่วยเป็น mm
- `VL53L0X_StartContinuous(tof, period)` : เริ่มโหมดวัดต่อเนื่อง
- `VL53L0X_ReadContinuous(tof, &dist)` : อ่านค่าในโหมดต่อเนื่อง (Non-blocking)
- `VL53L0X_StopContinuous(tof)` : หยุดโหมดวัดต่อเนื่อง
- `VL53L0X_SetAddress(tof, new_addr)` : เปลี่ยน I2C Address (ใช้เมื่อมีหลายตัว)

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)