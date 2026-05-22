# SimpleHAL — การใช้งาน Interrupt และ Callback

## หลักการ

SimpleHAL ใช้ระบบ **callback function** เพื่อจัดการ interrupt โดย callback จะถูกเรียก
เมื่อเกิดเหตุการณ์ที่สนใจ เช่น GPIO มีการเปลี่ยนแปลง电平, UART มีข้อมูลเข้า, Timer ครบรอบ ฯลฯ

```c
typedef void (*hal_callback_t)(void *arg);
```

callback ทุกตัวรับพารามิเตอร์ `void *arg` เพียงตัวเดียว
ซึ่งผู้ใช้สามารถส่งค่าที่ต้องการกลับไปยัง callback ได้

---

## การทำงานของ Interrupt Handler

SimpleHAL จะ register interrupt handler ของตัวเอง เช่น `GPIOA_IRQHandler`,
`UART_IRQHandler`, `TMR_IRQHandler` เป็นต้น

เมื่อเกิด interrupt handler จะ:
1. อ่านสถานะ interrupt
2. เรียก callback ที่ลงทะเบียนไว้
3. ล้าง flag interrupt

ไม่จำเป็นต้องเขียน interrupt handler เอง — แค่แนบ callback ก็พอ

---

## GPIO Interrupt

### ขั้นตอนการใช้งาน

```c
#include "simple_hal.h"

// ฟังก์ชัน callback
void เมื่อกดปุ่ม(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);  // กลับค่า LED ทุกครั้งที่กดปุ่ม
}

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    hal_gpio_handle_t btn = hal_gpio_init(PA5, HAL_GPIO_INPUT_PULLUP);

    // แนบ callback — เมื่อมีการขาลง (กดปุ่ม)
    hal_gpio_attach_irq(btn, HAL_GPIO_IRQ_FALLING,
        เมื่อกดปุ่ม, led);

    while (1);
}
```

### โหมดต่างๆ

| โหมด | เหตุการณ์ |
|---|---|
| `HAL_GPIO_IRQ_LOW_LEVEL` | ระดับต่ำ |
| `HAL_GPIO_IRQ_HIGH_LEVEL` | ระดับสูง |
| `HAL_GPIO_IRQ_FALLING` | ขาลง (1→0) |
| `HAL_GPIO_IRQ_RISING` | ขาขึ้น (0→1) |

---

## UART Interrupt

UART interrupt จะทำงานอัตโนมัติหลังจากเรียก `hal_uart_init()`
ข้อมูลที่รับเข้ามาจะถูกเก็บไว้ใน ring buffer

### การใช้งาน callback เมื่อมีข้อมูลเข้า

```c
void เมื่อมีข้อมูลเข้า(void *arg) {
    hal_uart_handle_t uart = (hal_uart_handle_t)arg;
    // ring buffer มีข้อมูลแล้ว สามารถอ่านได้
    uint8_t buf[64];
    uint16_t len;
    hal_uart_recv(uart, buf, sizeof(buf), &len);
}

int main() {
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_uart_attach_rx_cb(uart, เมื่อมีข้อมูลเข้า, uart);

    while (1);
}
```

### การส่งแบบ Async

```c
void เมื่อส่งเสร็จ(void *arg) {
    hal_gpio_toggle((hal_gpio_handle_t)arg);
}

int main() {
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    // ส่งแบบไม่บล็อก — callback เมื่อส่งครบ
    uint8_t data[] = "Hello Async!";
    hal_uart_send_async(uart, data, sizeof(data),
        เมื่อส่งเสร็จ, led);

    while (1);
}
```

### การรับแบบ Async

```c
uint8_t rx_buf[20];

void เมื่อรับครบ(void *arg) {
    hal_uart_handle_t uart = (hal_uart_handle_t)arg;
    // rx_buf มีข้อมูล 20 ไบต์แล้ว
    hal_uart_send(uart, rx_buf, 20);
}

int main() {
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    // รับ 20 ไบต์ — callback เมื่อครบ
    hal_uart_recv_async(uart, rx_buf, 20,
        เมื่อรับครบ, uart);

    while (1);
}
```

---

## Timer Interrupt

```c
uint32_t tick = 0;

void เมื่อtimerครบ(void *arg) {
    tick++;
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);  // กระพริบ LED ทุก 1 วินาที
}

int main() {
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    // Timer 1 วินาที, แบบต่อเนื่อง
    hal_timer_handle_t tmr = hal_timer_init(1000000,
        HAL_TIMER_MODE_PERIODIC);
    hal_timer_attach_cb(tmr, เมื่อtimerครบ, led);
    hal_timer_start(tmr);

    while (1);
}
```

