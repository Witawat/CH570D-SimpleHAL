# DS3231 Real-Time Clock Library

> **Library สำหรับใช้งาน DS3231 RTC Module บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Troubleshooting](#troubleshooting)
8. [API Reference](#api-reference)

---

## ภาพรวม

DS3231 คือ Real-Time Clock (RTC) แม่นยำสูงที่มี **Temperature-Compensated Crystal Oscillator (TCXO)** ในตัว  
ทำให้ไม่ต้องใช้ crystal ภายนอก และมีความแม่นยำ **±2ppm** (ราว ±1 นาทีต่อปี)

**คุณสมบัติพิเศษของ DS3231:**
- มี temperature sensor ในตัว (อ่านอุณหภูมิ ±3°C)
- Alarm 2 ช่อง (Alarm 1 และ Alarm 2)
- 32kHz oscillator output
- Battery backup (CR2032) — เวลาไม่หายเมื่อไฟดับ
- I2C address **0x68** (fixed, ไม่มี address pin)

---

## คุณสมบัติของ Library

- ✅ อ่าน/ตั้งค่าวันเวลาครบถ้วน (วินาที, นาที, ชั่วโมง, วัน, เดือน, ปี)
- ✅ รองรับ 24H และ 12H (AM/PM)
- ✅ วันในสัปดาห์ (1=อาทิตย์ ถึง 7=เสาร์)
- ✅ Alarm 1 — ตั้งได้ทุกวินาที, ตรงวินาที, นาที, ชั่วโมง, วันที่
- ✅ Alarm 2 — ตั้งได้ทุกนาที, ตรงนาที, ชั่วโมง, วันที่
- ✅ อ่านอุณหภูมิ (0.25°C resolution)
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### วงจร DS3231

```
CH57x (3.3V)       DS3231 Module
                       +-----------+
PC2 (SCL) ----------> | SCL        |
PC1 (SDA) <---------> | SDA        |
3.3V ----------------> | VCC (+3.3V)|
GND -----------------> | GND        |
(optional GPIO) <----- | SQW/INT    | ← Alarm interrupt
                       | 32K        | ← 32kHz output
                       +-----------+
                       | CR2032     | ← Battery backup
                       +-----------+

Pull-up:
  PC2 (SCL) ──[4.7kΩ]── 3.3V
  PC1 (SDA) ──[4.7kΩ]── 3.3V
```

> ℹ️ **หมายเหตุ**: DS3231 Module ส่วนใหญ่ (ZS-042 ฯลฯ) มี pull-up บน board แล้ว  
> ถ้าต่อหลาย module บน bus เดียว อาจต้องถอด pull-up บางตัวออก

### Alarm Interrupt (ไม่บังคับ)

```
DS3231 SQW/INT ──────────────────────── GPIO input (เช่น PD3)
                                         (ตั้ง internal pull-up หรือต่อ 10kΩ → 3.3V)
```

---

## หลักการทำงาน

### BCD Format

DS3231 เก็บเวลาในรูปแบบ **BCD (Binary-Coded Decimal)**

```
ตัวอย่าง: เวลา 14:30:59

Seconds register = 0x59  (BCD)
  bit7-4 = 0101 = 5
  bit3-0 = 1001 = 9
  → ค่าจริง = 59 ✓

Hours register = 0x14  (BCD, 24H mode)
  bit7 = 0 (reserved)
  bit6 = 0 (24H mode)
  bit5-4 = 01 = 1
  bit3-0 = 0100 = 4
  → ค่าจริง = 14 ✓
```

Library ทำการแปลง BCD↔Binary อัตโนมัติ ผู้ใช้ไม่ต้องแปลงเอง

### Register Map

```
Address | Register   | Bit 7 | Bits 6-0
--------|------------|-------|------------------
0x00    | Seconds    |   0   | 10_SEC | SEC
0x01    | Minutes    |   0   | 10_MIN | MIN
0x02    | Hours      |   0   | 12/24  | HOUR
0x03    | Day        |   0   | 0      | DAY (1-7)
0x04    | Date       |   0   | 10_DATE| DATE (1-31)
0x05    | Month/Cent | CENT  | 10_MON | MONTH
0x06    | Year       |   -   | 10_YR  | YEAR (00-99)
0x07    | Alarm1 Sec | A1M1  | ...
...
0x0E    | Control    |   -   | INTCN A2IE A1IE
0x0F    | Status     | OSF   | ... A2F A1F
0x11    | Temp MSB   | SIGN  | INTEGER (°C)
0x12    | Temp LSB   |FRAC[1]|FRAC[0] 0 0 0 0 0 0
```

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include

```c
#include "SimpleHAL.h"
#include "DS3231.h"
```

### ขั้นตอนที่ 2: ประกาศตัวแปร

```c
DS3231_Instance rtc;
```

### ขั้นตอนที่ 3: Init ใน main()

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);  // SCL=PC2, SDA=PC1

    if (DS3231_Init(&rtc) != DS3231_OK) {
        printf("DS3231 not found!\r\n");
        while (1) {}
    }

    // ตั้งเวลาครั้งแรก (ปรับแค่ครั้งแรกหรือเมื่อ battery หมด)
    // DS3231_SetDateTime(&rtc, 2026, 4, 29, 14, 30, 0, DS3231_WEDNESDAY);
```

### ขั้นตอนที่ 4: อ่านเวลา

```c
    while (1) {
        DS3231_DateTime dt;
        DS3231_GetDateTime(&rtc, &dt);

        printf("วัน%s %04d-%02d-%02d %02d:%02d:%02d\r\n",
               DS3231_DayOfWeekStrTH(dt.day_of_week),
               dt.year, dt.month, dt.day,
               dt.hour, dt.minute, dt.second);

        Delay_Ms(1000);
    }
}
```

**ตัวอย่าง output:**
```
วันพุธ 2026-04-29 14:30:00
วันพุธ 2026-04-29 14:30:01
วันพุธ 2026-04-29 14:30:02
```

---

## การใช้งานขั้นสูง

### อ่านอุณหภูมิ

```c
float temp;
DS3231_GetTemperature(&rtc, &temp);
printf("อุณหภูมิ: %.2f C\r\n", temp);
// เช่น: อุณหภูมิ: 28.50 C
```

### ตั้ง Alarm และตรวจสอบแบบ Polling

```c
// Alarm ทุกวัน เวลา 07:00:00
DS3231_SetAlarm1(&rtc, DS3231_A1_MATCH_HR_MIN_SEC, 0, 7, 0, 0);
DS3231_EnableAlarm1(&rtc, 1);

