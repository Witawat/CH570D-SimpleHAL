# SimpleHAL — UART Module

**ไฟล์:** `hal_uart.h` / `hal_uart.c`

สื่อสารแบบอนุกรม (UART) พร้อม ring buffer สำหรับรับ/ส่งแบบ async

---

## ฟังก์ชัน

### hal_uart_init

```c
hal_uart_handle_t hal_uart_init(uint32_t baudrate, uint8_t tx_remap, uint8_t rx_remap);
```

**คำอธิบาย:** เริ่มต้น UART พร้อม ring buffer และเปิด interrupt รับข้อมูลอัตโนมัติ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `baudrate` | `uint32_t` | อัตราบอด (bps) | `9600`, `115200`, `921600`, ฯลฯ |
| `tx_remap` | `uint8_t` | ขา TX | `UART_DEFAULT_TX_PIN` (= `UART_TX_REMAP_PA3`), `UART_TX_REMAP_PA2` |
| `rx_remap` | `uint8_t` | ขา RX | `UART_DEFAULT_RX_PIN` (= `UART_RX_REMAP_PA2`), `UART_RX_REMAP_PA3` |

**ค่าที่คืน:** `hal_uart_handle_t` — handle ถ้าสำเร็จ, คืน handle เดิมถ้าเรียกซ้ำ

**ตัวอย่าง:**

```c
hal_uart_handle_t uart = hal_uart_init(115200,
    UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
```

---

### hal_uart_send

```c
hal_status_t hal_uart_send(hal_uart_handle_t h, const uint8_t *data, uint16_t len);
```

**คำอธิบาย:** ส่งข้อมูลแบบบล็อก — รอจนส่งครบทุกไบต์ก่อนคืน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `data` | `const uint8_t *` | พอยน์เตอร์ไปยังข้อมูลที่จะส่ง |
| `len` | `uint16_t` | จำนวนไบต์ที่ต้องการส่ง |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้า handle ไม่ถูกต้อง

```c
hal_uart_send(uart, "Hello\r\n", 7);
```

---

### hal_uart_send_byte

```c
hal_status_t hal_uart_send_byte(hal_uart_handle_t h, uint8_t data);
```

**คำอธิบาย:** ส่งข้อมูล 1 ไบต์แบบบล็อก

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `data` | `uint8_t` | 1 ไบต์ที่จะส่ง |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_ERROR` ถ้า handle ไม่ถูกต้อง

---

### hal_uart_send_async

```c
hal_status_t hal_uart_send_async(hal_uart_handle_t h, const uint8_t *data, uint16_t len, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** ส่งข้อมูลแบบไม่บล็อก — ใช้ interrupt + ring buffer โดยฟังก์ชันจะคืนทันทีและส่งข้อมูลต่อใน background

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `data` | `const uint8_t *` | ข้อมูลที่จะส่ง |
| `len` | `uint16_t` | จำนวนไบต์ |
| `cb` | `hal_callback_t` | callback เมื่อส่งครบ (หรือ `NULL` ถ้าไม่ต้องการ) |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_BUSY` ถ้า ring buffer ส่งเต็ม

```c
hal_uart_send_async(uart, data, len, NULL, NULL);
```

---

### hal_uart_recv

```c
hal_status_t hal_uart_recv(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, uint16_t *out_len);
```

**คำอธิบาย:** อ่านข้อมูลจาก ring buffer รับแบบไม่บล็อก — คืนทันที ถ้าไม่มีข้อมูลจะคืน `HAL_BUSY`

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `data` | `uint8_t *` | บัฟเฟอร์สำหรับเก็บข้อมูลที่อ่าน |
| `max_len` | `uint16_t` | ขนาดสูงสุดที่อ่านได้ |
| `out_len` | `uint16_t *` | ตัวแปรรับจำนวนไบต์ที่อ่านได้จริง (หรือ `NULL`) |

**ค่าที่คืน:** `HAL_OK` ถ้ามีข้อมูล, `HAL_BUSY` ถ้าไม่มีข้อมูลใน ring buffer

```c
uint8_t buf[64];
uint16_t len;
if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {
    // มีข้อมูล len ไบต์ใน buf
}
```

---

### hal_uart_recv_byte

```c
uint8_t hal_uart_recv_byte(hal_uart_handle_t h);
```

**คำอธิบาย:** อ่าน 1 ไบต์จาก ring buffer **คืนค่า 0 ถ้าไม่มีข้อมูล** (ต้องตรวจสอบ `hal_uart_available()` ก่อน)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |

**ค่าที่คืน:** ข้อมูล 1 ไบต์ หรือ `0` ถ้าไม่มีข้อมูล

---

### hal_uart_recv_async

```c
hal_status_t hal_uart_recv_async(hal_uart_handle_t h, uint8_t *data, uint16_t max_len, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** รับข้อมูลแบบไม่บล็อก — callback จะถูกเรียกเมื่อได้รับข้อมูลครบตาม `max_len` ไบต์

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `data` | `uint8_t *` | บัฟเฟอร์สำหรับเก็บข้อมูล |
| `max_len` | `uint16_t` | จำนวนไบต์ที่ต้องการรับ |
| `cb` | `hal_callback_t` | callback เมื่อได้ข้อมูลครบ |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_BUSY` ถ้ากำลังรับ async อยู่แล้ว

```c
uint8_t rx_buf[20];
void on_rx_done(void *arg) {
    // rx_buf มี 20 ไบต์แล้ว
}
hal_uart_recv_async(uart, rx_buf, 20, on_rx_done, NULL);
```

---

### hal_uart_available

```c
uint16_t hal_uart_available(hal_uart_handle_t h);
```

**คำอธิบาย:** ตรวจสอบจำนวนไบต์ที่รออ่านใน ring buffer

**ค่าที่คืน:** จำนวนไบต์ที่รออ่าน

```c
if (hal_uart_available(uart) > 0) {
    // มีข้อมูลให้อ่าน
}
```

---

### hal_uart_flush

```c
void hal_uart_flush(hal_uart_handle_t h);
```

**คำอธิบาย:** ล้าง ring buffer ทั้งรับและส่ง (ข้อมูลที่ยังไม่ได้อ่านจะหาย)

---

### hal_uart_set_trig_bytes

```c
void hal_uart_set_trig_bytes(hal_uart_handle_t h, uint8_t bytes);
```

**คำอธิบาย:** ตั้งค่าจำนวนไบต์ที่จะ trigger interrupt รับข้อมูล ช่วยลด overhead ของ interrupt

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART | — |
| `bytes` | `uint8_t` | จำนวนไบต์ trigger | `1`, `2`, `4`, `7` |

---

### hal_uart_attach_rx_cb

```c
void hal_uart_attach_rx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อมีข้อมูลขาเข้า — callback จะถูกเรียกทุกครั้งที่มีข้อมูลเข้า ring buffer

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

```c
void on_rx(void *arg) {
    hal_uart_handle_t u = (hal_uart_handle_t)arg;
    // อ่านข้อมูลจาก ring buffer
}
hal_uart_attach_rx_cb(uart, on_rx, uart);
```

---

### hal_uart_attach_tx_cb

```c
void hal_uart_attach_tx_cb(hal_uart_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อส่งข้อมูล async เสร็จ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_uart_handle_t` | handle UART |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

---

### hal_uart_int_handler (Internal)

```c
void hal_uart_int_handler(void);
```

**คำอธิบาย:** ตัวจัดการ interrupt ภายใน — ลงทะเบียนเป็น `UART_IRQHandler` โดยอัตโนมัติ ไม่ควรเรียกจากโค้ดผู้ใช้โดยตรง
