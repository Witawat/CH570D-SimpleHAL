# Pinout CH57x

## QFN28 Pinout

| Pin | ชื่อ | GPIO | อินพุต 5V? | ฟังก์ชันหลัก | ฟังก์ชันสำรอง (remap) | SimpleHAL ใช้ |
|---|---|---|---|---|---|---|
| 1 | PA15 | PA15 | | GPIO | PWM4, RF_ANT | PWM1_CH4, RF (CH571/2) |
| 2 | PA14 | PA14 | | GPIO | PWM3, SDA1 (remap) | PWM1_CH3 |
| 3 | PA13 | PA13 | | GPIO | PWM2, SCL1 (remap) | PWM1_CH2 |
| 4 | PA12 | PA12 | | GPIO | PWM1, USB_DP | PWM1_CH1, USB_DP |
| 5 | PA11 | PA11 | **ใช่** | GPIO, USB_DM | TXD (remap), LED on EVB | USB_DM, UART TX alt |
| 6 | PA10 | PA10 | **ใช่** | GPIO, I2C_SCL | TXD (remap), KeyScan | I2C_SCL, KeyScan |
| 7 | PA9 | PA9 | **ใช่** | GPIO, I2C_SDA | RXD (remap), KeyScan | I2C_SDA, KeyScan |
| 8 | PA8 | PA8 | **ใช่** | GPIO, KeyScan | TXD/RXD (remap) | KeyScan, UART alt |
| 9 | PA7 | PA7 | | GPIO | ADC_4BIT, CMP(+) | ADC_CH8, CMP input |
| 10 | PA6 | PA6 | | GPIO | SPI_MOSI | SPI_MOSI |
| 11 | PA5 | PA5 | | GPIO | SPI_SCK | SPI_SCK |
| 12 | PA4 | PA4 | | GPIO | SPI_MISO, PWM5 | SPI_MISO, PWM2_CH1 |
| 13 | PA3 | PA3 | **ใช่** | GPIO, UART_TXD | CMP(+), RXD (remap) | UART_TX, CMP input |
| 14 | PA2 | PA2 | **ใช่** | GPIO, UART_RXD | ADC_8/9BIT, CMP(-) | UART_RX, ADC, CMP input |
| 15 | PA1 | PA1 | | GPIO | TXD (remap) | UART TX alt |
| 16 | PA0 | PA0 | | GPIO | RXD (remap) | UART RX alt |
| 17 | VDD | — | — | Power (3.3V) | — | — |
| 18 | VDD | — | — | Power (3.3V) | — | — |
| 19 | VDD | — | — | Power (3.3V) | — | — |
| 20 | VDDIO | — | — | I/O Power (3.3V) | — | — |
| 21 | VDD_18 | — | — | Internal 1.8V (decouple) | — | — |
| 22 | VINTA | — | — | Internal analog (decouple) | — | — |
| 23 | VINTRF | — | — | Internal RF (decouple) | — | — |
| 24 | XI / XTAL_OUT | — | — | HSE crystal out | — | Clock |
| 25 | XO / XTAL_IN | — | — | HSE crystal in | — | Clock |
| 26 | GND | — | — | Ground | — | — |
| 27 | GND | — | — | Ground | — | — |
| 28 | USB_DP | — | — | USB D+ (alternate with PA12) | — | USB |

## UART Remap Options

| Mode | TX | RX |
|---|---|---|
| Default | PA3 | PA2 |
| MODE1 | PA1 | PA0 |
| MODE2 | PA4 | PA9 |
| MODE3 | PA7 | PA8 |
| MODE4 | PA8 | PA10 |
| MODE5 | PA10 | PA11 |
| MODE6 | PA11 | PA4 |

```c
// ตัวอย่าง: remap UART TX→PA4, RX→PA9
GPIOPinRemap(REMAP_TXD_PA4);
GPIOPinRemap(REMAP_RXD_PA9);
```

## Timer Remap Options

| Mode | TMR_PWM | TMR_CAP |
|---|---|---|
| DEFAULT | PA12–PA15 | PA12–PA15 |
| MODE1 | PA4–PA7 | PA4–PA7 |
| MODE2 | PA0–PA3 | PA0–PA3 |
| MODE3 | PA8–PA11 | PA8–PA11 |

## I2C Remap Options

| Mode | SDA | SCL |
|---|---|---|
| DEFAULT | PA9 | PA10 |
| MODE1 | PA12 | PA13 |

## 5V Tolerant Pins

| ขา |  toleration |
|---|---|
| PA2, PA3, PA8, PA9, PA10, PA11 | **5V tolerant** — ต่อกับวงจร 5V logic โดยตรงโดยไม่ต้อง level shifter |

หมายเหตุ: PA0, PA1, PA4–PA7, PA12–PA15 **ไม่** 5V tolerant — input ต้องไม่เกิน VDD (3.3V)
