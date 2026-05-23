#ifndef __SIMPLEHAL_COMPAT_H__
#define __SIMPLEHAL_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "simple_hal.h"
#include <string.h>
#include <stdint.h>

/**
 * @brief   SimpleHAL version
 */
#define SIMPLE_HAL_VERSION_MAJOR  1
#define SIMPLE_HAL_VERSION_MINOR  9
#define SIMPLE_HAL_VERSION_PATCH  0

/**
 * @brief   เริ่มต้น SimpleHAL — ตั้งค่า flag ภายใน
 *
 * @note    ยังไม่ init ฮาร์ดแวร์ใด ๆ จริง — peripheral init เกิดขึ้นในฟังก์ชันของแต่ละ module
 */
void SimpleHAL_Init(void);

#ifndef HIGH
#define HIGH  1
#endif
#ifndef LOW
#define LOW   0
#endif
#ifndef LSBFIRST
#define LSBFIRST  0
#endif
#ifndef MSBFIRST
#define MSBFIRST  1
#endif

/**
 * @brief   I2C operation status
 */
#define I2C_OK   0
#define I2C_ERROR 1

/**
 * @brief   ค่าสูงสุดของ ADC (สมมติ 10-bit — จริง 8/9-bit)
 */
#define ADC_MAX_VALUE  1024

/**
 * @brief   Baud rate constants
 */
#define BAUD_9600   9600
#define BAUD_19200  19200
#define BAUD_38400  38400
#define BAUD_57600  57600
#define BAUD_115200 115200

/**
 * @brief   คำนวณเวลาที่ผ่านไป ปลอดภัยกับ wrap-around
 */
#define ELAPSED_TIME(last, now)  ((uint32_t)((now) - (last)))

/**
 * @brief   SPI mode constants (MODE1/MODE2 fallback เป็น MODE0/MODE3)
 */
#define SPI_MODE0  0
#define SPI_MODE1  1
#define SPI_MODE2  2
#define SPI_MODE3  3
#define SPI_1MHZ   1000000
#define SPI_2MHZ   2000000
#define SPI_4MHZ   4000000
#define SPI_8MHZ   8000000
#define SPI_PINS_DEFAULT  0
#define USART_PINS_DEFAULT  0

#define TIM_2  2

typedef uint16_t GPIO_Pin;
typedef uint8_t ADC_Channel;
typedef uint8_t SPI_Mode;
typedef uint32_t SPI_Speed;
typedef uint8_t SPI_PinConfig;
typedef uint8_t USART_PinConfig;
typedef uint32_t USART_BaudRate;
typedef uint8_t I2C_Status;

/**
 * @brief   GPIO port pointer (always GPIOA on CH57x)
 */
typedef void* GPIO_TypeDef;
#define GPIOA  ((GPIO_TypeDef*)1)

#ifndef GPIO_Pin_0
#define GPIO_Pin_0   0x0001
#endif
#ifndef GPIO_Pin_1
#define GPIO_Pin_1   0x0002
#endif
#ifndef GPIO_Pin_2
#define GPIO_Pin_2   0x0004
#endif
#ifndef GPIO_Pin_3
#define GPIO_Pin_3   0x0008
#endif
#ifndef GPIO_Pin_4
#define GPIO_Pin_4   0x0010
#endif
#ifndef GPIO_Pin_5
#define GPIO_Pin_5   0x0020
#endif
#ifndef GPIO_Pin_6
#define GPIO_Pin_6   0x0040
#endif
#ifndef GPIO_Pin_7
#define GPIO_Pin_7   0x0080
#endif
#ifndef GPIO_Pin_8
#define GPIO_Pin_8   0x0100
#endif
#ifndef GPIO_Pin_9
#define GPIO_Pin_9   0x0200
#endif
#ifndef GPIO_Pin_10
#define GPIO_Pin_10  0x0400
#endif
#ifndef GPIO_Pin_11
#define GPIO_Pin_11  0x0800
#endif
#ifndef GPIO_Pin_12
#define GPIO_Pin_12  0x1000
#endif
#ifndef GPIO_Pin_13
#define GPIO_Pin_13  0x2000
#endif
#ifndef GPIO_Pin_14
#define GPIO_Pin_14  0x4000
#endif
#ifndef GPIO_Pin_15
#define GPIO_Pin_15  0x8000
#endif
#ifndef GPIO_Pin_All
#define GPIO_Pin_All 0xFFFF
#endif

/**
 * @brief   GPIO_PinMode = hal_gpio_mode_t โดยตรง
 *
 * @note    PIN_MODE_OUTPUT_OD = 0xFF เป็น sentinel (จำลอง Open-Drain)
 */
typedef hal_gpio_mode_t GPIO_PinMode;

