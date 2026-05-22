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

**Non-blocking delay ด้วย hal_softimer (แนะนำ):**

`core/hal_softimer.h` เป็น soft timer แบบ polling-based ที่ออกแบบมาสำหรับ
non-blocking delay โดยเฉพาะ — ไม่ต้อง calibrate, ไม่เปลือง hardware timer

```c
#include "simple_hal.h"

static hal_softimer_t tmr;

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    hal_softimer_init(&tmr, HAL_SOFTIMER_PERIODIC);
    hal_softimer_start(&tmr, 500000);  // 500ms

    while (1) {
        if (hal_softimer_expired(&tmr)) {
            hal_gpio_toggle(led);
        }
        // ทำงานอื่นต่อ — UART, BLE, sensor ...
    }
}
```

ข้อดี:
- CPU ไม่หยุด — ทำ UART, BLE, sensor ไปพร้อมกัน
- ไม่เปลือง hardware timer
- มีกี่ตัวก็ได้ (แค่ประกาศ `hal_softimer_t`)
- ใช้ใน callback interrupt ได้ (เฉพาะ `hal_softimer_start`)
- ไม่ต้อง calibrate — ใช้ `hal_get_sys_tick()` อัตโนมัติ

ข้อควรระวัง:
- `hal_softimer_expired()` ใช้ polling — ควรเรียกใน main loop
- แม่นยำน้อยกว่า hardware timer (~5-10% error)

ดูตัวอย่างเพิ่มเติม: `examples/15_softimer_delay/` และ [Softimer module](../modules/softimer.md)

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
