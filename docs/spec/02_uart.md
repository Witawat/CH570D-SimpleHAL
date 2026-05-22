# UART

## Overview

UART asynchronous serial หนึ่งชุด (full-duplex) พร้อม ring buffer สำหรับรับ/ส่งข้อมูลแบบไม่บล็อก รองรับการเปลี่ยนขา TX/RX ผ่าน pin remap ได้หลายแบบ

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_uart_init(cfg)` | เริ่มต้น UART ตาม config struct |
| `hal_uart_send_byte(data)` | ส่ง 1 ไบต์ (blocking) |
| `hal_uart_recv_byte()` | รับ 1 ไบต์ (blocking, -1 ถ้า timeout) |
| `hal_uart_send(buf, len)` | ส่งหลายไบต์ (blocking) |
| `hal_uart_recv(buf, len)` | รับหลายไบต์ (blocking, คืนจำนวนที่รับได้) |
| `hal_uart_send_async(buf, len, cb)` | ส่งแบบไม่บล็อก (callback เมื่อส่งเสร็จ) |
| `hal_uart_recv_async(buf, len, cb)` | รับแบบไม่บล็อก (callback เมื่อได้ครบ len) |
| `hal_uart_attach_rx_cb(cb)` | callback ทุกครั้งที่รับไบต์ |
| `hal_uart_attach_tx_cb(cb)` | callback ทุกครั้งที่ส่งไบต์ |
| `hal_uart_set_trig_bytes(n)` | กำหนด trigger byte (1/2/4/7) |

**Config struct (`hal_uart_config_t`):** baud_rate (`uint32_t`), tx_pin, rx_pin (`uint8_t`)

## Circuit Guide

| สาย | หน้าที่ |
|---|---|
| TX → RX ของอีกฝ่าย | ส่งข้อมูล |
| RX ← TX ของอีกฝ่าย | รับข้อมูล |
| GND ↔ GND | กราวด์ร่วม (จำเป็นเสมอ) |

```
PC (USB-UART)          CH57x
  TX ───────────────── RX (PA2 default)
  RX ───────────────── TX (PA3 default)
  GND ──────────────── GND
```

**ระดับแรงดัน:** CH57x ทำงานที่ 3.3V logic — ถ้าต่อกับ Arduino 5V หรือ USB-UART 5V ต้องใช้ level shifter หรือ voltage divider ที่ขา RX ของ CH57x (ยกเว้น PA2 ซึ่ง 5V tolerant)

```
// Voltage divider สำหรับ RX (ถ้าแหล่งส่ง 5V logic)
แหล่ง 5V TX ──── R1(1kΩ) ──┬── CH57x RX (PA2)
                           │
                         R2(2kΩ)
                           │
                          GND
// CH57x RX ได้ ~3.3V จาก 5V
// ใช้ R1=1k, R2=2k หรือ R1=1k, R2=1k8
```

## CH57x Specifics

- **Baud rate:** 2400–2000000 (实测使用 9600/19200/38400/57600/115200/230400/460800/921600/1M/2M)
- **Pin remap options** (TX, RX):
  - Default: PA3 (TX), PA2 (RX)
  - PA1 (TX), PA0 (RX)
  - PA4 (TX), PA9 (RX)
  - PA7 (TX), PA8 (RX)
  - PA8 (TX), PA10 (RX)
  - PA10 (TX), PA11 (RX)
  - PA11 (TX), PA4 (RX)
- **RX pin 5V tolerant:** ทุกตัวเลือก RX (PA0, PA2, PA4, PA8, PA9, PA10, PA11) เป็น 5V tolerant
- **Ring buffer size:** 128 ไบต์ TX และ 128 ไบต์ RX (ปรับได้ใน `simple_hal_config.h`)
- **HW flow control:** ไม่มี (RTS/CTS)

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| UART instance เดียว | มี UART ฮาร์ดแวร์ตัวเดียว — ไม่สามารถเปิดสองชุดพร้อมกัน |
| TX/RX swap ไม่มี | ใช้ remap เท่านั้น — ถ้าต้องการกลับทิศต้อง remap |
| Trigger สูงสุด 7 ไบต์ | `set_trig_bytes(7)` — เกิน 7 ไม่ได้ |
| ไม่มี 9-bit mode | รองรับเฉพาะ 8-bit data + 1 start + 1 stop |
| Baud rate error | ที่ baud สูง (>921600) อาจมี error ~1–2% ถ้า sysclock ไม่หารลงตัว |

## Code สั้น

```c
hal_uart_config_t uart_cfg = {
    .baud_rate = 115200,
    .tx_pin = PA3,
    .rx_pin = PA2
};
hal_uart_init(&uart_cfg);

hal_uart_send_byte('A');              // ส่ง blocking
uint8_t c = hal_uart_recv_byte();     // รับ blocking

hal_uart_attach_rx_cb([](uint8_t d) { // callback ทุกครั้งที่มีข้อมูล
    echo_buf[i++] = d;
});
```
