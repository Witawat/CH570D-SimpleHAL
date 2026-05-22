# SPI

## Overview

SPI interface แบบ full-duplex master พร้อม DMA และ chip select ที่ควบคุมด้วย software รองรับทั้งโหมด 4-wire (full-duplex) และ 2-wire (half-duplex)

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_spi_init(cfg)` | เริ่มต้น SPI ตาม config struct |
| `hal_spi_transfer(tx, rx, len)` | ส่งและรับพร้อมกัน (full-duplex) |
| `hal_spi_send_byte(data)` | ส่ง 1 ไบต์ (ไม่สนของที่รับ) |
| `hal_spi_recv_byte()` | รับ 1 ไบต์ (ส่ง 0xFF ไปก่อน) |
| `hal_spi_set_speed(hz)` | เปลี่ยนความเร็ว SPI |
| `hal_spi_chip_select(pin, level)` | CS software control |
| `hal_spi_transfer_dma(tx, rx, len, cb)` | half-duplex DMA transfer |

**Config struct (`hal_spi_config_t`):** mode (`0` / `3`), data_mode (`MSB_FIRST` / `LSB_FIRST`), speed (`uint32_t` Hz), duplex (`FULL` / `HALF`)

**SPI Modes:**

| Mode | CPOL | CPHA | idle clock |
|---|---|---|---|
| MODE0 | 0 | 0 | low |
| MODE3 | 1 | 1 | high |

**Note:** MODE1 และ MODE2 ไม่มี — ถ้าส่งจะ fallback เป็น MODE0 หรือ MODE3 (CPHA ผิด) — ควรตรวจสอบ datasheet อุปกรณ์ก่อน

## Circuit Guide

```
SPI Master (CH57x)          SPI Slave (อุปกรณ์)
  SCK (PA5) ────────────── SCK
  MOSI (PA6) ───────────── MOSI (input)
  MISO (PA4) ───────────── MISO (output)
  CS (GPIO ใดก็ได้) ────── CS/SS
  GND ──────────────────── GND
```

| สาย | ขา CH57x | รายละเอียด |
|---|---|---|
| SCK | PA5 (fix) | Clock — กำหนดความเร็วผ่าน `hal_spi_set_speed()` |
| MOSI | PA6 (fix) | Master Out Slave In — ข้อมูลจาก CH57x ไปอุปกรณ์ |
| MISO | PA4 (fix) | Master In Slave Out — ข้อมูลจากอุปกรณ์เข้า CH57x |
| CS | GPIO ใดก็ได้ | Chip Select — ใช้ `hal_spi_chip_select()` หรือ pinMode + digitalWrite |

**Pull-up resistors:** SCK ควรมี pull-up 10kΩ ต่อ GND (ป้องกัน clock glitch ตอน CS inactive); MISO/MOSI ตาม spec อุปกรณ์

## CH57x Specifics

- **ความเร็วสูงสุด:** sysclock / 2 (เช่น 100MHz → SPI max 50MHz)
- **Modulo 2 divider:** ความเร็ว = sysclock / (2 × divider) โดย divider = 2, 4, 8, ... 2048
- **DMA:** รองรับเฉพาะ half-duplex — ใช้ `hal_spi_transfer_dma()` สำหรับรับ/ส่งทีละทิศทาง
- **CS:** ต้องใช้ software GPIO control — ไม่มี hardware CS อัตโนมัติ
- **Interrupt:** พร้อมใช้งาน (ไม่จำเป็นต้องใช้ polling)

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| Master mode เท่านั้น | ไม่มี Slave mode |
| MODE1/MODE2 fallback | MODE1 → MODE0, MODE2 → MODE3 (CPHA inverted) — อาจใช้กับอุปกรณ์ที่ตรวจสอบเฉพาะ edge ไม่ได้ |
| DMA half-duplex เท่านั้น | ไม่สามารถ full-duplex DMA |
| ขายึดตายตัว | SCK=PA5, MOSI=PA6, MISO=PA4 — ไม่สามารถ remap |
| ไม่มี hardware CS | ต้องใช้ software `hal_spi_chip_select()` ก่อน/หลังทุก transaction |
| FIFO จำกัด | DMA transfer สูงสุด 4095 ไบต์ต่อครั้ง |

## Code สั้น

```c
hal_spi_config_t spi_cfg = {
    .mode = 0,
    .data_mode = MSB_FIRST,
    .speed = 8000000,
    .duplex = FULL
};
hal_spi_init(&spi_cfg);

uint8_t cs = PA10;
pinMode(cs, OUTPUT);
digitalWrite(cs, HIGH);

digitalWrite(cs, LOW);                        // select
uint8_t resp = hal_spi_transfer(NULL, NULL, 1); // send cmd
hal_spi_transfer(&tx_data, &rx_data, 3);       // exchange
digitalWrite(cs, HIGH);                       // deselect
```
