# CH570D SimpleHAL v1.0.0

เฟิร์มแวร์สำหรับ **CH570D** (RISC-V MCU จาก WCH) พร้อม **SimpleHAL** — abstraction layer ที่เขียนด้วยภาษาไทย อ่านง่าย ใช้งานง่าย

## คุณสมบัติ

### SimpleHAL Modules
| โมดูล | คำอธิบาย |
|--------|----------|
| GPIO | อ่าน/เขียน/toggle ขาดิจิทัล |
| UART | รับ-ส่งแบบ blocking และ async พร้อม ring buffer |
| SPI | ส่ง-รับข้อมูล |
| I2C | สื่อสารอุปกรณ์ I2C (master) |
| Timer | หน่วงเวลา, periodic/one-shot |
| PWM | สร้างสัญญาณ PWM |
| ADC | อ่านค่าแอนะล็อก |
| Flash | อ่าน/เขียน/ลบ Flash ภายใน |
| RTC | ตั้งเวลาปลุกพร้อม callback |
| Power | จัดการ sleep/wake |
| Clock | ตั้งค่านาฬิการะบบ |
| KeyScan | สแกนคีย์เมทริกซ์ |
| RF | 2.4GHz RF (ต้องเปิด config + WCH library) |
| BLE | Bluetooth LE (ต้องเปิด config + WCH library) |
| **RingBuf** | ring buffer ทั่วไป (core) |
| **SoftTimer** | software timer แบบ non-blocking (core) |

### ตัวอย่าง (16 โปรเจกต์)

| # | ตัวอย่าง | คำอธิบาย |
|---|----------|----------|
| 00 | blink_led | กระพริบ LED ด้วย delay |
| 01 | uart_echo | UART echo |
| 02 | spi_lcd | แสดงผล SPI LCD |
| 03 | i2c_sensor | อ่านค่า sensor ผ่าน I2C |
| 04 | timer_blink | กระพริบ LED ด้วย Timer interrupt |
| 05 | pwm_fade | PWM ค่อย ๆ ติด/ดับ |
| 06 | adc_read | อ่านค่า ADC |
| 07 | flash_uid | อ่าน UID จาก Flash |
| 08 | rtc_alarm | RTC alarm callback |
| 09 | power_sleep | สลีปแล้วตื่นด้วย RTC |
| 10 | rf_2g4 | ส่ง-รับ RF 2.4GHz |
| 11 | ble_peripheral | BLE peripheral |
| 12 | keyscan | สแกนคีย์เมทริกซ์ |
| 13 | all_in_one | รวมทุกฟังก์ชัน |
| 14 | nonblock_delay | delay แบบไม่บล็อก |
| 15 | softimer_delay | delay ด้วย hal_softimer |

## Build

```batch
# เริ่มต้น
scripts\build.bat

# EXTRA_CFLAGS
set EXTRA_CFLAGS=-DHAL_ENABLE_BLE=1
scripts\build.bat

# ทดสอบทุก example อัตโนมัติ
scripts\test_examples.bat

# อัปโหลดผ่าน WCH-Link
scripts\upload.bat
```

## โครงสร้าง obj

object files mirror source tree:
```
obj/
├── CH570D.elf / .hex / .lst / .map
├── src/
│   ├── Main.o
│   └── SimpleHAL/...
├── StdPeriphDriver/
└── Startup/
```

## เปลี่ยนเเปลงตั้งแต่ commit แรก

- เพิ่ม SimpleHAL abstraction layer ครบทุกโมดูล
- เพิ่ม core modules: hal_ringbuf, hal_softimer, hal_types, hal_utils
- เพิ่มไฟล์ config: `simple_hal_config.h`
- เพิ่ม 16 ตัวอย่างการใช้งาน
- ระบบ build: build.bat, clean.bat, rebuild.bat, test_examples.bat, upload.bat
- รองรับ VS Code + clangd (c_cpp_properties.json, tasks.json)
- รองรับ EXTRA_CFLAGS สำหรับเปิด/ปิดโมดูล
- obj/ จัดระเบียบ mirror source tree
- แก้บั๊ก macro name: `RTC_TMR_CYC_S1` → `Period_32768`, `RB_PWR_RAM16K` → `RB_PWR_RAM12K`, `LL_TX_POWEER` → `LL_TX_PWR`
- เอกสารภาษาไทย: README_TH.md, api_reference.md, config_reference.md, module docs, LEARN_SIMPLEHAL.md
- เอกสารโมดูล: adc, ble, clock, flash, gpio, i2c, keyscan, power, pwm, rf, ringbuf, rtc, softimer, spi, system, timer, uart

## License

ซอร์สโค้ดภายใต้ลิขสิทธิ์ของ Nanjing Qinheng Microelectronics Co., Ltd. (WCH)
SimpleHAL เป็น Original work ที่พัฒนาเพิ่มเติม
