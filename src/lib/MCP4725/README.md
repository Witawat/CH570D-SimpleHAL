# MCP4725 12-bit DAC Library

> **Library สำหรับสร้างสัญญาณ Analog Output ด้วย MCP4725 I2C DAC สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [การใช้งาน EEPROM](#การใช้งาน-eeprom)
5. [API Reference](#api-reference)

---

## ภาพรวม

MCP4725 คือโมดูล Digital-to-Analog Converter (DAC) ความละเอียด **12-bit** (0-4095) ที่สื่อสารผ่าน I2C  
ใช้สำหรับสร้างแรงดันไฟฟ้ากระแสตรง (DC Voltage) ที่ปรับค่าได้ละเอียดมาก เหมาะสำหรับงานสร้างเครื่องกำเนิดสัญญาณ (Signal Generator), ควบคุมความเร็วรอบมอเตอร์แบบ Analog, หรือสร้างเสียง

---

## Hardware Setup

### การเชื่อมต่อ

| MCP4725 Pin | CH57x Pin | หมายเหตุ |
|-------------|--------------|----------|
| VDD         | 3.3V / 5V    | แรงดันไฟเลี้ยงและแรงดันอ้างอิง (Vref) |
| GND         | GND          | กราวด์ร่วม |
| SCL         | PC2          | สัญญาณนาฬิกา I2C (ต้องการ Pull-up 4.7kΩ) |
| SDA         | PC1          | ข้อมูล I2C (ต้องการ Pull-up 4.7kΩ) |
| A0          | GND / VCC    | กำหนด I2C Address (ดูด้านล่าง) |
| VOUT        | -            | สัญญาณ Analog ขาออก |

### การตั้ง I2C Address (A0 Pin)

- **A0 = GND**: Address **0x60** (Macro: `MCP4725_ADDR_A0_GND`)
- **A0 = VCC**: Address **0x61** (Macro: `MCP4725_ADDR_A0_VCC`)

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/MCP4725/MCP4725.h"

MCP4725_Instance dac;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้น DAC
    MCP4725_Init(&dac, MCP4725_ADDR_A0_GND);

    while(1) {
        // 2. ตั้งค่าแรงดันออกเป็น 1.65V (สมมติ VCC = 3.3V)
        MCP4725_SetVoltage(&dac, 1.65f, 3.3f);
        Delay_Ms(2000);

        // 3. ตั้งค่าแบบ Raw (0 - 4095)
        MCP4725_SetRaw(&dac, 4095); // Output = VCC (Max)
        Delay_Ms(2000);
        
        MCP4725_SetRaw(&dac, 0);    // Output = 0V
        Delay_Ms(2000);
    }
}
```

---

## การใช้งาน EEPROM

จุดเด่นของ MCP4725 คือมีหน่วยความจำ EEPROM ในตัว ทำให้สามารถบันทึกค่าแรงดันล่าสุดไว้ได้ เมื่อเปิดเครื่องใหม่ แรงดันจะถูกจ่ายออกมาตามค่าที่บันทึกไว้ทันทีโดยไม่ต้องรอคำสั่งจาก MCU

```c
// เขียนค่า 2048 ลงใน DAC และบันทึกลง EEPROM ด้วย
// (ใช้เวลาเขียนประมาณ 25ms และควรระวังเรื่อง Write Cycle ของ EEPROM)
MCP4725_SetRawEEPROM(&dac, 2048);
```

---

## API Reference

- `MCP4725_Init(dac, addr)` : เริ่มต้นโมดูล
- `MCP4725_SetRaw(dac, value)` : ตั้งค่า 0-4095 (Fast mode, ไม่บันทึก EEPROM)
- `MCP4725_SetVoltage(dac, voltage, vref)` : ตั้งค่าเป็นแรงดัน (V)
- `MCP4725_SetRawEEPROM(dac, value)` : ตั้งค่าและบันทึกลง EEPROM
- `MCP4725_GetRaw(dac, &value)` : อ่านค่าปัจจุบันจาก DAC
- `MCP4725_SetPowerDown(dac, mode)` : เข้าโหมดประหยัดไฟ (เลือกค่าความต้านทานที่ขา VOUT ได้)

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)