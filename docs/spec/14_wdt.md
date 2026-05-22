# Watchdog (WDT)

## Overview

CH57x มี Watchdog สองแบบ: WWDG (Window Watchdog — จำกัดช่วงเวลา refresh) และ IWDG (Independent Watchdog — ใช้ LSI โดยเฉพาะ) เพื่อป้องกันโปรแกรมค้าง

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_wdt_init(timeout_ms, mode)` | เริ่มต้น WDT |
| `hal_wdt_feed()` | 刷新 watchdog (reset counter) |
| `hal_wdt_enable_irq()` | เปิด interrupt ก่อน reset (WWDG เท่านั้น) |
| `hal_wdt_enable_reset()` | เปิด reset เมื่อ timeout |
| `hal_wdt_clear_flag()` | ล้าง interrupt flag |

**Mode:** `HAL_WDT_TYPE_WINDOW` (WWDG) หรือ `HAL_WDT_TYPE_INDEPENDENT` (IWDG)

## CH57x Specifics

| คุณสมบัติ | WWDG | IWDG |
|---|---|---|
| Clock source | sysclock (APB) | LSI (24MHz RC) |
| ทำงานตอน sleep | No | Yes |
| Window mode | Yes | No |
| Interrupt before reset | Yes | No |
| Timeout range | ~0.1–100ms | ~1–32000ms |
| ใช้ LSI | No | Yes |

## Circuit Guide

WDT ไม่ต้องการวงจรภายนอก — แต่ถ้าต้องการ debug โดยไม่ให้ WDT รีเซ็ต:

```
// ใน debug mode — กัน WDT รีเซ็ต
while (!debugger_attached) {
    hal_wdt_feed();
    // หรือ comment บรรทัด hal_wdt_init() ตอน debug
}
```

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| IWDG ไม่มี interrupt ก่อน reset | WWDG มี interrupt ก่อน reset ~1 sysclock แต่ IWDG รีเซ็ตทันที |
| WWDG หยุดตอน sleep | WWDG ใช้ sysclock — หยุดทำงานตอน sleep → ใช้ IWDG แทนถ้าต้องการ watchdog ขณะ sleep |
| IWDG timeout ไม่แม่นยำ | ใช้ LSI (±1–2%) → timeout อาจคลาดเคลื่อน |
| feed ถี่เกิน WWDG reject | WWDG ต้อง feed ใน window (ไม่ถี่เกิน ไม่ช้าเกิน) — IWDG feed เมื่อไหร่ก็ได้ |
| SimpleHAL ไม่ expose window limit | ถ้าต้องการปรับ WWDG window upper/lower ใช้ StdPeriphDriver โดยตรง |

## Code สั้น

```c
// IWDG — 5 วินาทีแล้ว reset
hal_wdt_init(5000, HAL_WDT_TYPE_INDEPENDENT);

while (1) {
    hal_wdt_feed();  // ต้องเรียกก่อน 5 วิ
    Delay_Ms(1000);
}

// WWDG — 10ms interrupt แล้ว reset
hal_wdt_init(10, HAL_WDT_TYPE_WINDOW);
hal_wdt_enable_irq();
hal_wdt_enable_reset();
```
