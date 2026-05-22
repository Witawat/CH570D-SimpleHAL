# Compatibility Shim — SimpleHAL.h + Simple1Wire.h

## ภาพรวม

Compatibility shim คือเลเยอร์ที่ทำให้โค้ดจากโปรเจกต์ CH32V003 (โดยเฉพาะไลบรารีใน `src/lib/`)
สามารถคอมไพล์และทำงานบน CH570D ได้ โดยไม่ต้องแก้ไข source code เดิมของไลบรารีเหล่านั้น

Shim ใช้ SimpleHAL (HAL ของ CH570D) เป็น backend เบื้องหลัง

---

## SimpleHAL.h — ฟังก์ชันสูงสุด 48 รายการ

### GPIO (Arduino-style API)

```c
void pinMode(uint8_t pin, GPIO_PinMode mode);
void digitalWrite(uint8_t pin, uint8_t value);
uint8_t digitalRead(uint8_t pin);
void digitalToggle(uint8_t pin);
void pinModeMultiple(const uint8_t *pins, GPIO_PinMode mode);
```

- `mode` = `PIN_MODE_INPUT`, `PIN_MODE_OUTPUT`, `PIN_MODE_INPUT_PULLUP`, `PIN_MODE_INPUT_PULLDOWN`, `PIN_MODE_OUTPUT_OD`
- `OUTPUT_OD` จะทำงานเหมือน push-pull ปกติ (CH570D ไม่มี open-drain output mode)
- รองรับ PA0–PA15 (CH570D มีเฉพาะ PORTA)

### Interrupt

```c
void attachInterrupt(uint8_t pin, void (*callback)(void), GPIO_InterruptMode mode);
void detachInterrupt(uint8_t pin);
```

- `mode` = `RISING`, `FALLING`, `CHANGE`
- ใช้ `_irq_wrapper` เพื่อแปลง callback signature ให้เข้ากับ SimpleHAL

### เวลา

```c
uint32_t Get_CurrentMs(void);       // 1ms resolution
uint32_t Get_CurrentUs(void);       // 1us resolution (ใช้ SysTick)
void Delay_Ms(uint16_t ms);
void Delay_Us(uint16_t us);
```

### PWM

```c
void PWM_Init(PWM_Channel channel, uint32_t frequency_hz);
void PWM_Start(PWM_Channel channel);
void PWM_Stop(PWM_Channel channel);
void PWM_SetDutyCycle(PWM_Channel channel, uint8_t duty_percent);
```

- `PWM_Channel` = `PWM1_CH1`–`PWM1_CH4`, `PWM2_CH1`–`PWM2_CH4`
- CH570D มี PWM 5 ช่องจริง (PA0–PA4): `PWM1_CH1`–`PWM1_CH4` = PA0–PA3, `PWM2_CH1` = PA4

### I2C (Master)

```c
void I2C_SimpleInit(uint32_t speed_hz, uint8_t pins);
uint8_t I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len);
uint8_t I2C_Read(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t val);
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg);
uint8_t I2C_ReadRegMulti(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
uint8_t I2C_IsDeviceReady(uint8_t dev_addr);
```

- `speed_hz` = 100000 (standard), 400000 (fast)
- ส่งคืน `I2C_OK` (0) หรือ `I2C_ERROR` (1)
- ใช้ `hal_i2c` backend (PA12=SCL, PA13=SDA)

### USART (UART)

```c
void USART_SimpleInit(uint32_t baudrate, uint8_t pins);
void USART_WriteByte(uint8_t data);
uint8_t USART_Available(void);
uint8_t USART_Read(void);
void USART_Flush(void);
void USART_Print(const char *str);
```

- `baudrate` = `BAUD_9600`, `BAUD_19200`, `BAUD_38400`, `BAUD_57600`, `BAUD_115200`
- ใช้ UART0 (PA3=TX, PA2=RX) — มีช่องเดียว
- ปัจจุบันเป็น polling mode (ไม่มี interrupt RX callback)

### SPI, Timer, ADC (stub — ส่งคืน 0 หรือ no-op)

```c
void SPI_SimpleInit(SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg);
uint8_t SPI_Transfer(uint8_t data);
void TIM_SimpleInit(uint8_t tim, uint32_t period_us);
void ADC_Read(ADC_Channel ch);
```

สามารถเชื่อมต่อกับ `hal_spi`, `hal_timer`, `hal_adc` จริงได้ในภายหลัง

