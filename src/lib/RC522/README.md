# RC522 Library

ไลบรารีอ่านบัตร/แท็ก RFID ผ่านโมดูล MFRC522 บน CH57x
รองรับการตรวจพบการ์ด อ่าน UID และสั่ง Halt การ์ด

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

RC522 (MFRC522) เป็น RFID reader ความถี่ 13.56MHz
ไลบรารีนี้เน้นการใช้งานพื้นฐานที่ใช้บ่อยในงาน access control

- ตรวจว่ามีการ์ดอยู่หน้าเสาอากาศหรือไม่
- อ่าน UID ของการ์ด
- หยุดการ์ดด้วยคำสั่ง Halt

ใช้ SPI แบบ manual CS control

---

## คุณสมบัติ

- รองรับ SPI default ของโปรเจกต์
- มีฟังก์ชันเช็ค card present แบบง่าย
- อ่าน UID ได้สูงสุด 10 ไบต์
- อ่าน version ของชิปเพื่อใช้วิเคราะห์การต่อสาย
- มี reset ชิปจาก MCU

---

## Hardware Setup

### Wiring (SPI default)

| CH57x | RC522 |
|---|---|
| PC5 (SCK) | SCK |
| PC6 (MOSI) | MOSI |
| PC7 (MISO) | MISO |
| GPIO (เช่น PC4) | SDA/CS |
| GPIO (เช่น PD2) | RST |
| 3.3V | 3.3V |
| GND | GND |

ข้อควรระวัง

- RC522 ใช้ไฟ 3.3V เท่านั้น
- ต้องเรียก SPI_SimpleInit ก่อน RC522_Init

---

## หลักการทำงาน

ลำดับการอ่าน UID โดยทั่วไป

1. Initialize SPI และ RC522
2. Poll ว่ามีการ์ดเข้ามาหรือไม่
3. Anti-collision และ select เพื่อดึง UID
4. ประมวลผล UID
5. ส่ง Halt ลดการอ่านซ้ำเฟรมเดิม

---

## การใช้งานพื้นฐาน

### Quick start

```c
#include "SimpleHAL.h"
#include "RC522.h"

RC522_Instance rfid;

int main(void)
{
    SystemCoreClockUpdate();
    Timer_Init();

    SPI_SimpleInit(SPI_MODE0, SPI_4MHZ, SPI_PINS_DEFAULT);
    RC522_Init(&rfid, PC4, PD2);   // CS=PC4, RST=PD2

    while (1) {
        uint8_t uid[RC522_UID_MAX_LEN];
        uint8_t uid_len = 0;

        if (RC522_IsCardPresent(&rfid)) {
            if (RC522_ReadUID(&rfid, uid, &uid_len) == RC522_OK) {
                // ใช้ uid[0..uid_len-1]
            }
            RC522_Halt(&rfid);
            Delay_Ms(200);
        }
    }
}
```

### พิมพ์ UID ทาง serial

```c
USART_SimpleInit(BAUD_115200, USART_PINS_DEFAULT);

if (RC522_ReadUID(&rfid, uid, &uid_len) == RC522_OK) {
    USART_Print("UID: ");
    for (uint8_t i = 0; i < uid_len; i++) {
        USART_PrintHex(uid[i], 1);
        USART_Print(" ");
    }
    USART_Print("\r\n");
}
```

---

## การใช้งานขั้นสูง

### ตรวจเวอร์ชันชิปตอนบูต

```c
uint8_t ver = RC522_GetVersion(&rfid);
// 0x91 = v1.0, 0x92 = v2.0
// 0x00/0xFF มักหมายถึงสื่อสารไม่ได้
```

### Hardware reset เมื่อสื่อสารผิดปกติ

```c
RC522_Reset(&rfid);
Delay_Ms(50);
```

### โค้ดตัวอย่าง whitelist UID

```c
uint8_t allow_uid[4] = {0xDE, 0xAD, 0xBE, 0xEF};

if (uid_len == 4 &&
    uid[0] == allow_uid[0] &&
    uid[1] == allow_uid[1] &&
    uid[2] == allow_uid[2] &&
    uid[3] == allow_uid[3]) {
    // grant access
}
```

---

## Troubleshooting

### อ่านการ์ดไม่เจอ

| สาเหตุ | วิธีแก้ |
|---|---|
| ไม่ได้ init SPI | เรียก SPI_SimpleInit ก่อน RC522_Init |
| CS/RST ต่อผิด | ตรวจขาที่ส่งเข้า RC522_Init |
| ไฟเลี้ยงไม่ใช่ 3.3V | ใช้ 3.3V เท่านั้น |

### RC522_GetVersion ได้ 0x00 หรือ 0xFF

| สาเหตุ | วิธีแก้ |
|---|---|
| สื่อสาร SPI ไม่สำเร็จ | ตรวจ SCK/MOSI/MISO/CS |
| สายหลวม/ยาวเกิน | ลดความยาวสายและจัดกราวด์ให้ดี |

### UID เด้งซ้ำหลายครั้ง

| สาเหตุ | วิธีแก้ |
|---|---|
| ไม่มี debounce ระดับแอป | ใส่ Delay หรือ state machine ฝั่งแอป |
| ไม่ได้ส่ง Halt | เรียก RC522_Halt หลังอ่านสำเร็จ |

---

## API Reference

### Constants

- RC522_UID_MAX_LEN = 10
- RC522_TIMEOUT_MS = 25

### Types

- RC522_Status
  - RC522_OK
  - RC522_ERROR_PARAM
  - RC522_ERROR_NOCARD
  - RC522_ERROR_COLLISION
  - RC522_ERROR_TIMEOUT
  - RC522_ERROR_CRC
  - RC522_ERROR_GENERAL
- RC522_Instance

### Functions

- RC522_Status RC522_Init(RC522_Instance* rfid, uint8_t pin_cs, uint8_t pin_rst)
- uint8_t RC522_IsCardPresent(RC522_Instance* rfid)
- RC522_Status RC522_ReadUID(RC522_Instance* rfid, uint8_t* uid, uint8_t* uid_len)
- void RC522_Halt(RC522_Instance* rfid)
- void RC522_Reset(RC522_Instance* rfid)
- uint8_t RC522_GetVersion(RC522_Instance* rfid)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)