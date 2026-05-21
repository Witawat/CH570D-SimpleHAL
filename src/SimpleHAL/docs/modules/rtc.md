# SimpleHAL — RTC Module

**ไฟล์:** `hal_rtc.h` / `hal_rtc.c`

Real-Time Clock — จับเวลาจริง ตั้งปลุก ตั้ง timer

---

## ชนิดข้อมูล

### hal_rtc_time_t

```c
typedef struct {
    uint16_t year;    // ปี (เช่น 2026)
    uint16_t month;   // เดือน (1–12)
    uint16_t day;     // วัน (1–31)
    uint16_t hour;    // ชั่วโมง (0–23)
    uint16_t minute;  // นาที (0–59)
    uint16_t second;  // วินาที (0–59)
} hal_rtc_time_t;
```

---

## ฟังก์ชัน

### hal_rtc_init

```c
hal_rtc_handle_t hal_rtc_init(RTC_OSCCntTypeDef osc_cycles);
```

**คำอธิบาย:** เริ่มต้น RTC พร้อม calibrate LSI oscillator

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `osc_cycles` | `RTC_OSCCntTypeDef` | จำนวนรอบ osc ที่ใช้ calibrate LSI | `Count_1`, `Count_2`, `Count_4`, `Count_8`, `Count_16`, `Count_32`, `Count_64`, `Count_128`, `Count_256` |

**ค่าที่คืน:** `hal_rtc_handle_t` — ถ้าสำเร็จ, `NULL` ถ้าไม่สำเร็จ

**ตัวอย่าง:**

```c
hal_rtc_handle_t rtc = hal_rtc_init(Count_1);
```

---

### hal_rtc_set_time

```c
hal_status_t hal_rtc_set_time(hal_rtc_handle_t h, const hal_rtc_time_t *time);
```

**คำอธิบาย:** ตั้งค่าเวลาปัจจุบันของ RTC

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rtc_handle_t` | handle RTC |
| `time` | `const hal_rtc_time_t *` | โครงสร้างเวลาที่ต้องการตั้ง |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้าค่าเวลาผิด

```c
hal_rtc_time_t t = {2026, 5, 16, 10, 30, 0};
hal_rtc_set_time(rtc, &t);
```

---

### hal_rtc_get_time

```c
hal_status_t hal_rtc_get_time(hal_rtc_handle_t h, hal_rtc_time_t *time);
```

**คำอธิบาย:** อ่านเวลาปัจจุบันจาก RTC

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rtc_handle_t` | handle RTC |
| `time` | `hal_rtc_time_t *` | พอยน์เตอร์รับค่าเวลา |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ, `HAL_INVALID` ถ้า handle ไม่ถูกต้อง

```c
hal_rtc_time_t now;
hal_rtc_get_time(rtc, &now);
printf("%04d-%02d-%02d %02d:%02d:%02d\n",
    now.year, now.month, now.day,
    now.hour, now.minute, now.second);
```

---

### hal_rtc_set_trigger

```c
hal_status_t hal_rtc_set_trigger(hal_rtc_handle_t h, uint32_t cycles);
```

**คำอธิบาย:** ตั้งค่า trigger — จะเกิด interrupt เมื่อ LSI cycle count ถึงค่าที่กำหนด

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rtc_handle_t` | handle RTC |
| `cycles` | `uint32_t` | จำนวน cycles LSI ที่จะ trigger |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ

---

### hal_rtc_set_timer

```c
hal_status_t hal_rtc_set_timer(hal_rtc_handle_t h, RTC_TMRCycTypeDef period);
```

**คำอธิบาย:** ตั้งค่า timer RTC แบบคาบเวลา (ให้ใช้ `RTC_TMRCycTypeDef` จาก StdPeriphDriver)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rtc_handle_t` | handle RTC |
| `period` | `RTC_TMRCycTypeDef` | คาบเวลา (ดูใน `CH57x_RTC.h`) |

**ค่าที่คืน:** `HAL_OK` ถ้าสำเร็จ

---

### hal_rtc_attach_cb

```c
void hal_rtc_attach_cb(hal_rtc_handle_t h, hal_callback_t cb, void *arg);
```

**คำอธิบาย:** แนบ callback function เมื่อ RTC timer หรือ trigger ครบ

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rtc_handle_t` | handle RTC |
| `cb` | `hal_callback_t` | ฟังก์ชัน callback |
| `arg` | `void *` | อาร์กิวเมนต์ส่งไปยัง callback |

---

### hal_rtc_get_cycle_lsi

```c
uint32_t hal_rtc_get_cycle_lsi(hal_rtc_handle_t h);
```

**คำอธิบาย:** อ่านค่า cycle LSI ปัจจุบัน

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `h` | `hal_rtc_handle_t` | handle RTC |

**ค่าที่คืน:** จำนวน cycle LSI ปัจจุบัน
