# SimpleHAL — Timer Module

**ไฟล์:** `hal_timer.h` / `hal_timer.c`

ตั้งเวลาจับเวลา (timer) ทั้งแบบ periodic (วนซ้ำ) และ one-shot (ครั้งเดียว)

---

## ฟังก์ชัน

### hal_timer_init

```c
hal_timer_handle_t hal_timer_init(uint32_t period_us, hal_timer_mode_t mode);
```

**คำอธิบาย:** เริ่มต้น timer ตามคาบเวลาและโหมดที่กำหนด

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `period_us` | `uint32_t` | คาบเวลาในหน่วยไมโครวินาที | 1 – 67,108,864 µs (~67 วินาที) ที่ 100MHz |
| `mode` | `hal_timer_mode_t` | โหมดการทำงาน | `HAL_TIMER_MODE_PERIODIC`, `HAL_TIMER_MODE_ONE_SHOT` |

**โหมด (`hal_timer_mode_t`):**

| ค่า | คำอธิบาย |
|---|---|
| `HAL_TIMER_MODE_PERIODIC` | วนซ้ำทุกครั้งที่ครบคาบ |
| `HAL_TIMER_MODE_ONE_SHOT` | นับครั้งเดียวแล้วหยุด |

**หมายเหตุ:** คาบสูงสุดขึ้นกับความถี่ sysclk — ที่ 100MHz คาบสูงสุดคือ `HAL_TIMER_MAX_PERIOD` (67,108,864 µs)

**ค่าที่คืน:** `hal_timer_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_timer_handle_t tmr = hal_timer_init(1000000, HAL_TIMER_MODE_PERIODIC);
```

---

### hal_timer_start

```c
hal_status_t hal_timer_start(hal_timer_handle_t h);
```

**คำอธิบาย:** เริ่มนับเวลา — timer จะเริ่มนับและเกิด interrupt เมื่อครบคาบ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_timer_handle_t` | handle timer |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้า handle ไม่ถูกต้อง

---

### hal_timer_stop

```c
void hal_timer_stop(hal_timer_handle_t h);
```

**คำอธิบาย:** หยุดนับเวลา สามารถเริ่มต่อได้ด้วย `hal_timer_start()`

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_timer_handle_t` | handle timer |

---

### hal_timer_reset

```c
void hal_timer_reset(hal_timer_handle_t h);
```

**คำอธิบาย:** รีเซ็ตค่านับเป็น 0 (ไม่หยุด timer)

---

### hal_timer_set_period

```c
hal_status_t hal_timer_set_period(hal_timer_handle_t h, uint32_t period_us);
```

**คำอธิบาย:** เปลี่ยนคาบเวลาใหม่ขณะรัน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_timer_handle_t` | handle timer |
| `period_us` | `uint32_t` | คาบเวลาใหม่ (µs) |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้า handle ไม่ถูกต้อง (ถ้าค่าเกินช่วงจะถูกจำกัดอัตโนมัติ)

---

### hal_timer_get_count

```c
uint32_t hal_timer_get_count(hal_timer_handle_t h);
```

**คำอธิบาย:** อ่านค่าปัจจุบันของ timer (จำนวน µs ที่นับไปแล้วตั้งแต่ start หรือ reset ล่าสุด)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_timer_handle_t` | handle timer |

**ค่าที่คืน:** ค่าปัจจุบันในหน่วย µs

---

### hal_timer_attach_cb

```c
void hal_timer_attach_cb(hal_timer_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function — เรียกทุกครั้งที่ timer ครบรอบ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_timer_handle_t` | handle timer |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

**หมายเหตุ:** callback ทำงานใน interrupt context — ควรสั้นและรวดเร็ว

```c
void on_timer(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);
}

hal_timer_attach_cb(tmr, on_timer, led);
hal_timer_start(tmr);
```
