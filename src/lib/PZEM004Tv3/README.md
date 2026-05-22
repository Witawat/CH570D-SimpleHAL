# PZEM004Tv3 Library (Modbus RTU)

ไลบรารีอ่านค่าพลังงานไฟฟ้า AC จาก PZEM-004T v3 ผ่าน Modbus RTU บน CH57x
รองรับอ่านค่า Voltage, Current, Power, Energy, Frequency และ Power Factor

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

ไลบรารีนี้ใช้กับ PZEM-004T v3 ซึ่งสื่อสารด้วย Modbus RTU ที่ 9600 baud
อ่านข้อมูลหลักทั้งหมดได้ในการร้องขอเดียว

ค่าที่รองรับ

- Voltage (V)
- Current (A)
- Active Power (W)
- Energy (Wh)
- Frequency (Hz)
- Power Factor
- Alarm Status

---

## คุณสมบัติ

- อ่านข้อมูลครบชุดด้วย PZEM004Tv3_ReadAll
- มีฟังก์ชันอ่านรายพารามิเตอร์
- รองรับ reset energy counter
- รองรับเปลี่ยน Modbus address
- มีการตรวจ CRC และ exception response

---

## Hardware Setup

### Wiring UART

| CH57x | PZEM-004T v3 |
|---|---|
| PD5 (TX) | RX |
| PD6 (RX) | TX |
| 5V | VCC |
| GND | GND |

### ข้อควรระวังด้านความปลอดภัย

- จุดวัดเป็นไฟ AC เมนส์ ต้องต่อวงจรอย่างระมัดระวัง
- ใช้ฟิวส์/เบรกเกอร์ที่เหมาะสม
- ผู้ปฏิบัติงานควรมีความรู้ด้านไฟฟ้ากำลัง

---

## หลักการทำงาน

### Modbus register หลัก

| Register | ความหมาย | Scale |
|---|---|---|
| 0x0000 | Voltage | 0.1 V |
| 0x0001-0x0002 | Current | 0.001 A |
| 0x0003-0x0004 | Power | 0.1 W |
| 0x0005-0x0006 | Energy | 1 Wh |
| 0x0007 | Frequency | 0.1 Hz |
| 0x0008 | Power Factor | 0.01 |
| 0x0009 | Alarm | 0/1 |

### ความต่างจาก PZEM004T v1/v2

| ประเด็น | v3 | v1/v2 |
|---|---|---|
| โปรโตคอล | Modbus RTU | Custom Serial |
| ฟังก์ชันอ่านหลัก | ReadAll 1 request | อ่านแยกหลาย request |
| ข้อมูลเสริม | Frequency, PF | ไม่มี |

---

## การใช้งานพื้นฐาน

### Quick start

```c
#include "SimpleHAL.h"
#include "PZEM004Tv3.h"

PZEM004Tv3 pzem;

int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    PZEM004Tv3_Init(&pzem, 0x01, USART_PINS_DEFAULT);

    PZEM004Tv3_Data d;
    if (PZEM004Tv3_ReadAll(&pzem, &d) == PZEM004Tv3_OK) {
        // d.voltage, d.current, d.power, d.energy, d.frequency, d.power_factor
    }

    while (1) {
    }
}
```

### อ่านรายค่า

```c
float v = PZEM004Tv3_GetVoltage(&pzem);
float i = PZEM004Tv3_GetCurrent(&pzem);
float p = PZEM004Tv3_GetPower(&pzem);
uint32_t e = PZEM004Tv3_GetEnergy(&pzem);
float f = PZEM004Tv3_GetFrequency(&pzem);
float pf = PZEM004Tv3_GetPowerFactor(&pzem);
```

---

## การใช้งานขั้นสูง

### รีเซ็ต energy counter

```c
if (PZEM004Tv3_ResetEnergy(&pzem) == PZEM004Tv3_OK) {
    // reset สำเร็จ
}
```

### เปลี่ยน Modbus address

```c
if (PZEM004Tv3_SetAddress(&pzem, 0x02) == PZEM004Tv3_OK) {
    // หลังจากนี้ควร init ใหม่ด้วย address ใหม่
    PZEM004Tv3_Init(&pzem, 0x02, USART_PINS_DEFAULT);
}
```

### polling loop ตัวอย่าง

```c
while (1) {
    PZEM004Tv3_Data d;
    if (PZEM004Tv3_ReadAll(&pzem, &d) == PZEM004Tv3_OK) {
        // process data
    }
    Delay_Ms(1000);
}
```

---

## Troubleshooting

### timeout หรือ CRC error

| สาเหตุ | วิธีแก้ |
|---|---|
| TX/RX ต่อผิด | ตรวจสายไขว้ TX->RX |
| noise สูง | จัดสายสัญญาณให้ห่างสายกำลัง |
| address ไม่ตรง | ตรวจ address ที่ init กับอุปกรณ์ |

### อ่านได้บางค่าแต่บางค่าผิด

| สาเหตุ | วิธีแก้ |
|---|---|
| ใช้ไลบรารีผิดรุ่น | ยืนยันว่าโมดูลเป็น v3 จริง |
| register parse ถูกแก้ไขในแอป | ใช้ ReadAll โดยตรงก่อนเพื่อเทียบค่า |

### หลัง SetAddress แล้วอ่านไม่ได้

| สาเหตุ | วิธีแก้ |
|---|---|
| ยังใช้ address เดิม | เรียก Init ใหม่ด้วย address ใหม่ |

---

## API Reference

### Constants

- PZEM004TV3_TIMEOUT_MS = 1000
- PZEM004TV3_BAUD = BAUD_9600
- PZEM004TV3_ADDR_BROADCAST = 0xF8

### Types

- PZEM004Tv3_Status
  - PZEM004Tv3_OK
  - PZEM004Tv3_ERROR_PARAM
  - PZEM004Tv3_ERROR_TIMEOUT
  - PZEM004Tv3_ERROR_CRC
  - PZEM004Tv3_ERROR_RESP
  - PZEM004Tv3_ERROR_EXCEPT
- PZEM004Tv3_Data
- PZEM004Tv3

### Functions

- PZEM004Tv3_Status PZEM004Tv3_Init(PZEM004Tv3* pzem, uint8_t modbus_addr, uint8_t pin_config)
- PZEM004Tv3_Status PZEM004Tv3_ReadAll(PZEM004Tv3* pzem, PZEM004Tv3_Data* data)
- float PZEM004Tv3_GetVoltage(PZEM004Tv3* pzem)
- float PZEM004Tv3_GetCurrent(PZEM004Tv3* pzem)
- float PZEM004Tv3_GetPower(PZEM004Tv3* pzem)
- uint32_t PZEM004Tv3_GetEnergy(PZEM004Tv3* pzem)
- float PZEM004Tv3_GetFrequency(PZEM004Tv3* pzem)
- float PZEM004Tv3_GetPowerFactor(PZEM004Tv3* pzem)
- PZEM004Tv3_Status PZEM004Tv3_ResetEnergy(PZEM004Tv3* pzem)
- PZEM004Tv3_Status PZEM004Tv3_SetAddress(PZEM004Tv3* pzem, uint8_t new_addr)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)