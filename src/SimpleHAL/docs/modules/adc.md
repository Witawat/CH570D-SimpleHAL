# SimpleHAL — ADC Module

**ไฟล์:** `hal_adc.h` / `hal_adc.c`

อ่านค่าแอนะล็อกผ่าน CMP (Comparator) emulation — รองรับ 4, 8, และ 9 บิต

---

## ฟังก์ชัน

### hal_adc_init

```c
hal_adc_handle_t hal_adc_init(hal_adc_resolution_t res, uint8_t pin);
```

**คำอธิบาย:** เริ่มต้น ADC (ใช้ CMP emulation ของ CH572)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `res` | `hal_adc_resolution_t` | ความละเอียด | `HAL_ADC_4BIT`, `HAL_ADC_8BIT`, `HAL_ADC_9BIT` |
| `pin` | `uint8_t` | ขาที่จะวัด | `PA2` (สำหรับ 8/9 บิต), `PA7` (สำหรับ 4 บิต) |

**ความละเอียด (`hal_adc_resolution_t`):**

| ค่า | บิต | ขาที่ใช้ |
|---|---|---|
| `HAL_ADC_4BIT` | 4 บิต | PA7 |
| `HAL_ADC_8BIT` | 8 บิต | PA2 |
| `HAL_ADC_9BIT` | 9 บิต | PA2 |

**ค่าที่คืน:** `hal_adc_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_adc_handle_t adc = hal_adc_init(HAL_ADC_9BIT, PA2);
```

---

### hal_adc_read

```c
uint16_t hal_adc_read(hal_adc_handle_t h);
```

**คำอธิบาย:** อ่านค่า ADC ดิบ (blocking)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_adc_handle_t` | handle ADC |

**ค่าที่คืน:** ค่า ADC ตามความละเอียด:

| ความละเอียด | ค่าสูงสุด |
|---|---|
| 4 บิต | 0–16 |
| 8 บิต | 0–511 |
| 9 บิต | 0–1023 |

```c
uint16_t raw = hal_adc_read(adc);
```

---

### hal_adc_read_mv

```c
uint32_t hal_adc_read_mv(hal_adc_handle_t h, uint16_t vref_mv);
```

**คำอธิบาย:** อ่านค่า ADC และแปลงเป็นมิลลิโวลต์

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_adc_handle_t` | handle ADC | — |
| `vref_mv` | `uint16_t` | แรงดันอ้างอิง (mV) | เช่น `3300` สำหรับ VDD = 3.3V |

**ค่าที่คืน:** ค่าแรงดันในหน่วย mV

**หมายเหตุ:** คำนวณจาก `value * vref_mv / max_value` โดย `max_value` ขึ้นกับ resolution

```c
uint32_t mv = hal_adc_read_mv(adc, 3300);
```

---

### hal_adc_free

```c
void hal_adc_free(hal_adc_handle_t h);
```

**คำอธิบาย:** คืนหน่วยความจำและทรัพยากร ADC

```c
hal_adc_free(adc);
```
