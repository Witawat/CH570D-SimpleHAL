# SimpleHAL — คู่มือการใช้งาน

**เวอร์ชันคู่มือ:** 1.0 — สำหรับ SimpleHAL 1.9.0  
**ไมโครคอนโทรลเลอร์:** WCH CH570D (RISC-V)  
**ภาษา:** C (Arduino-compatible API)

---

## สารบัญ

1. [บทนำ](#1-บทนำ)
2. [การเริ่มต้นใช้งาน](#2-การเริ่มต้นใช้งาน)
3. [GPIO](#3-gpio)
4. [GPIO Interrupt](#4-gpio-interrupt)
5. [เวลา / Delay](#5-เวลา--delay)
6. [PWM](#6-pwm)
7. [I2C](#7-i2c)
8. [USART (UART)](#8-usart-uart)
9. [SPI](#9-spi)
10. [Timer](#10-timer)
11. [ADC](#11-adc)
12. [Interrupt Control](#12-interrupt-control)
13. [OneWire (1-Wire)](#13-onewire-1-wire)
14. [Utility Functions](#14-utility-functions)
15. [GPIO Legacy (STM32 compat)](#15-gpio-legacy-stm32-compat)
16. [ข้อจำกัดทั้งหมด](#16-ข้อจำกัดทั้งหมด)
17. [API Reference ฉบับย่อ](#17-api-reference-ฉบับย่อ)
18. [Troubleshooting](#18-troubleshooting)

---

## 1. บทนำ

### SimpleHAL คืออะไร

SimpleHAL คือชุดซอฟต์แวร์ที่ทำหน้าที่เป็น **Compatibility Layer** ระหว่างโค้ดสไตล์ Arduino กับ HAL
(Hardware Abstraction Layer) ของชิป WCH CH57x (RISC-V)

หากคุณเคยเขียน Arduino มาก่อน คุณจะพบว่าฟังก์ชันส่วนใหญ่คุ้นเคย:

```c
pinMode(PA0, PIN_MODE_OUTPUT);
digitalWrite(PA0, HIGH);
delay_ms(500);
uint8_t val = digitalRead(PA1);
```

โดยที่ไม่ต้องยุ่งกับ register ของชิปโดยตรง

### ส่วนประกอบของ SimpleHAL

| ไฟล์ | หน้าที่ |
|---|---|
| `SimpleHAL.h` | Arduino-compat API — declare ฟังก์ชัน, define constants, typedef |
| `SimpleHAL.c` | Implementation + OneWire driver |
| `Simple1Wire.h` | OneWire data structures และ function declarations |

### ข้อแตกต่างจาก Arduino

| Arduino | SimpleHAL |
|---|---|
| C++ | C (extern "C" พร้อมสำหรับ C++ linker) |
| AVR / ARM / RISC-V | RISC-V (WCH QingKe V4) |
| delay(ms) → `delay_ms(ms)` | ต้อง include SimpleHAL.h |
| analogWrite → `PWM_Write()` | ต้อง Init ก่อน (หรือ auto-init ที่ 1kHz) |

---

## 2. การเริ่มต้นใช้งาน

### ขั้นตอนพื้นฐาน

```c
#include "SimpleHAL.h"

int main(void)
{
    SimpleHAL_Init();           // ตั้งค่าพื้นฐาน (flag กันเรียกซ้ำ)
    Timer_Init();               // kick SysTick (จำเป็นสำหรับ Get_CurrentMs/Us)

    pinMode(PA0, PIN_MODE_OUTPUT);

    while (1) {
        digitalWrite(PA0, HIGH);
        Delay_Ms(500);
        digitalWrite(PA0, LOW);
        Delay_Ms(500);
    }
}
```

**หมายเหตุ:**
- `SimpleHAL_Init()` ปัจจุบันไม่ได้ init ฮาร์ดแวร์ใด ๆ (แค่ป้องกันการเรียกซ้ำ) — subsystem แต่ละตัวต้อง init เอง
- **จำเป็นต้องมี SysTick interrupt** ทำงานจึงจะใช้ `Get_CurrentMs()`, `Get_CurrentUs()`, `Delay_Ms()`, `Delay_Us()` ได้
- หากใช้ MounRiver Studio ให้ตั้งค่า Systick Config ใน `system_ch57x.c` แล้ว `Timer_Init()` จะ kick ให้ทำงาน

### simple_hal_config.h

เปิด/ปิดโมดูลที่ต้องการในไฟล์ `simple_hal_config.h`:

```c
#define HAL_ENABLE_BLE     0
#define HAL_ENABLE_RF      0
#define HAL_ENABLE_USBHOST 0
#define HAL_ENABLE_USBDEV  0
```

โมดูลที่จำเป็นต้องเปิดสำหรับ SimpleHAL:
- GPIO, UART, SPI, I2C, TIMER, PWM, ADC, SYS

---

## 3. GPIO

### ฟังก์ชันหลัก

```c
void pinMode(uint8_t pin, GPIO_PinMode mode);
void digitalWrite(uint8_t pin, uint8_t value);
uint8_t digitalRead(uint8_t pin);
void digitalToggle(uint8_t pin);
void pinModeMultiple(const uint8_t *pins, GPIO_PinMode mode);
```

### โหมดที่รองรับ

| GPIO_PinMode | ความหมาย |
|---|---|
| `PIN_MODE_INPUT` | Input ลอย (Hi-Z) |
| `PIN_MODE_OUTPUT` | Output Push-Pull 5mA |
| `PIN_MODE_INPUT_PULLUP` | Input พุลอัป (ต้านทานภายใน ~50kΩ) |
| `PIN_MODE_INPUT_PULLDOWN` | Input พุลดาวน์ |
| `PIN_MODE_OUTPUT_OD` | Open-Drain (จำลองโดยสลับ Input/Output) |

### เลขขา

```c
PA0 = 0   PA1 = 1   PA2 = 2   PA3 = 3
PA4 = 4   PA5 = 5   PA6 = 6   PA7 = 7
PA8 = 8   PA9 = 9   PA10 = 10 PA11 = 11
PA12 = 12 PA13 = 13 PA14 = 14 PA15 = 15
```

### Pin Mask (สำหรับ GPIO_SetBits/ResetBits)

```c
GPIO_Pin_0   = 0x0001   GPIO_Pin_8   = 0x0100
GPIO_Pin_1   = 0x0002   GPIO_Pin_9   = 0x0200
GPIO_Pin_2   = 0x0004   GPIO_Pin_10  = 0x0400
GPIO_Pin_3   = 0x0008   GPIO_Pin_11  = 0x0800
GPIO_Pin_4   = 0x0010   GPIO_Pin_12  = 0x1000
GPIO_Pin_5   = 0x0020   GPIO_Pin_13  = 0x2000
GPIO_Pin_6   = 0x0040   GPIO_Pin_14  = 0x4000
GPIO_Pin_7   = 0x0080   GPIO_Pin_15  = 0x8000
GPIO_Pin_All = 0xFFFF
```

### กลไก Open-Drain (OUTPUT_OD)

CH570D ไม่่มี hardware open-drain ใน GPIO ทุกพิน SimpleHAL จึงจำลองโดย:

- **pinMode(PA0, OUTPUT_OD):** init เป็น PP_5mA → set HIGH → เปลี่ยนเป็น Input Floating
- **digitalWrite(PA0, HIGH):** เปลี่ยนเป็น Input Floating (pin ลอย, พุลอัปภายนอกดึง HIGH)
- **digitalWrite(PA0, LOW):** เปลี่ยนเป็น PP_5mA → write 0 (pin ดึง LOW แอคทีฟ)

**ต้องมี pull-up ภายนอก (4.7kΩ–10kΩ) จึงจะได้ HIGH**

### ตัวอย่าง: การอ่านสัญญาณจาก switch

```c
//              CH570D
//              ┌─────┐
//  PA1 ───────┤ SW  ├────── GND
//              └─────┘
//  (internal pull-up เปิดอัตโนมัติ)

pinMode(PA1, PIN_MODE_INPUT_PULLUP);

if (digitalRead(PA1) == LOW) {
    // กด switch
}
```

### pinModeMultiple

ปิดท้าย array ด้วย `0xFF`:

```c
const uint8_t led_pins[] = {PA0, PA1, PA2, 0xFF};
pinModeMultiple(led_pins, PIN_MODE_OUTPUT);
```

---

## 4. GPIO Interrupt

### ฟังก์ชัน

```c
typedef enum { RISING = 0, FALLING, CHANGE } GPIO_InterruptMode;

void attachInterrupt(uint8_t pin, void (*callback)(void), GPIO_InterruptMode mode);
void detachInterrupt(uint8_t pin);
```

### โหมด

| โหมด | ทำงานเมื่อ |
|---|---|
| `RISING` | สัญญาณเปลี่ยนจาก LOW → HIGH |
| `FALLING` | สัญญาณเปลี่ยนจาก HIGH → LOW |
| `CHANGE` | สัญญาณเปลี่ยนทั้งสองขอบ |

### CHANGE mode — auto-toggle

เมื่อใช้ `CHANGE` ตัว wrapper (`_irq_wrapper`) จะ:

1. อ่านระดับ current ของพิน
2. ตั้ง interrupt ไปที่ขอบตรงข้าม (LOW→RISING, HIGH→FALLING)
3. เรียก callback ของผู้ใช้
4. ครั้งต่อไป interrupt จะมาตกขอบตรงข้าม → สลับไปเรื่อย ๆ

### ข้อควรระวัง

- ถ้า `pinMode()` ยังไม่ถูกเรียกมาก่อน `attachInterrupt()` จะ auto-init เป็น `INPUT_FLOATING`
- Interrupt callback ต้องไม่ใช้ `Delay_Ms/Us` หรือ blocking operation
- การ attach interrupt ใน CHANGE mode มี race ขนาดเล็ก (อ่าน level ก่อน set irq) — ปกติไม่เป็นปัญหาในการใช้งานทั่วไป

### ตัวอย่าง: ตรวจจับการกด switch

```c
void on_button_press(void) {
    // เรียกใน ISR — ควรสั้นที่สุด
    volatile uint32_t tick = Get_CurrentMs();
    (void)tick;  // บันทึกเวลาสำหรับ debounce ใน main loop
}

void main(void) {
    pinMode(PA1, PIN_MODE_INPUT_PULLUP);
    attachInterrupt(PA1, on_button_press, FALLING);
    // เมื่อกด switch (PA1→LOW), on_button_press() ถูกเรียก
}
```

---

## 5. เวลา / Delay

### ฟังก์ชัน

```c
uint32_t  Get_CurrentMs(void);       // คืน millisecond ปัจจุบัน
uint32_t  Get_CurrentUs(void);       // คืน microsecond ปัจจุบัน (ละเอียด)
void      Delay_Ms(uint16_t ms);     // หน่วง ms (blocking)
void      Delay_Us(uint16_t us);     // หน่วง us (blocking)

#define ELAPSED_TIME(last, now) ((uint32_t)((now) - (last)))
```

### Get_CurrentUs — วิธีการทำงาน

```c
uint32_t Get_CurrentUs(void) {
    uint32_t cnt = SysTick->CNT;
    uint32_t ms = hal_get_sys_tick();
    uint32_t cmp = SysTick->CMP;
    uint32_t us_in_tick = (cmp - cnt) * 1000 / (cmp + 1);
    return ms * 1000 + us_in_tick;
}
```

SysTick นับถอยหลังจาก `CMP` → 0 ทุก 1ms ค่า `us_in_tick` คือเศษ microsecond ใน millisecond ปัจจุบัน

**ข้อสังเกต:** อ่าน `CNT` ก่อน `ms` เพื่อลด race condition หาก interrupt เกิดคั่นระหว่างอ่าน

### ตัวอย่าง: Non-blocking blink

```c
uint32_t last_tick = 0;

while (1) {
    uint32_t now = Get_CurrentMs();
    if (ELAPSED_TIME(last_tick, now) >= 500) {
        digitalToggle(PA0);
        last_tick = now;
    }
    // ทำอย่างอื่นได้ระหว่างรอ
    process_sensor();
}
```

---

## 6. PWM

### ช่องสัญญาณ

```c
typedef enum {
    PWM1_CH1 = 0,
    PWM1_CH2,
    PWM1_CH3,
    PWM1_CH4,
    PWM2_CH1
} PWM_Channel;
```

| ช่อง | PWM module | หมายเหตุ |
|---|---|---|
| `PWM1_CH1` | PWM1 | 8-bit หรือ 16-bit |
| `PWM1_CH2` | PWM1 | |
| `PWM1_CH3` | PWM1 | |
| `PWM1_CH4` | PWM1 | |
| `PWM2_CH1` | PWM2 (HAL_PWM_CH5) | |

### ฟังก์ชัน

```c
void PWM_Init(PWM_Channel channel, uint32_t frequency_hz);          // 8-bit mode
void PWM_Init16bit(PWM_Channel channel, uint32_t frequency_hz);     // 16-bit mode
void PWM_SetDutyCycle(PWM_Channel channel, uint8_t duty_percent);   // 0-100%
void PWM_SetDutyCycleRaw(PWM_Channel channel, uint16_t duty_value); // absolute value
uint16_t PWM_GetPeriod(PWM_Channel channel);
void PWM_Start(PWM_Channel channel);
void PWM_Stop(PWM_Channel channel);
void PWM_Write(PWM_Channel channel, uint8_t value);                 // 0-255 (analogWrite style)
```

### 8-bit vs 16-bit

| | 8-bit | 16-bit |
|---|---|---|
| ความละเอียด duty | 0–100% (เปอร์เซ็นต์) | 0–65535 (ค่าสัมบูรณ์) |
| `PWM_GetPeriod` | 256 | `cycle_16bit` |
| ค่าเริ่มต้น | `PWM_Init()` | `PWM_Init16bit()` |
| เรียกอัตโนมัติ | เมื่อ `frequency_hz >= 1000` | เมื่อ `frequency_hz < 1000` จาก `PWM_Init()` |

`PWM_Write(ch, value)` — ถ้ายังไม่เคย Init จะ auto-init ที่ 1kHz:

```c
void PWM_Write(PWM_Channel channel, uint8_t value) {
    if (!_pwm_inited[channel])
        PWM_Init(channel, 1000);     // auto-init
    uint8_t pct = (uint8_t)(((uint16_t)value * 100) / 255);
    PWM_SetDutyCycle(channel, pct);
    PWM_Start(channel);
}
```

### Wiring — PWM output

```
CH570D          LED
┌──────┐       ┌──┐
│ PA0  ├───────┤  ├─────┤ GND
└──────┘       └──┘
            (220Ω resistor)
```

### ตัวอย่าง: Breathing LED

```c
PWM_Init(PWM1_CH1, 1000);    // 1kHz, 8-bit
PWM_Start(PWM1_CH1);

while (1) {
    for (uint8_t i = 0; i < 255; i++) {
        PWM_Write(PWM1_CH1, i);
        Delay_Ms(5);
    }
    for (uint8_t i = 255; i > 0; i--) {
        PWM_Write(PWM1_CH1, i);
        Delay_Ms(5);
    }
}
```

---

## 7. I2C

### ฟังก์ชัน

```c
void    I2C_SimpleInit(uint32_t speed_hz, uint8_t pins);
uint8_t I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len);
uint8_t I2C_Read(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t val);
uint8_t I2C_ReadRegMulti(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg);
uint8_t I2C_IsDeviceReady(uint8_t dev_addr);
```

### สถานะ

```c
#define I2C_OK    0
#define I2C_ERROR 1
```

### พารามิเตอร์

- `speed_hz` — ความเร็ว เช่น `100000` (standard), `400000` (fast)
- `dev_addr` — ที่อยู่ I2C ของ device (7-bit, **เลื่อนซ้าย 0 บิต**)
- `pins` — ถูกละเว้น (ใช้พิน HW I2C ของ CH570D)
- `reg` — register address 8-bit

### I2C_WriteReg / I2C_ReadRegMulti — Repeated Start

ฟังก์ชันทั้งสองใช้ `hal_i2c_write()` / `hal_i2c_read()` ซึ่งสร้าง **repeated start** (START + addr(W) + reg + RESTART + addr(R) + data + STOP) แทนที่จะเป็นสอง transaction แยก

### Wiring

```
CH570D                    VCC (3.3V)
┌──────┐                    │
│ SDA  ├─────┬──────────────┤─── 4.7kΩ
│ SCL  ├─────┴──────┬───────┤─── 4.7kΩ
└──────┘            │
              ┌─────┴──────┐
              │ I2C Device │
              │ (BMP280,   │
              │  OLED,     │
              │  LM75...)  │
              └────────────┘
```

### ตัวอย่าง: I2C Scanner

```c
I2C_SimpleInit(400000, 0);
for (uint8_t addr = 1; addr < 127; addr++) {
    if (I2C_IsDeviceReady(addr)) {
        // เจอ device ที่ addr
    }
}
```

### ตัวอย่าง: อ่าน register

```c
uint8_t id = I2C_ReadReg(BMP280_ADDR, BMP280_REG_ID);
// id = 0x58 สำหรับ BMP280

uint8_t data[2];
I2C_ReadRegMulti(BMP280_ADDR, BMP280_REG_TEMP, data, 2);
```

---

## 8. USART (UART)

### ฟังก์ชัน

```c
void    USART_SimpleInit(uint32_t baudrate, uint8_t pins);
void    USART_WriteByte(uint8_t data);
uint8_t USART_Available(void);
uint8_t USART_Read(void);
void    USART_Flush(void);
void    USART_Print(const char *str);
void    USART_PrintNum(int32_t n);
```

### พิน

| สัญญาณ | พิน |
|---|---|
| TX | PA3 (UART_TX_REMAP_PA3) |
| RX | PA2 (UART_RX_REMAP_PA2) |

**แก้ไขไม่ได้** — `USART_SimpleInit()` ละเว้น `pins` parameter

### Baudrate constants

```c
BAUD_9600    = 9600
BAUD_19200   = 19200
BAUD_38400   = 38400
BAUD_57600   = 57600
BAUD_115200  = 115200
```

### Wiring

```
CH570D       USB-TTL (CP2102/CH340)
┌──────┐     ┌───────────┐
│ PA3  ├─────┤ RXD       │
│ PA2  ├─────┤ TXD       │
│ GND  ├─────┤ GND       │
└──────┘     └───────────┘
```

### ตัวอย่าง: Serial Echo

```c
USART_SimpleInit(BAUD_115200, 0);
USART_Print("Hello from CH570D!\r\n");

while (1) {
    if (USART_Available()) {
        uint8_t c = USART_Read();
        USART_WriteByte(c);   // echo กลับ
    }
}
```

### USART_PrintNum

```c
int32_t value = -12345;
USART_PrintNum(value);   // พิมพ์ "-12345"
```

ใช้ `sprintf` ภายใน — ระวังเรื่อง stack size บน MCU ที่มี RAM จำกัด

---

## 9. SPI

### ฟังก์ชัน

```c
void    SPI_SimpleInit(SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg);
uint8_t SPI_Transfer(uint8_t data);
```

### SPI Modes

```c
#define SPI_MODE0  0     // CPOL=0, CPHA=0
#define SPI_MODE1  1     // CPOL=0, CPHA=1  → fallback to MODE0
#define SPI_MODE2  2     // CPOL=1, CPHA=0  → fallback to MODE3
#define SPI_MODE3  3     // CPOL=1, CPHA=1
```

⚠️ **ข้อจำกัด:** HAL ของ CH570D รองรับเฉพาะ MODE0 และ MODE3 MODE1 ถูก map ไป MODE0, MODE2 ไป MODE3
(เนื่องจาก `hal_spi_mode_t` enum มีแค่ `HAL_SPI_MODE0 = 0` และ `HAL_SPI_MODE3 = 2`)
ตรวจสอบว่า device ที่ใช้รองรับ MODE0 หรือ MODE3 ก่อน

### ความเร็ว

```c
#define SPI_1MHZ   1000000
#define SPI_2MHZ   2000000
#define SPI_4MHZ   4000000
#define SPI_8MHZ   8000000
```

### การรับ-ส่ง

`SPI_Transfer()` ส่ง 1 byte และรับ 1 byte พร้อมกัน (full-duplex):

```c
uint8_t SPI_Transfer(uint8_t data) {
    hal_spi_send_byte(_spi_handle, data);      // ส่ง data, receive garbage
    return hal_spi_recv_byte();                 // ส่ง 0xFF, receive response
}
```

สำหรับการส่งหลาย byte ให้วน loop เรียก `SPI_Transfer()` หลายครั้ง

### หมายเหตุ

- `SPI_SimpleInit()` ละเว้น parameter `pin_cfg`
- ใช้งานเป็น SPI Master เท่านั้น
- ต้องจัดการ Chip Select (CS) ด้วย GPIO ปกติ: `digitalWrite(cs_pin, LOW)` ก่อน, `HIGH` หลัง

### Wiring

```
CH570D              SPI Device
┌──────┐           ┌──────────┐
│ PA5  ├─ SCK ─────┤ SCL      │
│ PA6  ├─ MOSI ────┤ MOSI     │
│ PA7  ├─ MISO ────┤ MISO     │
│ PA4  ├─ CS  ─────┤ CS       │  (GPIO ปกติ)
│ GND  ├───────────┤ GND      │
└──────┘           └──────────┘
```

### ตัวอย่าง: ส่งข้อมูล

```c
#define CS_PIN  PA4

pinMode(CS_PIN, PIN_MODE_OUTPUT);
digitalWrite(CS_PIN, HIGH);

SPI_SimpleInit(SPI_MODE0, SPI_1MHZ, 0);

digitalWrite(CS_PIN, LOW);
SPI_Transfer(0xAA);         // ส่งคำสั่ง
SPI_Transfer(0x55);         // ส่งข้อมูล
digitalWrite(CS_PIN, HIGH);
```

---

## 10. Timer

### ฟังก์ชัน

```c
void TIM_SimpleInit(uint8_t tim, uint32_t period_us);
void TIM_AttachInterrupt(uint8_t tim, void (*callback)(void));
void TIM_Start(uint8_t tim);
void TIM_Stop(uint8_t tim);
void TIM_DetachInterrupt(uint8_t tim);
```

### ข้อจำกัด

- รองรับ Timer **1 instance เท่านั้น**
- Parameter `tim` ถูกละเว้น (TIM_2, TIM_... → เหมือนกันหมด)
- ใช้ `HAL_TIMER_MODE_PERIODIC` เสมอ
- `period_us` — คาบเวลาในหน่วย microsecond (สูงสุด ~67,106,864µs ≈ 67 วินาที)

### ตัวอย่าง: กระพริบ LED ด้วย Timer

```c
volatile bool led_state = false;

void on_timer(void) {
    led_state = !led_state;
    digitalWrite(PA0, led_state);
}

void main(void) {
    pinMode(PA0, PIN_MODE_OUTPUT);

    TIM_SimpleInit(0, 500000);      // 500ms
    TIM_AttachInterrupt(0, on_timer);
    TIM_Start(0);

    while (1) {
        // main loop — ทำอย่างอื่นได้
    }
}
```

---

## 11. ADC

### ฟังก์ชัน

```c
void     ADC_SimpleInitChannels(const ADC_Channel *channels, uint8_t count);
uint16_t ADC_Read(ADC_Channel ch);
float    ADC_ToVoltage(uint16_t raw, float vref);
uint16_t analogRead(uint8_t pin);
```

### ความละเอียด

| Resolution | enum | ค่าสูงสุด |
|---|---|---|
| 4-bit | `HAL_ADC_4BIT` | 15 |
| 8-bit | `HAL_ADC_8BIT` (default) | 255 |
| 9-bit | `HAL_ADC_9BIT` | 511 |

### Channel Switching

`ADC_SimpleInitChannels()` รับ array ของ channel เพื่อตั้งต้น แต่ **HW รองรับได้ทีละ 1 channel จริง ๆ**

`ADC_Read(ch)` จะ auto switch channel โดย re-init ถ้า `ch` เปลี่ยน:

```c
uint16_t ADC_Read(ADC_Channel ch) {
    if (ch != _adc_current_channel) {
        hal_adc_free(_adc_handle);
        _adc_handle = hal_adc_init(_adc_resolution, ch);
        _adc_current_channel = ch;
    }
    return hal_adc_read(_adc_handle);
}
```

⚠️ ทุกครั้งที่ switch channel มี overhead จากการ re-init (ไม่กี่ µs)

### analogRead (standalone)

`analogRead(pin)` ใช้ `_adc_handle` ร่วมกับ `ADC_Read()`:

```c
uint16_t val = analogRead(PA0);   // init → read → keep handle
```

ถ้า `ADC_SimpleInitChannels()` ถูกเรียกก่อน `analogRead` จะ free handle ของ ADC_SimpleInitChannels และสร้างใหม่

### ADC_ToVoltage

```c
float voltage = ADC_ToVoltage(raw_value, 3.3f);
```

**ข้อสังเกต:** ใช้ `... / 1024.0f` เสมอ ไม่ได้ปรับตาม resolution จริง — ถ้าใช้ 8-bit ควรคำนวณเอง: `(float)raw * vref / 256.0f`

### ตัวอย่าง: อ่านโพเทนชิโอมิเตอร์

```
CH570D          Potentiometer
┌──────┐       ┌──────┐
│ VCC  ├───────┤ 3.3V │
│ PA0  ├───────┤ Wiper│
│ GND  ├───────┤ GND  │
└──────┘       └──────┘
```

```c
pinMode(PA0, PIN_MODE_INPUT);   // ADC pin

while (1) {
    uint16_t raw = analogRead(PA0);
    float volt = ADC_ToVoltage(raw, 3.3f);
    // volt ≈ 0–3.3V ตามตำแหน่ง pot
    Delay_Ms(100);
}
```

---

## 12. Interrupt Control

### ฟังก์ชัน

```c
void __disable_irq(void);
void __enable_irq(void);
```

### วิธีการทำงาน

ใช้การ save/restore ผ่าน CSR ของ RISC-V:

```c
void __disable_irq(void) {
    if (_irq_nest_cnt++ == 0)
        _irq_saved_state = __risc_v_disable_irq();  // csrrc (clear bit)
}

void __enable_irq(void) {
    if (_irq_nest_cnt > 0 && --_irq_nest_cnt == 0)
        __risc_v_enable_irq(_irq_saved_state);      // csrrs (set bit)
}
```

### Nesting Counter

ป้องกันการ enable interrupt ก่อนเวลาจาก nested call:

```c
__disable_irq();          // cnt=1, save state
  __disable_irq();        // cnt=2, skip
  __enable_irq();         // cnt=1, skip restore
__enable_irq();           // cnt=0, restore → interrupt enabled
```

### ใช้เมื่อใด

- timing-critical bit-banging (OneWire)
- ต้อง disable interrupt ไม่เกิน 100–200µs ติดต่อกัน

---

## 13. OneWire (1-Wire)

### ภาพรวม

OneWire เป็นโปรโตคอล bit-bang ที่ใช้ GPIO พินเดียว ทำงานโดย disable interrupt ระหว่างส่ง/รับเพื่อรักษา timing ที่แม่นยำ

**ข้อควรทราบ:** การทำงานจะ disable interrupt ทุกครั้งที่Reset/WriteBit/ReadBit — ระวังเรื่อง real-time constraint

### ฟังก์ชัน

```c
OneWire_Bus* OneWire_Init(uint8_t pin);
bool         OneWire_Reset(OneWire_Bus* bus);
void         OneWire_WriteBit(OneWire_Bus* bus, uint8_t bit);
uint8_t      OneWire_ReadBit(OneWire_Bus* bus);
void         OneWire_WriteByte(OneWire_Bus* bus, uint8_t data);
uint8_t      OneWire_ReadByte(OneWire_Bus* bus);
void         OneWire_WriteBytes(OneWire_Bus* bus, const uint8_t* data, uint8_t len);
void         OneWire_ReadBytes(OneWire_Bus* bus, uint8_t* buffer, uint8_t len);

void         OneWire_SkipROM(OneWire_Bus* bus);
bool         OneWire_ReadROM(OneWire_Bus* bus, uint8_t* rom);
void         OneWire_MatchROM(OneWire_Bus* bus, const uint8_t* rom);
bool         OneWire_Select(OneWire_Bus* bus, const uint8_t* rom);

void         OneWire_ResetSearch(OneWire_Bus* bus);
bool         OneWire_Search(OneWire_Bus* bus);
bool         OneWire_AlarmSearch(OneWire_Bus* bus);
void         OneWire_GetAddress(OneWire_Bus* bus, uint8_t* rom);

uint8_t      OneWire_CRC8(const uint8_t* data, uint8_t len);
bool         OneWire_VerifyCRC(const uint8_t* data, uint8_t len);

void         OneWire_Depower(OneWire_Bus* bus);
OneWire_Bus* OneWire_GetBusByPin(uint8_t pin);
```

### ค่า Timing

| Constant | ค่า (µs) | ความหมาย |
|---|---|---|
| `ONEWIRE_RESET_PULSE` | 480 | ระยะเวลาดึง LOW ใน Reset |
| `ONEWIRE_PRESENCE_WAIT` | 70 | รอ presence pulse |
| `ONEWIRE_PRESENCE_TIMEOUT` | 240 | หมดเวลารอ presence |
| `ONEWIRE_WRITE_0_LOW` | 60 | LOW ใน Write 0 slot |
| `ONEWIRE_WRITE_1_LOW` | 10 | LOW ใน Write 1 slot |
| `ONEWIRE_READ_LOW` | 3 | LOW ใน Read slot |
| `ONEWIRE_READ_WAIT` | 10 | รอ sample หลังจาก LOW |
| `ONEWIRE_SLOT_TIME` | 65 | ระยะเวลา 1 time slot |
| `ONEWIRE_WRITE_RECOVERY` | 1 | recovery time |
| `ONEWIRE_READ_RECOVERY` | 55 | recovery time |

### Constants

```c
#define ONEWIRE_CMD_SKIP_ROM      0xCC
#define ONEWIRE_CMD_READ_ROM      0x33
#define ONEWIRE_CMD_MATCH_ROM     0x55
#define ONEWIRE_CMD_SEARCH_ROM    0xF0
#define ONEWIRE_CMD_ALARM_SEARCH  0xEC
```

### จำนวน Bus สูงสุด

`ONEWIRE_MAX_BUSES` = 4 สามารถเปิดได้สูงสุด 4 พินพร้อมกัน

### โครงสร้าง OneWire_Bus

```c
typedef struct {
    uint8_t pin;
    uint8_t rom[8];
    uint8_t last_discrepancy;
    uint8_t last_family_discrepancy;
    bool last_device_flag;
    bool initialized;
} OneWire_Bus;
```

`last_discrepancy`, `last_family_discrepancy`, `last_device_flag` — ใช้สำหรับ Search algorithm ห้ามแก้ไขด้วยตนเอง

### CRC-8

Polynomial: `0x8C` (x⁸ + x⁵ + x⁴ + 1) — Dallas 1-Wire CRC-8

- `OneWire_CRC8(data, len)` — คำนวณ CRC ทับ `len` byte
- `OneWire_VerifyCRC(data, len)` — คืน `true` ถ้า CRC ผ่าน (CRC ของทั้ง message รวม CRC byte ต้องเป็น 0)

### Wiring

```
CH570D              DS18B20
┌──────┐           ┌────────┐
│ PA1  ├───────────┤ DATA   │
│ VCC  ├───────────┤ VDD    │
│ GND  ├───────────┤ GND    │
└──────┘           └────────┘
        │
      4.7kΩ
        │
       VCC (3.3V)
```

### ตัวอย่าง: อ่าน DS18B20

```c
#include "SimpleHAL.h"

OneWire_Bus* ow = OneWire_Init(PA1);
uint8_t rom[8];

if (OneWire_ReadROM(ow, rom)) {
    // เจอ DS18B20 ที่ rom[0..7]
}

// แปลงอุณหภูมิ
OneWire_Reset(ow);
OneWire_SkipROM(ow);
OneWire_WriteByte(ow, 0x44);        // Convert T
Delay_Ms(750);                      // รอแปลงเสร็จ

OneWire_Reset(ow);
OneWire_SkipROM(ow);
OneWire_WriteByte(ow, 0xBE);        // Read Scratchpad

uint8_t data[9];
OneWire_ReadBytes(ow, data, 9);

if (OneWire_VerifyCRC(data, 9)) {
    int16_t raw = (data[1] << 8) | data[0];
    float temp = raw * 0.0625f;
}
```

---

## 14. Utility Functions

### shiftOut

```c
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value);
```

ส่ง `value` ออกทีละบิตผ่าน `dataPin` โดย clock ด้วย `clockPin`

| Parameter | ค่า |
|---|---|
| `bitOrder` | `LSBFIRST` (0) หรือ `MSBFIRST` (1) |
| `value` | ข้อมูล 8-bit ที่จะส่ง |

**Implementation:**

```
MSBFIRST: ส่ง D7 → D6 → D5 → ... → D0
LSBFIRST: ส่ง D0 → D1 → D2 → ... → D7
```

### shiftIn

```c
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
```

อ่านข้อมูล 8-bit จาก `dataPin` โดย clock ด้วย `clockPin`

### pulseIn

```c
uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout);
```

รอให้สัญญาณที่ `pin` เปลี่ยนเป็น `state` แล้วจับเวลา (µs) จนกว่าสัญญาณจะเปลี่ยน

| Parameter | ความหมาย |
|---|---|
| `state` | `HIGH` หรือ `LOW` |
| `timeout` | µs ที่รอสูงสุด (0 = รอไม่จำกัด) |
| **คืน** | ความยาว pulse ใน µs, 0 ถ้า timeout |

**ข้อควรระวัง:** `pulseIn` เป็น blocking function — ถ้า signal ไม่มากายใน timeout จะคืนค่า 0

### Wiring — shiftOut + 74HC595

```
CH570D              74HC595
┌──────┐           ┌──────────┐
│ PA0  ├─ DS ──────┤ DS (14)  │
│ PA1  ├─ SH_CP ───┤ SH_CP (11)│
│ PA2  ├─ ST_CP ───┤ ST_CP (12)│
└──────┘           └──────────┘
                         │
                    Q0─Q7 → LED / 7-segment
```

### ตัวอย่าง: 74HC595

```c
#define DATA_PIN   PA0
#define CLOCK_PIN  PA1
#define LATCH_PIN  PA2

pinMode(DATA_PIN, PIN_MODE_OUTPUT);
pinMode(CLOCK_PIN, PIN_MODE_OUTPUT);
pinMode(LATCH_PIN, PIN_MODE_OUTPUT);

while (1) {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b10101010);
    digitalWrite(LATCH_PIN, HIGH);
    Delay_Ms(500);
}
```

---

## 15. GPIO Legacy (STM32 compat)

### ฟังก์ชัน

```c
void GPIO_SetBits(GPIO_TypeDef* port, GPIO_Pin pin);
void GPIO_ResetBits(GPIO_TypeDef* port, GPIO_Pin pin);
```

สำหรับผู้ที่ย้ายโค้ดจาก STM32 มาใช้:

```c
GPIO_SetBits(GPIOA, GPIO_Pin_5);     // = digitalWrite(PA5, HIGH)
GPIO_ResetBits(GPIOA, GPIO_Pin_5);   // = digitalWrite(PA5, LOW)
```

### ข้อจำกัด

- รองรับ **เฉพาะ GPIOA** — parameter `port` ที่ไม่ใช่ `GPIOA` จะถูกละเว้น
- แปลง pin mask เป็น pin number โดยวน shift (`while (pin > 1) { pin >>= 1; p++; }`)
- `GPIO_Pin_All` (0xFFFF) ใช้ได้ — จะกระพริบทุกพินพร้อมกัน (แต่ละครั้งที่เรียก)

---

## 16. ข้อจำกัดทั้งหมด

### SPI
- **MODE1 และ MODE2 ไม่มี** — HAL รองรับเฉพาะ MODE0 และ MODE3 (check `hal_spi.c:spi_mode_to_wch`)
- MODE1 → fallback เป็น MODE0 (CPHA ผิด)
- MODE2 → fallback เป็น MODE3 (CPHA ผิด)
- ใช้งาน SPI Master เท่านั้น (ไม่มี Slave mode)

### Timer
- **1 instance เท่านั้น** — ตัวแปร `_tim_handle` เป็น global ตัวเดียว
- Parameter `tim` ถูกละเว้น
- `TIM_SimpleInit()` ซ้ำจะทับของเก่าโดยไม่ free

### USART
- **พินตายตัว:** TX = PA3, RX = PA2 — ไม่สามารถเปลี่ยนพินได้
- `USART_SimpleInit()` ละเว้น `pins` parameter

### I2C
- Master mode เท่านั้น (ไม่มี Slave)
- `pins` parameter ถูกละเว้น

### PWM
- `PWM_Init()` ซ้ำจะไม่ free handle ก่อน
- 16-bit period ดึงค่าจาก `hal_pwm_obj.cycle_16bit` โดยตรง (เข้าถึง internal struct)
- `PWM1_CH4` → HAL_PWM_CH4, `PWM2_CH1` → HAL_PWM_CH5 (map ผ่าน `_map_pwm_channel`)

### ADC
- `ADC_MAX_VALUE` = 1024 (ไม่ตรงกับความละเอียด 8-bit ที่ใช้)
- `ADC_ToVoltage()` ใช้ `/ 1024.0f` เสมอ — ถ้าใช้ 8-bit คำนวนเอง: `raw * vref / 256.0f`
- `ADC_SimpleInitChannels()` เก็บ resolution เป็น `HAL_ADC_8BIT` hardcode — เปลี่ยนไม่ได้ใน API

### GPIO
- `GPIO_TypeDef` เป็น `void*` — ไม่ type safe
- `GPIO_SetBits/ResetBits` รองรับแค่ GPIOA

### OneWire
- disable interrupt ขณะทำงาน — สูงสุด ~480µs (reset pulse) ซึ่งอาจกระทบ real-time task
- สูงสุด 4 buses (`ONEWIRE_MAX_BUSES`)

### ทั่วไป
- `SimpleHAL_Init()` ไม่ได้ init ฮาร์ดแวร์ใด ๆ จริง — แค่ตั้ง flag
- `__enable_irq()` ใช้ `__risc_v_enable_irq()` เพื่อ set bits (ไม่ใช้ `PFIC_EnableAllIRQ` ซึ่งใช้ `csrw` ทับ mstatus)
- `Get_CurrentUs()` มี race condition น้อยมาก (อ่าน cnt ก่อน ms — bound error < 1 tick)

---

## 17. API Reference ฉบับย่อ

### GPIO

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `pinMode(pin, mode)` | `uint8_t pin`, `GPIO_PinMode mode` | `void` |
| `digitalWrite(pin, val)` | `uint8_t pin`, `uint8_t val` (0/1) | `void` |
| `digitalRead(pin)` | `uint8_t pin` | `uint8_t` (0/1) |
| `digitalToggle(pin)` | `uint8_t pin` | `void` |
| `pinModeMultiple(pins, mode)` | `const uint8_t* pins`, `GPIO_PinMode mode` | `void` |

**GPIO_PinMode:** `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`, `OUTPUT_OD`

### Interrupt

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `attachInterrupt(pin, cb, mode)` | `pin`, `void (*cb)(void)`, `GPIO_InterruptMode` | `void` |
| `detachInterrupt(pin)` | `pin` | `void` |

**GPIO_InterruptMode:** `RISING`, `FALLING`, `CHANGE`

### Time

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `Get_CurrentMs()` | — | `uint32_t` ms |
| `Get_CurrentUs()` | — | `uint32_t` µs |
| `Delay_Ms(ms)` | `uint16_t ms` | `void` |
| `Delay_Us(us)` | `uint16_t us` | `void` |

### PWM

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `PWM_Init(ch, freq)` | `PWM_Channel`, `uint32_t freq_hz` | `void` |
| `PWM_Init16bit(ch, freq)` | `PWM_Channel`, `uint32_t freq_hz` | `void` |
| `PWM_SetDutyCycle(ch, pct)` | `PWM_Channel`, `uint8_t 0–100` | `void` |
| `PWM_SetDutyCycleRaw(ch, val)` | `PWM_Channel`, `uint16_t` | `void` |
| `PWM_GetPeriod(ch)` | `PWM_Channel` | `uint16_t` |
| `PWM_Start(ch)` | `PWM_Channel` | `void` |
| `PWM_Stop(ch)` | `PWM_Channel` | `void` |
| `PWM_Write(ch, val)` | `PWM_Channel`, `uint8_t 0–255` | `void` |

**PWM_Channel:** `PWM1_CH1`, `PWM1_CH2`, `PWM1_CH3`, `PWM1_CH4`, `PWM2_CH1`

### I2C

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `I2C_SimpleInit(speed, pins)` | `uint32_t`, `uint8_t` | `void` |
| `I2C_Write(addr, data, len)` | `uint8_t addr`, `const uint8_t*`, `uint16_t` | `uint8_t` (OK/ERROR) |
| `I2C_Read(addr, buf, len)` | `uint8_t addr`, `uint8_t*`, `uint16_t` | `uint8_t` (OK/ERROR) |
| `I2C_WriteReg(addr, reg, val)` | `uint8_t addr`, `uint8_t reg`, `uint8_t val` | `uint8_t` (OK/ERROR) |
| `I2C_ReadRegMulti(addr, reg, buf, len)` | `uint8_t addr`, `uint8_t reg`, `uint8_t*`, `uint16_t` | `uint8_t` (OK/ERROR) |
| `I2C_ReadReg(addr, reg)` | `uint8_t addr`, `uint8_t reg` | `uint8_t` value |
| `I2C_IsDeviceReady(addr)` | `uint8_t addr` | `uint8_t` (0/1) |

**Return:** `I2C_OK` (0) หรือ `I2C_ERROR` (1)

### USART

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `USART_SimpleInit(baud, pins)` | `uint32_t baudrate`, `uint8_t` | `void` |
| `USART_WriteByte(data)` | `uint8_t data` | `void` |
| `USART_Read()` | — | `uint8_t` |
| `USART_Available()` | — | `uint8_t` |
| `USART_Flush()` | — | `void` |
| `USART_Print(str)` | `const char* str` | `void` |
| `USART_PrintNum(n)` | `int32_t n` | `void` |

### SPI

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `SPI_SimpleInit(mode, speed, cfg)` | `SPI_Mode`, `SPI_Speed`, `SPI_PinConfig` | `void` |
| `SPI_Transfer(data)` | `uint8_t data` | `uint8_t` received |

### Timer

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `TIM_SimpleInit(tim, period_us)` | `uint8_t tim`, `uint32_t period_us` | `void` |
| `TIM_AttachInterrupt(tim, cb)` | `uint8_t`, `void (*cb)(void)` | `void` |
| `TIM_Start(tim)` | `uint8_t` | `void` |
| `TIM_Stop(tim)` | `uint8_t` | `void` |
| `TIM_DetachInterrupt(tim)` | `uint8_t` | `void` |

### ADC

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `ADC_SimpleInitChannels(ch, count)` | `const ADC_Channel*`, `uint8_t count` | `void` |
| `ADC_Read(ch)` | `ADC_Channel` | `uint16_t raw` |
| `ADC_ToVoltage(raw, vref)` | `uint16_t raw`, `float vref` | `float` |
| `analogRead(pin)` | `uint8_t pin` | `uint16_t raw` |

### Misc

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `Timer_Init()` | — | `void` |
| `__disable_irq()` | — | `void` |
| `__enable_irq()` | — | `void` |
| `shiftOut(dp, cp, order, val)` | `uint8_t dp, cp, order, value` | `void` |
| `shiftIn(dp, cp, order)` | `uint8_t dp, cp, order` | `uint8_t` |
| `pulseIn(pin, state, timeout)` | `uint8_t pin, state`, `uint32_t timeout` | `uint32_t` µs |
| `GPIO_SetBits(port, pin)` | `GPIO_TypeDef*`, `GPIO_Pin` | `void` |
| `GPIO_ResetBits(port, pin)` | `GPIO_TypeDef*`, `GPIO_Pin` | `void` |

### OneWire

| ฟังก์ชัน | ค่าเข้า | คืน |
|---|---|---|
| `OneWire_Init(pin)` | `uint8_t pin` | `OneWire_Bus*` |
| `OneWire_Reset(bus)` | `OneWire_Bus*` | `bool` (มี device) |
| `OneWire_WriteBit(bus, bit)` | `OneWire_Bus*`, `uint8_t` | `void` |
| `OneWire_ReadBit(bus)` | `OneWire_Bus*` | `uint8_t` |
| `OneWire_WriteByte(bus, data)` | `OneWire_Bus*`, `uint8_t` | `void` |
| `OneWire_ReadByte(bus)` | `OneWire_Bus*` | `uint8_t` |
| `OneWire_WriteBytes(bus, data, len)` | `OneWire_Bus*`, `const uint8_t*`, `uint8_t` | `void` |
| `OneWire_ReadBytes(bus, buf, len)` | `OneWire_Bus*`, `uint8_t*`, `uint8_t` | `void` |
| `OneWire_SkipROM(bus)` | `OneWire_Bus*` | `void` |
| `OneWire_ReadROM(bus, rom)` | `OneWire_Bus*`, `uint8_t[8]` | `bool` (CRC OK) |
| `OneWire_MatchROM(bus, rom)` | `OneWire_Bus*`, `const uint8_t[8]` | `void` |
| `OneWire_Select(bus, rom)` | `OneWire_Bus*`, `const uint8_t[8]` | `bool` |
| `OneWire_ResetSearch(bus)` | `OneWire_Bus*` | `void` |
| `OneWire_Search(bus)` | `OneWire_Bus*` | `bool` (พบ device) |
| `OneWire_AlarmSearch(bus)` | `OneWire_Bus*` | `bool` |
| `OneWire_GetAddress(bus, rom)` | `OneWire_Bus*`, `uint8_t[8]` | `void` |
| `OneWire_CRC8(data, len)` | `const uint8_t*`, `uint8_t` | `uint8_t crc` |
| `OneWire_VerifyCRC(data, len)` | `const uint8_t*`, `uint8_t` | `bool` |
| `OneWire_Depower(bus)` | `OneWire_Bus*` | `void` |
| `OneWire_GetBusByPin(pin)` | `uint8_t` | `OneWire_Bus*` |

---

## 18. Troubleshooting

### SysTick ไม่ทำงาน / Get_CurrentMs คืน 0
- ต้องเรียก `Timer_Init()` อย่างน้อย 1 ครั้งเพื่อ kick
- ตรวจสอบว่า SysTick interrupt ถูก enable ใน `system_ch57x.c`

### ADC อ่าน 0 เสมอ
- พิน ADC ต้องไม่ถูกตั้งเป็น `PIN_MODE_OUTPUT` ก่อน
- สำหรับ PA0–PA7 เท่านั้นที่ใช้ ADC ได้
- ลองใช้ `analogRead(pin)` โดยตรง (ไม่ต้องผ่าน ADC_SimpleInitChannels)
- ถ้า `HAL_ADC_MAX_INSTANCES = 1` และมี handle เก่าค้างอยู่, `hal_adc_init()` จะคืน NULL

### I2C timeout ตลอด
- ตรวจสอบการต่อ SDA/SCL + pull-up 4.7kΩ
- ตรวจสอบว่า power ของ I2C device ตรงกัน (3.3V)
- `I2C_IsDeviceReady()` ใช้ `hal_i2c_write_raw` ส่ง 0 byte — ถ้าไม่พร้อม คืน 0
- `I2C_ReadRegMulti` ใช้ repeated start — อุปกรณ์บางรุ่นเก่าอาจไม่รองรับ

### UART พิมพ์ไม่ขึ้น
- สลับ TX/RX ระหว่าง CH570D (PA3 TX → USB-TTL RXD)
- ตรวจสอบ baudrate ให้ตรงกัน
- ใช้ `USART_Print()` แทน `USART_WriteByte()` สำหรับ string

### PWM ไม่มี output
- ต้องเรียก `PWM_Start()` หลังจาก `PWM_Init()`
- หรือใช้ `PWM_Write()` ที่มี auto-start
- ตรวจสอบความถี่: `PWM_Init()` รับ `>= 1000Hz` สำหรับ 8-bit mode

### OneWire ไม่เจอ device
- **วงจร:** ต้องมี pull-up 4.7kΩ ระหว่าง DATA กับ VCC
- **เวลา:** `ONEWIRE_RESET_PULSE` = 480µs — ถ้า device ต้องการ timing อื่น อาจต้องปรับ
- **CRC:** `OneWire_ReadROM()` คืน `false` ถ้า CRC ไม่ผ่าน — แสดงว่าสัญญาณมี noise หรือ wiring ไม่ดี
- **Bus:** ตรวจสอบว่า `OneWire_Init(pin)` สำเร็จ (ไม่เกิน `ONEWIRE_MAX_BUSES` = 4)

### Interrupt ไม่ทำงาน
- เรียก `attachInterrupt()` หลังจาก `pinMode()` หรือให้ attach auto-init ให้
- CHECK `_irq_nest_cnt` — ถ้ามี caller ใด disable irq โดยไม่ enable คืน, interrupt จะไม่กลับมาทำงาน (fallback: reset MCU)
- RISC-V มี interrupt priority — ตรวจสอบว่า PFIC อนุญาต interrupt นั้น

### ตัวแปร static ทับกัน
- `_adc_handle` ถูกใช้ร่วมระหว่าง `ADC_Read()`, `ADC_SimpleInitChannels()`, `analogRead()`
- ถ้าใช้หลายฟังก์ชันสลับกัน ฟังก์ชันสุดท้ายที่เรียกจะเป็นคนครอบครอง handle
- `_tim_handle` — timer instance เดียว — เรียก `TIM_SimpleInit()` ซ้ำจะทับของเก่า

---

*— จบเอกสาร —*
