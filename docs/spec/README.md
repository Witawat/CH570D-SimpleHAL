# สเปก CH57x (ผ่าน SimpleHAL)

เอกสารนี้อธิบายความสามารถของชิพตระกูล **CH570/CH571/CH572** จาก WCH ในมุมมองของ **SimpleHAL abstraction layer** — เน้นการใช้งานจริง วงจรที่แนะนำ ข้อจำกัดที่พบ และความแตกต่างระหว่างรุ่น

---

## ตระกูล CH57x

| คุณสมบัติ | CH570 | CH571 | CH572 |
|---|---|---|---|
| **Flash** | 240KB | 192KB | 192KB |
| **RAM** | 12KB | 24KB | 24KB |
| **BLE 4.x** | — | — | **มี** (peripheral) |
| **RF 2.4GHz** | — | **มี** (proprietary) | **มี** (proprietary) |
| **USB Host** | — | **มี** | **มี** |
| **USB Device** | **มี** | **มี** | **มี** |
| **GPIO** | 16 pins | 16 pins | 16 pins |
| **UART** | 1 | 1 | 1 |
| **SPI** | 1 (master) | 1 (master) | 1 (master) |
| **I2C** | 1 (master/slave) | 1 (master/slave) | 1 (master/slave) |
| **Timer** | 1 (16-bit) | 1 (16-bit) | 1 (16-bit) |
| **PWM** | 5 ch (8/16-bit) | 5 ch (8/16-bit) | 5 ch (8/16-bit) |
| **ADC** | 4/8/9-bit | 4/8/9-bit | 4/8/9-bit |
| **Comparator** | 1 | 1 | 1 |
| **KeyScan** | 5×? matrix | 5×? matrix | 5×? matrix |
| **RTC** | มี | มี | มี |
| **Watchdog** | WWDG + IWDG | WWDG + IWDG | WWDG + IWDG |
| **Voltage** | 2.3V–3.6V | 2.3V–3.6V | 2.3V–3.6V |
| **Temp** | -40～+85°C | -40～+85°C | -40～+85°C |
| **Package** | QFN28 / TSSOP20 | QFN28 / TSSOP20 | QFN28 / TSSOP20 |

**SimpleHAL รองรับ:** ทุกบรรทัดในตารางยกเว้น BLE/RF/USB Host จะต้องเปิดด้วย `HAL_ENABLE_BLE=1`, `HAL_ENABLE_RF=1`, `HAL_ENABLE_USBHOST=1` ใน `simple_hal_config.h`

---

## Clock Tree

```
                    ┌─────────────┐
LSI (24MHz RC) ────▶│             │
                    │  PLL Src    │──▶ PLL ──▶ SYS_CLK (20–100MHz)
HSE (XTAL) ─────────▶│  Mux        │        │
  (16/8/6.4/4/2/1   └─────────────┘        │
   MHz)                                     │
                                           ▼
                                   ┌──────────────┐
                                   │  APB Bridge   │
                                   │  (peripherals) │
                                   └──────────────┘
                                              │
                         ┌────────────────────┼────────────────────┐
                         ▼                    ▼                    ▼
                   ┌──────────┐        ┌──────────┐        ┌──────────┐
                   │  Timer   │        │   SPI    │        │   I2C    │
                   │  PWM     │        │   UART   │        │   GPIO   │
                   │  ADC     │        │   ...    │        │   ...    │
                   └──────────┘        └──────────┘        └──────────┘
```

**SysClock options:** `HAL_CLK_PLL_100/75/60/50/40/30/25/24/20MHz`, `HAL_CLK_HSE_16/8/6_4/4/2/1MHz`, `HAL_CLK_LSI`

**Note:** 100MHz ต้องใช้ HSE 16MHz + PLL; 50MHz ใช้ HSE 8MHz + PLL หรือ HSE 16MHz + PLL divide-by-2

---

## Bus / Interconnect

```
         ┌──────────────── System Bus ────────────────┐
         │                                            │
    ┌────┴────┐    ┌──────────┐    ┌──────────┐       │
    │ RISC-V  │    │  Flash   │    │   RAM    │       │
    │ V4A     │    │  240KB   │    │  12KB    │       │
    │ 100MHz  │    │          │    │          │       │
    └────┬────┘    └──────────┘    └──────────┘       │
         │                                            │
    ┌────┴────────────────────────────────────────────┘
    │
    ├── PFIC (Interrupt Controller)
    ├── GPIO (16 pins, IRQ on each)
    ├── UART (with pin remap, 4 trigger bytes)
    ├── SPI (master, with DMA)
    ├── I2C (master/slave, with DMA)
    ├── Timer (PWM, capture, encoder — with DMA)
    ├── PWM5 (PWM1-5, 8/16-bit, with DMA)
    ├── ADC (4/8/9-bit, single channel)
    ├── Comparator (to timer capture)
    ├── RTC (calendar + alarm + timer)
    ├── KeyScan (matrix scanner)
    ├── Watchdog (WWDG + IWDG)
    ├── USB (Device always; Host on CH571/CH572)
    ├── RF 2.4GHz (CH571/CH572)
    └── BLE (CH572 only)
```

**DMA:** Peripheral ที่รองรับ DMA — SPI TX/RX, Timer PWM/capture, PWM duty cycle update
**PFIC:** Programmable Fast Interrupt Controller — nested vectored interrupt, 4 priority levels

---

## สารบัญ

| # | โมดูล | SimpleHAL รองรับ | หน้าที่หลัก |
|---|---|---|---|
| 01 | [GPIO](01_gpio.md) | Full | digital I/O, interrupt, pin remap |
| 02 | [UART](02_uart.md) | Full | serial TX/RX, async, ringbuf, pin remap |
| 03 | [SPI](03_spi.md) | Full | master, DMA, CS control, full/half duplex |
| 04 | [I2C](04_i2c.md) | Full | master/slave, 100/400kHz, mem addressing |
| 05 | [Timer](05_timer.md) | Full | periodic/one-shot, capture, encoder |
| 06 | [PWM](06_pwm.md) | Full | 5ch, 8/16-bit, variable freq, start/stop |
| 07 | [ADC](07_adc.md) | Full | 4/8/9-bit, single-ended, auto channel switch |
| 08 | [Comparator](08_cmp.md) | Full | input mux, VREF, output to timer capture |
| 09 | [Flash](09_flash.md) | Read only | unique ID, option bytes |
| 10 | [RTC](10_rtc.md) | Full | calendar, alarm, periodic timer |
| 11 | [Power](11_pwr.md) | Full | idle/halt/sleep/shutdown, wake sources |
| 12 | [Clock](12_clk.md) | Full | PLL/HSE/LSI, sysclock selection |
| 13 | [KeyScan](13_keyscan.md) | Full | key matrix scan, wake |
| 14 | [Watchdog](14_wdt.md) | Full | WWDG + IWDG, feed, IRQ/reset |
| 15 | [RF & BLE](15_rf_ble.md) | Optional | 2.4GHz proprietary + BLE 4.x peripheral |
| 16 | [USB](16_usb.md) | Optional | Host (CH571/2) + Device (all) |
| A1 | [Pinout](appendix_pinout.md) | — | pin functions, alt, 5V tolerance |
| A2 | [Memory Map](appendix_memory.md) | — | SFR layout, Flash/RAM regions |
