# PCF8574 Library

ไลบรารีควบคุม PCF8574/PCF8574A (I2C GPIO Expander 8 บิต) บน CH57x
ช่วยขยาย GPIO ผ่านบัส I2C โดยใช้ขา MCU เพียง 2 เส้น (SCL/SDA)

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

PCF8574 เป็นชิปขยายพอร์ตดิจิทัล 8 ขาแบบ I2C
เหมาะกับงานที่ GPIO บน MCU ไม่พอ เช่น อ่านปุ่มหลายตัวหรือขับ LED หลายดวง

ช่วง address

- PCF8574: 0x20-0x27
- PCF8574A: 0x38-0x3F

Address ถูกกำหนดจากขา A0/A1/A2 ของโมดูล

---

## คุณสมบัติ

- รองรับอ่าน/เขียนรายขา (pin 0-7)
- รองรับอ่าน/เขียนทั้งพอร์ต 8 บิตในครั้งเดียว
- รองรับโหมด INPUT/OUTPUT ตามรูปแบบ quasi-bidirectional ของ PCF8574
- มีการเก็บสถานะพอร์ตล่าสุดใน instance

---

## Hardware Setup

### Wiring (Hardware I2C default)

| CH57x | PCF8574 |
|---|---|
| PC2 (SCL) | SCL |
| PC1 (SDA) | SDA |
| 3.3V | VCC |
| GND | GND |

### ตั้งค่า address ด้วย A0/A1/A2

| A2 | A1 | A0 | PCF8574 |
|---|---|---|---|
| 0 | 0 | 0 | 0x20 |
| 0 | 0 | 1 | 0x21 |
| 0 | 1 | 0 | 0x22 |
| ... | ... | ... | ... |
| 1 | 1 | 1 | 0x27 |

ข้อควรระวัง

- บัส I2C ต้องมี pull-up ที่ SDA/SCL (ส่วนมากมีบนบอร์ดอยู่แล้ว)
- ถ้าใช้หลายอุปกรณ์ I2C ต้องไม่ชน address กัน

---

## หลักการทำงาน

### Quasi-bidirectional I/O

PCF8574 ไม่มี direction register แยกเหมือน MCU ทั่วไป
หลักการคือ

- เขียน LOW: ดึงขานั้นลงต่ำแบบ output
- เขียน HIGH: ปล่อยขาให้เป็น weak pull-up และสามารถอ่านสถานะภายนอกได้

ดังนั้นในไลบรารี

- PCF8574_PinMode(..., PCF8574_INPUT) จะเขียน HIGH ที่ขานั้นก่อน
- แล้วจึงอ่านค่าได้ด้วย PCF8574_Read

---

## การใช้งานพื้นฐาน

### 1) Init

```c
#include "SimpleHAL.h"
#include "PCF8574.h"

PCF8574_Instance io;

int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
    PCF8574_Init(&io, 0x20);

    while (1) {
    }
}
```

### 2) ขับ LED ที่ pin0

```c
PCF8574_PinMode(&io, 0, PCF8574_OUTPUT);
PCF8574_Write(&io, 0, HIGH);
Delay_Ms(300);
PCF8574_Write(&io, 0, LOW);
```

### 3) อ่านปุ่มที่ pin7

```c
PCF8574_PinMode(&io, 7, PCF8574_INPUT);

if (PCF8574_Read(&io, 7) == LOW) {
    // ปุ่ม active low ถูกกด
}
```

---

## การใช้งานขั้นสูง

### เขียนทั้งพอร์ตครั้งเดียว

```c
// bit0=1, bit1=0, bit2=1, ...
PCF8574_WritePort(&io, 0b10101010);
```

### อ่านทั้งพอร์ตแล้ว mask bit

```c
uint8_t port;
if (PCF8574_ReadPort(&io, &port) == PCF8574_OK) {
    uint8_t key0 = (port >> 0) & 0x01;
    uint8_t key1 = (port >> 1) & 0x01;
}
```

### Toggle output

```c
PCF8574_PinMode(&io, 3, PCF8574_OUTPUT);
PCF8574_Toggle(&io, 3);
```

---

## Troubleshooting

### ติดต่อ I2C ไม่ได้

| สาเหตุ | วิธีแก้ |
|---|---|
| Address ผิด | ตรวจ A0/A1/A2 แล้วแก้ i2c_addr |
| ไม่ได้ init I2C ก่อน | เรียก I2C_SimpleInit ก่อน PCF8574_Init |
| SDA/SCL ไม่มี pull-up | ใส่ pull-up 4.7k-10k |

### อ่าน input ได้ค่าแปลก

| สาเหตุ | วิธีแก้ |
|---|---|
| ไม่ได้ตั้งเป็น INPUT ก่อน | เรียก PCF8574_PinMode(..., INPUT) |
| วงจรลอย (floating) | ใส่ pull-up/pull-down ที่ปุ่ม/สัญญาณ |

### LED ติดกลับ logic

| สาเหตุ | วิธีแก้ |
|---|---|
| ต่อแบบ active low | สลับ HIGH/LOW ให้ตรงฮาร์ดแวร์ |

---

## API Reference

### Constants

- PCF8574_ADDR_BASE = 0x20
- PCF8574A_ADDR_BASE = 0x38

### Types

- PCF8574_Status
  - PCF8574_OK
  - PCF8574_ERROR_PARAM
  - PCF8574_ERROR_I2C
- PCF8574_Mode
  - PCF8574_OUTPUT
  - PCF8574_INPUT
- PCF8574_Instance

### Functions

- PCF8574_Status PCF8574_Init(PCF8574_Instance* pcf, uint8_t i2c_addr)
- PCF8574_Status PCF8574_PinMode(PCF8574_Instance* pcf, uint8_t pin, PCF8574_Mode mode)
- PCF8574_Status PCF8574_Write(PCF8574_Instance* pcf, uint8_t pin, uint8_t value)
- uint8_t PCF8574_Read(PCF8574_Instance* pcf, uint8_t pin)
- PCF8574_Status PCF8574_WritePort(PCF8574_Instance* pcf, uint8_t value)
- PCF8574_Status PCF8574_ReadPort(PCF8574_Instance* pcf, uint8_t* value)
- PCF8574_Status PCF8574_Toggle(PCF8574_Instance* pcf, uint8_t pin)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)