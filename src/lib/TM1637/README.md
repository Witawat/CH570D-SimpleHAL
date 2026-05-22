# TM1637 7-Segment Display Library

> **Library สำหรับควบคุมหน้าจอ 7-Segment 4-digit และ 6-digit ด้วย IC TM1637 สำหรับ CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [การแสดงผลตัวอักษรและข้อความ](#การแสดงผลตัวอักษรและข้อความ)
5. [API Reference](#api-reference)

---

## ภาพรวม

TM1637 คือ IC Driver ยอดนิยมสำหรับควบคุมหน้าจอ 7-Segment สื่อสารผ่านโปรโตคอลกึ่ง I2C (ใช้สายสัญญาณ 2 เส้น) ช่วยลดการใช้ขา GPIO ของ MCU จากเดิมที่ต้องใช้หลายสิบขาเหลือเพียง 2 ขาเท่านั้น

---

## Hardware Setup

### การเชื่อมต่อ

| TM1637 Pin | CH57x Pin | หมายเหตุ |
|------------|--------------|----------|
| VCC        | 3.3V / 5V    | แรงดันไฟเลี้ยง |
| GND        | GND          | กราวด์ร่วม |
| CLK        | Any GPIO     | สัญญาณนาฬิกา (เช่น PC0) |
| DIO        | Any GPIO     | ข้อมูลรับ-ส่ง (เช่น PC1) |

---

## การใช้งานพื้นฐาน

```c
#include "main.h"
#include "Lib/TM1637/TM1637.h"

TM1637_Handle* display;

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    Timer_Init();

    // 1. เริ่มต้น TM1637 (CLK=PC0, DIO=PC1, 4-digit)
    display = TM1637_Init(PC0, PC1, 4);

    // 2. ตั้งค่าความสว่าง (0 - 7)
    TM1637_SetBrightness(display, 5);

    while(1) {
        // 3. แสดงตัวเลขจำนวนเต็ม
        TM1637_DisplayNumber(display, 1234, false);
        Delay_Ms(2000);

        // 4. แสดงตัวเลขทศนิยม
        TM1637_DisplayFloat(display, 25.5, 1);
        Delay_Ms(2000);
        
        // 5. ล้างหน้าจอ
        TM1637_Clear(display);
        Delay_Ms(500);
    }
}
```

---

## การแสดงผลตัวอักษรและข้อความ

หน้าจอ 7-Segment มีข้อจำกัดในการแสดงผลตัวอักษร แต่ Library นี้รองรับตัวอักษรที่พอจะอ่านออกได้:

```c
// แสดงข้อความสั้นๆ (ไม่เกินจำนวนหลักที่มี)
TM1637_DisplayString(display, "HELP", 0);

// แสดงทีละตัวอักษร
TM1637_DisplayChar(display, 0, 'H', false);
TM1637_DisplayChar(display, 1, 'E', false);
TM1637_DisplayChar(display, 2, 'L', false);
TM1637_DisplayChar(display, 3, 'P', false);
```

---

## API Reference

- `TM1637_Init(clk, dio, digits)` : เริ่มต้นโมดูลและคืนค่า Handle
- `TM1637_SetBrightness(handle, level)` : ตั้งความสว่าง (0-7)
- `TM1637_Clear(handle)` : ล้างหน้าจอ
- `TM1637_DisplayNumber(handle, num, leading_zero)` : แสดงตัวเลขจำนวนเต็ม
- `TM1637_DisplayFloat(handle, num, decimal_places)` : แสดงตัวเลขทศนิยม
- `TM1637_DisplayString(handle, text, start_pos)` : แสดงข้อความ String
- `TM1637_DisplayDigit(handle, pos, digit, show_dp)` : แสดงตัวเลขระบุตำแหน่ง
- `TM1637_DisplayRaw(handle, pos, segments)` : ควบคุม Segment แต่ละดวงโดยตรง

---
**พัฒนาโดย:** CH57x Library Team
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)