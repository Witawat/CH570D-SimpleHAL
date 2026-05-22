# CH570D Firmware — SimpleHAL

โปรเจกต์เฟิร์มแวร์สำหรับ **CH570D** (RISC-V MCU จาก WCH Nanjing Qinheng) พร้อม **SimpleHAL** — abstraction layer ที่เขียนด้วยภาษาไทย อ่านง่าย ใช้งานง่าย

## คุณสมบัติ

- **Timer** — หน่วงเวลา, periodic/one-shot ผ่าน Timer interrupt
- **GPIO** — อ่าน/เขียน/toggle ขาดิจิทัล
- **UART** — รับ-ส่งข้อมูลแบบ blocking และ async พร้อม ring buffer
- **SPI** — ส่ง-รับข้อมูลผ่าน SPI
- **I2C** — สื่อสารกับอุปกรณ์ I2C (master)
- **PWM** — สร้างสัญญาณ PWM ความถี่แปรผัน
- **ADC** — อ่านค่าแอนะล็อก
- **Flash** — อ่าน/เขียน/ลบหน่วยความจำ Flash ภายใน
- **RTC** — ตั้งเวลาปลุก (alarm) พร้อม callback
- **Power Management** — จัดการโหมด sleep/wake
- **Clock** — ตั้งค่านาฬิการะบบ
- **KeyScan** — สแกนคีย์เมทริกซ์

*(โมดูล BLE และ RF รองรับ แต่ต้องเปิดใช้งานใน `simple_hal_config.h` และมี library ของ WCH เพิ่มเติม)*

## โครงสร้างโปรเจกต์

```
CH570D/
├── src/
│   ├── Main.c                     # ไฟล์หลัก (LED กระพริบ + UART echo + ปุ่มกด)
│   └── SimpleHAL/                 # SimpleHAL abstraction layer
│       ├── core/                  # โมดูลพื้นฐาน (ringbuf, types, utils)
│       ├── hal_gpio.c/h           # GPIO
│       ├── hal_uart.c/h           # UART
│       ├── hal_spi.c/h            # SPI
│       ├── hal_i2c.c/h            # I2C
│       ├── hal_timer.c/h          # Timer
│       ├── hal_pwm.c/h            # PWM
│       ├── hal_adc.c/h            # ADC
│       ├── hal_flash.c/h          # Flash
│       ├── hal_rtc.c/h            # RTC
│       ├── hal_pwr.c/h            # Power management
│       ├── hal_clk.c/h            # Clock
│       ├── hal_sys.c/h            # System (delay, reset)
│       ├── hal_keyscan.c/h        # KeyScan matrix
│       ├── hal_ble.c/h            # BLE (ต้องเปิด + library WCH)
│       ├── hal_rf.c/h             # RF 2.4GHz (ต้องเปิด + library WCH)
│       └── simple_hal_config.h    # ตั้งค่ากลาง (ขนาด buffer, เปิด/ปิดโมดูล)
├── StdPeriphDriver/               # WCH Standard Peripheral Driver
│   ├── inc/                       # header files
│   ├── *.c                        # source files
│   └── libISP572.a                # ISP library
├── Startup/                       # startup code
├── RVMSIS/                        # core_riscv.h
├── Ld/                            # linker script
├── scripts/                       # build scripts
├── obj/                           # object files
└── output/                        # .elf, .hex, .lst
```

## การ Build

ใช้ MounRiver Studio 2 (ไฟล์โปรเจกต์: `CH570D.wvproj`)
หรือ build ผ่านบรรทัดคําสั่ง:

```batch
cd CH570D
scripts\build.bat
```

output อยู่ที่ `output/CH570D.hex`

## การต่อวงจร (เริ่มต้น)

| อุปกรณ์ | ขา | หมายเหตุ |
|---------|-----|----------|
| UART TX | PA3 | 115200 baud, 8N1 |
| UART RX | PA2 | |
| LED     | PA11 | กระพริบทุก 500 ms |
| ปุ่มกด  | PA8 | กดลง GND (active low) |

## ตัวอย่างการเขียนโค้ด

```c
#include "simple_hal.h"

hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
hal_gpio_write(led, 1);           // ติด LED
hal_gpio_toggle(led);             // กลับสถานะ

hal_uart_handle_t uart = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
hal_uart_send(uart, (const uint8_t *)"Hello\r\n", 7);

hal_delay_ms(500);                // หน่วง 500 ms
```

## License

ซอร์สโค้ดภายใต้ลิขสิทธิ์ของ Nanjing Qinheng Microelectronics Co., Ltd. (WCH)
ส่วน SimpleHAL เป็นOriginal work ที่พัฒนาเพิ่มเติม
