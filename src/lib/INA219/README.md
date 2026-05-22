# INA219 Current & Power Monitor Library

> **Library สำหรับวัดแรงดัน กระแส และกำลังไฟฟ้าด้วย INA219 ผ่าน I2C สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [การเลือกค่า Shunt Resistor](#การเลือกค่า-shunt-resistor)
5. [API Reference](#api-reference)

---

## ภาพรวม

INA219 คือโมดูลวัดพลังงานไฟฟ้า (Power Monitor) ที่มีความแม่นยำสูง สามารถวัดได้ทั้งแรงดัน (Bus Voltage) และกระแส (Current) โดยใช้ตัวต้านทาน Shunt ภายนอก เหมาะสำหรับงาน Monitoring ระบบโซลาร์เซลล์, การชาร์จแบตเตอรี่ หรือตรวจสอบการกินไฟของโปรเจกต์

---

## Hardware Setup

### การเชื่อมต่อ (High-side sensing)

```
Power Supply (+) ───[ VIN+ ]───[ R_shunt ]───[ VIN- ]─── Load (+)
                                   │
                                INA219
                                   │
CH57x                INA219
PC2 (SCL)  ───────────> SCL
PC1 (SDA)  ───────────> SDA
3.3V       ───────────> VCC
GND        ───────────> GND
```

### การตั้ง I2C Address

| A1 Pin | A0 Pin | I2C Address | Macro |
|--------|--------|-------------|-------|
| GND    | GND    | 0x40 (Default) | `INA219_ADDR_0` |
| GND    | VCC    | 0x41           | `INA219_ADDR_1` |
| VCC    | GND    | 0x44           | `INA219_ADDR_2` |
| VCC    | VCC    | 0x45           | `INA219_ADDR_3` |

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/INA219/INA219.h"

INA219_Instance ina;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    // 1. เริ่มต้น INA219 (Address 0x40, Shunt 0.1 Ohm, Max Current 3.2A)
    if (INA219_Init(&ina, INA219_ADDR_0, 0.1f, 3.2f) == INA219_OK) {
        printf("INA219 Initialized\r\n");
    }

    while(1) {
        // 2. อ่านค่าแรงดัน กระแส และกำลังไฟ
        float voltage = INA219_GetBusVoltage(&ina);
        float current = INA219_GetCurrent(&ina);
        float power   = INA219_GetPower(&ina);

        printf("V: %.2f V, I: %.3f A, P: %.3f W\r\n", voltage, current, power);
        
        Delay_Ms(1000);
    }
}
```

---

## การเลือกค่า Shunt Resistor

โดยปกติโมดูล INA219 ที่ขายทั่วไปจะมาพร้อมกับตัวต้านทาน **0.1 Ω (R100)**:
- **Shunt 0.1 Ω**: วัดกระแสสูงสุดได้ ±3.2A (ความละเอียด 0.8mA)
- **Shunt 0.01 Ω**: หากต้องการวัดกระแสสูงขึ้น (เช่น 30A) ต้องเปลี่ยน Shunt ให้มีค่าน้อยลง และระบุค่าในฟังก์ชัน `INA219_Init` ให้ถูกต้อง

---

## API Reference

- `INA219_Init(ina, addr, r_shunt, max_amps)` : เริ่มต้นและ Calibrate เซนเซอร์
- `INA219_GetBusVoltage(ina)` : อ่านแรงดันที่จ่ายให้ Load (หน่วย V)
- `INA219_GetShuntVoltage(ina)` : อ่านแรงดันตกคร่อม Shunt (หน่วย mV)
- `INA219_GetCurrent(ina)` : อ่านกระแสไฟฟ้า (หน่วย A)
- `INA219_GetPower(ina)` : อ่านกำลังไฟฟ้า (หน่วย W)
- `INA219_GetAll(ina, &v, &i, &p)` : อ่านค่าทั้งหมดพร้อมกัน
- `INA219_PowerDown(ina)` : เข้าโหมดประหยัดพลังงาน
- `INA219_PowerUp(ina)` : กลับมาทำงานโหมดต่อเนื่อง

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)