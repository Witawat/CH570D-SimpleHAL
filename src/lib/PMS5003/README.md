# PMS5003 / PMS7003 Library for CH57x

ไลบรารีนี้ช่วยให้ใช้งานเซนเซอร์ฝุ่น PM2.5 รุ่น **PMS5003 / PMS7003** ได้ง่ายบน CH57x ผ่าน UART
รองรับทั้งการอ่านแบบ non-blocking (polling ใน loop) และ blocking (รอ frame พร้อม timeout)

## สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Troubleshooting](#troubleshooting)
8. [API Reference](#api-reference)

## ภาพรวม

เซนเซอร์ตระกูล PMS5003/PMS7003 ส่งข้อมูลเป็น frame ขนาด 32 bytes ทาง UART ที่ 9600 baud
โดยใน frame จะมีข้อมูลสำคัญ เช่น:

- PM1.0, PM2.5, PM10 (แบบ CF=1 / Standard)
- PM1.0, PM2.5, PM10 (แบบ Atmospheric)
- จำนวนอนุภาคในขนาดต่างๆ (0.3um ถึง 10um)

ไลบรารีนี้รองรับการตรวจสอบ checksum อัตโนมัติ เพื่อคัดทิ้ง frame ที่เสียหาย

## คุณสมบัติ

- รองรับทั้ง **PMS5003** และ **PMS7003** (protocol เดียวกัน)
- ตรวจจับ header `0x42 0x4D` และประกอบ frame อัตโนมัติ
- ตรวจ checksum ก่อน parse ข้อมูล
- อ่านข้อมูล 2 รูปแบบ:
  - Non-blocking: `PMS5003_Update()` + `PMS5003_IsDataReady()`
  - Blocking: `PMS5003_Read()`
- รองรับคำสั่งควบคุมเซนเซอร์:
  - Sleep / Wakeup
  - Active mode / Passive mode
  - Request one frame (ใน passive mode)

## Hardware Setup

> เซนเซอร์ใช้ไฟ 5V แต่ขา UART logic มักเป็น 3.3V TTL (ตรวจ datasheet ของโมดูลที่ใช้งาน)

### การต่อสายพื้นฐาน

| PMS5003/PMS7003 | CH57x (USART1 Default) |
|---|---|
| TXD | PD6 (RX) |
| RXD | PD5 (TX) |
| GND | GND |
| VCC | 5V |

### หมายเหตุเรื่อง Pin Mapping

`SimpleUSART` รองรับหลาย pin mapping ผ่าน `USART_PinConfig`

- `USART_PINS_DEFAULT`: TX=PD5, RX=PD6
- `USART_PINS_REMAP1`: TX=PD0, RX=PD1
- `USART_PINS_REMAP2`: TX=PD6, RX=PD5

ถ้าใช้ mapping อื่น ให้ส่งค่า `pin_config` ให้ตรงกับฮาร์ดแวร์ตอน `PMS5003_Init()`

## หลักการทำงาน

### โครงสร้างข้อมูล 1 frame (32 bytes)

- Byte 0..1: Header `0x42 0x4D`
- Byte 2..3: Frame Length (ปกติ = 28)
- Byte 4..29: Payload (13 ค่า x 2 bytes)
- Byte 30..31: Checksum

### สูตร checksum

ผลรวมแบบ 16-bit ของ byte 0..29 ต้องเท่ากับค่า checksum ที่ byte 30..31

หาก checksum ไม่ตรง ไลบรารีจะทิ้ง frame นั้นทันที

## การใช้งานพื้นฐาน

### 1) Include และประกาศตัวแปร

```c
#include "Lib/PMS5003/PMS5003.h"

static PMS5003_Instance pms;
static PMS5003_Data pms_data;
```

### 2) เริ่มต้นใช้งาน

```c
if(PMS5003_Init(&pms, USART_PINS_DEFAULT) != PMS5003_OK) {
    // handle error
}
```

### 3) อ่านแบบ non-blocking ใน loop

```c
PMS5003_Update(&pms);

if(PMS5003_IsDataReady(&pms)) {
    if(PMS5003_GetData(&pms, &pms_data) == PMS5003_OK) {
        // ใช้งานค่า pms_data.pm2_5_atm, pms_data.pm10_atm ฯลฯ
    }
    PMS5003_ClearReady(&pms);
}
```

### 4) อ่านแบบ blocking (รอได้สูงสุดตาม timeout)

```c
if(PMS5003_Read(&pms, &pms_data, 2000) == PMS5003_OK) {
    // ได้ข้อมูลใหม่ภายใน 2 วินาที
} else {
    // timeout หรือ error
}
```

## การใช้งานขั้นสูง

### โหมด Active / Passive

โดยทั่วไปเซนเซอร์จะส่งข้อมูลต่อเนื่อง (active mode)

- เข้า passive mode:

```c
PMS5003_SetPassiveMode(&pms);
```

- ขอข้อมูล 1 frame (เหมาะกับ passive mode):

```c
PMS5003_Request(&pms);
if(PMS5003_Read(&pms, &pms_data, 1000) == PMS5003_OK) {
    // ได้ข้อมูลตาม request
}
```

- กลับไป active mode:

```c
PMS5003_SetActiveMode(&pms);
```

### Sleep / Wakeup

```c
PMS5003_Sleep(&pms);
Delay_Ms(5000);
PMS5003_Wakeup(&pms);

// หลัง wakeup ควรรอให้เซนเซอร์นิ่งก่อนอ่านค่า
Delay_Ms(30000);
```

## Troubleshooting

### 1) อ่านไม่ได้เลย / timeout ตลอด

- ตรวจสาย TX/RX ว่าต่อไขว้ถูกต้องหรือไม่ (TX sensor -> RX MCU)
- ตรวจ GND ต้องร่วมกัน
- ตรวจ baud ต้องเป็น 9600
- ตรวจ pin mapping ของ `USART_PinConfig`

### 2) ค่า PM แปลกมาก หรือแกว่งสูงช่วงแรก

- หลังเปิดเครื่องใหม่หรือ wakeup ให้ warm-up 20-30 วินาที
- หลีกเลี่ยงลมเป่าโดยตรงหรือฝุ่นเข้มข้นเฉียบพลันระหว่างสอบเทียบ

### 3) frame checksum fail บ่อย

- ตรวจคุณภาพสายและการต่อกราวด์
- ลดสัญญาณรบกวนจากโหลดกำลังสูงใกล้สาย UART
- พิจารณาลดความยาวสาย

### 4) ใช้ร่วมกับไลบรารี UART อื่นแล้วชนกัน

- `SimpleUSART` ของโปรเจกต์เป็น peripheral เดียวกัน (USART1)
- ถ้าจะใช้หลายโมดูล UART บน USART1 ต้องบริหารลำดับการใช้งานให้ชัดเจน

## API Reference

### Types

- `PMS5003_Status`
  - `PMS5003_OK`
  - `PMS5003_ERROR_PARAM`
  - `PMS5003_ERROR_TIMEOUT`
  - `PMS5003_ERROR_CHECKSUM`
  - `PMS5003_ERROR_NOT_INIT`

- `PMS5003_Data`
  - `frame_len`
  - `pm1_0_std`, `pm2_5_std`, `pm10_std`
  - `pm1_0_atm`, `pm2_5_atm`, `pm10_atm`
  - `cnt_0_3`, `cnt_0_5`, `cnt_1_0`, `cnt_2_5`, `cnt_5_0`, `cnt_10`
  - `reserved`, `checksum`

- `PMS5003_Instance`
  - โครงสร้าง instance หลักของไลบรารี

### Functions

- `PMS5003_Status PMS5003_Init(PMS5003_Instance* sensor, USART_PinConfig pin_config)`
  - เริ่มต้นเซนเซอร์และ USART ที่ 9600 baud

- `PMS5003_Status PMS5003_Update(PMS5003_Instance* sensor)`
  - ประมวลผลข้อมูล UART ที่ค้างอยู่แบบ non-blocking

- `PMS5003_Status PMS5003_Read(PMS5003_Instance* sensor, PMS5003_Data* out, uint32_t timeout_ms)`
  - อ่าน frame แบบ blocking พร้อม timeout

- `uint8_t PMS5003_IsDataReady(PMS5003_Instance* sensor)`
  - เช็คว่ามี frame ใหม่พร้อมใช้งานหรือไม่

- `void PMS5003_ClearReady(PMS5003_Instance* sensor)`
  - ล้างสถานะ frame_ready

- `PMS5003_Status PMS5003_GetData(PMS5003_Instance* sensor, PMS5003_Data* out)`
  - ดึงข้อมูลล่าสุดที่ parse แล้ว

- `PMS5003_Status PMS5003_Sleep(PMS5003_Instance* sensor)`
  - ส่งคำสั่ง sleep

- `PMS5003_Status PMS5003_Wakeup(PMS5003_Instance* sensor)`
  - ส่งคำสั่ง wakeup

- `PMS5003_Status PMS5003_SetPassiveMode(PMS5003_Instance* sensor)`
  - เปลี่ยนเป็น passive mode

- `PMS5003_Status PMS5003_SetActiveMode(PMS5003_Instance* sensor)`
  - เปลี่ยนเป็น active mode

- `PMS5003_Status PMS5003_Request(PMS5003_Instance* sensor)`
  - ขอข้อมูล 1 frame (เหมาะกับ passive mode)

---

รองรับ PMS5003/PMS7003 โดยไม่ต้องเปลี่ยนโค้ดไลบรารี เพียงต่อสายและ init ให้ถูกต้อง

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)