---

## RTC Interrupt

```c
void on_rtc_trigger(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);
}

int main() {
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    hal_rtc_handle_t rtc = hal_rtc_init(Count_1);

    hal_rtc_set_trigger(rtc, 32768);  // ~1 วินาทีที่ LSI 32kHz
    hal_rtc_attach_cb(rtc, on_rtc_trigger, led);

    while (1);
}
```

---

## KeyScan Interrupt

```c
void on_keypress(void *arg) {
    hal_keyscan_handle_t ks = (hal_keyscan_handle_t)arg;
    uint32_t val = hal_keyscan_get_value(ks);
    // ตรวจสอบค่าแป้นที่กด
}

int main() {
    hal_keyscan_handle_t ks = hal_keyscan_init(KEYSCAN_ALL,
        KEYSCAN_DIV4, KEYSCAN_REP3);
    hal_keyscan_attach_cb(ks, on_keypress, ks);

    while (1);
}
```

---

## SPI DMA Interrupt

```c
void on_spi_dma_done(void *arg) {
    hal_gpio_toggle((hal_gpio_handle_t)arg);
}

int main() {
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    hal_spi_handle_t spi = hal_spi_init(8000000, HAL_SPI_MODE0, HAL_SPI_MSB_FIRST);

    uint8_t tx[256], rx[256];
    hal_spi_transfer_dma(spi, tx, rx, 256, on_spi_dma_done, led);

    while (1);
}
```

---

## USB Device Interrupt

USB Device mode ใช้ interrupt-driven — เมื่อ Host ส่งคำสั่ง (SETUP, IN, OUT) MCU จะเกิด `USB_IRQHandler` และเรียก `hal_usbdev_int_handler()` เพื่อประมวลผล

```c
void on_usb_event(hal_usbdev_handle_t h, hal_usbdev_evt_t evt, void *arg) {
    switch (evt) {
    case HAL_USBDEV_EVT_RESET:
        // Host รีเซ็ตบัส — reset state
        break;
    case HAL_USBDEV_EVT_CONFIGURED:
        // Host ตั้งค่า configuration แล้ว — พร้อมส่ง HID report
        break;
    case HAL_USBDEV_EVT_SETUP_REQ:
        // Class-specific setup request
        break;
    }
}

int main() {
    hal_usbdev_handle_t dev = hal_usbdev_init();
    hal_usbdev_attach_cb(dev, on_usb_event, NULL);
    while (1);
}
```

> หมายเหตุ: USB Host mode ใช้ **polling** ไม่ใช้ interrupt — เรียก `hal_usbhost_poll()` ใน main loop แทน

---

## การทำงานของ Interrupt Handler ที่ SimpleHAL Register

SimpleHAL จะ register interrupt handler อัตโนมัติ:

| โมดูล | Interrupt Handler | เงื่อนไข |
|---|---|---|
| GPIO | `GPIOA_IRQHandler` | เมื่อเรียก `hal_gpio_attach_irq()` |
| UART | `UART_IRQHandler` | เมื่อเรียก `hal_uart_init()` |
| Timer | `TMR_IRQHandler` | เมื่อเรียก `hal_timer_start()` |
| RTC | `RTC_IRQHandler` | เมื่อเรียก `hal_rtc_init()` |
| KeyScan | `KEYSCAN_IRQHandler` | เมื่อเรียก `hal_keyscan_attach_cb()` |
| SPI | `SPI_IRQHandler` | เมื่อเรียก `hal_spi_transfer_dma()` |
| RF | `BB_IRQHandler`, `LLE_IRQHandler` | เมื่อเรียก `hal_rf_init()` |
| USB Device | `USB_IRQHandler` | เมื่อเรียก `hal_usbdev_init()` (จัดการ standard request + dispatch event) |

### ข้อควรระวัง

1. **ห้ามใช้ interrupt handler ชื่อเดียวกัน** ในโปรเจกต์ เพราะจะทับกัน (linker จะเลือกตัวที่ไม่ได้ weak)
2. **callback ควรสั้นและรวดเร็ว** — ไม่ควรมี delay หรือ blocking operation ใน callback
3. **ring buffer overflow** — ถ้าใช้ callback ร่วมกับ ring buffer ควรอ่านข้อมูลออกจาก ring buffer ใน callback เพื่อไม่ให้ข้อมูลล้น
4. **อย่าเรียก HAL API ที่ blocking** ภายใน callback (เช่น `hal_uart_send`, `hal_delay_ms`) เพราะจะทำให้ระบบช้าลงหรือ deadlock
