# SimpleHAL — Configuration & Core API Reference

เอกสารนี้อธิบายการตั้งค่าและ API ภายในของ SimpleHAL ที่ผู้ใช้สามารถปรับแต่งหรือเรียกใช้ได้

---

## simple_hal_config.h

ไฟล์คอนฟิกสำหรับปรับพฤติกรรมของ HAL ก่อน compile

| Macro | ค่าเริ่มต้น | คำอธิบาย |
|---|---|---|
| `HAL_UART_RB_SIZE` | `128` | ขนาด ring buffer รับข้อมูล UART (ไบต์) |
| `HAL_UART_TX_RB_SIZE` | `128` | ขนาด ring buffer ส่งข้อมูล UART (ไบต์) |
| `bTXD_0` | `GPIO_Pin_3` | ขา TX default (PA3) |
| `bRXD_0` | `GPIO_Pin_2` | ขา RX default (PA2) |

สามารถ override ค่าได้โดยกำหนด `#define` ก่อน include `simple_hal.h`:

```c
#define HAL_UART_RB_SIZE 256
#include "simple_hal.h"
```

---

## core/hal_types.h

ชนิดข้อมูลพื้นฐานที่ใช้ทั่วทั้ง HAL

### hal_status_t

```c
typedef enum {
    HAL_OK       = 0,   // ดำเนินการสำเร็จ
    HAL_ERROR    = 1,   // เกิดข้อผิดพลาดทั่วไป
    HAL_BUSY     = 2,   // กำลังทำงานหรือ ring buffer เต็ม
    HAL_TIMEOUT  = 3,   // หมดเวลารอ
    HAL_INVALID  = 4,   // พารามิเตอร์ไม่ถูกต้อง
} hal_status_t;
```

ฟังก์ชัน HAL ส่วนใหญ่คืนค่า `hal_status_t` เพื่อบอกสถานะ

### hal_callback_t

```c
typedef void (*hal_callback_t)(void *arg);
```

รูปแบบ callback ที่ใช้ทั่ว HAL:
- `arg` — พอยน์เตอร์ส่งกลับไปยัง callback (用户可以ส่งค่าผ่านพารามิเตอร์ `arg` ตอนแนบ callback)

---

## core/hal_utils.h

Macro ช่วยเหลือทั่วไป

| Macro | รูปแบบ | คำอธิบาย |
|---|---|---|
| `MIN(a, b)` | `(((a) < (b)) ? (a) : (b))` | ค่าน้อยที่สุด |
| `MAX(a, b)` | `(((a) < (b)) ? (b) : (a))` | ค่ามากที่สุด |
| `ABS(x)` | `(((x) < 0) ? -(x) : (x))` | ค่าสัมบูรณ์ |
| `BV(n)` | `(1 << (n))` | Bit value — สร้าง mask บิตที่ n |
| `ARRAY_SIZE(a)` | `(sizeof(a) / sizeof((a)[0]))` | จำนวนสมาชิกในอาร์เรย์ |

---

## core/hal_ringbuf.h — Ring Buffer API

Ring buffer แบบ circular ที่ใช้ภายใน UART สำหรับรับ/ส่งข้อมูลแบบ async

### โครงสร้าง

```c
typedef struct {
    uint8_t *buf;     // พอยน์เตอร์ไปยังบัฟเฟอร์
    uint16_t size;    // ขนาดบัฟเฟอร์
    uint16_t head;    // ตำแหน่งเขียน
    uint16_t tail;    // ตำแหน่งอ่าน
} hal_ringbuf_t;
```

### ฟังก์ชัน

#### hal_ringbuf_init

```c
void hal_ringbuf_init(hal_ringbuf_t *rb, uint8_t *buf, uint16_t size);
```

**คำอธิบาย:** เริ่มต้น ring buffer โดยระบุบัฟเฟอร์และขนาด

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `rb` | `hal_ringbuf_t *` | พอยน์เตอร์ไปยัง struct ring buffer |
| `buf` | `uint8_t *` | พอยน์เตอร์ไปยังอาเรย์ที่จะใช้เป็นบัฟเฟอร์ |
| `size` | `uint16_t` | ขนาดของบัฟเฟอร์ (ไบต์) |

---

#### hal_ringbuf_put

```c
hal_status_t hal_ringbuf_put(hal_ringbuf_t *rb, uint8_t data);
```

**คำอธิบาย:** เพิ่มข้อมูล 1 ไบต์เข้า ring buffer

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `rb` | `hal_ringbuf_t *` | พอยน์เตอร์ ring buffer |
| `data` | `uint8_t` | ข้อมูล 1 ไบต์ที่จะเพิ่ม |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_BUSY` ถ้า ring buffer เต็ม

---

#### hal_ringbuf_get

```c
hal_status_t hal_ringbuf_get(hal_ringbuf_t *rb, uint8_t *data);
```

**คำอธิบาย:** อ่านข้อมูล 1 ไบต์จาก ring buffer (คืนค่าผ่านพอยน์เตอร์)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `rb` | `hal_ringbuf_t *` | พอยน์เตอร์ ring buffer |
| `data` | `uint8_t *` | พอยน์เตอร์รับข้อมูล 1 ไบต์ |

**ค่าที่คืน:** `HAL_OK` ถ้ามีข้อมูล, `HAL_BUSY` ถ้า ring buffer ว่าง

---

#### hal_ringbuf_available

```c
uint16_t hal_ringbuf_available(hal_ringbuf_t *rb);
```

**คำอธิบาย:** ตรวจสอบจำนวนไบต์ที่รออ่านใน ring buffer

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `rb` | `hal_ringbuf_t *` | พอยน์เตอร์ ring buffer |

**ค่าที่คืน:** จำนวนไบต์ที่รออ่าน

---

#### hal_ringbuf_free_space

```c
uint16_t hal_ringbuf_free_space(hal_ringbuf_t *rb);
```

**คำอธิบาย:** ตรวจสอบพื้นที่ว่างใน ring buffer

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `rb` | `hal_ringbuf_t *` | พอยน์เตอร์ ring buffer |

**ค่าที่คืน:** จำนวนไบต์ที่สามารถเขียนเพิ่มได้

---

#### hal_ringbuf_flush

```c
void hal_ringbuf_flush(hal_ringbuf_t *rb);
```

**คำอธิบาย:** ล้าง ring buffer (รีเซ็ต head/tail)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `rb` | `hal_ringbuf_t *` | พอยน์เตอร์ ring buffer |

---

#### hal_ringbuf_is_empty

```c
uint8_t hal_ringbuf_is_empty(hal_ringbuf_t *rb);
```

**คำอธิบาย:** ตรวจสอบว่า ring buffer ว่างหรือไม่

**ค่าที่คืน:** `1` ถ้าว่าง, `0` ถ้าไม่ว่าง

---

#### hal_ringbuf_is_full

```c
uint8_t hal_ringbuf_is_full(hal_ringbuf_t *rb);
```

**คำอธิบาย:** ตรวจสอบว่า ring buffer เต็มหรือไม่

**ค่าที่คืน:** `1` ถ้าเต็ม, `0` ถ้าไม่เต็ม
