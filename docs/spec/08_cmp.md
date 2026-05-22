# Comparator (CMP)

## Overview

Comparator อนาล็อก 1 ตัว สามารถเลือก input+ และ input- จากพินหรือ VREF ภายใน มี interrupt เมื่อ output เปลี่ยน และสามารถ route output ไปยัง timer capture input ได้

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_cmp_init(cfg)` | เริ่มต้น comparator ตาม config |
| `hal_cmp_deinit()` | ปิด comparator |
| `hal_cmp_read()` | อ่าน output ปัจจุบัน (0/1) |
| `hal_cmp_enable()` | เปิด comparator (หลัง init) |
| `hal_cmp_disable()` | ปิด comparator |
| `hal_cmp_route_to_timer_cap(enable)` | ต่อ output ไปยัง timer capture |
| `hal_cmp_attach_irq(cb, mode)` | ตั้ง interrupt callback |

**Config struct (`hal_cmp_config_t`):** input_p (`CMP_IN_PA3` / `CMP_IN_PA7`), input_n (`CMP_IN_PA2` / `CMP_IN_VREF`), vref_mv (`uint16_t`, 50–800mV)

**Input combinations:**

| Input+ | Input- | ใช้กับ |
|---|---|---|
| PA3 | PA2 | เปรียบเทียบสัญญาณอนาล็อกสองขา (เช่น differential sensor) |
| PA3 | VREF | เปรียบเทียบ PA3 กับแรงดันอ้างอิงภายใน |
| PA7 | PA2 | เปรียบเทียบ PA7 กับ PA2 |
| PA7 | VREF | เปรียบเทียบ PA7 กับแรงดันอ้างอิงภายใน — **นิยมที่สุด** |

## Circuit Guide

### Zero-crossing / Threshold detection

```
สัญญาณ Input ──── PA7 (+)
VREF (50–800mV) ──── (internal)
Output ───→ interrupt หรือ timer capture
```

### Photocell (LDR) + Threshold

```
VCC_3.3V
   │
   R(10k)
   ├── PA3 (+) — แรงดันเพิ่มเมื่อแสงน้อย
   │
  LDR
   │
  GND

VREF = 500mV → interrupt เมื่อแสงน้อยกว่า threshold
```

### Battery voltage monitor

```
Battery 3.7V ── R1(10k) ──┬── PA7 (+)
                          │
                        R2(10k)
                          │
                         GND
// PA7 ≈ 1.85V (battery full)

VREF = 1.5V → interrupt เมื่อ battery < ~3.0V
```

## CH57x Specifics

- **VREF range:** 50mV ถึง 800mV (ปรับขั้นละ ~50mV)
- **Output to timer capture:** ต่อ `hal_cmp_route_to_timer_cap(1)` → timer capture input → จับเวลาที่ comparator เปลี่ยน state ได้แม่นยำ
- **Interrupt:** Rising, Falling, หรือ Both edge
- **ตอบสนองเร็ว:** ~0.2µs (เร็วกว่า ADC sampling มาก)

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| VREF สูงสุด 800mV | ถ้าต้องการเทียบกับแรงดัน > 800mV ต้องใช้ voltage divider ก่อนเข้า CMP input |
| Input voltage range | 0 – VCC (rail-to-rail) |
| ไม่มี hysteresis ในตัว | อาจต้องป้อนสัญญาณกลับ (positive feedback) ด้วย resistor ภายนอกถ้าสัญญาณมี noise (Rf 100k–1MΩ จาก output → input+) |
| 1 comparator เท่านั้น | มี CMP ตัวเดียว |
| SimpleHAL ไม่มี hysteresis API | ต้องใช้ StdPeriphDriver `CMP_Init()` ถ้าต้องการปรับ hysteresis |

## Code สั้น

```c
// ตรวจสอบว่า PA7 > 500mV หรือไม่
hal_cmp_config_t cmp_cfg = {
    .input_p = CMP_IN_PA7,
    .input_n = CMP_IN_VREF,
    .vref_mv = 500
};
hal_cmp_init(&cmp_cfg);
hal_cmp_enable();

uint8_t state = hal_cmp_read(); // 1 = PA7 > 500mV

// ตั้ง interrupt เมื่อ PA7 > 500mV → ชาร์จ timer capture
hal_cmp_route_to_timer_cap(1);
hal_cmp_attach_irq([]() {
    // comparator output เปลี่ยน state
    uint32_t capture_time = hal_timer_get_count();
}, RISING);
```