#define PIN_MODE_INPUT        HAL_GPIO_INPUT_FLOATING
#define PIN_MODE_OUTPUT       HAL_GPIO_OUTPUT_PP_5mA
#define PIN_MODE_INPUT_PULLUP HAL_GPIO_INPUT_PULLUP
#define PIN_MODE_INPUT_PULLDOWN HAL_GPIO_INPUT_PULLDOWN
#define PIN_MODE_OUTPUT_OD    0xFF

/**
 * @brief   กำหนดโหมดการทำงานของขา GPIO
 *
 * @param   pin - หมายเลขขา (PA0–PA15)
 * @param   mode - โหมด (INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT_OD)
 */
void pinMode(uint8_t pin, GPIO_PinMode mode);

/**
 * @brief   เขียน digital output (HIGH/LOW)
 *
 * @param   pin - หมายเลขขา
 * @param   value - 0 (LOW) หรือ 1 (HIGH)
 */
void digitalWrite(uint8_t pin, uint8_t value);

/**
 * @brief   อ่าน digital input
 *
 * @param   pin - หมายเลขขา
 *
 * @return  0 (LOW) หรือ 1 (HIGH)
 */
uint8_t digitalRead(uint8_t pin);

/**
 * @brief   กลับค่าขา (toggle)
 *
 * @param   pin - หมายเลขขา
 */
void digitalToggle(uint8_t pin);

/**
 * @brief   กำหนดโหมดขาหลายขาพร้อมกัน
 *
 * @param   pins - array ของหมายเลขขา (terminated ด้วย 0xFF)
 * @param   mode - โหมดที่ต้องการ
 */
void pinModeMultiple(const uint8_t *pins, GPIO_PinMode mode);

/**
 * @brief   โหมด interrupt สำหรับ GPIO
 */
typedef enum { RISING = 0, FALLING, CHANGE } GPIO_InterruptMode;

/**
 * @brief   ตั้ง interrupt callback เมื่อสัญญาณ GPIO เปลี่ยน
 *
 * @param   pin - หมายเลขขา
 * @param   callback - ฟังก์ชันที่จะเรียกเมื่อ interrupt เกิด
 * @param   mode - RISING / FALLING / CHANGE
 */
void attachInterrupt(uint8_t pin, void (*callback)(void), GPIO_InterruptMode mode);

/**
 * @brief   ยกเลิก interrupt ของขา
 *
 * @param   pin - หมายเลขขา
 */
void detachInterrupt(uint8_t pin);

/**
 * @brief   อ่าน millisecond ปัจจุบัน (นับจาก boot)
 *
 * @return  จำนวน ms ที่ผ่านไป
 */
static inline uint32_t Get_CurrentMs(void) { return hal_get_sys_tick(); }

/**
 * @brief   หน่วงเวลา (millisecond)
 *
 * @param   ms - จำนวน ms ที่ต้องการหน่วง
 */
static inline void Delay_Ms(uint16_t ms) { hal_delay_ms(ms); }

/**
 * @brief   หน่วงเวลา (microsecond)
 *
 * @param   us - จำนวน µs ที่ต้องการหน่วง
 */
static inline void Delay_Us(uint16_t us) { hal_delay_us(us); }

/**
 * @brief   อ่าน microsecond ปัจจุบัน (นับจาก boot)
 *
 * @return  จำนวน µs ที่ผ่านไป
 */
uint32_t Get_CurrentUs(void);

/**
 * @brief   ช่อง PWM ที่รองรับ
 */
typedef enum {
    PWM1_CH1 = 0, PWM1_CH2, PWM1_CH3, PWM1_CH4,
    PWM2_CH1
} PWM_Channel;

/**
 * @brief   เริ่มต้น PWM 8-bit mode
 *
 * @param   channel - ช่อง PWM
 * @param   frequency_hz - ความถี่ (Hz)
 */
void PWM_Init(PWM_Channel channel, uint32_t frequency_hz);

/**
 * @brief   เริ่มต้น PWM 16-bit mode
 *
 * @param   channel - ช่อง PWM
 * @param   frequency_hz - ความถี่ (Hz)
 */
void PWM_Init16bit(PWM_Channel channel, uint32_t frequency_hz);

/**
 * @brief   ตั้ง duty cycle (0–100%)
 *
 * @param   channel - ช่อง PWM
 * @param   duty_percent - 0–100
 */
void PWM_SetDutyCycle(PWM_Channel channel, uint8_t duty_percent);

/**
 * @brief   ตั้ง duty cycle (raw value 0–period)
 *
 * @param   channel - ช่อง PWM
 * @param   duty_value - ค่า duty โดยตรง (8-bit: 0–255, 16-bit: 0–period)
 */
void PWM_SetDutyCycleRaw(PWM_Channel channel, uint16_t duty_value);

/**
 * @brief   อ่านค่า period ปัจจุบัน
 *
 * @param   channel - ช่อง PWM
 *
 * @return  period (8-bit = 256, 16-bit = ค่าจริง)
 */
