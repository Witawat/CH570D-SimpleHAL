# MQGas Gas Sensor Library

> **Library สำหรับใช้งาน MQ Series Gas Sensors บน CH57x**

## ภาพรวม

MQ Gas Sensors เป็น Chemo-resistive sensor ที่ตอบสนองต่อก๊าซชนิดต่างๆ  
Library นี้รองรับ MQ2, MQ3, MQ4, MQ5, MQ6, MQ7, MQ9, MQ135

| Sensor | ก๊าซที่ตรวจจับ | ช่วง PPM |
|--------|--------------|---------|
| MQ2 | LPG, Propane, H2, Smoke | 200-10000 |
| MQ3 | Alcohol/Ethanol | 25-500 |
| MQ4 | Methane, CNG | 200-10000 |
| MQ5 | LPG, Natural Gas | 200-10000 |
| MQ6 | LPG, Butane | 200-10000 |
| MQ7 | Carbon Monoxide (CO) | 20-2000 |
| MQ9 | CO + Flammable Gas | 20-1000 |
| MQ135 | Air Quality (NH3, NOx, Benzene) | 10-300 |

---

## Hardware Setup

### วงจร MQ2 กับ CH57x (3.3V)

```
5V ─────────────── VCC (MQ Module)
                   │
                   ├── Heater (RL ~33Ω)
                   │
GND ─────────────── GND (MQ Module)

AOUT ──[10kΩ]──── ADC pin (PA2)
         │
        [10kΩ]
         │
        GND

(Voltage divider: 5V → 2.5V สำหรับ 3.3V MCU)
```

> ⚠️ **สำคัญ**:
> - MQ sensor ต้องการ **5V** สำหรับ heater (ไม่ทำงานถูกต้องที่ 3.3V)
> - ต้องต่อ **voltage divider** เพื่อลดแรงดัน AOUT จาก 5V → 2.5V ก่อนต่อกับ MCU 3.3V
> - ต้อง **warmup 2-5 นาที** หลังเปิดไฟก่อนจะอ่านค่าได้แม่นยำ

---

## หลักการทำงาน

```
Rs เปลี่ยนแปลงตามความเข้มข้นก๊าซ:
  ก๊าซน้อย → Rs สูง → Vout ต่ำ
  ก๊าซมาก → Rs ต่ำ  → Vout สูง

Rs = RL × (Vcc - Vout) / Vout

สูตร PPM (จาก datasheet curve):
  PPM = A × (Rs/Ro)^B
  
  โดย Ro = ค่า Rs ใน clean air / ratio (จาก datasheet)
```

---

## การใช้งานพื้นฐาน

```c
#include "SimpleHAL.h"
#include "MQGas.h"

MQGas_Instance mq2;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    ADC_SimpleInit();

    MQGas_Init(&mq2, ADC_CH_PA2, MQ_TYPE_MQ2, 5.0f, 3.3f);

    // Warmup + Calibrate
    printf("Warming up 2 min...\r\n");
    Delay_Ms(120000);  // 2 นาที
    MQGas_Calibrate(&mq2, 50);
    printf("Ro = %.2f kOhm\r\n", mq2.ro);

    // ตั้ง alarm threshold
    MQGas_SetThreshold(&mq2, 300.0f);  // 300 PPM

    while (1) {
        float ppm = MQGas_GetPPM(&mq2);
        printf("Gas: %.1f PPM", ppm);

        if (MQGas_IsAlarm(&mq2)) {
            printf(" *** ALARM ***");
        }
        printf("\r\n");
        Delay_Ms(500);
    }
}
```

---

## Calibration

### วิธีที่ 1: Auto Calibrate (ต้องอยู่ใน clean air)

```c
Delay_Ms(120000);  // warmup 2 นาที
MQGas_Calibrate(&mq2, 50);
printf("Ro = %.2f\r\n", mq2.ro);
// บันทึก Ro ลง EEPROM เพื่อใช้ครั้งต่อไป
```

### วิธีที่ 2: ตั้ง Ro โดยตรง (จาก datasheet หรือ calibration ก่อนหน้า)

```c
MQGas_SetRo(&mq2, 9.83f);  // MQ2 ใน clean air ≈ 9.83kΩ
```

---

## Troubleshooting

| ปัญหา | สาเหตุ | วิธีแก้ |
|-------|--------|---------|
| ค่า PPM = -1 | ยังไม่ calibrate | เรียก `MQGas_Calibrate()` หรือ `MQGas_SetRo()` |
| ค่า PPM สูงมากผิดปกติ | ยังไม่ warmup | รอ 2-5 นาทีหลังเปิดไฟ |
| ค่า ADC = 0 | VCC sensor ผิด | ตรวจว่า sensor ได้ 5V |
| ค่า ADC > 800 ตลอด | Voltage divider ขาด | ต่อ voltage divider (2×10kΩ) |

---

## API Reference

| Function | คำอธิบาย |
|----------|----------|
| `MQGas_Init(mq, ch, type, vcc, vref)` | Init sensor |
| `MQGas_Calibrate(mq, samples)` | Calibrate Ro ใน clean air |
| `MQGas_SetRo(mq, ro)` | ตั้ง Ro โดยตรง (kΩ) |
| `MQGas_SetCurve(mq, a, b)` | ตั้ง custom curve (สำหรับ GENERIC) |
| `MQGas_ReadRaw(mq)` | อ่าน ADC raw (0-1023) |
| `MQGas_ReadVoltage(mq)` | อ่านแรงดัน AOUT (V) |
| `MQGas_GetRs(mq)` | คำนวณ Rs (kΩ) |
| `MQGas_GetPPM(mq)` | คำนวณ PPM (คืน -1 ถ้าไม่ calibrate) |
| `MQGas_SetThreshold(mq, ppm)` | ตั้ง alarm threshold |
| `MQGas_IsAlarm(mq)` | ตรวจสอบ alarm (1=เกิน threshold) |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)