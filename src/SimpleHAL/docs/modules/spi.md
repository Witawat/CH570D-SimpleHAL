# SimpleHAL — SPI Module

**ไฟล์:** `hal_spi.h` / `hal_spi.c`

สื่อสาร SPI แบบ master ทั้งแบบ polling และ DMA

---

## ฟังก์ชัน

### hal_spi_init

```c
hal_spi_handle_t hal_spi_init(uint32_t clock_hz, hal_spi_mode_t mode, hal_spi_bit_order_t order);
```

**คำอธิบาย:** เริ่มต้น SPI ในโหมด master

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `clock_hz` | `uint32_t` | ความถี่ SPI (Hz) | สูงสุด ~50MHz ที่ sysclk 100MHz |
| `mode` | `hal_spi_mode_t` | โหมด SPI | `HAL_SPI_MODE0`, `HAL_SPI_MODE3` |
| `order` | `hal_spi_bit_order_t` | ลำดับบิต | `HAL_SPI_MSB_FIRST`, `HAL_SPI_LSB_FIRST` |

**โหมด SPI (`hal_spi_mode_t`):**

| ค่า | CPOL | CPHA | คำอธิบาย |
|---|---|---|---|
| `HAL_SPI_MODE0` | 0 | 0 | SCK idle LOW, sample on rising edge |
| `HAL_SPI_MODE3` | 1 | 1 | SCK idle HIGH, sample on falling edge |

**ลำดับบิต (`hal_spi_bit_order_t`):**

| ค่า | คำอธิบาย |
|---|---|
| `HAL_SPI_MSB_FIRST` | ส่งบิตที่มีนัยสำคัญสูงสุดก่อน |
| `HAL_SPI_LSB_FIRST` | ส่งบิตที่มีนัยสำคัญต่ำสุดก่อน |

**ค่าที่คืน:** `hal_spi_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_spi_handle_t spi = hal_spi_init(8000000, HAL_SPI_MODE0, HAL_SPI_MSB_FIRST);
```

---

### hal_spi_transfer

```c
hal_status_t hal_spi_transfer(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len);
```

**คำอธิบาย:** ส่งและรับข้อมูลพร้อมกันแบบ full-duplex (blocking)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_spi_handle_t` | handle SPI |
| `tx` | `const uint8_t *` | ข้อมูลที่จะส่ง (หรือ `NULL` ถ้าต้องการส่ง dummy 0xFF) |
| `rx` | `uint8_t *` | บัฟเฟอร์รับข้อมูล (หรือ `NULL` ถ้าไม่ต้องการรับ) |
| `len` | `uint16_t` | จำนวนไบต์ |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้าพารามิเตอร์ผิด

```c
uint8_t tx_data[] = {0xAA, 0xBB, 0xCC};
uint8_t rx_data[3];
hal_spi_transfer(spi, tx_data, rx_data, 3);
```

---

### hal_spi_transfer_dma

```c
hal_status_t hal_spi_transfer_dma(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** ส่ง/รับข้อมูลผ่าน DMA แบบไม่บล็อก — callback เมื่อเสร็จ

**หมายเหตุ:** DMA รองรับเฉพาะส่งอย่างเดียว (`rx = NULL`) หรือรับอย่างเดียว (`tx = NULL`) สำหรับ full-duplex จะคืน `HAL_BUSY` — ผู้ใช้ต้องใช้ `hal_spi_transfer` แบบ polling แทน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_spi_handle_t` | handle SPI |
| `tx` | `const uint8_t *` | ข้อมูลส่ง (หรือ `NULL` สำหรับรับอย่างเดียว) |
| `rx` | `uint8_t *` | บัฟเฟอร์รับ (หรือ `NULL` สำหรับส่งอย่างเดียว) |
| `len` | `uint16_t` | จำนวนไบต์ |
| `cb` | `hal_callback_t` | callback เมื่อเสร็จ (หรือ `NULL`) |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_BUSY` ถ้ากำลัง DMA อยู่แล้ว

```c
void on_spi_done(void *arg) {
    // DMA transfer เสร็จ
}
hal_spi_transfer_dma(spi, tx_buf, NULL, 256, on_spi_done, NULL);
```

---

### hal_spi_send_byte

```c
hal_status_t hal_spi_send_byte(hal_spi_handle_t h, uint8_t data);
```

**คำอธิบาย:** ส่ง 1 ไบต์ผ่าน SPI (blocking)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_spi_handle_t` | handle SPI |
| `data` | `uint8_t` | 1 ไบต์ที่จะส่ง |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้า handle ไม่ถูกต้อง

---

### hal_spi_recv_byte

```c
uint8_t hal_spi_recv_byte(hal_spi_handle_t h);
```

**คำอธิบาย:** รับ 1 ไบต์ (ส่ง 0xFF พร้อมกัน) — blocking

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_spi_handle_t` | handle SPI |

**ค่าที่คืน:** ข้อมูล 1 ไบต์ที่รับได้

---

### hal_spi_set_speed

```c
void hal_spi_set_speed(hal_spi_handle_t h, uint32_t clock_hz);
```

**คำอธิบาย:** เปลี่ยนความถี่ SPI ขณะรัน (ไม่ต้อง init ใหม่)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_spi_handle_t` | handle SPI |
| `clock_hz` | `uint32_t` | ความถี่ใหม่ (Hz) |

---

### hal_spi_chip_select

```c
void hal_spi_chip_select(hal_spi_handle_t h, uint8_t level);
```

**คำอธิบาย:** ควบคุมขา Chip Select (CS = PA15)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_spi_handle_t` | handle SPI | — |
| `level` | `uint8_t` | ระดับสัญญาณ CS | `0` = active LOW (select), `1` = deselect |

```c
hal_spi_chip_select(spi, 0);  // เลือกอุปกรณ์
hal_spi_transfer(spi, tx, rx, len);
hal_spi_chip_select(spi, 1);  // ยกเลิกการเลือก
```
