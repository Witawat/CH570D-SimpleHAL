# ESP-01 / ESP-01S Library for CH57x series

ไลบรารีนี้ช่วยให้ CH57x series ใช้งาน Wi-Fi ผ่าน ESP-01/ESP-01S (AT firmware) โดยใช้ UART

## 1) การต่อวงจร

```
CH57x/006                     ESP-01 / ESP-01S
--------------------------------------------------------
PD5 (USART1 TX) ---------------> RX
PD6 (USART1 RX) <--------------- TX
3.3V --------------------------> VCC
GND ---------------------------> GND
CH_PD / EN --------------------> 3.3V
RST (optional GPIO) -----------> RST

หมายเหตุ:
- ใช้ไฟ 3.3V เท่านั้น
- แนะนำ regulator 3.3V รองรับ peak >= 500mA (แนะนำเผื่อ 700mA)
- ใส่ C ใกล้โมดูล: 220-470uF + 0.1uF
```

## 2) ตัวอย่างใช้งาน

```c
#include "User/Lib/ESP01/ESP01.h"

ESP01_Instance wifi;

void app_wifi_start(void) {
    if (ESP01_Init(&wifi, BAUD_9600, USART_PINS_DEFAULT) != ESP01_OK) {
        return;
    }

    ESP01_SetMode(&wifi, ESP01_MODE_STA);
    ESP01_JoinAP(&wifi, "MySSID", "MyPassword", 15000);

    char ip[32];
    if (ESP01_GetIP(&wifi, ip, sizeof(ip), 2000) == ESP01_OK) {
        // ip พร้อมใช้งาน
    }
}
```

HTTP GET อย่างง่าย:

```c
char resp[256];
ESP01_HTTPGet(&wifi, "example.com", "/", resp, sizeof(resp), 6000);
```

HTTP POST อย่างง่าย:

```c
char resp[256];
const char* body = "{\"temp\":26.5,\"hum\":58}";
ESP01_HTTPPost(&wifi,
               "example.com",
               "/api/telemetry",
               "application/json",
               body,
               resp,
               sizeof(resp),
               8000);
```

## 3) API หลัก

### Core
| Function | รายละเอียด |
|---|---|
| `ESP01_Init` | เริ่มต้น UART + ทดสอบ `AT` + ปิด Echo |
| `ESP01_TestAT` | ส่ง `AT` ตรวจว่า module พร้อม |
| `ESP01_Reset` | รีเซ็ต module รอบูต 2 วินาที + retry |
| `ESP01_GetVersion` | อ่าน firmware version (`AT+GMR`) |

### Wi-Fi
| Function | รายละเอียด |
|---|---|
| `ESP01_SetMode` | ตั้งโหมด `STA/AP/STA+AP` |
| `ESP01_JoinAP` | เชื่อมต่อ Wi-Fi AP |
| `ESP01_DisconnectAP` | ตัดการเชื่อมต่อ AP |
| `ESP01_IsConnected` | ตรวจสอบสถานะการเชื่อมต่อ |
| `ESP01_GetIP` | อ่าน IP ปัจจุบัน |
| `ESP01_SetBaud` | เปลี่ยน baud rate (บันทึกใน Flash) |

### TCP / HTTP
| Function | รายละเอียด |
|---|---|
| `ESP01_TCPConnect` | เปิด TCP client |
| `ESP01_TCPSend` | ส่ง payload ผ่าน TCP |
| `ESP01_TCPRead` | รับข้อมูล TCP (parse `+IPD`) |
| `ESP01_TCPReadAvailable` | ตรวจว่ามีข้อมูลรอรับ |
| `ESP01_TCPClose` | ปิด TCP |
| `ESP01_HTTPGet` | HTTP GET helper |
| `ESP01_HTTPPost` | HTTP POST helper (header+body ส่งใน 1 CIPSEND) |

### Power
| Function | รายละเอียด |
|---|---|
| `ESP01_SetSleepMode` | ตั้ง sleep mode (None/Light/Modem) |
| `ESP01_SetTxPowerQuarterDbm` | ลด TX power ลดกระแส |
| `ESP01_EnterDeepSleepUs` | เข้า deep sleep (µs) |
| `ESP01_GetCurrentProfile` | คืนค่าช่วงกระแสโดยประมาณ |

## 4) การกินกระแส (Design Target)

ค่าด้านล่างเป็นช่วงโดยประมาณจากพฤติกรรม ESP8266/ESP-01 ทั่วไป (ขึ้นกับ firmware, RSSI, Tx power):

| State | Typical | Peak | หมายเหตุ |
|---|---:|---:|---|
| Deep Sleep | 0.02-0.20 mA | 0.20 mA | บอร์ดจริงมักสูงกว่าเฉพาะชิป |
| Light/Modem Sleep | 1-20 mA | 20 mA | ขณะยัง associate AP |
| Idle Connected | 50-80 mA | 90 mA | มี beacon traffic |
| RX Active | 50-70 mA | 80 mA | รับข้อมูล |
| TX Burst | 120-220 mA | 300 mA | ออกแบบภาคจ่ายไฟให้พอ peak |

ข้อแนะนำภาคจ่ายไฟ:
- Regulator 3.3V >= 500mA (แนะนำ >= 700mA)
- C bulk 220-470uF ใกล้โมดูล
- Decoupling 0.1uF วางชิด VCC/GND
- หลีกเลี่ยงสายไฟยาว/เส้นกราวด์เล็ก

## 5) Troubleshooting

| อาการ | สาเหตุที่เป็นไปได้ | วิธีแก้ |
|---|---|---|
| `AT` ไม่ตอบ | baud ไม่ตรง / wiring ผิด | เริ่มที่ 9600, เช็ค TX/RX สลับ |
| รีเซ็ตเองบ่อย | ไฟตกตอน TX peak | เพิ่ม regulator และ C bulk |
| `busy p...` | ส่งคำสั่งถี่เกิน | เพิ่ม delay/retry, อย่าส่งซ้อน |
| Join AP ไม่ผ่าน | SSID/PASS หรือสัญญาณอ่อน | ลองใกล้ AP, เช็ครหัสผ่าน |
| HTTP ไม่ได้ | DNS/port/firewall | ทดสอบด้วย IP ตรงก่อน |

## 6) หมายเหตุ

- แนะนำใช้ firmware AT รุ่นที่เสถียรและรู้จักชุดคำสั่งเดียวกัน
- ฟังก์ชันบางตัว เช่น `AT+RFPOWER` ขึ้นกับเวอร์ชัน firmware
- ถ้าต้องใช้ TLS หนัก แนะนำ offload งานฝั่ง gateway หรือใช้โมดูลที่รองรับตรงกว่า

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)