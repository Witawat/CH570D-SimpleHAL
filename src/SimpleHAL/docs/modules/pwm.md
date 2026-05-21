# SimpleHAL — PWM Module

**ไฟล์:** `hal_pwm.h` / `hal_pwm.c`

สร้างสัญญาณ PWM 5 ช่อง รองรับทั้ง 8 บิต และ 16 บิต (ความละเอียดสูง)

---

## ฟังก์ชัน

### hal_pwm_init

```c
hal_pwm_handle_t hal_pwm_init(hal_pwm_channel_t ch, uint32_t freq_hz, uint8_t duty_pct);
```

**คำอธิบาย:** เริ่มต้น PWM แบบ 8 บิต (duty cycle 0–100%)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `ch` | `hal_pwm_channel_t` | ช่อง PWM | `HAL_PWM_CH1`–`HAL_PWM_CH5` |
| `freq_hz` | `uint32_t` | ความถี่ (Hz) | ขึ้นกับ sysclk (เช่น 1Hz–MHz) |
| `duty_pct` | `uint8_t` | รอบทำงาน | `0`–`100` (เปอร์เซ็นต์) |

**ช่อง (`hal_pwm_channel_t`):**

| ค่า | คำอธิบาย |
|---|---|
| `HAL_PWM_CH1` | PWM ช่อง 1 |
| `HAL_PWM_CH2` | PWM ช่อง 2 |
| `HAL_PWM_CH3` | PWM ช่อง 3 |
| `HAL_PWM_CH4` | PWM ช่อง 4 |
| `HAL_PWM_CH5` | PWM ช่อง 5 |

**ค่าที่คืน:** `hal_pwm_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ (เช่น ช่องซ้ำ)

**ตัวอย่าง:**

```c
hal_pwm_handle_t pwm = hal_pwm_init(HAL_PWM_CH1, 1000, 50);
```

---

### hal_pwm_init_16bit

```c
hal_pwm_handle_t hal_pwm_init_16bit(hal_pwm_channel_t ch, uint32_t freq_hz, uint16_t duty);
```

**คำอธิบาย:** เริ่มต้น PWM แบบ 16 บิต (ความละเอียดสูงกว่า 8 บิต)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `ch` | `hal_pwm_channel_t` | ช่อง PWM | `HAL_PWM_CH1`–`HAL_PWM_CH5` |
| `freq_hz` | `uint32_t` | ความถี่ (Hz) | — |
| `duty` | `uint16_t` | ค่ารอบทำงาน | `0`–`65535` (ขึ้นกับ cycle ที่คำนวณจาก freq) |

**ค่าที่คืน:** `hal_pwm_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

---

### hal_pwm_set_duty

```c
void hal_pwm_set_duty(hal_pwm_handle_t h, uint8_t duty_pct);
```

**คำอธิบาย:** เปลี่ยนรอบทำงานแบบ 8 บิต

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_pwm_handle_t` | handle PWM | — |
| `duty_pct` | `uint8_t` | รอบทำงานเปอร์เซ็นต์ | `0`–`100` |

---

### hal_pwm_set_duty_16bit

```c
void hal_pwm_set_duty_16bit(hal_pwm_handle_t h, uint16_t duty);
```

**คำอธิบาย:** เปลี่ยนรอบทำงานแบบ 16 บิต

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_pwm_handle_t` | handle PWM | — |
| `duty` | `uint16_t` | ค่ารอบทำงาน | `0`–`65535` |

---

### hal_pwm_set_freq

```c
void hal_pwm_set_freq(hal_pwm_handle_t h, uint32_t freq_hz);
```

**คำอธิบาย:** เปลี่ยนความถี่ PWM ขณะรัน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_pwm_handle_t` | handle PWM |
| `freq_hz` | `uint32_t` | ความถี่ใหม่ (Hz) |

---

### hal_pwm_start

```c
void hal_pwm_start(hal_pwm_handle_t h);
```

**คำอธิบาย:** เริ่มส่งสัญญาณ PWM ออกขา

---

### hal_pwm_stop

```c
void hal_pwm_stop(hal_pwm_handle_t h);
```

**คำอธิบาย:** หยุดส่งสัญญาณ PWM