while (1) {
    if (DS3231_IsAlarm1Fired(&rtc)) {
        printf("Alarm 1 fired! Wake up!\r\n");
        DS3231_ClearAlarmFlag(&rtc, 1);  // ล้าง flag เพื่อรับ alarm ครั้งต่อไป
    }
    Delay_Ms(100);
}
```

### ตั้ง Alarm และใช้ Interrupt (SQW pin)

```c
volatile uint8_t alarm_triggered = 0;

// Callback เมื่อ SQW pin ลง LOW
void alarm_isr(void) {
    alarm_triggered = 1;
}

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
    DS3231_Init(&rtc);

    // ตั้ง Alarm 2: ทุกชั่วโมง ที่นาทีที่ 30
    DS3231_SetAlarm2(&rtc, DS3231_A2_MATCH_MINUTES, 0, 0, 30);
    DS3231_EnableAlarm2(&rtc, 1);

    // ตั้ง interrupt ที่ขา PD3 (SQW pin)
    attachInterrupt(PIN_PD3, alarm_isr, FALLING);

    while (1) {
        if (alarm_triggered) {
            alarm_triggered = 0;
            printf("Alarm 2 fired at :30!\r\n");
            DS3231_ClearAlarmFlag(&rtc, 2);
        }
        // ทำงานอื่น...
    }
}
```

### Alarm Modes สรุป

**Alarm 1:**

| Mode | ความหมาย |
|------|----------|
| `DS3231_A1_EVERY_SECOND` | ทุก 1 วินาที |
| `DS3231_A1_MATCH_SECONDS` | ตรงวินาทีที่ตั้ง |
| `DS3231_A1_MATCH_MIN_SEC` | ตรงนาทีและวินาที |
| `DS3231_A1_MATCH_HR_MIN_SEC` | ตรงชั่วโมง นาที วินาที |
| `DS3231_A1_MATCH_DATE` | ตรงวันที่ ชั่วโมง นาที วินาที |

**Alarm 2** (ไม่มีวินาที):

| Mode | ความหมาย |
|------|----------|
| `DS3231_A2_EVERY_MINUTE` | ทุก 1 นาที |
| `DS3231_A2_MATCH_MINUTES` | ตรงนาทีที่ตั้ง |
| `DS3231_A2_MATCH_HR_MIN` | ตรงชั่วโมงและนาที |
| `DS3231_A2_MATCH_DATE` | ตรงวันที่ ชั่วโมง นาที |

### นาฬิกาแสดงบน OLED / LCD

```c
#include "DS3231.h"
// #include "OLED.h"  // ตัวอย่างเท่านั้น

