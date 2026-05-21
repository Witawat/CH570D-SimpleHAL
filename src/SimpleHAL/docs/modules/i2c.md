# SimpleHAL — I2C Module

**ไฟล์:** `hal_i2c.h` / `hal_i2c.c`

สื่อสาร I2C ทั้งแบบ master และ slave

---

## ฟังก์ชัน

### hal_i2c_init

```c
hal_i2c_handle_t hal_i2c_init(uint32_t clock_hz, hal_i2c_role_t role, uint16_t own_addr);
```

**คำอธิบาย:** เริ่มต้น I2C

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `clock_hz` | `uint32_t` | ความถี่ I2C (Hz) | `100000` (standard), `400000` (fast) |
| `role` | `hal_i2c_role_t` | บทบาท | `HAL_I2C_MASTER`, `HAL_I2C_SLAVE` |
| `own_addr` | `uint16_t` | ที่อยู่ของตัวเอง | 7-bit address (ใช้ `0` สำหรับ master) |

**บทบาท (`hal_i2c_role_t`):**

| ค่า | คำอธิบาย |
|---|---|
| `HAL_I2C_MASTER` | โหมด master — ควบคุมการสื่อสาร |
| `HAL_I2C_SLAVE` | โหมด slave — รอการเรียกจาก master |

**ค่าที่คืน:** `hal_i2c_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_i2c_handle_t i2c = hal_i2c_init(400000, HAL_I2C_MASTER, 0);
```

---

### hal_i2c_write

```c
hal_status_t hal_i2c_write(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
```

**คำอธิบาย:** เขียนข้อมูลไปยัง register ของอุปกรณ์ I2C — ส่ง `dev_addr` + `reg` + `data`

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C |
| `dev_addr` | `uint8_t` | ที่อยู่ 7-bit ของอุปกรณ์ (shifted left) |
| `reg` | `uint8_t` | ที่อยู่ register ภายในอุปกรณ์ |
| `data` | `const uint8_t *` | ข้อมูลที่จะเขียน |
| `len` | `uint16_t` | จำนวนไบต์ |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้าสื่อสารล้มเหลว

```c
uint8_t val = 0x42;
hal_i2c_write(i2c, 0x3C, 0x01, &val, 1);
```

---

### hal_i2c_read

```c
hal_status_t hal_i2c_read(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
```

**คำอธิบาย:** อ่านข้อมูลจาก register ของอุปกรณ์ I2C — ส่ง `dev_addr` + `reg` จากนั้นรับ `data` ใช้ **RESTART (repeated start)** แทน STOP+START เพื่อความเข้ากันได้กับอุปกรณ์บางประเภท

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C |
| `dev_addr` | `uint8_t` | ที่อยู่ 7-bit ของอุปกรณ์ |
| `reg` | `uint8_t` | ที่อยู่ register |
| `data` | `uint8_t *` | บัฟเฟอร์รับข้อมูล |
| `len` | `uint16_t` | จำนวนไบต์ที่ต้องการอ่าน |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้าสื่อสารล้มเหลว

---

### hal_i2c_write_raw

```c
hal_status_t hal_i2c_write_raw(hal_i2c_handle_t h, uint8_t dev_addr, const uint8_t *data, uint16_t len);
```

**คำอธิบาย:** เขียนข้อมูลตรงไปยังอุปกรณ์ I2C โดยไม่มี register address — ส่ง `dev_addr` + `data` เท่านั้น

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C |
| `dev_addr` | `uint8_t` | ที่อยู่ 7-bit ของอุปกรณ์ |
| `data` | `const uint8_t *` | ข้อมูลที่จะเขียน |
| `len` | `uint16_t` | จำนวนไบต์ |

**ค่าที่คืน:** `HAL_OK` หรือ `HAL_ERROR`

---

### hal_i2c_read_raw

```c
hal_status_t hal_i2c_read_raw(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t *data, uint16_t len);
```

**คำอธิบาย:** อ่านข้อมูลตรงจากอุปกรณ์ I2C โดยไม่มี register address

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C |
| `dev_addr` | `uint8_t` | ที่อยู่ 7-bit ของอุปกรณ์ |
| `data` | `uint8_t *` | บัฟเฟอร์รับข้อมูล |
| `len` | `uint16_t` | จำนวนไบต์ |

**ค่าที่คืน:** `HAL_OK` หรือ `HAL_ERROR`

---

### hal_i2c_mem_write

```c
hal_status_t hal_i2c_mem_write(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, const uint8_t *data, uint16_t len);
```

**คำอธิบาย:** เขียนข้อมูลไปยังที่อยู่หน่วยความจำของอุปกรณ์ (เช่น EEPROM 24Cxx) รองรับที่อยู่ 16 บิต

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C | — |
| `dev_addr` | `uint8_t` | ที่อยู่ 7-bit ของอุปกรณ์ | — |
| `mem_addr` | `uint16_t` | ที่อยู่หน่วยความจำภายในอุปกรณ์ | 0–65535 |
| `addr_len` | `uint8_t` | จำนวนไบต์ของที่อยู่ | `1` (8-bit address), `2` (16-bit address) |
| `data` | `const uint8_t *` | ข้อมูลที่จะเขียน | — |
| `len` | `uint16_t` | จำนวนไบต์ | — |

**ค่าที่คืน:** `HAL_OK` หรือ `HAL_ERROR`

---

### hal_i2c_mem_read

```c
hal_status_t hal_i2c_mem_read(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, uint8_t *data, uint16_t len);
```

**คำอธิบาย:** อ่านข้อมูลจากที่อยู่หน่วยความจำของอุปกรณ์ (เช่น EEPROM) ใช้ **RESTART (repeated start)** เช่นเดียวกับ `hal_i2c_read`

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C |
| `dev_addr` | `uint8_t` | ที่อยู่ 7-bit ของอุปกรณ์ |
| `mem_addr` | `uint16_t` | ที่อยู่หน่วยความจำ |
| `addr_len` | `uint8_t` | จำนวนไบต์ของที่อยู่ (1 หรือ 2) |
| `data` | `uint8_t *` | บัฟเฟอร์รับข้อมูล |
| `len` | `uint16_t` | จำนวนไบต์ |

**ค่าที่คืน:** `HAL_OK` หรือ `HAL_ERROR`

---

### hal_i2c_set_speed

```c
void hal_i2c_set_speed(hal_i2c_handle_t h, uint32_t clock_hz);
```

**คำอธิบาย:** เปลี่ยนความถี่ I2C ขณะรัน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_i2c_handle_t` | handle I2C |
| `clock_hz` | `uint32_t` | ความถี่ใหม่ (100000 หรือ 400000) |
