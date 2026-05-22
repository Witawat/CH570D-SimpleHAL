# LCD1602 I2C Library สำหรับ CH57x

ไลบรารีสำหรับควบคุมจอ LCD1602 และ LCD2004 ผ่านโมดูล I2C (PCF8574/PCF8574A) ออกแบบมาให้ใช้งานง่ายในรูปแบบ Arduino-style สำหรับไมโครคอนโทรลเลอร์ CH57x

## 🌟 คุณสมบัติ
- รองรับจอ LCD ขนาด **16x2** และ **20x4**
- เชื่อมต่อผ่าน I2C ใช้สายสัญญาณเพียง 2 เส้น (SDA, SCL)
- API ใช้งานง่ายเหมือน Arduino LiquidCrystal_I2C
- รองรับการสร้าง **Custom Characters** (ตัวอักษรพิเศษ) ได้สูงสุด 8 ตัว
- ควบคุม **Backlight** (เปิด/ปิด) ได้ผ่านซอฟต์แวร์
- มีฟังก์ชันช่วยแสดงผลตัวเลข **Integer** และ **Float** ในตัว
- รองรับการเลื่อนข้อความ (Scrolling) และการตั้งค่าทิศทางการเขียน

## 🔌 การเชื่อมต่อฮาร์ดแวร์

| LCD I2C Module | CH57x Pin | หมายเหตุ |
|----------------|--------------|----------|
| VCC            | 5V           | แรงดันไฟเลี้ยง 5V |
| GND            | GND          | กราวด์ร่วม |
| SDA            | PC1          | ข้อมูล I2C (ต้องการ Pull-up 4.7kΩ) |
| SCL            | PC2          | สัญญาณนาฬิกา I2C (ต้องการ Pull-up 4.7kΩ) |

> **คำแนะนำ:** หากหน้าจอไม่แสดงตัวอักษร ให้ลองหมุนตัวต้านทานปรับค่าได้ (Potentiometer) ด้านหลังโมดูล I2C เพื่อปรับความคมชัด (Contrast)

## 🚀 การเริ่มต้นใช้งาน

### 1. การตั้งค่าพื้นฐาน
ก่อนใช้งาน LCD ต้องทำการเริ่มต้นระบบและ Timer ของระบบก่อน:

```c
#include "main.h"
#include "Lib/LCD1602_I2C/lcd1602_i2c.h"

LCD1602_Handle lcd;

int main(void) {
    // 1. อัปเดตความถี่สัญญาณนาฬิกาและตั้งค่า Priority (จำเป็น)
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    // 2. เริ่มต้น Timer สำหรับฟังก์ชัน Delay
    Timer_Init();

    // 3. เริ่มต้น I2C (100kHz, ใช้ขา Default: SDA=PC1, SCL=PC2)
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
    
    // 4. เริ่มต้น LCD (Address 0x27, ขนาด 16x2)
    LCD_Init(&lcd, 0x27, LCD_16x2);
    
    // 5. ใช้งานได้ทันที!
    LCD_Print(&lcd, "Hello CH57x");

    while(1) {
        // วนลูปการทำงาน
    }
}
```

## 📚 รายละเอียดฟังก์ชัน (API Reference)

### ฟังก์ชันหลัก (Core Functions)
- `LCD_Init(handle, addr, size)` : เริ่มต้นใช้งาน LCD
- `LCD_Clear(handle)` : ล้างหน้าจอ (ใช้เวลาประมาณ 2ms)
- `LCD_Home(handle)` : กลับไปตำแหน่ง (0,0)
- `LCD_SetCursor(handle, col, row)` : ย้าย Cursor ไปยังตำแหน่งที่ต้องการ
- `LCD_Print(handle, string)` : แสดงข้อความ String
- `LCD_PrintChar(handle, char)` : แสดงตัวอักษรตัวเดียว

