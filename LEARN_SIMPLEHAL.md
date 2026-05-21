# เรียนรู้การเขียนโค้ดด้วย SimpleHAL สำหรับ CH572/CH570

เอกสารนี้รวบรวมตัวอย่างและแนวทางการเขียนโค้ดตั้งแต่เริ่มต้น สำหรับผู้ที่เริ่มใช้งาน SimpleHAL

---

## สารบัญ

1. [เริ่มต้นโปรเจกต์แรก](#1-เริ่มต้นโปรเจกต์แรก)
2. [GPIO — จัดการขา I/O](#2-gpio--จัดการขา-io)
3. [UART — สื่อสารอนุกรม](#3-uart--สื่อสารอนุกรม)
4. [Timer — ตั้งเวลาจับเวลา](#4-timer--ตั้งเวลาจับเวลา)
5. [PWM — สร้างสัญญาณ PWM](#5-pwm--สร้างสัญญาณ-pwm)
6. [ADC — อ่านค่าแอนะล็อก](#6-adc--อ่านค่าแอนะล็อก)
7. [I2C — สื่อสารกับเซนเซอร์](#7-i2c--สื่อสารกับเซนเซอร์)
8. [SPI — สื่อสารความเร็วสูง](#8-spi--สื่อสารความเร็วสูง)
9. [Interrupt และ Callback](#9-interrupt-และ-callback)
10. [การใช้งานแบบ Async](#10-การใช้งานแบบ-async)
11. [Non-blocking Delay](#11-non-blocking-delay)
12. [Watchdog](#12-watchdog)
13. [BLE — บลูทูธพลังงานต่ำ](#13-ble--บลูทูธพลังงานต่ำ)
14. [RF — 2.4GHz ไร้สาย](#14-rf--24ghz-ไร้สาย)
15. [Power Management — ประหยัดพลังงาน](#15-power-management--ประหยัดพลังงาน)

---

## 1. เริ่มต้นโปรเจกต์แรก

### โครงสร้างพื้นฐานของทุกโปรเจกต์

```c
#include "simple_hal.h"

int main() {
    // ขั้นตอนที่ 1: ตั้งค่า HSE oscillator capacitance
    HSECFG_Capacitance(HSECap_18p);

    // ขั้นตอนที่ 2: ตั้งความถี่นาฬิกา (เลือกความถี่ที่ต้องการ)
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // ขั้นตอนที่ 3: เริ่มต้น peripheral ที่ต้องการ
    // (เขียนโค้ดเริ่มต้นอุปกรณ์ต่างๆ ที่นี่)

    // ขั้นตอนที่ 4: เขียนโปรแกรมหลัก
    while (1) {
        // โค้ดหลักทำงานวนลูป
    }
}
```

### การตั้งค่าความถี่นาฬิกา

| ฟังก์ชัน | ความถี่ |
|---|---|
| `CLK_SOURCE_HSE_PLL_100MHz` | 100 MHz |
| `CLK_SOURCE_HSE_PLL_60MHz` | 60 MHz |
| `CLK_SOURCE_HSE_PLL_24MHz` | 24 MHz |
| `CLK_SOURCE_HSE_16MHz` | 16 MHz (ตรง ไม่ผ่าน PLL) |
| `CLK_SOURCE_HSE_PLL_20MHz` | 20 MHz |

### ค่าความจุ HSE Capacitance

| Macro | ค่าประมาณ |
|---|---|
| `HSECap_18p` | 18 pF |
| `HSECap_12p` | 12 pF |
| `HSECap_6p` | 6 pF |

---

## 2. GPIO — จัดการขา I/O

### กระพริบ LED

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // เริ่มต้น GPIO ขา PA11 เป็น output
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    while (1) {
        hal_gpio_toggle(led);   // กลับค่าขา
        hal_delay_ms(500);      // หน่วง 500ms
    }
}
```

### อ่านค่า Button

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_gpio_handle_t btn = hal_gpio_init(PA5, HAL_GPIO_INPUT_PULLUP);
    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    while (1) {
        if (hal_gpio_read(btn) == 0) {  // กดปุ่ม (active low)
            hal_gpio_write(led, 1);     // ติด LED
        } else {
            hal_gpio_write(led, 0);     // ดับ LED
        }
    }
}
```

### โหมด GPIO

| โหมด | ใช้กับ |
|---|---|
| `HAL_GPIO_INPUT_FLOATING` | อินพุตไม่มี pull |
| `HAL_GPIO_INPUT_PULLUP` | อินพุต pull-up (เช่น ปุ่มกด) |
| `HAL_GPIO_INPUT_PULLDOWN` | อินพุต pull-down |
| `HAL_GPIO_OUTPUT_PP_5mA` | เอาต์พุตทั่วไป (LED, ฯลฯ) |
| `HAL_GPIO_OUTPUT_PP_20mA` | เอาต์พุตกระแสสูง |

### ฟังก์ชัน GPIO ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_gpio_write(h, val)` | เขียนค่า 0 หรือ 1 |
| `hal_gpio_read(h)` | อ่านค่าปัจจุบัน |
| `hal_gpio_toggle(h)` | กลับค่าขา |
| `hal_gpio_set(h)` | ตั้งค่าเป็น 1 |
| `hal_gpio_reset(h)` | ตั้งค่าเป็น 0 |

---

## 3. UART — สื่อสารอนุกรม

### ส่งข้อความ

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // เริ่มต้น UART ที่ 115200 baud ใช้ขา TX=PA3, RX=PA2
    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    while (1) {
        hal_uart_send(uart, "Hello World!\r\n", 14);
        hal_delay_ms(1000);
    }
}
```

### รับข้อมูล และส่งกลับ (echo)

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    while (1) {
        if (hal_uart_available(uart) > 0) {
            uint8_t buf[64];
            uint16_t len;
            hal_uart_recv(uart, buf, sizeof(buf), &len);
            hal_uart_send(uart, buf, len);  // ส่งกลับ
        }
    }
}
```

### ส่งข้อความแบบมีตัวเลข (ใช้ sprintf)

```c
#include "simple_hal.h"
#include <stdio.h>  // สำหรับ sprintf

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    uint32_t counter = 0;
    char msg[64];

    while (1) {
        sprintf(msg, "Count: %lu\r\n", counter++);
        hal_uart_send(uart, (uint8_t*)msg, strlen(msg));
        hal_delay_ms(1000);
    }
}
```

### ฟังก์ชัน UART ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_uart_send(h, data, len)` | ส่งข้อมูลแบบ blocking |
| `hal_uart_send_byte(h, data)` | ส่ง 1 ไบต์ |
| `hal_uart_recv(h, buf, max_len, &out_len)` | อ่านจาก ring buffer |
| `hal_uart_recv_byte(h)` | อ่าน 1 ไบต์ |
| `hal_uart_available(h)` | ดูจำนวนไบต์ที่รออ่าน |
| `hal_uart_flush(h)` | ล้าง ring buffer |
| `hal_uart_set_trig_bytes(h, n)` | ตั้งค่า trigger byte (1,2,4,7) |

---

## 4. Timer — ตั้งเวลาจับเวลา

### กระพริบ LED ด้วย Timer (ทุก 1 วินาที)

```c
#include "simple_hal.h"

void on_timer(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);
}

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);

    // Timer 1,000,000 µs = 1 วินาที, แบบต่อเนื่อง
    hal_timer_handle_t tmr = hal_timer_init(1000000, HAL_TIMER_MODE_PERIODIC);
    hal_timer_attach_cb(tmr, on_timer, led);
    hal_timer_start(tmr);

    while (1) {
        // CPU ทำงานอื่นได้โดยไม่ต้องรอ
    }
}
```

### One-shot Timer (ทำงานครั้งเดียวแล้วหยุด)

```c
void on_timeout(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_write(led, 1);  // ติด LED หลังจาก 2 วินาที
}

hal_timer_handle_t tmr = hal_timer_init(2000000, HAL_TIMER_MODE_ONE_SHOT);
hal_timer_attach_cb(tmr, on_timeout, led);
hal_timer_start(tmr);  // ทำงานครั้งเดียว 2 วินาที แล้วหยุด
```

### ฟังก์ชัน Timer ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_timer_init(period_us, mode)` | เริ่มต้น timer |
| `hal_timer_start(h)` | เริ่มนับเวลา |
| `hal_timer_stop(h)` | หยุดนับ |
| `hal_timer_reset(h)` | รีเซ็ตค่านับ |
| `hal_timer_set_period(h, period_us)` | เปลี่ยนคาบเวลา |
| `hal_timer_get_count(h)` | อ่านค่าปัจจุบัน (µs) |
| `hal_timer_attach_cb(h, cb, arg)` | แนบ callback |

**หมายเหตุ:** คาบเวลาสูงสุดที่ 100MHz คือ ~67 วินาที

---

## 5. PWM — สร้างสัญญาณ PWM

### สร้าง PWM และปรับความสว่าง LED

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // PWM ช่อง 1, ความถี่ 1000 Hz, duty 50%
    hal_pwm_handle_t pwm = hal_pwm_init(HAL_PWM_CH1, 1000, 50);
    hal_pwm_start(pwm);

    uint8_t duty = 0;

    while (1) {
        // ค่อยๆ เพิ่มความสว่าง
        for (duty = 0; duty <= 100; duty += 5) {
            hal_pwm_set_duty(pwm, duty);
            hal_delay_ms(100);
        }
        // ค่อยๆ ลดความสว่าง
        for (duty = 100; duty > 0; duty -= 5) {
            hal_pwm_set_duty(pwm, duty);
            hal_delay_ms(100);
        }
    }
}
```

### PWM 16 บิต (ความละเอียดสูง)

```c
// 16-bit PWM
hal_pwm_handle_t pwm = hal_pwm_init_16bit(HAL_PWM_CH1, 1000, 32768);
hal_pwm_set_duty_16bit(pwm, 16384);  // ~50%
```

### ฟังก์ชัน PWM ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_pwm_init(ch, freq, duty_pct)` | เริ่มต้น PWM 8 บิต |
| `hal_pwm_init_16bit(ch, freq, duty)` | เริ่มต้น PWM 16 บิต |
| `hal_pwm_set_duty(h, pct)` | ปรับ duty 8 บิต (0-100) |
| `hal_pwm_set_duty_16bit(h, duty)` | ปรับ duty 16 บิต |
| `hal_pwm_set_freq(h, freq)` | เปลี่ยนความถี่ |
| `hal_pwm_start(h)` | เริ่มส่งสัญญาณ |
| `hal_pwm_stop(h)` | หยุดส่งสัญญาณ |

---

## 6. ADC — อ่านค่าแอนะล็อก

### อ่านค่าแรงดันจากขา PA2 (9 บิต)

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_adc_handle_t adc = hal_adc_init(HAL_ADC_9BIT, PA2);

    char msg[64];

    while (1) {
        uint16_t raw = hal_adc_read(adc);
        uint32_t mv = hal_adc_read_mv(adc, 3300);  // Vref = 3.3V

        sprintf(msg, "ADC raw: %u, mV: %lu\r\n", raw, mv);
        hal_uart_send(uart, (uint8_t*)msg, strlen(msg));
        hal_delay_ms(500);
    }
}
```

### ความละเอียด ADC

| โหมด | บิต | ขา | ค่าสูงสุด |
|---|---|---|---|
| `HAL_ADC_4BIT` | 4 | PA7 | 0-16 |
| `HAL_ADC_8BIT` | 8 | PA2 | 0-511 |
| `HAL_ADC_9BIT` | 9 | PA2 | 0-1023 |

---

## 7. I2C — สื่อสารกับเซนเซอร์

### อ่านค่าเซนเซอร์ผ่าน I2C (เช่น MPU6050)

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    // เริ่มต้น I2C ความเร็ว 400kHz, โหมด master
    hal_i2c_handle_t i2c = hal_i2c_init(400000, HAL_I2C_MASTER, 0);

    uint8_t who_am_i;
    hal_i2c_read(i2c, 0x68, 0x75, &who_am_i, 1);  // อ่าน WHO_AM_I

    char msg[64];
    sprintf(msg, "MPU6050 WHO_AM_I: 0x%02X\r\n", who_am_i);
    hal_uart_send(uart, (uint8_t*)msg, strlen(msg));

    while (1);
}
```

### เขียน register ของอุปกรณ์ I2C

```c
uint8_t config = 0x00;
hal_i2c_write(i2c, 0x68, 0x6B, &config, 1);  // ปลุก MPU6050
```

### ฟังก์ชัน I2C ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_i2c_write(h, dev_addr, reg, data, len)` | เขียน register |
| `hal_i2c_read(h, dev_addr, reg, buf, len)` | อ่าน register |
| `hal_i2c_write_raw(h, dev_addr, data, len)` | เขียนโดยตรง |
| `hal_i2c_read_raw(h, dev_addr, buf, len)` | อ่านโดยตรง |
| `hal_i2c_mem_write/read(h, addr, mem_addr, addr_len, ...)` | อ่าน/เขียน EEPROM |

---

## 8. SPI — สื่อสารความเร็วสูง

### ส่งข้อมูลผ่าน SPI

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // SPI ความเร็ว 8MHz, โหมด 0, MSB first
    hal_spi_handle_t spi = hal_spi_init(8000000, HAL_SPI_MODE0, HAL_SPI_MSB_FIRST);

    uint8_t tx_data[] = {0xAA, 0xBB, 0xCC};
    uint8_t rx_data[3];

    hal_spi_chip_select(spi, 0);  // เลือกอุปกรณ์
    hal_spi_transfer(spi, tx_data, rx_data, 3);  // ส่ง/รับพร้อมกัน
    hal_spi_chip_select(spi, 1);  // ยกเลิกการเลือก

    while (1);
}
```

### ฟังก์ชัน SPI ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_spi_init(clock, mode, order)` | เริ่มต้น SPI |
| `hal_spi_transfer(h, tx, rx, len)` | ส่ง/รับ full-duplex |
| `hal_spi_send_byte(h, data)` | ส่ง 1 ไบต์ |
| `hal_spi_recv_byte(h)` | รับ 1 ไบต์ |
| `hal_spi_chip_select(h, level)` | ควบคุม CS (PA15) |
| `hal_spi_set_speed(h, clock_hz)` | เปลี่ยนความเร็ว |

---

## 9. Interrupt และ Callback

### GPIO Interrupt — กดปุ่มแล้วกระพริบ LED

```c
#include "simple_hal.h"

void on_btn_press(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);
}

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_gpio_handle_t led = hal_gpio_init(PA11, HAL_GPIO_OUTPUT_PP_5mA);
    hal_gpio_handle_t btn = hal_gpio_init(PA5, HAL_GPIO_INPUT_PULLUP);

    // แนบ callback เมื่อมีการขาลง (กดปุ่ม)
    hal_gpio_attach_irq(btn, HAL_GPIO_IRQ_FALLING, on_btn_press, led);

    while (1) {
        // CPU ทำงานอื่นได้
    }
}
```

### โหมด GPIO Interrupt

| โหมด | เกิดเมื่อ |
|---|---|
| `HAL_GPIO_IRQ_RISING` | ขาขึ้น (0→1) |
| `HAL_GPIO_IRQ_FALLING` | ขาลง (1→0) |
| `HAL_GPIO_IRQ_LOW_LEVEL` | ระดับต่ำ |
| `HAL_GPIO_IRQ_HIGH_LEVEL` | ระดับสูง |

### UART Interrupt — รับข้อมูลและตอบกลับ

```c
void on_uart_rx(void *arg) {
    hal_uart_handle_t uart = (hal_uart_handle_t)arg;
    uint8_t buf[64];
    uint16_t len;

    if (hal_uart_recv(uart, buf, sizeof(buf), &len) == HAL_OK) {
        hal_uart_send(uart, buf, len);  // echo กลับ
    }
}

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_uart_attach_rx_cb(uart, on_uart_rx, uart);

    while (1);
}
```

### Timer Interrupt — ทำงานทุก 1 วินาที

```c
uint32_t sys_tick = 0;

void on_timer_tick(void *arg) {
    sys_tick++;
    // ทำงานทุก 1 วินาที
}

hal_timer_handle_t tmr = hal_timer_init(1000000, HAL_TIMER_MODE_PERIODIC);
hal_timer_attach_cb(tmr, on_timer_tick, NULL);
hal_timer_start(tmr);
```

### ข้อควรระวังใน Callback

- **ห้าม** ใช้ `hal_delay_ms()` หรือ blocking operation ภายใน callback
- callback ควรสั้นและรวดเร็ว
- อย่าเรียก HAL API ที่ blocking ใน callback

### Interrupt Handler ที่ SimpleHAL Register อัตโนมัติ

| โมดูล | Handler | เปิดเมื่อ |
|---|---|---|
| GPIO | `GPIOA_IRQHandler` | เรียก `hal_gpio_attach_irq()` |
| UART | `UART_IRQHandler` | เรียก `hal_uart_init()` |
| Timer | `TMR_IRQHandler` | เรียก `hal_timer_start()` |
| RTC | `RTC_IRQHandler` | เรียก `hal_rtc_init()` |
| SPI | `SPI_IRQHandler` | เรียก `hal_spi_transfer_dma()` |
| KeyScan | `KEYSCAN_IRQHandler` | เรียก `hal_keyscan_attach_cb()` |
| RF | `BB_IRQHandler`, `LLE_IRQHandler` | เรียก `hal_rf_init()` |

---

## 10. การใช้งานแบบ Async

### UART ส่งแบบไม่บล็อก

```c
void on_tx_done(void *arg) {
    hal_gpio_handle_t led = (hal_gpio_handle_t)arg;
    hal_gpio_toggle(led);  // กระพริบ LED เมื่อส่งเสร็จ
}

// ส่งข้อมูลแบบ async — คืนทันที ไม่รอ
hal_uart_send_async(uart, data, len, on_tx_done, led);
```

### UART รับแบบไม่บล็อก (ครบตามจำนวนที่กำหนด)

```c
uint8_t rx_buf[20];

void on_rx_done(void *arg) {
    hal_uart_handle_t uart = (hal_uart_handle_t)arg;
    // rx_buf มี 20 ไบต์แล้ว พร้อมใช้งาน
    hal_uart_send(uart, rx_buf, 20);
}

// รับ 20 ไบต์ — callback เมื่อครบ
hal_uart_recv_async(uart, rx_buf, 20, on_rx_done, uart);
```

### SPI DMA (ส่งหรือรับอย่างเดียว)

```c
void on_spi_dma_done(void *arg) {
    // DMA transfer เสร็จ
}

// ส่งผ่าน DMA 256 ไบต์
hal_spi_transfer_dma(spi, tx_buf, NULL, 256, on_spi_dma_done, NULL);

// รับผ่าน DMA 256 ไบต์
hal_spi_transfer_dma(spi, NULL, rx_buf, 256, on_spi_dma_done, NULL);
```

**หมายเหตุ:** DMA สำหรับ SPI รองรับเฉพาะส่งอย่างเดียวหรือรับอย่างเดียวเท่านั้น

---

## 11. Non-blocking Delay

### ปัญหาของ hal_delay_ms()

```c
hal_delay_ms(1000);  // blocking — CPU หยุดทำงาน 1 วินาที
```

ไม่เหมาะกับ:
- callback interrupt
- โปรแกรมที่ต้องทำงาน BLE + UART + sensor พร้อมกัน
- ระบบที่ต้องการ response time ต่ำ

### วิธีที่ 1: ใช้ soft_timer ด้วย hal_get_sys_tick()

```c
typedef struct {
    uint8_t  active;
    uint32_t start_tick;
    uint32_t delay_us;
} soft_timer_t;

static soft_timer_t timer;

void start_delay(uint32_t us) {
    timer.start_tick = hal_get_sys_tick();
    timer.delay_us = us;
    timer.active = 1;
}

uint8_t is_expired(void) {
    if (!timer.active) return 1;
    uint32_t elapsed = timer.start_tick - hal_get_sys_tick();
    if (elapsed >= timer.delay_us * (FREQ_SYS / 1000000)) {
        timer.active = 0;
        return 1;
    }
    return 0;
}

// ตัวอย่างการใช้งาน
void some_function(void) {
    start_delay(500000);  // เริ่มนับ 500ms
}

void main_loop(void) {
    if (is_expired()) {
        // ครบ 500ms แล้ว
        hal_gpio_toggle(led);
    }
}
```

### วิธีที่ 2: ใช้ Timer แบบ One-shot

```c
volatile uint8_t flag_done = 0;

void on_delay_done(void *arg) {
    flag_done = 1;
}

hal_timer_handle_t tmr = hal_timer_init(500000, HAL_TIMER_MODE_ONE_SHOT);
hal_timer_attach_cb(tmr, on_delay_done, NULL);
hal_timer_start(tmr);

// ใน main loop
if (flag_done) {
    flag_done = 0;
    hal_gpio_toggle(led);
}
```

---

## 12. Watchdog

### ใช้งาน Watchdog ป้องกันค้าง

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_wdt_init(255);              // เริ่มต้น watchdog
    hal_wdt_enable_reset(1);        // รีเซ็ตเมื่อ watchdog หมดเวลา

    while (1) {
        hal_wdt_feed(255);          // ป้อน watchdog ก่อนหมดเวลา
        hal_uart_send(uart, "Alive\r\n", 7);
        hal_delay_ms(500);
    }
}
```

---

## 13. BLE — บลูทูธพลังงานต่ำ

### BLE Peripheral พื้นฐาน

```c
#include "simple_hal.h"

void on_connect(void *arg) {
    hal_uart_handle_t uart = (hal_uart_handle_t)arg;
    hal_uart_send(uart, "Connected!\r\n", 12);
}

void on_disconnect(void *arg) {
    hal_uart_handle_t uart = (hal_uart_handle_t)arg;
    hal_uart_send(uart, "Disconnected!\r\n", 15);
}

void on_ble_data(void *arg) {
    hal_ble_handle_t ble = (hal_ble_handle_t)arg;
    // มีข้อมูลขาเข้า ประมวลผลต่อ
}

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_60MHz);  // BLE มักใช้ 60MHz

    hal_uart_handle_t uart = hal_uart_init(115200,
        UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);

    hal_ble_handle_t ble = hal_ble_init("MyCH572", LL_TX_POWEER_0_DBM);
    hal_ble_attach_connect_cb(ble, on_connect, uart);
    hal_ble_attach_disconnect_cb(ble, on_disconnect, uart);
    hal_ble_attach_data_cb(ble, on_ble_data, ble);
    hal_ble_advertise_start(ble, HAL_BLE_ADV_MODE_CONNECTABLE);

    while (1) {
        hal_ble_process();  // ต้องเรียกเป็นระยะ
    }
}
```

### ฟังก์ชัน BLE ที่ใช้บ่อย

| ฟังก์ชัน | การทำงาน |
|---|---|
| `hal_ble_init(name, tx_power)` | เริ่มต้น BLE |
| `hal_ble_advertise_start(h, mode)` | เริ่มโฆษณาตัว |
| `hal_ble_advertise_stop(h)` | หยุดโฆษณา |
| `hal_ble_send(h, data, len)` | ส่งข้อมูล |
| `hal_ble_is_connected(h)` | เช็คสถานะเชื่อมต่อ |
| `hal_ble_process()` | ประมวลผล BLE stack (เรียกใน loop) |

---

## 14. RF — 2.4GHz ไร้สาย

### ส่งข้อมูลไร้สาย

```c
#include "simple_hal.h"

int main() {
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    // RF ความถี่ 2.402 GHz, PHY 2.4G (nRF24L01 compatible), กำลังส่ง 0 dBm
    hal_rf_handle_t rf = hal_rf_init(2402000, HAL_RF_PHY_2G4, 0);
    hal_rf_calibrate(rf);

    uint8_t msg[] = "Hello RF!";

    while (1) {
        hal_rf_send(rf, msg, sizeof(msg));
        hal_delay_ms(1000);
    }
}
```

### รับข้อมูลไร้สาย

```c
uint8_t buf[32];
uint16_t len;
int8_t rssi;

if (hal_rf_recv(rf, buf, sizeof(buf), &len, &rssi, 1000000) == HAL_OK) {
    // ได้ข้อมูล len ไบต์ สัญญาณแรง rssi dBm
}
```

---

## 15. Power Management — ประหยัดพลังงาน

### โหมดประหยัดพลังงานต่างๆ

```c
// โหมด Idle — หยุด CPU, peripheral ทำงาน, ปลุกด้วย interrupt ใดๆ
hal_pwr_idle();

// โหมด Halt — หยุด CPU + peripheral, ปลุกด้วย interrupt ภายนอก
hal_pwr_halt();

// โหมด Sleep — ประหยัดไฟสูง ต้องตั้งค่า wakeup source
hal_pwr_wakeup_cfg(RB_SLP_RTC_WAKE, HAL_PWR_WAKE_DELAY_64);
hal_pwr_sleep(RB_PWR_RAM2K);

// โหมด Shutdown — ประหยัดไฟสูงสุด ต้องรีเซ็ตเพื่อปลุก
hal_pwr_shutdown(RB_PWR_RAM2K);
```

### ปิด peripheral ที่ไม่ใช้เพื่อประหยัดไฟ

```c
// ปิดนาฬิกาของ peripheral ที่ไม่ใช้
hal_pwr_periph_clk(0, RB_CLK_USB);   // ปิด USB
hal_pwr_periph_clk(0, RB_CLK_SPI);   // ปิด SPI
hal_pwr_periph_clk(0, RB_CLK_I2C);   // ปิด I2C
```

### ตั้งค่า Wakeup Source

```c
// ปลุกจาก sleep ด้วย RTC และ GPIO
hal_pwr_wakeup_cfg(RB_SLP_RTC_WAKE | RB_SLP_GPIO_WAKE, HAL_PWR_WAKE_DELAY_64);
```

### retention_mask สำหรับ Sleep/Shutdown

| ค่า | คงสถานะ |
|---|---|
| `RB_PWR_RAM2K` | RAM 2KB |
| `RB_PWR_RAM16K` | RAM 16KB |
| `RB_PWR_EXTEND` | USB/BLE |
| `RB_PWR_XROM` | Flash |
| `0` | ไม่คงอะไร |

---

## ตารางสรุปฟังก์ชัน Handles

ทุกโมดูลใช้รูปแบบ handle-based API:

```c
hal_xxx_handle_t hal_xxx_init(...);  // เริ่มต้น ได้ handle
hal_xxx_action(handle, ...);          // ทำงานผ่าน handle
```

| โมดูล | Handle Type | ฟังก์ชัน init |
|---|---|---|
| GPIO | `hal_gpio_handle_t` | `hal_gpio_init(pin, mode)` |
| UART | `hal_uart_handle_t` | `hal_uart_init(baud, tx, rx)` |
| SPI | `hal_spi_handle_t` | `hal_spi_init(clock, mode, order)` |
| I2C | `hal_i2c_handle_t` | `hal_i2c_init(clock, role, addr)` |
| Timer | `hal_timer_handle_t` | `hal_timer_init(period_us, mode)` |
| PWM | `hal_pwm_handle_t` | `hal_pwm_init(ch, freq, duty)` |
| ADC | `hal_adc_handle_t` | `hal_adc_init(resolution, pin)` |
| RTC | `hal_rtc_handle_t` | `hal_rtc_init(osc_cycles)` |
| KeyScan | `hal_keyscan_handle_t` | `hal_keyscan_init(pins, div, repeat)` |
| RF | `hal_rf_handle_t` | `hal_rf_init(freq, phy, power)` |
| BLE | `hal_ble_handle_t` | `hal_ble_init(name, tx_power)` |

---

## ค่าคงที่สถานะ (hal_status_t)

| ค่า | ความหมาย |
|---|---|
| `HAL_OK` | ดำเนินการสำเร็จ |
| `HAL_ERROR` | เกิดข้อผิดพลาดทั่วไป |
| `HAL_BUSY` | กำลังทำงาน หรือ ring buffer เต็ม/ว่าง |
| `HAL_TIMEOUT` | หมดเวลารอ |
| `HAL_INVALID` | พารามิเตอร์ไม่ถูกต้อง |

---

## แนวทางการเขียนโค้ด

### ลำดับการเขียนโปรแกรม

1. ตั้งค่า `HSECFG_Capacitance()` ให้ตรงกับ crystal ที่ใช้
2. ตั้งค่า `SetSysClock()` เลือกความถี่ที่ต้องการ
3. เริ่มต้น peripheral ด้วย `hal_xxx_init()` เก็บค่า handle
4. แนบ callback (ถ้าต้องการ) ด้วย `hal_xxx_attach_cb()`
5. เริ่มการทำงาน (เช่น `hal_timer_start()`, `hal_pwm_start()`)
6. ใน `while (1)` เขียน logic หลัก

### ข้อควรระวัง

- **ห้ามใช้ `hal_delay_ms()` ใน callback** — ใช้ non-blocking delay แทน
- **ตรวจสอบค่าที่คืนจาก init** — ถ้าได้ `NULL` แสดงว่ามีปัญหา
- **BLE ต้องเรียก `hal_ble_process()` ใน loop หลัก**
- **RF ต้อง calibrate ก่อนใช้** `hal_rf_calibrate()`
- **ตั้งค่า HSE capacitance ให้ถูกต้อง** ไม่เช่นนั้นนาฬิกาจะไม่ stable
- **UART ring buffer ล้นได้** ถ้าอ่านข้อมูลไม่ทัน — ปรับ `HAL_UART_RB_SIZE` ใน `simple_hal_config.h`

### เมื่อเจอปัญหา

| ปัญหา | วิธีแก้ |
|---|---|
| คอมไพล์ไม่พบ header | เพิ่ม include path ไปยัง `StdPeriphDriver/inc` และ `RVMSIS` |
| Linker error `XXX_IRQHandler` | ตรวจสอบว่าเพิ่มไฟล์ SimpleHAL ครบ |
| UART ไม่มีข้อมูล | ตรวจสอบ TX/RX พิน (CH572: TX=PA3, RX=PA2) |
| RF ไม่ทำงาน | เรียก `hal_rf_calibrate()` ก่อนใช้ |
| BLE ไม่พบอุปกรณ์ | เรียก `hal_ble_process()` ใน loop |
| นาฬิกาไม่ stable | ตรวจสอบค่า `HSECFG_Capacitance()` |

---

> ดูรายละเอียดเพิ่มเติม: `src/SimpleHAL/docs/` — มีเอกสาร API ครบทุกโมดูล
