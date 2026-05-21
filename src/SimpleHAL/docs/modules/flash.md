# SimpleHAL — Flash Module

**ไฟล์:** `hal_flash.h` / `hal_flash.c`

อ่านข้อมูลจาก Flash ROM และอ่านค่าเฉพาะของชิป (อ่านอย่างเดียว ไม่มีการเขียน)

---

## ฟังก์ชัน

### hal_flash_read

```c
hal_status_t hal_flash_read(uint32_t addr, void *buf, uint32_t len);
```

**คำอธิบาย:** อ่านข้อมูลจาก Flash ROM ที่ตำแหน่ง addr

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `addr` | `uint32_t` | ที่อยู่เริ่มต้นใน Flash (เช่น `0x00000000`) |
| `buf` | `void *` | บัฟเฟอร์สำหรับเก็บข้อมูลที่อ่าน |
| `len` | `uint32_t` | จำนวนไบต์ที่ต้องการอ่าน |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้าที่อยู่ไม่ถูกต้อง

```c
uint8_t data[64];
hal_flash_read(0x00000000, data, 64);  // อ่าน 64 ไบต์แรก
```

---

### hal_flash_get_unique_id

```c
hal_status_t hal_flash_get_unique_id(uint8_t buf[8]);
```

**คำอธิบาย:** อ่านหมายเลข ID เฉพาะของชิป (unique ID) ขนาด 8 ไบต์

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `buf` | `uint8_t[8]` | อาเรย์ขนาด 8 ไบต์สำหรับรับ ID |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ

```c
uint8_t uid[8];
hal_flash_get_unique_id(uid);
```

---

### hal_flash_read_config

```c
hal_status_t hal_flash_read_config(uint16_t *config_val);
```

**คำอธิบาย:** อ่านค่า configuration word จาก Flash (ใช้กำหนดค่าเริ่มต้นของชิป เช่น boot behavior)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `config_val` | `uint16_t *` | พอยน์เตอร์รับค่า config word |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ
