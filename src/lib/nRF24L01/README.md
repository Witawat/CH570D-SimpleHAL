# nRF24L01+ Wireless Transceiver Library

> **Library สำหรับใช้งาน nRF24L01+ 2.4GHz Wireless บน CH57x**

## ภาพรวม

nRF24L01+ เป็น 2.4GHz transceiver IC จาก Nordic Semiconductor  
รองรับการส่ง/รับข้อมูลไร้สายระยะ 100m+ (outdoor, 0dBm)

| คุณสมบัติ | ค่า |
|----------|-----|
| ความถี่ | 2.400 - 2.525 GHz (126 channels) |
| Data Rate | 250kbps / 1Mbps / 2Mbps |
| TX Power | -18, -12, -6, 0 dBm |
| Payload | 1-32 bytes |
| VCC | **3.3V เท่านั้น** (ห้ามต่อ 5V!) |
| กระแสไฟ TX | ~11.3mA (0dBm) |

---

## Hardware Setup

```
   CH57x           nRF24L01+
   ─────────          ──────────
   PC5 (SCK) ──────>  SCK
   PC6 (MOSI)──────>  MOSI
   PC7 (MISO)<──────  MISO
   PC4 (CSN) ──────>  CSN
   PC3 (CE)  ──────>  CE
   3.3V ───────────>  VCC  ⚠️ 3.3V เท่านั้น!
   GND ────────────>  GND

   ตัวเก็บประจุ: 100µF + 100nF (Electrolytic + Ceramic)
   ระหว่าง VCC และ GND ใกล้ module เพื่อลด noise
```

> ⚠️ **คำเตือนสำคัญ**:
> - **ห้ามต่อ VCC เกิน 3.3V** จะทำให้ module เสียหายถาวร
> - ควรต่อ capacitor กรอง power supply เสมอ (module กิน current สูงช่วง TX)
> - ต้องใช้ **SPI_MODE0** (CPOL=0, CPHA=0)

---

## การใช้งาน

### ส่งข้อมูล (Transmitter)

```c
#include "SimpleHAL.h"
#include "nRF24L01.h"

nRF24_Instance radio;
const uint8_t pipe_addr[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    SPI_SimpleInit(SPI_MODE0, SPI_1MHZ, SPI_PINS_DEFAULT);

    // CSN = PC4, CE = PC3
    nRF24_Init(&radio, PIN_PC4, PIN_PC3);
    nRF24_SetChannel(&radio, 76);
    nRF24_SetDataRate(&radio, NRF24_DR_1MBPS);
    nRF24_SetTxAddr(&radio, pipe_addr);
    nRF24_SetPayloadSize(&radio, 8);

    while (1) {
        uint8_t msg[8] = {0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0};

        nRF24_Status result = nRF24_Transmit(&radio, msg, 8);
        if (result == NRF24_OK) {
            printf("TX OK\r\n");
        } else if (result == NRF24_ERROR_TX_FAIL) {
            printf("TX FAIL (no ACK)\r\n");
        }
        Delay_Ms(1000);
    }
}
```

### รับข้อมูล (Receiver)

```c
#include "SimpleHAL.h"
#include "nRF24L01.h"

nRF24_Instance radio;
const uint8_t pipe_addr[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    SPI_SimpleInit(SPI_MODE0, SPI_1MHZ, SPI_PINS_DEFAULT);

    nRF24_Init(&radio, PIN_PC4, PIN_PC3);
    nRF24_SetChannel(&radio, 76);
    nRF24_SetDataRate(&radio, NRF24_DR_1MBPS);
    nRF24_SetRxAddr(&radio, pipe_addr);
    nRF24_SetPayloadSize(&radio, 8);

    nRF24_StartListening(&radio);  // เข้า RX mode

    while (1) {
        if (nRF24_Available(&radio)) {
            uint8_t buf[32] = {0};
            nRF24_Receive(&radio, buf);
            printf("Received: %02X %02X %02X %02X\r\n",
                   buf[0], buf[1], buf[2], buf[3]);
        }
        Delay_Ms(10);
    }
}
```

---

## RF Channel

ความถี่ = **2400 + channel** MHz (channel = 0-125)

| Channel | ความถี่ | หมายเหตุ |
|---------|---------|---------|
| 0 | 2400 MHz | ชนกับ Bluetooth |
| 25 | 2425 MHz | แนะนำ |
| 76 | 2476 MHz | **default** (ห่างจาก WiFi) |
| 100 | 2500 MHz | แนะนำ |

> ❗ หลีกเลี่ยง channel 0-13 (ชนกับ WiFi 2.4GHz) และ channel 64-70 (Bluetooth)

---

## Data Rate vs ระยะทาง

| Data Rate | ระยะทาง (approx.) | ใช้เมื่อ |
|-----------|-----------------|---------|
| 250kbps | ไกลสุด | ต้องการระยะไกล |
| 1Mbps | ปานกลาง | ทั่วไป (default) |
| 2Mbps | สั้นสุด | ต้องการความเร็ว |

---

## Troubleshooting

| ปัญหา | สาเหตุ | วิธีแก้ |
|-------|--------|---------|
| TX FAIL ทุกครั้ง | channel, address ไม่ตรง | ตรวจสอบ RX/TX ตั้งค่าเหมือนกัน |
| Module ไม่ตอบสนอง | power supply noise | เพิ่ม capacitor 100µF |
| รับได้บ้างไม่ได้บ้าง | คลื่นรบกวน | เปลี่ยน channel ห่างจาก WiFi |
| module เสีย | ต่อ 5V | ตรวจ VCC = 3.3V เท่านั้น |

---

## API Reference

| Function | คำอธิบาย |
|----------|---------|
| `nRF24_Init(radio, csn, ce)` | Init (ต้อง SPI_SimpleInit ก่อน) |
| `nRF24_SetChannel(radio, 0-125)` | ตั้ง RF channel |
| `nRF24_SetDataRate(radio, dr)` | 250kbps / 1Mbps / 2Mbps |
| `nRF24_SetTxPower(radio, pwr)` | -18 ถึง 0 dBm |
| `nRF24_SetPayloadSize(radio, size)` | ขนาด payload (1-32) |
| `nRF24_SetTxAddr(radio, addr[5])` | ตั้ง TX address |
| `nRF24_SetRxAddr(radio, addr[5])` | ตั้ง RX address pipe 0 |
| `nRF24_Transmit(radio, data, len)` | ส่งข้อมูล (blocking) |
| `nRF24_StartListening(radio)` | เข้า RX mode (CE=HIGH) |
| `nRF24_StopListening(radio)` | ออกจาก RX mode |
| `nRF24_Available(radio)` | ตรวจสอบข้อมูลรอรับ |
| `nRF24_Receive(radio, buf)` | รับข้อมูลจาก FIFO |
| `nRF24_GetStatus(radio)` | อ่าน STATUS register |
| `nRF24_PowerDown(radio)` | Power down mode |
| `nRF24_PowerUp(radio)` | Wake up |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)