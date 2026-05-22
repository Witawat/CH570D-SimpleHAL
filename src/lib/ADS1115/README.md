# ADS1115 16-bit ADC Library

> **Library สำหรับอ่านค่า ADC 16-bit 4-channel ด้วย ADS1115 ผ่าน I2C สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
5. [การตั้งค่า Gain และความละเอียด](#การตั้งค่า-gain-และความละเอียด)
6. [API Reference](#api-reference)

---

## ภาพรวม

ADS1115 คือโมดูล Analog-to-Digital Converter (ADC) ความละเอียดสูง **16-bit** ซึ่งละเอียดกว่า ADC ภายในของ CH57x (10-bit) ถึง 64 เท่า เหมาะสำหรับงานที่ต้องการความแม่นยำสูง เช่น การวัดแรงดันแบตเตอรี่, เซนเซอร์น้ำหนัก (Load Cell), หรือการวัดกระแส

---

## คุณสมบัติ

- ✅ ความละเอียด 16-bit (Precision ADC)
- ✅ 4 Input Channels (AIN0-AIN3)
- ✅ รองรับโหมด Single-ended (เทียบ GND) และ Differential (เทียบกันเอง)
- ✅ มี Programmable Gain Amplifier (PGA) ในตัว ขยายสัญญาณได้ถึง 16 เท่า
- ✅ อัตราการสุ่มตัวอย่างปรับได้ 8 ถึง 860 SPS
- ✅ สื่อสารผ่าน I2C เปลี่ยน Address ได้ 4 ค่า (ต่อได้สูงสุด 4 ตัวบน bus เดียวกัน)

---

## Hardware Setup

### การเชื่อมต่อ

| ADS1115 Pin | CH57x Pin | หมายเหตุ |
|-------------|--------------|----------|
| VDD         | 3.3V / 5V    | แรงดันไฟเลี้ยง (2.0V - 5.5V) |
| GND         | GND          | กราวด์ร่วม |
| SCL         | PC2          | สัญญาณนาฬิกา I2C (ต้องการ Pull-up 4.7kΩ) |
| SDA         | PC1          | ข้อมูล I2C (ต้องการ Pull-up 4.7kΩ) |
| ADDR        | GND          | กำหนด I2C Address (ดูตารางด้านล่าง) |
| ALRT        | -            | Alert/Ready (ไม่จำเป็นต้องต่อ) |

### การตั้ง I2C Address (ADDR Pin)

| ADDR ต่อกับ | I2C Address | Macro |
|-------------|-------------|-------|
| GND         | 0x48 (Default) | `ADS1115_ADDR_GND` |
| VDD         | 0x49        | `ADS1115_ADDR_VDD` |
| SDA         | 0x4A        | `ADS1115_ADDR_SDA` |
| SCL         | 0x4B        | `ADS1115_ADDR_SCL` |

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/ADS1115/ADS1115.h"

ADS1115_Instance adc;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้น ADS1115 (Address 0x48)
    ADS1115_Init(&adc, ADS1115_ADDR_GND);

    // 2. ตั้งค่า Gain (±4.096V)
    ADS1115_SetGain(&adc, ADS1115_PGA_4096);

    while(1) {
        // 3. อ่านค่าแรงดันจากช่อง AIN0
        float voltage = ADS1115_ReadVoltage(&adc, ADS1115_CH_AIN0);
        
        printf("Voltage CH0: %.4f V\r\n", voltage);
        Delay_Ms(500);
    }
}
```

---

## การตั้งค่า Gain และความละเอียด

ค่า PGA (Programmable Gain Amplifier) จะกำหนดช่วงแรงดันสูงสุดที่วัดได้ (Full-Scale Range) และความละเอียดต่อบิต (LSB)

| PGA Setting | Full-Scale Range | ความละเอียด (LSB) |
|-------------|------------------|-------------------|
| `ADS1115_PGA_6144` | ±6.144V | 187.5 µV |
| `ADS1115_PGA_4096` | ±4.096V | 125 µV |
| `ADS1115_PGA_2048` | ±2.048V (Default) | 62.5 µV |
| `ADS1115_PGA_1024` | ±1.024V | 31.25 µV |
| `ADS1115_PGA_512`  | ±0.512V | 15.625 µV |
| `ADS1115_PGA_256`  | ±0.256V | 7.8125 µV |

> ⚠️ **คำเตือน**: ห้ามป้อนแรงดันเข้าขา AIN เกินกว่า VDD + 0.3V แม้จะตั้ง PGA ไว้สูงก็ตาม

---

## API Reference

### ฟังก์ชันหลัก
- `ADS1115_Init(ads, addr)` : เริ่มต้นโมดูล
- `ADS1115_ReadRaw(ads, channel)` : อ่านค่าดิบ (Raw Data) 16-bit (-32768 ถึง 32767)
- `ADS1115_ReadVoltage(ads, channel)` : อ่านค่าเป็นแรงดัน (V)
- `ADS1115_ToVoltage(ads, raw)` : แปลงค่าดิบเป็นแรงดัน

### การเลือกช่องสัญญาณ (Channel)
- `ADS1115_CH_AIN0` ถึง `ADS1115_CH_AIN3` : วัดเทียบ GND
- `ADS1115_CH_DIFF_01` : วัดผลต่างระหว่าง AIN0 และ AIN1
- `ADS1115_CH_DIFF_23` : วัดผลต่างระหว่าง AIN2 และ AIN3

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)