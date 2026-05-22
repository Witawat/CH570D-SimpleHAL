# Power Management

## Overview

CH57x มี 4 โหมดประหยัดพลังงาน: Idle → Halt → Sleep → Shutdown (ประหยัดไฟมากขึ้นตามลำดับ) พร้อมเลือก wakeup source และ RAM retention mask

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_pwr_idle()` | Idle mode — CPU หยุด, peripheral ทำงาน, wake ทันทีเมื่อ interrupt |
| `hal_pwr_halt()` | Halt mode — CPU + peripheral หยุด, RAM รักษา, wake ด้วย interrupt |
| `hal_pwr_sleep()` | Sleep mode — CPU + peripheral หยุด, RAM (บางส่วน) รักษา, wake ด้วย interrupt |
| `hal_pwr_shutdown()` | Shutdown — เหมือน reset, wake = reboot |
| `hal_pwr_config_wake(src, delay)` | ตั้งค่า wakeup source และ wake delay |
| `hal_pwr_config_ram_retention(mask)` | ตั้งค่า RAM retention mask |

**Wakeup source (`hal_pwr_wake_src_t`):** `USB`, `RTC`, `GPIO`, `BAT`

**RAM retention mask:** `RAM2K` (0–2KB), `RAM16K` (0–16KB), `EXTEND`, `XROM`

## Circuit Guide

### Wake from GPIO

```
PA0 ──── SW ──── GND   // กดปุ่ม wake จาก sleep

// ต้อง config PA0 เป็น INPUT_PULLUP ก่อน sleep
pinMode(PA0, INPUT_PULLUP);
hal_pwr_config_wake(HAL_PWR_WAKE_GPIO, 100);
```

### Wake from RTC

```
// ใช้ RTC alarm wake — ไม่ต้องใช้วงจรเพิ่ม
hal_rtc_set_trigger(&alarm_time, NULL);
hal_pwr_config_wake(HAL_PWR_WAKE_RTC, 100);
```

## CH57x Specifics

| Mode | CPU | Peripherals | RAM | Wake method | Current (typ) |
|---|---|---|---|---|---|
| **Active** | Run | All on | Full | — | ~20mA @ 100MHz |
| **Idle** | Stop | On | Full | Any interrupt | ~6mA |
| **Halt** | Stop | Stop | Full | GPIO / RTC / USB | ~200µA |
| **Sleep** | Stop | Stop | Selected | GPIO / RTC | ~10µA |
| **Shutdown** | Off | Off | Lost | Reset (wake = reboot) | ~1µA |

**Note:** กระแสเป็นค่าประมาณ — ขึ้นกับ sysclock, peripheral ที่เปิด, และ VCC จริง

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| Shutdown = reboot | ไม่มีการกู้ state — ต้อง init ทุกอย่างใหม่ |
| Wake delay config | 1–8191 cycles (~0.02µs–163µs ที่ 100MHz) — ไม่มีผลกับ Shutdown |
| GPIO wake ต้องเป็น rising/falling | ต้องตั้งค่า interrupt mode ก่อน sleep |
| Sleep ต้องปิด peripheral บางตัว | ถ้าไม่ปิดอาจกินไฟเกิน — ใช้ `hal_pwr_sleep()` แทน `LowPower_Sleep()` โดยตรงถ้าต้องการ granularity |
| RTC wake ต้อง LSI ทำงาน | ถ้าเลือก RTC เป็น wake source ต้องมั่นใจว่า LSI เปิดและ RTC กำลังทำงาน |

## Code สั้น

```c
// Sleep 10 วินาที แล้ว Wake ด้วย RTC
hal_rtc_time_t now, alarm;
hal_rtc_get_time(&now);
alarm = now;
alarm.second = (now.second + 10) % 60;

hal_rtc_set_trigger(&alarm, NULL);
hal_pwr_config_wake(HAL_PWR_WAKE_RTC, 100);
hal_pwr_sleep();  // หยุดที่นี่ — wake อีก 10 วินาที
// หลังจาก wake — ทำงานต่อ
printf("Woke up!\r\n");
```
