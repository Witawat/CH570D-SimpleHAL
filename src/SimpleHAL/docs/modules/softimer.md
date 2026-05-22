# SimpleHAL — Softimer Module (Core)

**ไฟล์:** `core/hal_softimer.h` / `core/hal_softimer.c`

Soft timer แบบ polling-based สำหรับ non-blocking delay
โดยใช้ `hal_get_sys_tick()` เป็นตัวจับเวลา — ไม่ต้องใช้ hardware timer

---

## ฟังก์ชัน

### hal_softimer_init

```c
void hal_softimer_init(hal_softimer_t *t, hal_softimer_mode_t mode);
```

**คำอธิบาย:** เริ่มต้น soft timer

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `t` | `hal_softimer_t *` | พอยน์เตอร์ไปยัง struct soft timer | — |
| `mode` | `hal_softimer_mode_t` | โหมดการทำงาน | `HAL_SOFTIMER_ONESHOT`, `HAL_SOFTIMER_PERIODIC` |

**ตัวอย่าง:**
```c
static hal_softimer_t tmr;
hal_softimer_init(&tmr, HAL_SOFTIMER_PERIODIC);
```

---

### hal_softimer_start

```c
void hal_softimer_start(hal_softimer_t *t, uint32_t delay_us);
```

**คำอธิบาย:** เริ่มนับเวลา (ไม่บล็อก — CPU ทำงานต่อได้ทันที)

**พารามิเตอร์:**

| พารามิเตอร์ | Type | คำอธิบาย | ค่าที่เป็นไปได้ |
|---|---|---|---|
| `t` | `hal_softimer_t *` | พอยน์เตอร์ soft timer | — |
| `delay_us` | `uint32_t` | ระยะเวลาในหน่วยไมโครวินาที | `0`–`4294967295` (~71 นาที) |

```c
hal_softimer_start(&tmr, 500000);  // 500ms
```

---

### hal_softimer_stop

```c
void hal_softimer_stop(hal_softimer_t *t);
```

**คำอธิบาย:** หยุด soft timer (ยกเลิกการนับ)

```c
hal_softimer_stop(&tmr);
```

---

### hal_softimer_expired

```c
uint8_t hal_softimer_expired(hal_softimer_t *t);
```

**คำอธิบาย:** ตรวจสอบว่า soft timer ครบกำหนดหรือยัง

เรียกใน main loop ทุกรอบ:
- ถ้าโหมด `PERIODIC` จะเริ่มนับรอบถัดไปอัตโนมัติ
- ถ้าโหมด `ONESHOT` จะหยุดนับและต้องเรียก `hal_softimer_start()` อีกครั้ง

**ค่าที่คืน:** `0` = ยังไม่ครบ, `1` = ครบกำหนด

```c
if (hal_softimer_expired(&tmr)) {
    hal_gpio_toggle(led);
}
```

---

## ตัวอย่างสมบูรณ์

```c
#include "simple_hal.h"

static hal_softimer_t blink_tmr;
static hal_softimer_t uart_tmr;
static hal_gpio_handle_t led;
static hal_uart_handle_t uart;
static uint32_t counter = 0;

int main() {
    char msg[32];

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    uart = hal_uart_init(115200, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_softimer_init(&blink_tmr, HAL_SOFTIMER_PERIODIC);
    hal_softimer_start(&blink_tmr, 500000);  // กระพริบ LED ทุก 500ms

    hal_softimer_init(&uart_tmr, HAL_SOFTIMER_PERIODIC);
    hal_softimer_start(&uart_tmr, 1000000);  // ส่งข้อความทุก 1 วินาที

    while (1) {
        if (hal_softimer_expired(&blink_tmr)) {
            hal_gpio_toggle(led);
        }
        if (hal_softimer_expired(&uart_tmr)) {
            sprintf(msg, "Count: %lu\r\n", counter++);
            hal_uart_send(uart, (uint8_t*)msg, strlen(msg));
        }
    }
}
```
