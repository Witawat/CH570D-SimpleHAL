# HC-05 Bluetooth Library

> **Library สำหรับใช้งานโมดูลบลูทูธ HC-05 ผ่าน USART สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งาน Data Mode (ส่ง/รับปกติ)](#การใช้งาน-data-mode-ส่งรับปกติ)
4. [การใช้งาน AT Mode (ตั้งค่าโมดูล)](#การใช้งาน-at-mode-ตั้งค่าโมดูล)
5. [API Reference](#api-reference)

---

## ภาพรวม

HC-05 คือโมดูล Bluetooth Classic (2.0/2.1) ที่นิยมที่สุดสำหรับการสื่อสารไร้สายแบบ Serial  
Library นี้ช่วยให้ CH57x สามารถรับ-ส่งข้อมูลกับมือถือ (Android) หรือคอมพิวเตอร์ได้ง่ายขึ้น รวมถึงการตั้งค่าชื่อ (Name), รหัสผ่าน (PIN), และ Baud rate ผ่าน AT Commands

---

## Hardware Setup

### การเชื่อมต่อ

| HC-05 Pin | CH57x Pin | หมายเหตุ |
|-----------|--------------|----------|
| VCC       | 5V / 3.3V    | แรงดันไฟเลี้ยงโมดูล |
| GND       | GND          | กราวด์ร่วม |
| TXD       | PD6 (RX1)    | ข้อมูลส่งออกจากบลูทูธ |
| RXD       | PD5 (TX1)    | ข้อมูลส่งเข้าบลูทูธ (แนะนำต่อผ่าน Resistor Divider ถ้าใช้ 5V) |
| EN / KEY  | GPIO (Option)| ลาก HIGH ตอนเปิดเครื่องเพื่อเข้า AT Mode |
| STATE     | GPIO (Option)| ตรวจสอบสถานะการเชื่อมต่อ (1 = Connected) |

---

## การใช้งาน Data Mode (ส่ง/รับปกติ)

โหมดนี้ใช้สำหรับการส่งข้อความ หรือสั่งงานผ่านแอปบลูทูธ (เช่น Serial Bluetooth Terminal)

```c
#include "main.h"
#include "Lib/HC05/HC05.h"
#include <string.h>

HC05_Instance bt;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();

    // 1. เริ่มต้นบลูทูธ (Default Baud rate ของ HC-05 ส่วนใหญ่คือ 9600)
    HC05_Init(&bt, 9600);

    HC05_SendString(&bt, "HC-05 Ready!\r\n");

    char rxBuffer[64];

    while(1) {
        // 2. ตรวจสอบและอ่านข้อมูลที่ส่งมา (อ่านจนจบบรรทัด)
        if (HC05_ReadLine(&bt, rxBuffer, 64, 100) == HC05_OK) {
            printf("Received: %s\r\n", rxBuffer);
            
            // ตัวอย่างการสั่งงาน
            if (strcmp(rxBuffer, "LED_ON") == 0) {
                // ทำการเปิดไฟ...
                HC05_SendString(&bt, "LED is now ON\r\n");
            }
        }
    }
}
```

---

## การใช้งาน AT Mode (ตั้งค่าโมดูล)

**ขั้นตอนการเข้า AT Mode:**
1. ปิดไฟเลี้ยง HC-05
2. ต่อขา **EN / KEY** เข้ากับ **VCC (3.3V)**
3. เปิดไฟเลี้ยงโมดูล (ไฟ LED จะกระพริบช้าๆ ทุก 2 วินาที)
4. ตั้ง Baud rate ในโค้ดเป็น **38400** (Baud rate มาตรฐานของ AT Mode)

```c
char response[64];
HC05_Init(&bt, 38400); // AT Mode ใช้ 38400 เสมอ

// ตรวจสอบสถานะ
HC05_ATCommand(&bt, "AT", response, 64, 1000); // ควรได้รับ "OK"

// เปลี่ยนชื่อบลูทูธ
HC05_ATCommand(&bt, "AT+NAME=CH32_BT", response, 64, 1000);

// เปลี่ยนรหัสผ่าน (PIN)
HC05_ATCommand(&bt, "AT+PSWD=1234", response, 64, 1000);
```

---

## API Reference

- `HC05_Init(bt, baudrate)` : เริ่มต้นใช้งาน
- `HC05_SendString(bt, str)` : ส่งข้อความ String
- `HC05_SendByte(bt, byte)` : ส่งข้อมูล 1 Byte
- `HC05_Available(bt)` : ตรวจสอบจำนวนข้อมูลที่รอรับใน Buffer
- `HC05_ReadByte(bt, &byte)` : อ่านข้อมูล 1 Byte
- `HC05_ReadLine(bt, buf, max_len, timeout)` : อ่านข้อมูลจนจบบรรทัด (`\n`)
- `HC05_ATCommand(bt, cmd, resp, len, timeout)` : ส่งคำสั่ง AT Command
- `HC05_Flush(bt)` : ล้างข้อมูลใน Buffer รับ

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)