# SimpleHAL — API Reference

เอกสารอ้างอิง API ครบทุกโมดูลของ SimpleHAL สำหรับ WCH CH572/CH570

---

## ดัชนีโมดูล

| # | โมดูล | คำอธิบาย | ไฟล์ |
|---|---|---|---|
| 1 | [GPIO](modules/gpio.md) | จัดการขา I/O ดิจิตอล พร้อม interrupt callback | `hal_gpio.h/.c` |
| 2 | [UART](modules/uart.md) | สื่อสารแบบอนุกรม พร้อม ring buffer และ async | `hal_uart.h/.c` |
| 3 | [SPI](modules/spi.md) | สื่อสาร SPI ทั้งแบบ polling และ DMA | `hal_spi.h/.c` |
| 4 | [I2C](modules/i2c.md) | สื่อสาร I2C แบบ master/slave | `hal_i2c.h/.c` |
| 5 | [Timer](modules/timer.md) | ตั้งเวลาจับเวลา แบบ periodic หรือ one-shot | `hal_timer.h/.c` |
| 6 | [PWM](modules/pwm.md) | สร้างสัญญาณ PWM 5 ช่อง 8 หรือ 16 บิต | `hal_pwm.h/.c` |
| 7 | [ADC](modules/adc.md) | อ่านค่าแอนะล็อกผ่าน CMP (4/8/9 บิต) | `hal_adc.h/.c` |
| 8 | [Flash](modules/flash.md) | อ่านข้อมูลจาก Flash ROM | `hal_flash.h/.c` |
| 9 | [RTC](modules/rtc.md) | จับเวลาจริง ตั้งปลุก ตั้ง timer | `hal_rtc.h/.c` |
| 10 | [Power](modules/power.md) | จัดการโหมดประหยัดพลังงาน | `hal_pwr.h/.c` |
| 11 | [Clock](modules/clock.md) | ตั้งค่าความถี่นาฬิกา | `hal_clk.h/.c` |
| 12 | [System](modules/system.md) | Delay, รีเซ็ต, Watchdog | `hal_sys.h/.c` |
| 13 | [KeyScan](modules/keyscan.md) | สแกนเมทริกซ์คีย์บอร์ด | `hal_keyscan.h/.c` |
| 14 | [RF](modules/rf.md) | สื่อสาร 2.4GHz ไร้สาย | `hal_rf.h/.c` |
| 15 | [BLE](modules/ble.md) | บลูทูธพลังงานต่ำ (BLE peripheral) | `hal_ble.h/.c` |
| 16 | [System](modules/system.md) | Non-blocking delay pattern | `hal_sys.h/.c` + `hal_timer.h/.c` |

---

## เอกสารเพิ่มเติม

- [Configuration & Core API Reference](config_reference.md) — การปรับแต่ง HAL และ Ring Buffer API
- [Interrupt & Callback Handling](interrupt_handling.md) — วิธีใช้งาน interrupt ในแต่ละโมดูล

---

## รูปแบบเอกสารในแต่ละโมดูล

ทุกโมดูลใช้รูปแบบมาตรฐานเดียวกัน:

```
### hal_xxx_init

```c
hal_xxx_handle_t hal_xxx_init(...);
```

**คำอธิบาย:** คำอธิบายการทำงาน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `param1` | `type` | คำอธิบาย | ค่าที่รับได้ |

**ค่าที่คืน:** ค่าที่ส่งกลับ

**ตัวอย่าง:**
```c
// โค้ดตัวอย่าง
```
```
