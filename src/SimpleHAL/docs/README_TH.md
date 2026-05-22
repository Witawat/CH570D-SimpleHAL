# SimpleHAL — คู่มือการใช้งาน

**SimpleHAL** คือ Hardware Abstraction Layer สำหรับไมโครคอนโทรลเลอร์ WCH CH572/CH570
ที่ออกแบบให้ใช้งานง่าย ลดความซับซ้อนของไลบรารีมาตรฐาน (StdPeriphDriver)
โดยใช้รูปแบบ Handle-based API พร้อม callback และ ring buffer

---

## สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [การติดตั้ง](#การติดตั้ง)
3. [โครงสร้างโปรเจกต์](#โครงสร้างโปรเจกต์)
4. [เริ่มต้นใช้งาน](#เริ่มต้นใช้งาน)
5. [API Reference](#api-reference)
6. [การใช้งาน Interrupt และ Callback](#การใช้งาน-interrupt-และ-callback)
7. [การใช้งาน Async และ DMA](#การใช้งาน-async-และ-dma)
8. [การประหยัดพลังงาน](#การประหยัดพลังงาน)
9. [การแก้ไขปัญหา](#การแก้ไขปัญหา)

---

## ภาพรวม

SimpleHAL ประกอบด้วย 15 โมดูลหลัก + 2 core modules:

| โมดูล | ไฟล์ | คำอธิบาย |
|---|---|---|
| GPIO | `hal_gpio` | จัดการขา I/O ดิจิตอล พร้อม interrupt callback |
| UART | `hal_uart` | สื่อสารแบบอนุกรม พร้อม ring buffer และ async |
| SPI | `hal_spi` | สื่อสาร SPI ทั้งแบบ polling และ DMA |
| I2C | `hal_i2c` | สื่อสาร I2C แบบ master |
| Timer | `hal_timer` | ตั้งเวลาจับเวลา แบบ periodic หรือ one-shot |
| PWM | `hal_pwm` | สร้างสัญญาณ PWM 5 ช่อง 8 หรือ 16 บิต |
| ADC | `hal_adc` | อ่านค่าแอนะล็อกผ่าน CMP (4/8/9 บิต) |
| Flash | `hal_flash` | อ่านข้อมูลจาก Flash ROM |
| RTC | `hal_rtc` | จับเวลาจริง ตั้งปลุก ตั้ง timer |
| Power | `hal_pwr` | จัดการโหมดประหยัดพลังงาน |
| Clock | `hal_clk` | ตั้งค่าความถี่นาฬิกา |
| System | `hal_sys` | Delay, รีเซ็ต, Watchdog |
| KeyScan | `hal_keyscan` | สแกนเมทริกซ์คีย์บอร์ด |
| RF | `hal_rf` | สื่อสาร 2.4GHz ไร้สาย |
| BLE | `hal_ble` | บลูทูธพลังงานต่ำ (BLE peripheral) |

---

## การติดตั้ง

### ขั้นตอนที่ 1: เพิ่ม Include Path

เปิดโปรเจกต์ใน MounRiver Studio แล้วเพิ่ม path ใน **Project > Properties > C/C++ Build > Settings > Include Paths**:

```
../SimpleHAL
../SimpleHAL/core
```

### ขั้นตอนที่ 2: เพิ่ม Source Files

เพิ่มไฟล์ .c จาก `SimpleHAL/` และ `SimpleHAL/core/` ทั้งหมดเข้าในโปรเจกต์

### ขั้นตอนที่ 3: Include ในโค้ด

```c
#include "simple_hal.h"
```

### การเปิดใช้งาน BLE และ RF

โมดูล BLE และ RF ถูกปิดเป็นค่าเริ่มต้น (`HAL_ENABLE_BLE=0`, `HAL_ENABLE_RF=0`)
ดูวิธีเปิดได้ที่ [LEARN_SIMPLEHAL.md](../../../LEARN_SIMPLEHAL.md#1-เริ่มต้นโปรเจกต์แรก)

---

## โครงสร้างโปรเจกต์

```
src/SimpleHAL/
├── simple_hal.h              # รวม header ทั้งหมด
├── simple_hal_config.h       # ตั้งค่าต่างๆ (ขนาด ring buffer, BLE/RF)
├── core/
│   ├── hal_types.h           # ชนิดข้อมูลพื้นฐาน
│   ├── hal_utils.h           # Macro ช่วยเหลือ
│   ├── hal_ringbuf.h/.c      # Ring buffer (ใช้ใน UART)
│   └── hal_softimer.h/.c     # Soft timer (non-blocking delay)
├── hal_gpio.h/.c             # GPIO
├── hal_uart.h/.c             # UART
├── hal_spi.h/.c              # SPI
├── hal_i2c.h/.c              # I2C
├── hal_timer.h/.c            # Timer
├── hal_pwm.h/.c              # PWM
├── hal_adc.h/.c              # ADC
├── hal_flash.h/.c            # Flash
├── hal_rtc.h/.c              # RTC
├── hal_pwr.h/.c              # Power
├── hal_clk.h/.c              # Clock
├── hal_sys.h/.c              # System (delay, reset, WDT)
├── hal_keyscan.h/.c          # KeyScan
├── hal_rf.h/.c               # RF 2.4GHz
└── hal_ble.h/.c              # BLE
```

---

## เริ่มต้นใช้งาน

### ขั้นตอนพื้นฐานสำหรับทุกโปรเจกต์

```c
#include "simple_hal.h"

int main() {
    // 1. ตั้งค่า HSE oscillator capacitance
    HSECFG_Capacitance(HSECap_18p);

    // 2. ตั้งความถี่นาฬิกา (100MHz, 60MHz, ฯลฯ)
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // 3. เริ่มต้น peripheral ที่ต้องการ
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    // 4. เขียนโปรแกรมหลัก
    while (1) {
        hal_gpio_toggle(led);
        hal_uart_send(uart, "สวัสดีครับ!\r\n", 14);
        hal_delay_ms(1000);
    }
}
```

---

## API Reference

ดูรายละเอียดทุกฟังก์ชันแบบละเอียด (พร้อมพารามิเตอร์, ค่าที่คืน, ตัวอย่าง) ได้ที่:

- [api_reference.md](api_reference.md) — สารบัญลิงก์ไปยังแต่ละโมดูล
- [config_reference.md](config_reference.md) — การตั้งค่า HAL และ Ring Buffer API
- [compat_shim.md](compat_shim.md) — Compatibility shim (SimpleHAL.h + Simple1Wire.h) สำหรับพอร์ตจาก CH32V003
- แต่ละโมดูลใน [modules/](modules/)

---

## การใช้งาน Interrupt และ Callback

ดูรายละเอียดเพิ่มเติมที่ [interrupt_handling.md](interrupt_handling.md)

---

## การใช้งาน Async และ DMA

SimpleHAL รองรับการทำงานแบบไม่บล็อก (non-blocking) สำหรับ:

### UART Async

```c
// ส่งแบบไม่รอ
hal_uart_send_async(uart, data, len, เมื่อส่งเสร็จ, NULL);

// รับแบบไม่รอ (callback เมื่อได้ข้อมูลครบตามจำนวน)
uint8_t buf[20];
hal_uart_recv_async(uart, buf, 20, เมื่อรับครบ, NULL);

// แนบ callback เมื่อมีข้อมูลขาเข้า
hal_uart_attach_rx_cb(uart, เมื่อมีข้อมูลเข้า, NULL);
```

### SPI DMA

```c
// ส่ง/รับผ่าน DMA
hal_spi_transfer_dma(spi, tx_buf, rx_buf, 256, เมื่อเสร็จ, NULL);
```

---

## การประหยัดพลังงาน

CH572 มีโหมดประหยัดพลังงาน 4 ระดับ:

```c
// Idle — หยุด CPU แต่ peripheral ทำงานได้
hal_pwr_idle();

// Halt — หยุด CPU + peripheral, ปลุกด้วย interrupt
hal_pwr_halt();

// Sleep — ประหยัดไฟสูง, ปลุกด้วย RTC/GPIO/USB
hal_pwr_wakeup_cfg(RB_SLP_RTC_WAKE, HAL_PWR_WAKE_DELAY_64);
hal_pwr_sleep(RB_PWR_RAM2K);

// Shutdown — ประหยัดไฟสูงสุด
hal_pwr_shutdown(RB_PWR_RAM2K);
```

---

## การแก้ไขปัญหา

### 1. คอมไพล์ไม่ผ่าน — ไม่พบไฟล์ header

```
fatal error: CH57x_common.h: No such file or directory
```

**แก้ไข:** เพิ่ม Include Path ไปยัง `EVT/EXAM/SRC/StdPeriphDriver/inc` และ `EVT/EXAM/SRC/RVMSIS`

### 2. Linker error — undefined reference to `XXX_IRQHandler`

**แก้ไข:** ตรวจสอบว่าได้เพิ่มไฟล์ SimpleHAL ทั้งหมดในโปรเจกต์แล้ว
ไฟล์บางไฟล์มี interrupt handler ในตัว

### 3. UART ไม่มีข้อมูลขาเข้า

**สาเหตุ:** UART interrupt ต้องเปิดใช้งาน `PFIC_EnableIRQ(UART_IRQn)` ซึ่งทำไว้ใน `hal_uart_init` แล้ว
ตรวจสอบว่าต่อสาย TX/RX ถูกต้อง (CH572: TX=PA3, RX=PA2)

### 4. RF ไม่ทำงาน

**สาเหตุ:** RF ต้อง calibrate ก่อนใช้ `hal_rf_calibrate()` และต้องเปิด interrupt `PFIC_EnableIRQ(BLEB_IRQn)` และ `PFIC_EnableIRQ(BLEL_IRQn)` ซึ่งทำไว้ใน `hal_rf_init` แล้ว
