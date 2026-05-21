# SimpleHAL — System Module

**ไฟล์:** `hal_sys.h` / `hal_sys.c`

ฟังก์ชันระบบทั่วไป — delay, รีเซ็ต, SysTick, Watchdog

---

## ฟังก์ชัน

### hal_delay_ms

```c
void hal_delay_ms(uint16_t ms);
```

**คำอธิบาย:** หน่วงเวลาแบบ **blocking** ในหน่วยมิลลิวินาที (ใช้ busy-wait)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `ms` | `uint16_t` | เวลาที่ต้องการหน่วง (ms) | `0`–`65535` |

**ข้อควรระวัง:** ฟังก์ชันนี้ **blocking** — CPU หยุดทำงานตามเวลาที่กำหนด ไม่ควรใช้ใน callback interrupt

```c
hal_delay_ms(1000);  // หน่วง 1 วินาที
```

---

### hal_delay_us

```c
void hal_delay_us(uint16_t us);
```

**คำอธิบาย:** หน่วงเวลาแบบ **blocking** ในหน่วยไมโครวินาที

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `us` | `uint16_t` | เวลาที่ต้องการหน่วง (µs) |

```c
hal_delay_us(10);  // หน่วง 10 ไมโครวินาที
```

---

### hal_get_sys_tick

```c
uint32_t hal_get_sys_tick(void);
```

**คำอธิบาย:** อ่านค่า SysTick counter ปัจจุบันของ RISC-V core

SysTick เป็น down-counter: ค่าจะลดลงเรื่อยๆ เมื่อเวลาผ่านไป (ที่ 100MHz ลดลง ~100 ล้านครั้ง/วินาที)

**ค่าที่คืน:** ค่า SysTick register ปัจจุบัน (uint32_t, 32-bit แบบไม่มีเครื่องหมาย)

**การใช้งาน Non-blocking delay:**
เนื่องจาก `hal_delay_ms()` เป็น blocking (CPU หยุด) จึงไม่เหมาะกับ:
- callback interrupt
- โปรแกรมที่ต้องทำงานหลายอย่างพร้อมกัน (BLE + UART + sensor)
- ระบบที่ต้องการ response time ต่ำ

ทางเลือก: ใช้ `hal_get_sys_tick()` + state machine

```c
// Non-blocking delay pattern ด้วย hal_get_sys_tick()
// (ดูตัวอย่างเต็มใน examples/14_nonblock_delay/)

typedef struct {
    uint8_t  active;
    uint32_t start_tick;
    uint32_t delay_us;
} soft_timer_t;

static soft_timer_t timer;

// เริ่ม delay (ไม่บล็อก — CPU ทำงานต่อได้ทันที)
void start_delay(uint32_t us) {
    timer.start_tick = hal_get_sys_tick();
    timer.delay_us = us;
    timer.active = 1;
}

// เช็คว่าครบกำหนดหรือยัง (เรียกใน main loop)
uint8_t is_expired(void) {
    if (!timer.active) return 1;
    // ใช้ uint32_t wraparound arithmetic (safe)
    uint32_t elapsed = timer.start_tick - hal_get_sys_tick();
    uint32_t needed = timer.delay_us * (FREQ_SYS / 1000000);
    // หมายเหตุ: FREQ_SYS = 100000000 ที่ 100MHz
    // ต้อง calibrate ticks/µs ตาม SysTick configuration
    if (elapsed >= needed) { timer.active = 0; return 1; }
    return 0;
}
```

ข้อดีของ non-blocking delay:
- CPU ไม่หยุด — ทำ UART, BLE, sensor ไปพร้อมกัน
- ไม่เปลือง hardware timer
- มีกี่ตัวก็ได้ (แค่เพิ่ม struct)
- ใช้ใน callback interrupt ได้ (เฉพาะ start, ไม่ใช้ is_expired)

ข้อควรระวัง:
- แม่นยำน้อยกว่า Timer hardware (~5-10% error)
- ต้อง calibrate ค่า ticks/µs สำหรับ SysTick ของ HW จริง
- is_expired() ใช้ polling — ควรเรียกใน main loop เท่านั้น

**ตัวอย่างเพิ่มเติม (แบบ Timer oneshot):**
ดูตัวอย่างที่ 14 (`examples/14_nonblock_delay/`) ซึ่งสาธิต:
1. **Pattern 1:** ใช้ `hal_timer` แบบ `HAL_TIMER_MODE_ONE_SHOT` + callback flag
2. **Pattern 2:** ใช้ `hal_get_sys_tick()` + state machine

---

### hal_reset

```c
void hal_reset(void);
```

**คำอธิบาย:** รีเซ็ตระบบ (software reset) — เทียบเท่ากับการกดปุ่มรีเซ็ต

```c
hal_reset();  // รีเซ็ตชิป
```

---

### hal_get_reset_status

```c
uint8_t hal_get_reset_status(void);
```

**คำอธิบาย:** อ่านสถานะการรีเซ็ตครั้งล่าสุด

**ค่าที่คืน:** ค่า bitmask จาก `SYS_ResetStaTypeDef` (ดูใน StdPeriphDriver)

| Bit | ความหมาย |
|---|---|
| `RST_POR` | Power-on reset |
| `RST_SW` | Software reset |
| `RST_WDOG` | Watchdog reset |
| `RST_PIN` | External pin reset |

---

## Watchdog (WDT)

### hal_wdt_init

```c
void hal_wdt_init(uint8_t counter);
```

**คำอธิบาย:** เริ่มต้น watchdog timer ด้วยค่า counter เริ่มต้น

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `counter` | `uint8_t` | ค่า counter เริ่มต้น (กำหนดระยะเวลา timeout) |

---

### hal_wdt_feed

```c
void hal_wdt_feed(uint8_t counter);
```

**คำอธิบาย:** ป้อนค่าให้ watchdog — ต้องเรียกฟังก์ชันนี้เป็นระยะก่อน timer หมดเวลา มิฉะนั้นระบบจะรีเซ็ต

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย |
|---|---|---|
| `counter` | `uint8_t` | ค่า counter ที่ต้องการป้อน |

```c
while (1) {
    hal_wdt_feed(128);  // รีเซ็ต watchdog
    // ... โค้ดหลัก ...
}
```

---

### hal_wdt_enable_irq

```c
void hal_wdt_enable_irq(uint8_t enable);
```

**คำอธิบาย:** เปิด/ปิด interrupt เมื่อ watchdog หมดเวลา

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `enable` | `uint8_t` | เปิดหรือปิด | `0` = ปิด, `1` = เปิด |

---

### hal_wdt_enable_reset

```c
void hal_wdt_enable_reset(uint8_t enable);
```

**คำอธิบาย:** เปิด/ปิดการรีเซ็ตระบบเมื่อ watchdog หมดเวลา

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `enable` | `uint8_t` | เปิดหรือปิด | `0` = ไม่รีเซ็ต (เฉพาะ interrupt), `1` = รีเซ็ตเมื่อหมดเวลา |

---

### hal_wdt_clear_flag

```c
void hal_wdt_clear_flag(void);
```

**คำอธิบาย:** ล้างค่า flag interrupt ของ watchdog
