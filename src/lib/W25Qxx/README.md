# W25Qxx SPI Flash Memory Library

> **Library สำหรับอ่าน/เขียนหน่วยความจำภายนอก SPI Flash (W25Q16, W25Q32, W25Q64, W25Q128) สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [โครงสร้างของ Flash](#โครงสร้างของ-flash)
4. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
5. [ข้อควรระวังในการเขียน](#ข้อควรระวังในการเขียน)
6. [API Reference](#api-reference)

---

## ภาพรวม

โมดูล W25Qxx คือหน่วยความจำแบบ **Non-volatile NOR Flash** ที่สื่อสารผ่านโปรโตคอล SPI  
เหมาะสำหรับเก็บข้อมูลจำนวนมากที่หน่วยความจำภายใน MCU ไม่พอ เช่น รูปภาพสำหรับจอ OLED, ไฟล์เสียง, ข้อมูล Log ระยะยาว หรือ Font ขนาดใหญ่

---

## Hardware Setup

### การเชื่อมต่อ

| W25Qxx Pin | CH57x Pin | หมายเหตุ |
|------------|--------------|----------|
| VCC        | 3.3V         | แรงดันไฟเลี้ยง (2.7V - 3.6V) |
| GND        | GND          | กราวด์ร่วม |
| CS         | PC4 (Option) | Chip Select (สามารถเปลี่ยนได้ในโค้ด) |
| DO (MISO)  | PC7          | ข้อมูลส่งออกจาก Flash |
| DI (MOSI)  | PC6          | ข้อมูลส่งเข้า Flash |
| CLK (SCK)  | PC5          | สัญญาณนาฬิกา SPI |

---

## โครงสร้างของ Flash

เพื่อให้ใช้งานได้ถูกต้อง ต้องเข้าใจการแบ่งระดับของหน่วยความจำ (ตัวอย่างรุ่น W25Q32 - 4MB):
- **1 Block** = 64 KB (มี 64 blocks)
- **1 Sector** = 4 KB (มี 16 sectors ใน 1 block)
- **1 Page** = 256 Bytes (มี 16 pages ใน 1 sector)

> ⚠️ **กฎเหล็ก**: การเขียนข้อมูลจะเขียนเป็น **Page** แต่การลบข้อมูลต้องลบขั้นต่ำเป็น **Sector** (4KB)

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/W25Qxx/W25Qxx.h"

W25Qxx_Instance flash;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();
    
    // 1. เริ่มต้น SPI และ Flash (ใช้ CS ที่ขา PC4)
    SPI_SimpleInit(SPI_MODE0, SPI_8MHZ, SPI_PINS_DEFAULT);
    W25Qxx_Init(&flash, PC4);

    printf("Flash ID: %06lX\r\n", flash.jedec_id);
    printf("Capacity: %lu KB\r\n", flash.capacity / 1024);

    // 2. การเขียนข้อมูล (ต้อง Erase ก่อนเสมอ)
    uint32_t addr = 0x001000; // เริ่มที่ Sector 1
    W25Qxx_EraseSector(&flash, addr);
    
    uint8_t writeData[] = "Hello from CH57x!";
    W25Qxx_Write(&flash, addr, writeData, sizeof(writeData));

    // 3. การอ่านข้อมูล
    uint8_t readBuf[32];
    W25Qxx_Read(&flash, addr, readBuf, 32);
    
    printf("Read data: %s\r\n", readBuf);

    while(1) {}
}
```

---

## ข้อควรระวังในการเขียน

1. **Erase Before Write**: เซลล์หน่วยความจำ Flash สามารถเปลี่ยนจาก 1 เป็น 0 ได้เท่านั้น หากต้องการเขียนทับข้อมูลเดิมที่มีค่าเป็น 0 ต้องสั่ง Erase เพื่อคืนค่าเป็น 1 ทั้ง Sector ก่อน
2. **Page Boundary**: ฟังก์ชัน `W25Qxx_Write` ใน Library นี้ได้จัดการเรื่องการขึ้นหน้าใหม่ (Page Boundary) ให้อัตโนมัติแล้ว คุณสามารถเขียนข้อมูลยาวๆ ข้าม Page ได้เลย
3. **Write Endurance**: Flash มีอายุการใช้งานประมาณ 100,000 รอบการเขียน/ลบ

---

## API Reference

- `W25Qxx_Init(flash, pin_cs)` : เริ่มต้นใช้งานและอ่านข้อมูลชิป
- `W25Qxx_Read(flash, addr, buf, len)` : อ่านข้อมูลจาก Address ที่กำหนด
- `W25Qxx_Write(flash, addr, data, len)` : เขียนข้อมูล (จัดการ Erase ภายนอกเอง)
- `W25Qxx_EraseSector(flash, addr)` : ลบข้อมูลขนาด 4KB
- `W25Qxx_EraseChip(flash)` : ลบข้อมูลทั้งชิป (ใช้เวลานาน)
- `W25Qxx_PowerDown(flash)` : เข้าโหมดประหยัดพลังงาน
- `W25Qxx_WakeUp(flash)` : ปลุกชิปให้กลับมาทำงาน

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)