# Flash

## Overview

Flash memory ภายในใช้เก็บโปรแกรมและข้อมูล ผู้ใช้สามารถอ่าน unique ID และ option bytes ได้ผ่าน SimpleHAL แต่การเขียน/ลบ Flash ต้องใช้ StdPeriphDriver โดยตรง (libISP572.a)

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_flash_read(addr, buf, len)` | อ่านข้อมูลจาก Flash |
| `hal_flash_get_unique_id()` | อ่าน unique ID 8 ไบต์ (burn-in ที่โรงงาน) |
| `hal_flash_read_config()` | อ่าน option byte การทำงาน |

## CH57x Specifics

- **Flash size:** 240KB (CH570), 192KB (CH571/CH572)
- **Address range:** `0x00000000`–`0x0003BFFF` (240KB)
- **Unique ID:** 8 ไบต์ ที่ address `0x0003FFF0`–`0x0003FFF7` (อ่านด้วย `FLASH_ROM_READ()`)
- **Option bytes:** User option byte ที่ `0x0003FFF8` — config เช่น disable SWD, boot mode
- **Write:** ต้อง unlock + เขียนทีละ block (4KB erase + byte write) ผ่าน `libISP572.a`
- **EEPROM emulation:** สามารถใช้ Flash sector เป็น EEPROM ได้ (ระวัง write cycle ~10,000 ครั้ง)

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| ไม่มี API เขียน/ลบใน SimpleHAL | SimpleHAL รองรับแค่อ่าน — ถ้าต้องการเขียนใช้ `libISP572.a` หรือ WCH ISP tool |
| Write cycle ~10K | Flash endurance ~10,000 erase/write cycles — ไม่เหมาะกับ data logging ถี่ ๆ |
| Erase unit = 4KB | ลบทีละ 4KB — ถ้าต้องการเปลี่ยน 1 ไบต์ต้อง buffer ทั้ง block |
| Writing ต้องปลอด interrupt | ระหว่างเขียน Flash ต้อง disable interrupt — `__disable_irq()` / `__enable_irq()` |
| Power loss = corrupt | ถ้าไฟดับระหว่าง write → block นั้น corrupt |

## Code สั้น

```c
// อ่าน unique ID
uint8_t uid[8];
hal_flash_get_unique_id(uid);
printf("UID: %02X%02X%02X%02X %02X%02X%02X%02X\r\n",
       uid[0],uid[1],uid[2],uid[3],uid[4],uid[5],uid[6],uid[7]);

// อ่าน option byte
hal_flash_config_t cfg;
hal_flash_read_config(&cfg);

// อ่านค่าจาก Flash address ใด ๆ
uint8_t buf[16];
hal_flash_read(0x00001000, buf, 16);
```