### การควบคุมการแสดงผล (Display Control)
- `LCD_Backlight(handle, status)` : เปิด (1) หรือ ปิด (0) ไฟพื้นหลัง
- `LCD_Display(handle, status)` : เปิด (1) หรือ ปิด (0) การแสดงผลตัวอักษร
- `LCD_Cursor(handle, status)` : เปิด (1) หรือ ปิด (0) ขีด Cursor
- `LCD_Blink(handle, status)` : เปิด (1) หรือ ปิด (0) การกระพริบของ Cursor

### ฟังก์ชันขั้นสูง (Advanced Functions)
- `LCD_CreateChar(handle, location, charmap)` : สร้างตัวอักษรพิเศษ (0-7)
- `LCD_ScrollDisplayLeft(handle)` : เลื่อนหน้าจอไปทางซ้าย 1 ช่อง
- `LCD_ScrollDisplayRight(handle)` : เลื่อนหน้าจอไปทางขวา 1 ช่อง
- `LCD_AutoScroll(handle, status)` : เปิด/ปิด ระบบเลื่อนหน้าจออัตโนมัติ

### ฟังก์ชันช่วย (Helper Functions)
- `LCD_PrintInt(handle, number)` : แสดงตัวเลขจำนวนเต็ม
- `LCD_PrintFloat(handle, number, decimals)` : แสดงตัวเลขทศนิยม (กำหนดจำนวนหลักได้)
- `LCD_PrintAt(handle, col, row, string)` : ย้ายตำแหน่งและแสดงข้อความในคำสั่งเดียว
- `LCD_Printf(handle, format, ...)` : แสดงผลแบบฟอร์แมต (เหมือน `printf`)
- `LCD_ClearLine(handle, row)` : ล้างข้อความเฉพาะบรรทัดที่กำหนด
- `LCD_CenterPrint(handle, row, string)` : แสดงข้อความกึ่งกลางบรรทัด
- `LCD_ToggleBacklight(handle)` : สลับการเปิด/ปิดไฟ Backlight

## 💡 ตัวอย่างการใช้งานที่น่าสนใจ

### การแสดงผลแบบ Formatted (Printf)
```c
int temp = 25;
float humidity = 60.5;
LCD_SetCursor(&lcd, 0, 0);
LCD_Printf(&lcd, "T:%dC H:%.1f%%", temp, humidity);
```

### การทำหน้าจอ Welcome แบบกึ่งกลาง
```c
LCD_CenterPrint(&lcd, 0, "CH57x");
LCD_CenterPrint(&lcd, 1, "SimpleHAL");
```

### การอัปเดตค่าเฉพาะบรรทัด (ไม่ให้จอกระพริบ)
แทนที่จะใช้ `LCD_Clear()` ซึ่งจะล้างทั้งหน้าจอและอาจทำให้จอกระพริบ ให้ใช้:
```c
LCD_ClearLine(&lcd, 1); // ล้างเฉพาะบรรทัดที่ 2
LCD_Print(&lcd, "New Value: 123");
```

### การสร้างตัวอักษรพิเศษ (Custom Character)
```c
uint8_t heart[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000
};

LCD_CreateChar(&lcd, 0, heart); // เก็บไว้ที่ตำแหน่ง 0
LCD_SetCursor(&lcd, 0, 1);
LCD_PrintChar(&lcd, 0);         // แสดงรูปหัวใจ
```

## 🛠 การแก้ไขปัญหา (Troubleshooting)
1. **หน้าจอว่างเปล่า (Backlight ติดแต่ไม่มีตัวอักษร):** ให้ปรับตัวต้านทานสีฟ้าด้านหลังโมดูล I2C
2. **I2C Address ไม่ถูกต้อง:** โดยปกติจะเป็น `0x27` หากไม่ทำงานให้ลอง `0x3F`
3. **ตัวอักษรเพี้ยน:** ตรวจสอบความต้านทาน Pull-up ที่สาย SDA/SCL และตรวจสอบว่าเรียก `Timer_Init()` หรือยัง
4. **Build Error:** ตรวจสอบว่าได้เพิ่มไฟล์ `SimpleI2C.c` และ `SimpleDelay.c` เข้าไปในโปรเจกต์แล้วหรือยัง

---
**พัฒนาโดย:** MAKER WITAWAT
**รองรับบอร์ด:** CH57x Development Board

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)