uint16_t PWM_GetPeriod(PWM_Channel channel);

/**
 * @brief   เริ่มสัญญาณ PWM
 *
 * @param   channel - ช่อง PWM
 */
void PWM_Start(PWM_Channel channel);

/**
 * @brief   หยุดสัญญาณ PWM
 *
 * @param   channel - ช่อง PWM
 */
void PWM_Stop(PWM_Channel channel);

/**
 * @brief   เขียนค่า PWM แบบ Arduino analogWrite (auto-init)
 *
 * @param   channel - ช่อง PWM
 * @param   value - 0–255
 */
void PWM_Write(PWM_Channel channel, uint8_t value);

/**
 * @brief   เริ่มต้น I2C master
 *
 * @param   speed_hz - ความเร็ว (100000 หรือ 400000)
 * @param   pins - ไม่ได้ใช้ (reserved)
 */
void I2C_SimpleInit(uint32_t speed_hz, uint8_t pins);

/**
 * @brief   เขียนข้อมูลไปยัง I2C slave (แยก transaction)
 *
 * @param   dev_addr - address ของ slave (7-bit, left-aligned)
 * @param   data - pointer ข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  I2C_OK หรือ I2C_ERROR
 */
uint8_t I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len);

/**
 * @brief   อ่านข้อมูลจาก I2C slave (แยก transaction)
 *
 * @param   dev_addr - address ของ slave
 * @param   data - buffer สำหรับรับ
 * @param   len - จำนวนไบต์ที่ต้องการ
 *
 * @return  I2C_OK หรือ I2C_ERROR
 */
uint8_t I2C_Read(uint8_t dev_addr, uint8_t *data, uint16_t len);

/**
 * @brief   เขียน register ของ I2C device (combined transaction)
 *
 * @param   dev_addr - address ของ slave
 * @param   reg - register address
 * @param   val - ค่าที่จะเขียน
 *
 * @return  I2C_OK หรือ I2C_ERROR
 */
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t val);

/**
 * @brief   อ่าน register ของ I2C device หลายไบต์ (combined transaction)
 *
 * @param   dev_addr - address ของ slave
 * @param   reg - register address
 * @param   data - buffer สำหรับรับ
 * @param   len - จำนวนไบต์
 *
 * @return  I2C_OK หรือ I2C_ERROR
 */
uint8_t I2C_ReadRegMulti(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

/**
 * @brief   อ่าน register ของ I2C device 1 ไบต์ (combined transaction)
 *
 * @param   dev_addr - address ของ slave
 * @param   reg - register address
 *
 * @return  ค่าที่อ่านได้
 */
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg);

/**
 * @brief   ตรวจสอบว่า I2C slave พร้อมทำงาน
 *
 * @param   dev_addr - address ของ slave
 *
 * @return  1 พร้อม, 0 ไม่พร้อม
 */
uint8_t I2C_IsDeviceReady(uint8_t dev_addr);

/**
 * @brief   เริ่มต้น UART
 *
 * @param   baudrate - อัตราบอด
 * @param   pins - ไม่ได้ใช้ (remap ใช้ผ่าน hal_uart โดยตรง)
 */
void USART_SimpleInit(uint32_t baudrate, uint8_t pins);

/**
 * @brief   ส่ง 1 ไบต์ทาง UART (blocking)
 *
 * @param   data - ข้อมูลที่จะส่ง
 */
void USART_WriteByte(uint8_t data);

/**
 * @brief   ตรวจสอบว่ามีข้อมูลรออ่านใน buffer หรือไม่
 *
 * @return  จำนวนไบต์ที่รออ่าน
 */
uint8_t USART_Available(void);

/**
 * @brief   อ่าน 1 ไบต์จาก UART (ไม่ blocking)
 *
 * @return  ข้อมูลที่อ่านได้ (0 ถ้าไม่มี)
 */
uint8_t USART_Read(void);

/**
 * @brief   ล้าง buffer รับ UART
 */
void USART_Flush(void);

/**
 * @brief   พิมพ์ string ทาง UART
 *
 * @param   str - string ที่ต้องการพิมพ์
 */
void USART_Print(const char *str);

/**
 * @brief   พิมพ์ตัวเลขจำนวนเต็มทาง UART
 *
 * @param   n - ค่าที่ต้องการพิมพ์
 */
void USART_PrintNum(int32_t n);

/**
 * @brief   เริ่มต้น SPI master
 *
 * @param   mode - SPI_MODE0–3 (MODE1→0, MODE2→3 fallback)
 * @param   speed - ความเร็ว (เช่น SPI_1MHZ)
 * @param   pin_cfg - ไม่ได้ใช้ (reserved)
 */
void SPI_SimpleInit(SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg);