### อื่น ๆ

```c
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout);
void GPIO_SetBits(GPIO_TypeDef* port, GPIO_Pin pin);
void GPIO_ResetBits(GPIO_TypeDef* port, GPIO_Pin pin);
void __disable_irq(void);
void __enable_irq(void);
```

- `GPIO_SetBits/ResetBits` ทำงานเฉพาะ `port==GPIOA` (CH570D ไม่มี GPIOC/GPIOD)

---

## Simple1Wire.h — 1-Wire Protocol

### ใช้ตอนไหน

สำหรับไลบรารีที่สื่อสารผ่าน 1-Wire protocol เช่น **DS18B20** (temperature sensor), DS2431, iButton
โดย `DS18B20.h` จะ include `Simple1Wire.h` อัตโนมัติ

### ฟังก์ชัน

```c
OneWire_Bus* OneWire_Init(uint8_t pin);
bool OneWire_Reset(OneWire_Bus* bus);
void OneWire_WriteBit(OneWire_Bus* bus, uint8_t bit);
uint8_t OneWire_ReadBit(OneWire_Bus* bus);
void OneWire_WriteByte(OneWire_Bus* bus, uint8_t data);
uint8_t OneWire_ReadByte(OneWire_Bus* bus);
void OneWire_SkipROM(OneWire_Bus* bus);
bool OneWire_ReadROM(OneWire_Bus* bus, uint8_t* rom);
void OneWire_MatchROM(OneWire_Bus* bus, const uint8_t* rom);
bool OneWire_Select(OneWire_Bus* bus, const uint8_t* rom);
void OneWire_ResetSearch(OneWire_Bus* bus);
bool OneWire_Search(OneWire_Bus* bus);
bool OneWire_AlarmSearch(OneWire_Bus* bus);
void OneWire_GetAddress(OneWire_Bus* bus, uint8_t* rom);
uint8_t OneWire_CRC8(const uint8_t* data, uint8_t len);
```

### ตัวอย่าง

```c
#include "SimpleHAL/Simple1Wire.h"

OneWire_Bus* bus = OneWire_Init(PA5);
if (OneWire_Reset(bus)) {
    OneWire_SkipROM(bus);
    OneWire_WriteByte(bus, 0x44);  // DS18B20: start conversion
}
```

### ข้อควรระวัง

- ใช้ bit-bang GPIO ผ่าน `pinMode()`/`digitalWrite()`/`digitalRead()` + `Delay_Us()`
- Timing เป็น Standard Speed (ไม่รองรับ Overdrive)
- ต้องมี pull-up resistor 4.7kΩ ภายนอก
- ใช้ `__disable_irq()` ช่วง bit-bang ดังนั้นอาจกระทบ timing จริงของระบบ

---

## GPIO_Pin ชนิดและค่า

```c
GPIO_Pin_0  = 0x0001     GPIO_Pin_8  = 0x0100
GPIO_Pin_1  = 0x0002     GPIO_Pin_9  = 0x0200
GPIO_Pin_2  = 0x0004     GPIO_Pin_10 = 0x0400
GPIO_Pin_3  = 0x0008     GPIO_Pin_11 = 0x0800
GPIO_Pin_4  = 0x0010     GPIO_Pin_12 = 0x1000
GPIO_Pin_5  = 0x0020     GPIO_Pin_13 = 0x2000
GPIO_Pin_6  = 0x0040     GPIO_Pin_14 = 0x4000
GPIO_Pin_7  = 0x0080     GPIO_Pin_15 = 0x8000
GPIO_Pin_All = 0xFFFF
```

มี `#ifndef` guard ป้องกันการซ้ำกับ `CH57x_gpio.h`

---

## หมายเหตุสำคัญ

- CH570D มี **PORTA เท่านั้น** (PA0–PA15) — ไม่มี GPIOC, GPIOD
- หากไลบรารีอ้างถึง PC0–PC7, PD2–PD7 ต้องเปลี่ยนเป็น PAx ก่อนใช้งาน
- `PIN_MODE_OUTPUT_OD` → ใช้ push-pull ทดแทน (ไม่มี open-drain hardware)
- ฟังก์ชันที่ยังเป็น stub (return 0): `ADC_Read`, `SPI_Transfer`, `analogRead`
- `--gc-sections` จะตัด library code ที่ไม่ได้เรียกออก ทำให้ final binary เล็ก
