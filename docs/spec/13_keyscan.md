# KeyScan

## Overview

KeyScan เป็นฮาร์ดแวร์สแกน matrix keypad โดยเฉพาะ — รองรับการสแกน pins PA2/PA3/PA8/PA10/PA11 แบบ matrix สูงสุด 5×? (ขึ้นกับจำนวน pins ที่ใช้) พร้อม debounce ในตัว, interrupt, และ wake จาก sleep

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_keyscan_init(cfg)` | เริ่มต้น KeyScan ตาม config |
| `hal_keyscan_get_value(ch)` | อ่าน key value ของแถว |
| `hal_keyscan_get_count(ch)` | อ่านจำนวนรอบที่ detect |
| `hal_keyscan_attach_cb(cb)` | ตั้ง callback เมื่อกด |
| `hal_keyscan_enable_wakeup(bool)` | เปิด/ปิด wake จาก sleep |

**Config struct (`hal_keyscan_config_t`):** clk_div (`1/2/4/8/16`), repeat (`1–7`)

**Pin ที่ใช้ได้ (fix):** PA2, PA3, PA8, PA10, PA11

## Circuit Guide

### 4×4 Matrix Keypad

```
               Col1    Col2    Col3    Col4
               PA2     PA3     PA8     PA10
              ┌──┐   ┌──┐   ┌──┐   ┌──┐
  Row1 PA11 ──┤SW├───┤SW├───┤SW├───┤SW├──
              └──┘   └──┘   └──┘   └──┘
              ┌──┐   ┌──┐   ┌──┐   ┌──┐
  Row2 PA? ───┤SW├───┤SW├───┤SW├───┤SW├──
              └──┘   └──┘   └──┘   └──┘
              ┌──┐   ┌──┐   ┌──┐   ┌──┐
  Row3 PA? ───┤SW├───┤SW├───┤SW├───┤SW├──
              └──┘   └──┘   └──┘   └──┘
              ┌──┐   ┌──┐   ┌──┐   ┌──┐
  Row4 PA? ───┤SW├───┤SW├───┤SW├───┤SW├──
              └──┘   └──┘   └──┘   └──┘
```

**Note:** KeyScan ใช้ขาที่ fix มาให้ (PA2/3/8/10/11) — matrix 4×4 = 4 cols + 4 rows → ใช้ 8 pins ต้องเพิ่มขา GPIO ทั่วไปเป็น rows (KeyScan hardware สแกน col ส่วน row อ่านด้วย polling)

## CH57x Specifics

- **Col pins:** PA2, PA3, PA8, PA10, PA11 (output)
- **Row pins:** ไม่ fix — ใช้ GPIO อ่านแยก (input pull-up)
- **Debounce:** KeyScan hardware กรองสัญญาณในตัว (repeat count = จำนวนรอบที่ยืนยันก่อนแจ้ง interrupt)
- **Wakeup:** เปิด `hal_keyscan_enable_wakeup(1)` → กดปุ่มใน matrix ปลุกจาก sleep ได้

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| Col pins ตายตัว | PA2/3/8/10/11 เท่านั้น — ไม่สามารถเปลี่ยน |
| SimpleHAL API อ่านเฉพาะ col | `hal_keyscan_get_value()` คืนค่าสถานะของ col — ถ้าต้องการ matrix ขยายต้องใช้ GPIO เพิ่ม |
| Interrupt แบบเดียว | interrupt เกิดเมื่อ key press (ไม่แยก press/release) |
| scan speed จำกัด | clk_div 1 = ~3MHz, div 16 = ~187kHz — ต้อง balance ระหว่าง speed กับ noise immunity |

## Code สั้น

```c
hal_keyscan_config_t kscfg = {
    .clk_div = 4,
    .repeat = 2
};
hal_keyscan_init(&kscfg);

hal_keyscan_attach_cb([]() {
    uint8_t val = hal_keyscan_get_value(0);
    printf("Key pressed at col %d\r\n", val);
});

// หรือ polling
while (1) {
    for (int i = 0; i < 5; i++) {
        uint8_t v = hal_keyscan_get_value(i);
        if (v) printf("Col %d: %d\r\n", i, v);
    }
    Delay_Ms(50);
}
```