/**
 * @brief   ส่งและรับ 1 ไบต์พร้อมกัน (full-duplex)
 *
 * @param   data - ข้อมูลที่จะส่ง
 *
 * @return  ข้อมูลที่รับได้
 */
uint8_t SPI_Transfer(uint8_t data);

/**
 * @brief   เซ็ตบิต GPIO (ใช้ bitmask แบบ STM32)
 *
 * @param   port - เฉพาะ GPIOA
 * @param   pin - bitmask ของขา (GPIO_Pin_x)
 */
void GPIO_SetBits(GPIO_TypeDef* port, GPIO_Pin pin);

/**
 * @brief   เคลียร์บิต GPIO (ใช้ bitmask แบบ STM32)
 *
 * @param   port - เฉพาะ GPIOA
 * @param   pin - bitmask ของขา (GPIO_Pin_x)
 */
void GPIO_ResetBits(GPIO_TypeDef* port, GPIO_Pin pin);

/**
 * @brief   เริ่มต้น Timer
 *
 * @param   tim - ไม่ได้ใช้ (reserved)
 * @param   period_us - period ใน microsecond
 */
void TIM_SimpleInit(uint8_t tim, uint32_t period_us);

/**
 * @brief   ตั้ง callback เมื่อ timer ครบ period
 *
 * @param   tim - ไม่ได้ใช้ (reserved)
 * @param   callback - ฟังก์ชันที่จะเรียก
 */
void TIM_AttachInterrupt(uint8_t tim, void (*callback)(void));

/**
 * @brief   เริ่มนับ timer
 *
 * @param   tim - ไม่ได้ใช้ (reserved)
 */
void TIM_Start(uint8_t tim);

/**
 * @brief   หยุดนับ timer
 *
 * @param   tim - ไม่ได้ใช้ (reserved)
 */
void TIM_Stop(uint8_t tim);

/**
 * @brief   ยกเลิก callback ของ timer
 *
 * @param   tim - ไม่ได้ใช้ (reserved)
 */
void TIM_DetachInterrupt(uint8_t tim);

/**
 * @brief   เริ่มต้น ADC หลาย channel (auto-switch)
 *
 * @param   channels - array ของ channel
 * @param   count - จำนวน channel
 */
void ADC_SimpleInitChannels(const ADC_Channel *channels, uint8_t count);

/**
 * @brief   อ่านค่า ADC ของ channel
 *
 * @param   ch - channel ที่ต้องการอ่าน
 *
 * @return  ค่า raw ADC
 */
uint16_t ADC_Read(ADC_Channel ch);

/**
 * @brief   แปลงค่า raw ADC เป็นแรงดัน (volt)
 *
 * @param   raw - ค่า raw
 * @param   vref - แรงดันอ้างอิง (mV หรือ V)
 *
 * @return  แรงดันที่คำนวณได้
 */
float ADC_ToVoltage(uint16_t raw, float vref);

/**
 * @brief   ปิด interrupt ทั้งหมด (มี nesting counter)
 *
 * @note    เรียกซ้อนกันได้ — ต้องเรียก __enable_irq ให้เท่ากับจำนวนที่ disable
 */
void __disable_irq(void);

/**
 * @brief   เปิด interrupt กลับคืน (มี nesting counter)
 *
 * @note    คืนค่าเฉพาะเมื่อเรียกครบตาม nesting depth
 */
void __enable_irq(void);

/**
 * @brief   เริ่มต้น time base ของระบบ (ให้ hal_get_sys_tick() ทำงาน)
 */
void Timer_Init(void);

/**
 * @brief   อ่านค่า analog (Arduino-compat)
 *
 * @param   pin - หมายเลขขา
 *
 * @return  ค่า raw ADC (0–1023)
 */
uint16_t analogRead(uint8_t pin);

/**
 * @brief   ส่งข้อมูลแบบ shiftOut (bit-bang SPI)
 *
 * @param   dataPin - ขาข้อมูล
 * @param   clockPin - ขานาฬิกา
 * @param   bitOrder - LSBFIRST หรือ MSBFIRST
 * @param   value - ข้อมูล 1 ไบต์
 */
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value);

/**
 * @brief   รับข้อมูลแบบ shiftIn (bit-bang SPI)
 *
 * @param   dataPin - ขาข้อมูล
 * @param   clockPin - ขานาฬิกา
 * @param   bitOrder - LSBFIRST หรือ MSBFIRST
 *
 * @return  ข้อมูล 1 ไบต์
 */
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

/**
 * @brief   วัดความกว้าง pulse (µs)
 *
 * @param   pin - ขาที่วัด
 * @param   state - HIGH หรือ LOW ที่ต้องการวัด
 * @param   timeout - timeout (µs), 0 = ไม่จำกัด
 *
 * @return  ความกว้าง pulse (µs) หรือ 0 ถ้า timeout
 */
uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
