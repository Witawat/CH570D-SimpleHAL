# Timer

## Overview

16-bit timer หนึ่งชุด ใช้จับเวลา設定 period (counting up) รองรับ PWM, input capture, external counter, และ quadrature encoder พร้อม DMA

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_timer_init(cfg)` | เริ่มต้น timer ตาม config struct |
| `hal_timer_start()` | เริ่มนับ |
| `hal_timer_stop()` | หยุดนับ |
| `hal_timer_reset()` | รีเซ็ต counter เป็น 0 |
| `hal_timer_set_period(us)` | กำหนด period (µs) |
| `hal_timer_get_count()` | อ่านค่าปัจจุบัน |
| `hal_timer_attach_cb(cb)` | ตั้ง callback เมื่อ period ครบ |

**Config struct (`hal_timer_config_t`):** mode (`PERIODIC` / `ONE_SHOT`), period_us (`uint32_t`), prescaler (`uint8_t`)

## Circuit Guide

Timer ไม่ได้ใช้กับวงจรภายนอกโดยตรง — ใช้สร้าง time base สำหรับ:
- **Delay แบบไม่บล็อก:** ผ่าน hal_softimer หรือเช็ค `hal_timer_get_count()` + period
- **Periodic callback:** `hal_timer_attach_cb()` สำหรับ polling sensor, blink LED, update display
- **Input capture:** นับความถี่ / pulse width ของสัญญาณภายนอก (ต้องใช้ pin remap)
- **Quadrature encoder:** อ่านความเร็ว/ตำแหน่งของ motor encoder (A/B phase)

## CH57x Specifics

- **16-bit counter:** ค่าสูงสุด = 65535 (ก่อน overflow)
- **Max period:** ~67 วินาที ที่ sysclock 100MHz + prescaler สูงสุด
- **Prescaler:** 1–256 (ค่า + 1 จริง = prescaler × 2)
- **PWM via timer:** `hal_timer.c` ไม่ได้ใช้ — ใช้ `hal_pwm.c` (PWM5 module) แทน
- **DMA:** รองรับ — สามารถอัปเดต period อัตโนมัติผ่าน DMA ได้
- **Pin remap:** Timer input capture / PWM output สามารถ remap ได้ (ดู [appendix_pinout.md](appendix_pinout.md))

**ช่วง period ที่ sysclock 100MHz:**

| Prescaler | Tick | Max period |
|---|---|---|
| 1 | 0.02µs | ~1.3ms |
| 2 | 0.04µs | ~2.6ms |
| 16 | 0.32µs | ~21ms |
| 128 | 2.56µs | ~168ms |
| 256 (max) | 5.12µs | ~336ms |

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| 1 instance เท่านั้น | `hal_timer` มีตัวแปร global ตัวเดียว — เรียก `hal_timer_init` ซ้ำจะทับของเก่า |
| `hal_timer_stop` ไม่ reset | stop แค่หยุดนับ — ต้องเรียก `reset()` ด้วยถ้าต้องการเริ่มใหม่จาก 0 |
| Capture/encoder ต้องใช้ Low-Level API | SimpleHAL timer HAL ไม่ expose capture/encoder — ต้องเรียก StdPeriphDriver (`CH57x_timer.c`) โดยตรง |
| PWM output ผ่าน timer | ใช้ `hal_pwm` (PWM5 module) แทน — timer PWM และ PWM5 เป็น module คนละตัวกัน |
| period_us parameter | ความละเอียดจำกัดที่ ~1µs (ขึ้นกับ sysclock) — ไม่สามารถ set period ระดับ ns ได้ |

## Code สั้น

```c
hal_timer_config_t tmr_cfg = {
    .mode = HAL_TIMER_PERIODIC,
    .period_us = 1000000  // 1 วินาที
};
hal_timer_init(&tmr_cfg);

volatile uint32_t ms_ticks = 0;
hal_timer_attach_cb([]() {
    ms_ticks++;
});

hal_timer_start();  // เริ่ม — callback ถูกเรียกทุก 1 วินาที
while (1) {
    if (ms_ticks >= 10) {
        digitalToggle(PA11);  // กระพริบทุก 10 วินาที
        ms_ticks = 0;
    }
}
```
