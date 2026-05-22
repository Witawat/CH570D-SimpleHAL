# I2CScan — I2C Address Scanner

Library สำหรับ scan หา I2C device บน bus และแสดงผลเป็นตาราง  
ผ่าน USART คล้ายกับคำสั่ง `i2cdetect` บน Linux

---

## ไฟล์

| ไฟล์ | คำอธิบาย |
|------|-----------|
| `I2CScan.h` | Header file — function prototypes และ configuration |
| `I2CScan.c` | Implementation |

---

## Dependencies

| Library | ไฟล์ |
|---------|------|
| SimpleI2C | `User/SimpleHAL/SimpleI2C.h` |
| SimpleUSART | `User/SimpleHAL/SimpleUSART.h` |
| SimpleDelay | `User/SimpleHAL/SimpleDelay.h` |

---

## การ Include ไฟล์ใน Makefile / Project

เพิ่มไฟล์ `I2CScan.c` เข้า build และ include path:

```
User/Lib/I2CScan/I2CScan.c
```

---

## การใช้งานพื้นฐาน

```c
#include "User/SimpleHAL/SimpleUSART.h"
#include "User/SimpleHAL/SimpleI2C.h"
#include "User/SimpleHAL/SimpleDelay.h"
#include "User/Lib/I2CScan/I2CScan.h"

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();

    /* เริ่มต้น USART สำหรับแสดงผล */
    USART_SimpleInit(BAUD_115200, USART_PINS_DEFAULT);

    /* เริ่มต้น I2C */
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    /* Scan และแสดงผล */
    I2C_Scan();

    while(1) {}
}
```

---

## ตัวอย่างผลลัพธ์

```
I2C_SCAN: Starting I2C scan...
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                                                  
10:                                                  
20:                                                  
30:                            -- -- 3c -- -- 3f -- --
40:                                                  
50:                                                  
60:                            68                   
70:                                                  
Scan finished. Found 3 device(s).
```

> อ่านผลลัพธ์: แถว `30:` คอลัมน์ `c` → address **0x3C** (เช่น OLED SSD1306)

---

## API Reference

### `void I2C_Scan(void)`

Scan I2C bus ทั้งหมด (address 0x08–0x77) และพิมพ์ตารางผ่าน USART

**ตัวอย่าง:**
```c
I2C_Scan();
```

---

### `uint8_t I2C_Probe(uint8_t addr)`

ตรวจสอบว่า device ตอบสนองที่ address ที่ระบุหรือไม่

| Parameter | ชนิด | คำอธิบาย |
|-----------|------|-----------|
| `addr` | `uint8_t` | 7-bit I2C address (0x00–0x7F) |

**Return:** `1` ถ้าพบ device (ACK), `0` ถ้าไม่พบ (NACK / timeout)

**ตัวอย่าง:**
```c
if (I2C_Probe(0x3C)) {
    USART_Print("OLED found at 0x3C\r\n");
}

if (I2C_Probe(0x68)) {
    USART_Print("MPU-6050 found at 0x68\r\n");
}
```

---

## Configuration (I2CScan.h)

| Define | Default | คำอธิบาย |
|--------|---------|-----------|
| `I2C_SCAN_TIMEOUT_MS` | `10` | Timeout ต่อ address (ms) |
| `I2C_SCAN_SHOW_EMPTY` | `0` | `1` = แสดง `--` สำหรับ address ที่ว่าง, `0` = แสดงเว้นว่าง |

ตัวอย่างเปลี่ยนค่า:
```c
// ใน I2CScan.h หรือใช้ -D flag
#define I2C_SCAN_TIMEOUT_MS  20
#define I2C_SCAN_SHOW_EMPTY   1   // แสดง "--" เหมือนในรูป
```

ผลลัพธ์เมื่อ `I2C_SCAN_SHOW_EMPTY = 1`:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                                                  
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- 3c -- -- 3f --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --                        
```

---

## Pin Configuration

### Default (I2C_PINS_DEFAULT)
| Signal | Pin |
|--------|-----|
| SCL | PC2 |
| SDA | PC1 |

### Remap (I2C_PINS_REMAP)
| Signal | Pin |
|--------|-----|
| SCL | PD0 |
| SDA | PD1 |

> ต้องต่อ pull-up resistor **4.7kΩ** ที่ SDA และ SCL ทุกครั้ง

---

## Common I2C Addresses

| Address | Device |
|---------|--------|
| 0x3C / 0x3D | OLED SSD1306 / SH1106 |
| 0x3F / 0x27 | LCD1602 PCF8574 |
| 0x48–0x4B | ADS1115 / PCF8591 |
| 0x57 | AT24C32 EEPROM (DS3231 module) |
| 0x68 | MPU-6050 / DS3231 RTC |
| 0x76 / 0x77 | BMP280 / BME280 |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)