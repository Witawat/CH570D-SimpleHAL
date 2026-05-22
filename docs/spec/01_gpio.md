# GPIO

## Overview

16 digital I/O pins (PA0–PA15) บน CH57x แต่ละขาสามารถเป็น Input หรือ Output ได้อิสระต่อกัน มี interrupt แบบ edge/level ได้ทุกขา และสามารถ remap UART/Timer/I2C ไปยังขาอื่นได้

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_gpio_init(pin, mode)` | ตั้งค่าโหมดของ pin |
| `hal_gpio_write(pin, val)` | เขียน 0 หรือ 1 |
| `hal_gpio_read(pin)` | อ่านค่าปัจจุบัน |
| `hal_gpio_toggle(pin)` | กลับค่าขา |
| `hal_gpio_set(pin)` | เซ็ตเป็น HIGH |
| `hal_gpio_reset(pin)` | เซ็ตเป็น LOW |
| `hal_gpio_attach_irq(pin, cb, mode)` | ตั้ง interrupt callback |
| `hal_gpio_detach_irq(pin)` | ยกเลิก interrupt |

**Mode (`GPIO_PinMode` = `hal_gpio_mode_t`):** `INPUT_FLOATING` (0), `INPUT_PULLUP` (1), `INPUT_PULLDOWN` (2), `OUTPUT_PP_5mA` (3), `OUTPUT_PP_20mA` (4), `PIN_MODE_OUTPUT_OD` (sentinel 0xFF)

**Interrupt mode (`GPIO_InterruptMode`):** `LOW_LEVEL`, `HIGH_LEVEL`, `FALLING`, `RISING`

`pinMode()` และ `digitalWrite/Read/Toggle()` คือ alias ของ Arduino-compat layer ที่เรียกฟังก์ชัน HAL ข้างต้น

## Circuit Guide

| โหมด | ต่อวงจร |
|---|---|
| **INPUT_FLOATING** | สัญญาณ digital จากอุปกรณ์อื่น (MCU, sensor digital, comparator output) — ขา Output ของอีกฝ่ายต้อง push-pull |
| **INPUT_PULLUP** | สวิตช์ / ปุ่มกด ต่อระหว่างขา ↔ GND (กด = LOW, ปล่อย = HIGH) — ไม่ต้องใช้ resistor ภายนอก |
| **INPUT_PULLDOWN** | สวิตช์ / ปุ่มกด ต่อระหว่างขา ↔ VCC (กด = HIGH, ปล่อย = LOW) |
| **OUTPUT_PP_5mA** | LED ผ่าน resistor 220Ω–1kΩ (กระแส ~3–15mA), ขา CS/SCLK/MOSI ของ SPI, buzzer ทั่วไป |
| **OUTPUT_PP_20mA** | LED กำลังสูง, SSR, รีเลย์ขนาดเล็ก (ตรวจสอบ total current limit ของชิพ), หรือกรณีที่ 5mA ขับไม่พอ |
| **OUTPUT_OD (0xFF)** | I²C SDA/SCL (ต้องมี pull-up 1k–10kΩ ไป VCC), สาย shared bus หลายอุปกรณ์, level shifter ขาเดียว (pull-up ไป VCC ของฝ่ายรับ) |

```
// สวิตช์กดติด — INPUT_PULLUP
PA0 ──── SW ──── GND

// LED — OUTPUT_PP_5mA (resistor ประมาณ 470Ω)
PA1 ──── R ──── LED ──── GND

// Open-Drain + Pull-Up (เช่น I²C simulation)
PA2 ──┬── R (4.7kΩ) ──── VCC_3.3V
      └── SDA bus
```

## CH57x Specifics

- **ทุกขารองรับ interrupt:** เลือก Rising/Falling edge หรือ Low/High level
- **Pin remap:** UART, Timer, I2C สามารถเปลี่ยนขาได้ (ผ่าน `GPIOPinRemap()`) — ดูตารางใน [appendix_pinout.md](appendix_pinout.md)
- **5V tolerant:** ขา PA2, PA3, PA8–PA11 ทน 5V (ใช้กับวงจร 5V logic ได้โดยตรง)
- **Total current:** ไม่ควรเกิน 80mA รวมทุกขา (absolute maximum) — ระวังถ้าใช้ OUTPUT_PP_20mA หลายขาพร้อมกัน
- **PA11 ใช้เชื่อมต่อ LED ในแผ่นพัฒนา:** บอร์ด EVB ส่วนใหญ่มี LED ติดที่ PA11 (active-high) — ใช้ digitalWrite(PA11, HIGH) เพื่อติด

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| `GPIO_TypeDef` เป็น `void*` | ไม่ type safe — รับประกันว่าใช้งานได้แค่ GPIOA เท่านั้น |
| `GPIO_SetBits/ResetBits` | รองรับแค่ GPIOA (ไม่สามารถใช้กับพอร์ตอื่น) |
| `PIN_MODE_OUTPUT_OD` เป็น sentinel | ไม่มีใน HAL โดยตรง — SimpleHAL จำลองโดยสลับ Input Floating ↔ PP_5mA ตามค่าที่เขียน |
| Remap: SPI ไม่มี | SPI ขายึดตายตัว — ดูตาราง pinout |
| 100MHz max | GPIO toggle speed ประมาณ ~25MHz ที่ 100MHz sysclock (ขึ้นกับ bus timing) |

## Code สั้น

```c
pinMode(PA0, INPUT_PULLUP);           // ปุ่มกด
pinMode(PA1, OUTPUT_PP_5mA);          // LED
digitalWrite(PA1, !digitalRead(PA0)); // กดปุ่ม → LED ติด

pinModeMultiple((uint8_t[]){PA2, PA3, PA4}, OUTPUT_PP_5mA); // หลายขาพร้อมกัน

attachInterrupt(PA0, []() {
    pressed = true;
}, FALLING);
```
