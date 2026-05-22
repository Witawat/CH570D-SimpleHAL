# RTC

## Overview

Real-Time Clock ทำงานด้วย LSI (24MHz RC) หรือ HSE ผ่าน prescaler เพื่อให้ได้ clock 1Hz สำหรับนับเวลา calendar (year/month/day/hour/minute/second) พร้อม alarm และ periodic timer

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_rtc_init()` | เริ่มต้น RTC ด้วย LSI cycle |
| `hal_rtc_set_time(t)` | ตั้งเวลา calendar |
| `hal_rtc_get_time(t)` | อ่านเวลาปัจจุบัน |
| `hal_rtc_set_trigger(t, cb)` | ตั้ง alarm แบบ one-shot (callback เมื่อถึงเวลา) |
| `hal_rtc_set_timer(ms, cb)` | ตั้ง periodic timer (callback ทุก ms) |
| `hal_rtc_attach_cb(cb)` | callback ทั่วไป |

**Time struct (`hal_rtc_time_t`):** year, month, day, hour, minute, second (`uint16_t` + `uint8_t`×5)

## Circuit Guide

RTC ไม่ต้องการวงจรภายนอก (ใช้ LSI 24MHz ภายใน) แต่ถ้าต้องการความแม่นยำสูงสามารถใช้ HSE (XTAL) ภายนอก:

```
          CH57x
    XTAL_OUT ───┨┃───┤├── 16MHz XTAL
    XTAL_IN  ───┨┃───┤├──
                    │
               C1(12pF)  C2(12pF)
                    │       │
                   GND     GND
```

## CH57x Specifics

- **LSI (24MHz RC):** ใช้กับ RTC ได้เลย — drift ±1–2% (ถูก temperature compensates ภายใน)
- **HSE + ext XTAL:** drift ±20–50ppm — แม่นยำกว่ามาก แต่กินไฟเพิ่มและต้องการ crystal ภายนอก
- **NVR (Non-Volatile Register):** RAM ภายใน 24 ไบต์ที่รักษาข้อมูลตอน sleep/shutdown

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| LSI drift สูง | ±1–2% = ผิด ~15–30 นาทีต่อวัน — ถ้าต้องการเวลาที่แม่นยำต้องใช้ HSE + XTAL |
| `hal_rtc_get_time()` overhead | อ่านค่าผ่าน register — อาจเพี้ยน 1 วินาทีถ้าอ่านตรงช่วงเปลี่ยนวัน |
| ไม่มี battery backup | RAM + RTC หายเมื่อไม่มีไฟเลี้ยง — ทุกครั้งที่ boot ต้องตั้งเวลาใหม่ |
| Year จำกัด | ตั้ง year ได้สูงสุด 255 (uint8_t) |

## Code สั้น

```c
hal_rtc_time_t now = {
    .year = 2026, .month = 5, .day = 22,
    .hour = 17, .minute = 0, .second = 0
};

hal_rtc_init();
hal_rtc_set_time(&now);

// Alarm เมื่อถึง 17:05:00
hal_rtc_time_t alarm = { .hour = 17, .minute = 5, .second = 0 };
hal_rtc_set_trigger(&alarm, []() {
    printf("Alarm! Time's up!\r\n");
});

// หรือ timer callback ทุก 1 วินาที
hal_rtc_set_timer(1000, []() {
    hal_rtc_time_t t;
    hal_rtc_get_time(&t);
    printf("%04d-%02d-%02d %02d:%02d:%02d\r\n",
           t.year, t.month, t.day, t.hour, t.minute, t.second);
});
```
