# SimpleHAL — GPIO Module

**ไฟล์:** `hal_gpio.h` / `hal_gpio.c`

จัดการขา I/O ดิจิตอล พร้อม interrupt callback

---

## ฟังก์ชัน

### hal_gpio_init

```c
hal_gpio_handle_t hal_gpio_init(uint8_t pin, hal_gpio_mode_t mode);
```

**คำอธิบาย:** เริ่มต้น GPIO ขาเดียว กำหนดโหมดการทำงาน ฟังก์ชันนี้จะ reserve ขาไม่ให้ซ้ำ (คืนค่า `NULL` ถ้าขานั้นถูก init แล้ว)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `pin` | `uint8_t` | หมายเลขขาสัญญาณ | `PA0`(0)–`PA15`(15) |
| `mode` | `hal_gpio_mode_t` | โหมด GPIO | ดูตารางด้านล่าง |

**โหมด (`hal_gpio_mode_t`):**

| ค่า | คำอธิบาย |
|---|---|
| `HAL_GPIO_INPUT_FLOATING` | อินพุตลอย (ไม่ pull) |
| `HAL_GPIO_INPUT_PULLUP` | อินพุต pull-up ภายใน |
| `HAL_GPIO_INPUT_PULLDOWN` | อินพุต pull-down ภายใน |
| `HAL_GPIO_OUTPUT_PP_5mA` | เอาต์พุต push-pull 5mA |
| `HAL_GPIO_OUTPUT_PP_20mA` | เอาต์พุต push-pull 20mA |

**ค่าที่คืน:** `hal_gpio_handle_t` — handle ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ (pin ซ้ำ หรือพารามิเตอร์ผิด)

**ตัวอย่าง:**

```c
hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
hal_gpio_handle_t btn = hal_gpio_init(PA5, HAL_GPIO_INPUT_PULLUP);
```

---

### hal_gpio_write

```c
void hal_gpio_write(hal_gpio_handle_t h, uint8_t val);
```

**คำอธิบาย:** เขียนค่าลอจิกไปยังขา GPIO (ใช้ได้กับขา output เท่านั้น)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_gpio_handle_t` | handle จาก `hal_gpio_init` | — |
| `val` | `uint8_t` | ค่าลอจิก | `0` ( LOW), `1` (HIGH) |

**ตัวอย่าง:**

```c
hal_gpio_write(led, 1);  // ติด LED
hal_gpio_write(led, 0);  // ดับ LED
```

---

### hal_gpio_read

```c
uint8_t hal_gpio_read(hal_gpio_handle_t h);
```

**คำอธิบาย:** อ่านค่าลอจิกปัจจุบันจากขา GPIO

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_gpio_handle_t` | handle จาก `hal_gpio_init` |

**ค่าที่คืน:** `0` หรือ `1`

```c
uint8_t val = hal_gpio_read(btn);
```

---

### hal_gpio_toggle

```c
void hal_gpio_toggle(hal_gpio_handle_t h);
```

**คำอธิบาย:** กลับค่าขา GPIO (0→1, 1→0)

```c
hal_gpio_toggle(led);  // กระพริบ LED
```

---

### hal_gpio_set

```c
void hal_gpio_set(hal_gpio_handle_t h);
```

**คำอธิบาย:** ตั้งค่าขาเป็น 1 (HIGH)

---

### hal_gpio_reset

```c
void hal_gpio_reset(hal_gpio_handle_t h);
```

**คำอธิบาย:** ตั้งค่าขาเป็น 0 (LOW)

---

### hal_gpio_attach_irq

```c
hal_status_t hal_gpio_attach_irq(hal_gpio_handle_t h, hal_gpio_irq_mode_t mode, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function สำหรับ interrupt เมื่อเกิดเหตุการณ์ที่ขา GPIO ที่กำหนด ฟังก์ชันนี้จะ enable interrupt และ register handler อัตโนมัติ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_gpio_handle_t` | handle GPIO |
| `mode` | `hal_gpio_irq_mode_t` | โหมด trigger interrupt |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback (`void callback(void *arg)`) |
| `arg` | `void *` | พอยน์เตอร์ส่งไปยัง callback (หรือ `NULL`) |

**โหมด interrupt (`hal_gpio_irq_mode_t`):**

| ค่า | เหตุการณ์ |
|---|---|
| `HAL_GPIO_IRQ_LOW_LEVEL` | ระดับต่ำ ( LOW level) |
| `HAL_GPIO_IRQ_HIGH_LEVEL` | ระดับสูง (HIGH level) |
| `HAL_GPIO_IRQ_FALLING` | ขาลง (1→0) |
| `HAL_GPIO_IRQ_RISING` | ขาขึ้น (0→1) |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้า h ไม่ถูกต้อง

**หมายเหตุ:** callback **ห้าม** มี `hal_delay_ms` หรือ blocking operation — ควรสั้นและรวดเร็ว

**ตัวอย่าง:**

```c
void on_btn_press(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);
}

hal_gpio_attach_irq(btn, HAL_GPIO_IRQ_FALLING, on_btn_press, led);
```

---

### hal_gpio_detach_irq

```c
void hal_gpio_detach_irq(hal_gpio_handle_t h);
```

**คำอธิบาย:** ยกเลิกการแนบ callback interrupt สำหรับขา GPIO

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_gpio_handle_t` | handle GPIO |

---

### hal_gpio_int_handler (Internal)

```c
void hal_gpio_int_handler(void);
```

**คำอธิบาย:** ตัวจัดการ interrupt ภายใน — ลงทะเบียนเป็น `GPIOA_IRQHandler` โดยอัตโนมัติ ไม่ควรเรียกจากโค้ดผู้ใช้โดยตรง