void display_time(DS3231_DateTime* dt) {
    char buf[32];

    // วันที่: 29/04/2026
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", dt->day, dt->month, dt->year);
    // OLED_PrintAt(0, 0, buf);

    // เวลา: 14:30:59
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", dt->hour, dt->minute, dt->second);
    // OLED_PrintAt(0, 1, buf);

    printf("%s %s\r\n", buf, DS3231_DayOfWeekStr(dt->day_of_week));
}
```

---

## Troubleshooting

### ปัญหา: DS3231_Init() คืน DS3231_ERROR_I2C

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Pull-up ขาด | ต่อ 4.7kΩ ที่ SDA และ SCL → 3.3V |
| I2C address ผิด | DS3231 ใช้ 0x68 เสมอ ใช้ I2CScan ตรวจ |
| VCC ผิด | DS3231 ทำงานที่ 2.3V–5.5V, ใช้ 3.3V |
| I2C ไม่ได้ Init | เรียก `I2C_SimpleInit()` ก่อน `DS3231_Init()` |

### ปัญหา: เวลาคลาดเคลื่อนเร็ว

| สาเหตุ | วิธีแก้ |
|--------|---------|
| OSF flag ถูกตั้ง (oscillator หยุด) | เรียก `DS3231_Init()` เพื่อล้าง OSF flag |
| Battery หมดหรือไม่มี | ใส่ CR2032 ใหม่ |
| ใช้ DS3231 ปลอม | ลอง chip จาก vendor อื่น |

### ปัญหา: Alarm ไม่ทำงาน

| สาเหตุ | วิธีแก้ |
|--------|---------|
| ลืม `DS3231_EnableAlarm1/2()` | เรียก enable หลัง SetAlarm |
| ลืมล้าง alarm flag | เรียก `DS3231_ClearAlarmFlag()` หลัง alarm fired |
| INTCN bit ไม่ถูกตั้ง | library ตั้งอัตโนมัติใน `EnableAlarm()` |

---

## API Reference

### `DS3231_Init(rtc)` → `DS3231_Status`
เริ่มต้น DS3231 — ตรวจสอบการเชื่อมต่อและล้าง OSF flag

| Parameter | Type | คำอธิบาย |
|-----------|------|----------|
| `rtc` | `DS3231_Instance*` | ตัวแปร instance |

---

### `DS3231_SetDateTime(rtc, year, month, day, hour, min, sec, dow)` → `DS3231_Status`
ตั้งค่าวันเวลา

| Parameter | Type | Range |
|-----------|------|-------|
| `year` | uint16_t | 2000-2099 |
| `month` | uint8_t | 1-12 |
| `day` | uint8_t | 1-31 |
| `hour` | uint8_t | 0-23 |
| `min` | uint8_t | 0-59 |
| `sec` | uint8_t | 0-59 |
| `dow` | DS3231_DayOfWeek | SUNDAY=1 ถึง SATURDAY=7 |

---

### `DS3231_GetDateTime(rtc, &dt)` → `DS3231_Status`
อ่านวันเวลา → เก็บใน `DS3231_DateTime` struct

---

### `DS3231_GetTemperature(rtc, &temp)` → `DS3231_Status`
อ่านอุณหภูมิ (°C, ±3°C accuracy, 0.25°C resolution)

---

### `DS3231_SetAlarm1(rtc, mode, day, hour, min, sec)` → `DS3231_Status`
ตั้ง Alarm 1

---

### `DS3231_SetAlarm2(rtc, mode, day, hour, min)` → `DS3231_Status`
ตั้ง Alarm 2 (ไม่มี parameter วินาที)

---

### `DS3231_EnableAlarm1/2(rtc, enable)` → `DS3231_Status`
เปิด/ปิด alarm interrupt บน SQW pin (`enable=1` เปิด, `enable=0` ปิด)

---

### `DS3231_IsAlarm1/2Fired(rtc)` → `uint8_t`
ตรวจสอบ alarm flag (1=fired, 0=ยังไม่ fire)

---

### `DS3231_ClearAlarmFlag(rtc, alarm_no)` → `DS3231_Status`
ล้าง alarm flag (`alarm_no=1` หรือ `2`)

---

### `DS3231_DayOfWeekStr(dow)` → `const char*`
แปลง enum → ชื่อวันภาษาอังกฤษ (เช่น `"Wednesday"`)

---

### `DS3231_DayOfWeekStrTH(dow)` → `const char*`
แปลง enum → ชื่อวันภาษาไทย (เช่น `"พุธ"`)

---

### DS3231_Status Values

| ค่า | ความหมาย |
|-----|----------|
| `DS3231_OK` | สำเร็จ |
| `DS3231_ERROR_I2C` | I2C communication error |
| `DS3231_ERROR_PARAM` | Parameter ผิด (NULL pointer หรือไม่ได้ Init) |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)