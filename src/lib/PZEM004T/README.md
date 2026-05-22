# PZEM004T Library (v1/v2)

ไลบรารีอ่านค่าพลังงานไฟฟ้า AC จาก PZEM-004T เวอร์ชัน v1/v2 (โปรโตคอล Custom Serial)
รองรับอ่าน Voltage, Current, Power, Energy และตั้งค่า Alarm

## สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Troubleshooting](#troubleshooting)
8. [API Reference](#api-reference)

---

## ภาพรวม

ไลบรารีนี้ใช้กับ PZEM-004T รุ่นเก่า (v1/v2) ที่สื่อสารแบบ custom protocol ที่ 9600 baud

วัดค่าได้

- Voltage (V)
- Current (A)
- Power (W)
- Energy (Wh)

หมายเหตุสำคัญ

- ถ้าเป็น PZEM-004T v3 ต้องใช้ไลบรารีอีกตัวคือ PZEM004Tv3

---

## คุณสมบัติ

- อ่านค่าแยกทีละพารามิเตอร์
- อ่านค่าทั้งหมดแบบรวมในโครงสร้างเดียว
- รองรับเปลี่ยน address ของอุปกรณ์
- รองรับตั้ง power alarm threshold
- มี timeout/checksum/response validation

---

## Hardware Setup

### Wiring UART

| CH57x | PZEM-004T |
|---|---|
| PD5 (TX) | RX |
| PD6 (RX) | TX |
| 5V | VCC |
| GND | GND |

### วงจรแรงดัน/กระแส AC

- ต่อ L/N และ CT clamp ตามคู่มือผู้ผลิตอย่างเคร่งครัด
- ห้ามจับจุดที่มีไฟ AC ขณะจ่ายไฟ

คำเตือนความปลอดภัย

- มีความเสี่ยงไฟดูด/ไฟไหม้หากต่อผิด
- ควรทดสอบผ่านเบรกเกอร์และฟิวส์
- หากไม่ชำนาญงานไฟฟ้า ให้ผู้เชี่ยวชาญดำเนินการ

---

## หลักการทำงาน

### Protocol

PZEM v1/v2 ใช้แพ็กเก็ตคำสั่งความยาวคงที่และ response แยกแต่ละชนิดข้อมูล
ไลบรารีส่งคำสั่งรายพารามิเตอร์และตรวจ checksum ทุกครั้ง

### Address

ค่า default คือ 192.168.1.1 (4 ไบต์)
สามารถเปลี่ยน address ใน instance ด้วย PZEM004T_SetAddress

---

## การใช้งานพื้นฐาน

### Quick start

```c
#include "SimpleHAL.h"
#include "PZEM004T.h"

PZEM004T pzem;

int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    PZEM004T_Init(&pzem, USART_PINS_DEFAULT);

    float v = PZEM004T_GetVoltage(&pzem);
    float i = PZEM004T_GetCurrent(&pzem);
    float p = PZEM004T_GetPower(&pzem);
    uint32_t e = PZEM004T_GetEnergy(&pzem);

    (void)v; (void)i; (void)p; (void)e;

    while (1) {
    }
}
```

### อ่านทุกค่าในครั้งเดียว

```c
PZEM004T_Data data;
if (PZEM004T_ReadAll(&pzem, &data) == PZEM004T_OK) {
    // data.voltage, data.current, data.power, data.energy, data.alarm
}
```

---

## การใช้งานขั้นสูง

### ตั้ง address อุปกรณ์

```c
uint8_t addr[4] = {0xC0, 0xA8, 0x01, 0x02};
PZEM004T_SetAddress(&pzem, addr);
```

### ตั้ง alarm threshold

```c
PZEM004T_SetAlarm(&pzem, 1000); // 1000W
```

### ตัวอย่าง polling ทุก 1 วินาที

```c
while (1) {
    PZEM004T_Data d;
    PZEM004T_ReadAll(&pzem, &d);
    Delay_Ms(1000);
}
```

---

## Troubleshooting

### อ่านค่าไม่ได้ (ได้ -1 หรือ timeout)

| สาเหตุ | วิธีแก้ |
|---|---|
| ต่อ TX/RX ไม่ถูก | ตรวจสายไขว้ TX->RX, RX->TX |
| ไม่ได้ใช้ 9600 baud | ตรวจการ init ผ่าน PZEM004T_Init |
| ใช้รุ่น v3 แต่ลงไลบรารี v1/v2 | เปลี่ยนไปใช้ PZEM004Tv3 |

### ค่าเพี้ยนหรือเป็นศูนย์

| สาเหตุ | วิธีแก้ |
|---|---|
| ต่อ CT clamp ผิดทิศ/ไม่แน่น | ตรวจการหนีบ CT ใหม่ |
| โหลดต่ำมาก | ทดสอบกับโหลดที่ชัดเจน |

### สื่อสารได้บ้างไม่ได้บ้าง

| สาเหตุ | วิธีแก้ |
|---|---|
| noise จากสาย AC | แยกสายสัญญาณจากสายกำลัง |
| GND ไม่เสถียร | ตรวจกราวด์ร่วมและภาคจ่ายไฟ |

---

## API Reference

### Constants

- PZEM004T_TIMEOUT_MS = 1000
- PZEM004T_BAUD = BAUD_9600

### Types

- PZEM004T_Status
  - PZEM004T_OK
  - PZEM004T_ERROR_PARAM
  - PZEM004T_ERROR_TIMEOUT
  - PZEM004T_ERROR_CHECKSUM
  - PZEM004T_ERROR_RESP
- PZEM004T_Data
- PZEM004T

### Functions

- PZEM004T_Status PZEM004T_Init(PZEM004T* pzem, uint8_t pin_config)
- void PZEM004T_SetAddress(PZEM004T* pzem, uint8_t addr[4])
- float PZEM004T_GetVoltage(PZEM004T* pzem)
- float PZEM004T_GetCurrent(PZEM004T* pzem)
- float PZEM004T_GetPower(PZEM004T* pzem)
- uint32_t PZEM004T_GetEnergy(PZEM004T* pzem)
- PZEM004T_Status PZEM004T_ReadAll(PZEM004T* pzem, PZEM004T_Data* data)
- PZEM004T_Status PZEM004T_SetAlarm(PZEM004T* pzem, uint16_t watts)

---

## เปรียบเทียบกับ PZEM004Tv3 แบบย่อ

| ประเด็น | PZEM004T (ไฟล์นี้) | PZEM004Tv3 |
|---|---|---|
| โปรโตคอล | Custom Serial | Modbus RTU |
| ชุดข้อมูล | V, I, P, E | V, I, P, E, Frequency, PF |
| ไลบรารีที่ใช้ | PZEM004T | PZEM004Tv3 |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)