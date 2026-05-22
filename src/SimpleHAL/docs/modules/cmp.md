# SimpleHAL — CMP Module

**ไฟล์:** `hal_cmp.h` / `hal_cmp.c`

Comparator (ตัวเปรียบเทียบสัญญาณอนาล็อก) — เปรียบเทียบแรงดันไฟฟ้าระหว่างขา + และ - แล้วให้ผลลัพธ์เป็นดิจิตอล 0 หรือ 1

---

## รูปแบบข้อมูล

### hal_cmp_input_t

```c
typedef enum {
    HAL_CMP_INPUT_PA3_PA2   = 0,  // +PA3, -PA2
    HAL_CMP_INPUT_PA3_VREF  = 1,  // +PA3, -VREF ภายใน
    HAL_CMP_INPUT_PA7_PA2   = 2,  // +PA7, -PA2
    HAL_CMP_INPUT_PA7_VREF  = 3,  // +PA7, -VREF ภายใน
} hal_cmp_input_t;
```

### hal_cmp_vref_t

```c
typedef enum {
    HAL_CMP_VREF_50MV   = 0,   //  50mV
    HAL_CMP_VREF_100MV  = 1,   // 100mV
    // ... เพิ่มครั้งละ 50mV ...
    HAL_CMP_VREF_800MV  = 15,  // 800mV
} hal_cmp_vref_t;
```

### hal_cmp_irq_t

```c
typedef enum {
    HAL_CMP_IRQ_HIGH_LEVEL = 0,  // ระดับสูง
    HAL_CMP_IRQ_LOW_LEVEL  = 1,  // ระดับต่ำ
    HAL_CMP_IRQ_FALLING    = 2,  // ขาลง
    HAL_CMP_IRQ_RISING     = 3,  // ขาขึ้น
} hal_cmp_irq_t;
```

---

## ฟังก์ชัน

### hal_cmp_init

```c
hal_cmp_handle_t hal_cmp_init(hal_cmp_input_t input, hal_cmp_vref_t vref);
```

**คำอธิบาย:** เริ่มต้น Comparator

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `input` | `hal_cmp_input_t` | การเลือกขาเข้า (+) และ (-) |
| `vref` | `hal_cmp_vref_t` | แรงดันอ้างอิงภายใน (50-800mV) — ใช้ได้เมื่อ (-) ต่อกับ VREF |

**ค่าที่คืน:** `hal_cmp_handle_t` — ถ้าสำเร็จ

**ตัวอย่าง:**
```c
hal_cmp_handle_t cmp = hal_cmp_init(HAL_CMP_INPUT_PA7_PA2, HAL_CMP_VREF_500MV);
```

---

### hal_cmp_deinit

```c
void hal_cmp_deinit(hal_cmp_handle_t h);
```

**คำอธิบาย:** ปิด Comparator และล้างสถานะ

---

### hal_cmp_read

```c
uint8_t hal_cmp_read(hal_cmp_handle_t h);
```

**คำอธิบาย:** อ่านค่า output ของ Comparator

**ค่าที่คืน:** `1` ถ้าแรงดัน + > - , `0` ถ้าแรงดัน + < -

**ตัวอย่าง:**
```c
uint8_t val = hal_cmp_read(cmp);
if (val) {
    // แรงดันที่ PA7 สูงกว่า 500mV
} else {
    // แรงดันที่ PA7 ต่ำกว่า 500mV
}
```

---

### hal_cmp_enable / hal_cmp_disable

```c
void hal_cmp_enable(hal_cmp_handle_t h);
void hal_cmp_disable(hal_cmp_handle_t h);
```

**คำอธิบาย:** เปิด/ปิดการทำงานของ Comparator

---

### hal_cmp_route_to_timer_cap

```c
void hal_cmp_route_to_timer_cap(hal_cmp_handle_t h, uint8_t en);
```

**คำอธิบาย:** ส่งสัญญาณ output ของ Comparator ไปยัง Timer capture input (TIM_CAP1)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `en` | `uint8_t` | `1` = เชื่อมต่อ, `0` = ตัดการเชื่อมต่อ |

---

### hal_cmp_attach_irq

```c
void hal_cmp_attach_irq(hal_cmp_handle_t h, hal_cmp_irq_t trigger,
                        hal_callback_t cb, void *arg);
```

**คำอธิบาย:** เปิด interrupt ของ Comparator และแนบ callback

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_cmp_handle_t` | handle |
| `trigger` | `hal_cmp_irq_t` | เงื่อนไข trigger |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | ข้อมูลส่งกลับไปยัง callback |

**ตัวอย่าง:**
```c
void on_cmp_trigger(void *arg) {
    hal_gpio_toggle((hal_gpio_handle_t)arg);
}

hal_cmp_attach_irq(cmp, HAL_CMP_IRQ_RISING, on_cmp_trigger, led);
```

---

### hal_cmp_int_handler

```c
void hal_cmp_int_handler(void);
```

**คำอธิบาย:** ฟังก์ชันจัดการ interrupt — เรียกโดย `CMP_IRQHandler` อัตโนมัติ

---

## ตัวอย่างการต่อวงจร

```
PA7 (CMP_P0) ──┬── สัญญาณที่ต้องการวัด
              R1
              ┷
             3.3V
             
PA2 (CMP_N) ──── สัญญาณอ้างอิงจากภายนอก
             
หรือใช้ VREF ภายใน (50-800mV) โดยไม่ต้องต่อภายนอก
```

---

## ข้อควรระวัง

1. **ADC ใช้ CMP ด้วย** — ถ้าใช้ `hal_adc_read()` และ `hal_cmp` พร้อมกันจะ conflict
2. **เปิด CMP ทิ้งไว้กินกระแส** — ควร `hal_cmp_disable()` เมื่อไม่ใช้งาน
3. **VREF ภายในแม่นยำ ±10%** — สำหรับงานที่ต้องการ precision ควรใช้ reference ภายนอก
4. **Output มี hysteresis ~20mV** — ป้องกัน oscillation ใกล้ threshold
