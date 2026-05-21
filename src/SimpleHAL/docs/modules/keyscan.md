# SimpleHAL — KeyScan Module

**ไฟล์:** `hal_keyscan.h` / `hal_keyscan.c`

สแกนเมทริกซ์คีย์บอร์ด (key matrix scan) สำหรับตรวจจับการกดแป้นพิมพ์

---

## ฟังก์ชัน

### hal_keyscan_init

```c
hal_keyscan_handle_t hal_keyscan_init(uint16_t pins, uint8_t clk_div, uint8_t repeat);
```

**คำอธิบาย:** เริ่มต้น KeyScan สำหรับสแกนเมทริกซ์คีย์บอร์ด

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `pins` | `uint16_t` | ขาที่ใช้ scan | `KEYSCAN_PA2`, `KEYSCAN_PA3`, `KEYSCAN_PA8`, `KEYSCAN_PA10`, `KEYSCAN_PA11`, `KEYSCAN_ALL` (รวมทุกขา) |
| `clk_div` | `uint8_t` | ตัวหารความถี่สแกน | `KEYSCAN_DIV1`–`KEYSCAN_DIV16` (ดู `hal_keyscan_div_t`) |
| `repeat` | `uint8_t` | จำนวนรอบยืนยัน | `KEYSCAN_REP1`–`KEYSCAN_REP7` |

**ตัวหารความถี่ (`hal_keyscan_div_t`):**

| ค่า | ตัวหาร |
|---|---|
| `HAL_KEYSCAN_DIV1` | ÷1 |
| `HAL_KEYSCAN_DIV2` | ÷2 |
| `HAL_KEYSCAN_DIV4` | ÷4 |
| `HAL_KEYSCAN_DIV8` | ÷8 |
| `HAL_KEYSCAN_DIV16` | ÷16 |

**ค่าที่คืน:** `hal_keyscan_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_keyscan_handle_t ks = hal_keyscan_init(KEYSCAN_ALL, KEYSCAN_DIV4, KEYSCAN_REP3);
```

---

### hal_keyscan_get_value

```c
uint32_t hal_keyscan_get_value(hal_keyscan_handle_t h);
```

**คำอธิบาย:** อ่านค่าแป้นที่กดล่าสุด

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_keyscan_handle_t` | handle KeyScan |

**ค่าที่คืน:** ค่า 32 บิตแทนแป้นที่กด (bit layout ขึ้นกับการต่อเมทริกซ์)

---

### hal_keyscan_get_count

```c
uint32_t hal_keyscan_get_count(hal_keyscan_handle_t h);
```

**คำอธิบาย:** อ่านจำนวนรอบการสแกนที่เกิดขึ้นแล้ว

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_keyscan_handle_t` | handle KeyScan |

**ค่าที่คืน:** จำนวนรอบการสแกน

---

### hal_keyscan_attach_cb

```c
void hal_keyscan_attach_cb(hal_keyscan_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function — เรียกเมื่อมีการกดแป้น

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_keyscan_handle_t` | handle KeyScan |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

---

### hal_keyscan_enable_wakeup

```c
void hal_keyscan_enable_wakeup(hal_keyscan_handle_t h, uint8_t enable);
```

**คำอธิบาย:** เปิด/ปิดการปลุกจากโหมด sleep เมื่อมีการกดแป้น

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_keyscan_handle_t` | handle KeyScan | — |
| `enable` | `uint8_t` | เปิดหรือปิด wakeup | `0` = ปิด, `1` = เปิด |
