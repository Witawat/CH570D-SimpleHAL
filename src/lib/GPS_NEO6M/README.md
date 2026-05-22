# GPS_NEO6M — GPS Module Library

> **สำหรับ CH57x series**

---

## 📦 Module: NEO-6M GPS Module

NEO-6M เป็น GPS module ราคาถูกจาก u-blox สื่อสารผ่าน UART (9600 baud default)

| Property | Value |
|----------|-------|
| Protocol | UART (NMEA 0183) |
| VCC | 3.3V |
| Baud Rate | 9600 (default) |
| Update Rate | 1 Hz (1 fix/sec) |
| Sentences | $GPGGA, $GPRMC |
| RAM Usage | ~256 bytes (buffer + struct) |

---

## 🔌 การต่อสาย

```
NEO-6M             CH57x
VCC  ----------->  3.3V
GND  ----------->  GND
TX   ----------->  USART1 RX pin
RX   ----------->  (optional) USART1 TX pin
```

> **⚠️ ข้อจำกัด:** CH57x มี USART1 เพียง 1 ช่อง — เมื่อใช้ GPS จะใช้ `USART_Print` / debug ไม่ได้พร้อมกัน

---

## 🚀 Quick Start

```c
#include "Lib/GPS_NEO6M/GPS.h"

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();

    GPS_Instance gps;
    GPS_Init(&gps, BAUD_9600, USART_PINS_DEFAULT);

    while (1) {
        GPS_Update(&gps);

        if (gps.data_ready) {
            gps.data_ready = 0;

            if (GPS_IsFixValid(&gps)) {
                float lat = GPS_GetLatitude(&gps);
                float lon = GPS_GetLongitude(&gps);
                float alt = GPS_GetAltitude(&gps);
                float spd = GPS_GetSpeed(&gps);
                uint8_t sats = GPS_GetSatellites(&gps);

                /* แสดงผลผ่านช่องทางอื่น (เช่น OLED, LCD I2C) */
                OLED_Printf("Lat: %.4f", lat);
                OLED_Printf("Lon: %.4f", lon);
                OLED_Printf("Alt: %.1fm", alt);
                OLED_Printf("Sats: %d", sats);
            }
        }

        Delay_Ms(100);
    }
}
```

---

## 📖 API Reference

| Function | Description |
|----------|-------------|
| `GPS_Init(gps, baud, pin_config)` | เริ่มต้น GPS module |
| `GPS_Update(gps)` | อ่านและ parse NMEA จาก UART |
| `GPS_IsFixValid(gps)` | เช็คว่ามี GPS fix หรือไม่ |
| `GPS_GetLatitude(gps)` | Latitude (degrees, +N/-S) |
| `GPS_GetLongitude(gps)` | Longitude (degrees, +E/-W) |
| `GPS_GetAltitude(gps)` | Altitude (meters) |
| `GPS_GetSpeed(gps)` | Speed over ground (km/h) |
| `GPS_GetSatellites(gps)` | จำนวนดาวเทียมที่ใช้ |
| `GPS_GetDateTime(gps, dt)` | UTC date/time |

---

## 🛰️ NMEA Sentences ที่รองรับ

| Sentence | Description | ข้อมูลที่ดึง |
|----------|-------------|-------------|
| `$GPGGA` | Fix data | latitude, longitude, altitude, satellites, fix quality, time |
| `$GPRMC` | Recommended minimum | latitude, longitude, speed, date, time, status |

---

## 💡 Tips

- **Cold start** อาจใช้เวลา 30-60 วินาทีกว่าจะมี fix แรก (ต้องเห็นท้องฟ้า)
- **Hot start** (มี backup battery) ใช้ 1-5 วินาที
- ดูสถานะ fix ได้จาก `GPS_IsFixValid()`
- ใช้ `gps.data_ready` flag เพื่อเช็คว่ามีข้อมูลใหม่
- สำหรับ debug: แนะนำให้ใช้ OLED/LCD I2C หรือ SWIO แทน USART

---

## 📝 Author

- **CH57x Library Team**
- License: MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)