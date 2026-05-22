# ADC

## Overview

ADC แบบ successive approximation (SAR) หนึ่งช่องต่อครั้ง รองรับ 3 ความละเอียด: 4-bit (PA7), 8-bit (PA2), 9-bit (PA2) พร้อม auto channel switch สำหรับอ่านหลาย pin สลับกัน

## SimpleHAL API

| ฟังก์ชัน | คำอธิบาย |
|---|---|
| `hal_adc_init(resolution)` | เริ่มต้น ADC ตามความละเอียด |
| `hal_adc_read(ch)` | อ่านค่า raw จาก channel |
| `hal_adc_read_mv(ch)` | อ่านค่าเป็น mV |
| `hal_adc_free()` | คืนทรัพยากร ADC (ปิด) |

**Resolution (`hal_adc_resolution_t`):** `HAL_ADC_4BIT` (PA7, 0–16), `HAL_ADC_8BIT` (PA2, 0–511), `HAL_ADC_9BIT` (PA2, 0–1023)

**Channel mapping:**

| ADC Channel | ขาจริง |
|---|---|
| `ADC_CH0`–`ADC_CH7` | PA2 (ผ่าน analog mux) |
| `ADC_CH8` | PA7 (4-bit mode เท่านั้น) |

## Circuit Guide

### Potentiometer (แบ่งแรงดัน)

```
VCC_3.3V
   │
   R(10kΩ potentiometer)
   ├── PA2 (ADC input)
   │
  GND
```

### Sensor analog ทั่วไป

```
เซ็นเซอร์ analog (Output 0–3.3V)
   │
   └── PA2 (ADC input)
   │
  GND ─── GND (ร่วม)
```

**ถ้า sensor output > 3.3V:** ต้องใช้ voltage divider ก่อนเข้า ADC (เช่น output 5V → R1(10k) + R2(10k) → 2.5V)

```
Sensor 5V ──── R1(10k) ──┬── PA2
                         │
                       R2(10k)
                         │
                        GND
```

**ถ้า sensor output < 50mV:** ต้อง use op-amp ขยายก่อน หรือใช้ comparator แทน (ดู [08_cmp.md](08_cmp.md))

## CH57x Specifics

- **4-bit mode:** ใช้ PA7 เท่านั้น, ค่า 0–16, ความละเอียด ~200mV/step
- **8-bit mode:** ใช้ PA2 เท่านั้น, ค่า 0–511, ความละเอียด ~6.5mV/step
- **9-bit mode:** ใช้ PA2 เท่านั้น, ค่า 0–1023, ความละเอียด ~3.2mV/step
- **VREF:** AVDD (3.3V) — internal reference ไม่สามารถเปลี่ยนได้
- **Conversion time:** ~36 sysclock cycles (เช่น 100MHz → 0.36µs)
- **Auto channel switch:** `ADC_SimpleInitChannels()` ใช้ internal timer สลับ channel — อ่านค่าได้ต่อเนื่องโดยไม่ต้องรอ

## Limitations

| ข้อจำกัด | รายละเอียด |
|---|---|
| 1 ช่องต่อครั้ง (single-ended) | ไม่มี differential หรือ simultaneous sampling |
| 4-bit = PA7, 8/9-bit = PA2 | เปลี่ยน resolution ไม่ได้โดยไม่เปลี่ยน pin |
| `ADC_MAX_VALUE` = 1024 ในสูตร | แต่ถ้าใช้ 8-bit จริงค่า max = 511 — `ADC_ToVoltage()` ใช้ `/ 1024.0f` เสมอ → ได้ค่าผิด ถ้าต้องการแม่นยำคำนวณเอง: `raw * vref / 511.0f` |
| `ADC_SimpleInitChannels()` hardcode 8-bit | Resolution ถูก hardcode เป็น `HAL_ADC_8BIT` — ไม่สามารถเปลี่ยนผ่าน API (แก้ได้ใน source) |
| Impedance สูง | ถ้า source impedance > 10kΩ ต้องใส่ buffer op-amp หรือ capacitor 0.1µF ที่ขา ADC |
| No internal VREF | VREF = AVDD = VCC — ถ้า VCC ไม่ stable ค่า ADC จะเพี้ยนตาม |

## Code สั้น

```c
// อ่านค่า pot ที่ PA2 แบบ 9-bit (0–1023)
hal_adc_init(HAL_ADC_9BIT);
uint16_t raw = hal_adc_read(ADC_CH0);
uint32_t mv = hal_adc_read_mv(ADC_CH0);
printf("raw=%d mV=%d\r\n", raw, mv);

// หรือใช้ Arduino-compat API
analogRead(PA2);  // เรียกครั้งเดียว (init + read)

// หลาย channel auto-switch
uint8_t channels[] = {ADC_CH0, ADC_CH1, ADC_CH2};
ADC_SimpleInitChannels(channels, 3);  // เริ่มสลับอัตโนมัติ
while (1) {
    uint16_t val = ADC_Read(ADC_CH0);    // ค่าล่าสุดของ CH0
    uint16_t val2 = ADC_Read(ADC_CH1);   // ค่าล่าสุดของ CH1
    Delay_Ms(100);
}
```
