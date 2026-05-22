#ifndef __SIMPLEHAL_COMPAT_H__
#define __SIMPLEHAL_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "simple_hal.h"
#include <string.h>
#include <stdint.h>

#define SIMPLE_HAL_VERSION_MAJOR  1
#define SIMPLE_HAL_VERSION_MINOR  9
#define SIMPLE_HAL_VERSION_PATCH  0

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

#define I2C_OK   0
#define I2C_ERROR 1

#define ADC_MAX_VALUE  1024

#define BAUD_9600   9600
#define BAUD_19200  19200
#define BAUD_38400  38400
#define BAUD_57600  57600
#define BAUD_115200 115200

#define ELAPSED_TIME(last, now)  ((uint32_t)((now) - (last)))

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

typedef enum {
    PIN_MODE_INPUT = 0x10,
    PIN_MODE_OUTPUT,
    PIN_MODE_INPUT_PULLUP,
    PIN_MODE_INPUT_PULLDOWN,
    PIN_MODE_OUTPUT_OD
} GPIO_PinMode;

void pinMode(uint8_t pin, GPIO_PinMode mode);
void digitalWrite(uint8_t pin, uint8_t value);
uint8_t digitalRead(uint8_t pin);
void digitalToggle(uint8_t pin);
void pinModeMultiple(const uint8_t *pins, GPIO_PinMode mode);

typedef enum { RISING = 0, FALLING, CHANGE } GPIO_InterruptMode;
void attachInterrupt(uint8_t pin, void (*callback)(void), GPIO_InterruptMode mode);
void detachInterrupt(uint8_t pin);

static inline uint32_t Get_CurrentMs(void) { return hal_get_sys_tick(); }
static inline void Delay_Ms(uint16_t ms) { hal_delay_ms(ms); }
static inline void Delay_Us(uint16_t us) { hal_delay_us(us); }
uint32_t Get_CurrentUs(void);

typedef enum {
    PWM1_CH1 = 0, PWM1_CH2, PWM1_CH3, PWM1_CH4,
    PWM2_CH1
} PWM_Channel;

void PWM_Init(PWM_Channel channel, uint32_t frequency_hz);
void PWM_Init16bit(PWM_Channel channel, uint32_t frequency_hz);
void PWM_SetDutyCycle(PWM_Channel channel, uint8_t duty_percent);
void PWM_SetDutyCycleRaw(PWM_Channel channel, uint16_t duty_value);
uint16_t PWM_GetPeriod(PWM_Channel channel);
void PWM_Start(PWM_Channel channel);
void PWM_Stop(PWM_Channel channel);
void PWM_Write(PWM_Channel channel, uint8_t value);

void I2C_SimpleInit(uint32_t speed_hz, uint8_t pins);
uint8_t I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len);
uint8_t I2C_Read(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t val);
uint8_t I2C_ReadRegMulti(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg);
uint8_t I2C_IsDeviceReady(uint8_t dev_addr);

void USART_SimpleInit(uint32_t baudrate, uint8_t pins);
void USART_WriteByte(uint8_t data);
uint8_t USART_Available(void);
uint8_t USART_Read(void);
void USART_Flush(void);
void USART_Print(const char *str);
void USART_PrintNum(int32_t n);

void SPI_SimpleInit(SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg);
uint8_t SPI_Transfer(uint8_t data);

void GPIO_SetBits(GPIO_TypeDef* port, GPIO_Pin pin);
void GPIO_ResetBits(GPIO_TypeDef* port, GPIO_Pin pin);

void TIM_SimpleInit(uint8_t tim, uint32_t period_us);
void TIM_AttachInterrupt(uint8_t tim, void (*callback)(void));
void TIM_Start(uint8_t tim);
void TIM_Stop(uint8_t tim);
void TIM_DetachInterrupt(uint8_t tim);

void ADC_SimpleInitChannels(const ADC_Channel *channels, uint8_t count);
uint16_t ADC_Read(ADC_Channel ch);
float ADC_ToVoltage(uint16_t raw, float vref);

void __disable_irq(void);
void __enable_irq(void);

void Timer_Init(void);
uint16_t analogRead(uint8_t pin);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
