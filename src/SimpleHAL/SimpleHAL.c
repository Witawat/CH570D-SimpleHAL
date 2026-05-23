#include "SimpleHAL.h"
#include "Simple1Wire.h"
#include "core_riscv.h"
#include <stdio.h>

#define COMPAT_GPIO_MAX_PINS  HAL_GPIO_MAX_PINS

/* --- GPIO state arrays --- */
static hal_gpio_handle_t _gpio_handles[COMPAT_GPIO_MAX_PINS] = {0};
static GPIO_PinMode      _shim_modes[COMPAT_GPIO_MAX_PINS] = {0};
static uint8_t           _gpio_initialized[COMPAT_GPIO_MAX_PINS] = {0};

/* --- I2C state --- */
static hal_i2c_handle_t  _i2c_handle = NULL;
static uint8_t           _i2c_ready = 0;

/* --- UART state --- */
static hal_uart_handle_t _uart_handle = NULL;
static uint8_t           _uart_ready = 0;

/* --- GPIO interrupt state --- */
#define COMPAT_IRQ_CALLBACKS_MAX  HAL_GPIO_MAX_PINS
static void (*_irq_user_callbacks[COMPAT_IRQ_CALLBACKS_MAX])(void) = {0};
static GPIO_InterruptMode _irq_modes[COMPAT_IRQ_CALLBACKS_MAX];

/**
 * @brief   ตัวกลาง interrupt GPIO — เรียก callback ของผู้ใช้ และจัดการ CHANGE mode
 *
 * @param   arg - หมายเลขขา (uintptr_t)
 */
static void _irq_wrapper(void *arg)
{
    uintptr_t pin = (uintptr_t)arg;
    if (pin < COMPAT_IRQ_CALLBACKS_MAX) {
        if (_irq_modes[pin] == CHANGE) {
            uint8_t level = hal_gpio_read(_gpio_handles[pin]);
            hal_gpio_irq_mode_t next = level ? HAL_GPIO_IRQ_FALLING : HAL_GPIO_IRQ_RISING;
            hal_gpio_attach_irq(_gpio_handles[pin], next, _irq_wrapper, (void*)(uintptr_t)pin);
        }
        if (_irq_user_callbacks[pin])
            _irq_user_callbacks[pin]();
    }
}

/* --- PWM state --- */
#define COMPAT_PWM_CHANNELS 5
static hal_pwm_handle_t _pwm_handles[COMPAT_PWM_CHANNELS] = {0};
static uint8_t          _pwm_inited[COMPAT_PWM_CHANNELS] = {0};
static uint8_t          _pwm_is_16bit[COMPAT_PWM_CHANNELS] = {0};
static uint16_t         _pwm_period[COMPAT_PWM_CHANNELS] = {0};

/* --- Timer state --- */
static hal_timer_handle_t _tim_handle = NULL;
static void (*_tim_callback)(void) = NULL;

/* --- SPI state --- */
static hal_spi_handle_t _spi_handle = NULL;
static uint8_t          _spi_ready = 0;

/* --- ADC state --- */
static hal_adc_handle_t _adc_handle = NULL;
static hal_adc_resolution_t _adc_resolution = HAL_ADC_8BIT;
static ADC_Channel _adc_current_channel = 0;

/* --- HAL init flag --- */
static uint8_t _hal_inited = 0;

/**
 * @brief   แปลง PWM_Channel (Arduino-compat) → hal_pwm_channel_t
 *
 * @param   ch - ช่อง PWM
 *
 * @return  channel enum ของ HAL
 */
static hal_pwm_channel_t _map_pwm_channel(PWM_Channel ch)
{
    switch (ch) {
        case PWM1_CH1: return HAL_PWM_CH1;
        case PWM1_CH2: return HAL_PWM_CH2;
        case PWM1_CH3: return HAL_PWM_CH3;
        case PWM1_CH4: return HAL_PWM_CH4;
        case PWM2_CH1: return HAL_PWM_CH5;
        default:       return 0;
    }
}

/**
 * @brief   เริ่มต้น SimpleHAL (ตั้ง flag เท่านั้น)
 *
 * @note    เรียกครั้งเดียวตอน boot — ฮาร์ดแวร์ init ในแต่ละ module
 */
void SimpleHAL_Init(void)
{
    if (_hal_inited) return;
    _hal_inited = 1;
}

/**
 * @brief   กำหนดโหมดขา GPIO
 *
 * @note    PIN_MODE_OUTPUT_OD: จำลอง OD โดยสลับ Input Floating / PP_5mA
 */
void pinMode(uint8_t pin, GPIO_PinMode mode)
{
    if (pin >= COMPAT_GPIO_MAX_PINS) return;
    _shim_modes[pin] = mode;
    if (mode == PIN_MODE_OUTPUT_OD) {
        _gpio_handles[pin] = hal_gpio_init(pin, HAL_GPIO_OUTPUT_PP_5mA);
        if (_gpio_handles[pin]) hal_gpio_write(_gpio_handles[pin], 1);
        _gpio_handles[pin] = hal_gpio_init(pin, HAL_GPIO_INPUT_FLOATING);
    } else {
        _gpio_handles[pin] = hal_gpio_init(pin, (hal_gpio_mode_t)mode);
    }
    _gpio_initialized[pin] = (_gpio_handles[pin] != NULL) ? 1 : 0;
}

/**
 * @brief   กำหนดโหมดขาหลายขาพร้อมกัน (terminated ด้วย 0xFF)
 */
void pinModeMultiple(const uint8_t *pins, GPIO_PinMode mode)
{
    if (!pins) return;
    while (*pins != 0xFF) {
        pinMode(*pins, mode);
        pins++;
    }
}

/**
 * @brief   เขียน digital output — จัดการ OPEN_DRAIN ด้วยการสลับ Input/Output
 *
 * @note    OD mode: HIGH → Input Floating, LOW → Output PP_5mA + write 0
 */
void digitalWrite(uint8_t pin, uint8_t value)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return;
    if (_shim_modes[pin] == PIN_MODE_OUTPUT_OD) {
        if (value) {
            _gpio_handles[pin] = hal_gpio_init(pin, HAL_GPIO_INPUT_FLOATING);
        } else {
            _gpio_handles[pin] = hal_gpio_init(pin, HAL_GPIO_OUTPUT_PP_5mA);
            hal_gpio_write(_gpio_handles[pin], 0);
        }
        return;
    }
    hal_gpio_write(_gpio_handles[pin], value);
}

/**
 * @brief   อ่าน digital input
 */
uint8_t digitalRead(uint8_t pin)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return 0;
    return hal_gpio_read(_gpio_handles[pin]);
}

/**
 * @brief   กลับค่า output (toggle)
 */
void digitalToggle(uint8_t pin)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return;
    hal_gpio_toggle(_gpio_handles[pin]);
}

/**
 * @brief   ตั้ง interrupt callback สำหรับ GPIO
 *
 * @note    CHANGE mode: อ่าน level ปัจจุบัน เลือก edge ตรงกันข้าม
 *          แล้วสลับ edge ทุกครั้งที่เกิด interrupt
 */
void attachInterrupt(uint8_t pin, void (*callback)(void), GPIO_InterruptMode mode)
{
    if (pin >= COMPAT_GPIO_MAX_PINS) return;
    if (!_gpio_initialized[pin]) {
        _gpio_handles[pin] = hal_gpio_init(pin, HAL_GPIO_INPUT_FLOATING);
        _gpio_initialized[pin] = (_gpio_handles[pin] != NULL) ? 1 : 0;
    }
    if (!_gpio_initialized[pin]) return;
    _irq_user_callbacks[pin] = callback;
    _irq_modes[pin] = mode;

    if (mode == CHANGE) {
        uint8_t level = hal_gpio_read(_gpio_handles[pin]);
        hal_gpio_irq_mode_t irq_mode = level ? HAL_GPIO_IRQ_FALLING : HAL_GPIO_IRQ_RISING;
        hal_gpio_attach_irq(_gpio_handles[pin], irq_mode, _irq_wrapper, (void*)(uintptr_t)pin);
    } else {
        hal_gpio_irq_mode_t irq_mode;
        switch (mode) {
            case RISING:  irq_mode = HAL_GPIO_IRQ_RISING; break;
            default:      irq_mode = HAL_GPIO_IRQ_FALLING; break;
        }
        hal_gpio_attach_irq(_gpio_handles[pin], irq_mode, _irq_wrapper, (void*)(uintptr_t)pin);
    }
}

/**
 * @brief   ยกเลิก interrupt GPIO
 */
void detachInterrupt(uint8_t pin)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return;
    _irq_user_callbacks[pin] = NULL;
    _irq_modes[pin] = FALLING;
    hal_gpio_detach_irq(_gpio_handles[pin]);
}

/**
 * @brief   อ่าน microsecond ปัจจุบัน (จาก SysTick)
 *
 * @note    อ่าน SysTick->CNT ก่อน hal_get_sys_tick() เพื่อลด race condition
 */
uint32_t Get_CurrentUs(void)
{
    uint32_t cnt = SysTick->CNT;
    uint32_t ms = hal_get_sys_tick();
    uint32_t cmp = SysTick->CMP;
    uint32_t us_in_tick = (cmp - cnt) * 1000 / (cmp + 1);
    return ms * 1000 + us_in_tick;
}

/**
 * @brief   เริ่มต้น PWM 8-bit mode
 *
 * @note    ถ้า frequency < 1000 Hz จะ fallback เป็น 16-bit mode
 */
void PWM_Init(PWM_Channel channel, uint32_t frequency_hz)
{
    if (channel >= COMPAT_PWM_CHANNELS) return;
    hal_pwm_channel_t hch = _map_pwm_channel(channel);
    if (frequency_hz < 1000) {
        PWM_Init16bit(channel, frequency_hz);
        return;
    }
    _pwm_handles[channel] = hal_pwm_init(hch, frequency_hz, 0);
    _pwm_inited[channel] = (_pwm_handles[channel] != NULL) ? 1 : 0;
    _pwm_is_16bit[channel] = 0;
    _pwm_period[channel] = 256;
}

/**
 * @brief   เริ่มต้น PWM 16-bit mode
 *
 * @note    อ่าน period จริงจาก internal struct (cycle_16bit)
 */
void PWM_Init16bit(PWM_Channel channel, uint32_t frequency_hz)
{
    if (channel >= COMPAT_PWM_CHANNELS) return;
    hal_pwm_channel_t hch = _map_pwm_channel(channel);
    _pwm_handles[channel] = hal_pwm_init_16bit(hch, frequency_hz, 0);
    _pwm_inited[channel] = (_pwm_handles[channel] != NULL) ? 1 : 0;
    _pwm_is_16bit[channel] = 1;
    if (_pwm_inited[channel] && _pwm_handles[channel] != NULL) {
        _pwm_period[channel] = _pwm_handles[channel]->cycle_16bit;
    }
}

/**
 * @brief   ตั้ง duty cycle 8-bit (0–100%)
 */
void PWM_SetDutyCycle(PWM_Channel channel, uint8_t duty_percent)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    hal_pwm_set_duty(_pwm_handles[channel], duty_percent);
}

/**
 * @brief   ตั้ง duty cycle แบบ raw value (0–period)
 */
void PWM_SetDutyCycleRaw(PWM_Channel channel, uint16_t duty_value)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    if (_pwm_is_16bit[channel]) {
        hal_pwm_set_duty_16bit(_pwm_handles[channel], duty_value);
    } else {
        hal_pwm_set_duty(_pwm_handles[channel], (uint8_t)(duty_value & 0xFF));
    }
}

/**
 * @brief   อ่าน period ของ channel
 */
uint16_t PWM_GetPeriod(PWM_Channel channel)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return 0;
    return _pwm_period[channel];
}

/**
 * @brief   เริ่มสัญญาณ PWM
 */
void PWM_Start(PWM_Channel channel)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    hal_pwm_start(_pwm_handles[channel]);
}

/**
 * @brief   หยุดสัญญาณ PWM
 */
void PWM_Stop(PWM_Channel channel)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    hal_pwm_stop(_pwm_handles[channel]);
}

/**
 * @brief   Arduino-compat analogWrite — auto-init ที่ 1kHz ถ้ายังไม่ init
 */
void PWM_Write(PWM_Channel channel, uint8_t value)
{
    if (!_pwm_inited[channel]) PWM_Init(channel, 1000);
    uint8_t pct = (uint8_t)(((uint16_t)value * 100) / 255);
    PWM_SetDutyCycle(channel, pct);
    PWM_Start(channel);
}

/**
 * @brief   เริ่มต้น I2C master
 *
 * @note    pins parameter ถูกละเว้น — ใช้ I2C default pins (SDA=PA9, SCL=PA10)
 */
void I2C_SimpleInit(uint32_t speed_hz, uint8_t pins)
{
    (void)pins;
    _i2c_handle = hal_i2c_init(speed_hz, HAL_I2C_MASTER, 0);
    _i2c_ready = (_i2c_handle != NULL) ? 1 : 0;
}

/**
 * @brief   เขียนข้อมูลไปยัง I2C slave (แยก transaction — มี STOP คั่น)
 */
uint8_t I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (!_i2c_ready) return I2C_ERROR;
    return (hal_i2c_write_raw(_i2c_handle, dev_addr, data, len) == HAL_OK) ? I2C_OK : I2C_ERROR;
}

/**
 * @brief   อ่านข้อมูลจาก I2C slave (แยก transaction — มี STOP คั่น)
 */
uint8_t I2C_Read(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (!_i2c_ready) return I2C_ERROR;
    return (hal_i2c_read_raw(_i2c_handle, dev_addr, data, len) == HAL_OK) ? I2C_OK : I2C_ERROR;
}

/**
 * @brief   เขียน register ของ I2C device (combined transaction — repeated start)
 *
 * @note    ใช้ hal_i2c_write() ซึ่งส่ง reg address แล้วตามด้วย data ในครั้งเดียว
 */
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t val)
{
    if (!_i2c_ready) return I2C_ERROR;
    return (hal_i2c_write(_i2c_handle, dev_addr, reg, &val, 1) == HAL_OK) ? I2C_OK : I2C_ERROR;
}

/**
 * @brief   อ่าน register ของ I2C device หลายไบต์ (combined transaction)
 *
 * @note    ใช้ hal_i2c_read() ซึ่งส่ง reg address แล้วตามด้วย read ด้วย repeated start
 */
uint8_t I2C_ReadRegMulti(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!_i2c_ready) return I2C_ERROR;
    return (hal_i2c_read(_i2c_handle, dev_addr, reg, data, len) == HAL_OK) ? I2C_OK : I2C_ERROR;
}

/**
 * @brief   อ่าน register ของ I2C device 1 ไบต์
 */
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg)
{
    uint8_t val = 0;
    if (I2C_ReadRegMulti(dev_addr, reg, &val, 1) != I2C_OK) return 0;
    return val;
}

/**
 * @brief   ตรวจสอบ I2C slave พร้อมทำงานหรือไม่
 *
 * @note    ส่ง dummy write (0 bytes) ดูว่า slave ACK หรือ NAK
 */
uint8_t I2C_IsDeviceReady(uint8_t dev_addr)
{
    if (!_i2c_ready) return 0;
    uint8_t dummy = 0;
    return (hal_i2c_write_raw(_i2c_handle, dev_addr, &dummy, 0) == HAL_OK) ? 1 : 0;
}

/**
 * @brief   เริ่มต้น UART
 *
 * @note    pins parameter ถูกละเว้น — ใช้ UART default pins (TX=PA3, RX=PA2)
 */
void USART_SimpleInit(uint32_t baudrate, uint8_t pins)
{
    (void)pins;
    _uart_handle = hal_uart_init(baudrate, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    _uart_ready = (_uart_handle != NULL) ? 1 : 0;
}

/**
 * @brief   ส่ง 1 ไบต์ทาง UART (blocking)
 */
void USART_WriteByte(uint8_t data)
{
    if (!_uart_ready) return;
    hal_uart_send_byte(_uart_handle, data);
}

/**
 * @brief   ตรวจสอบจำนวนไบต์ที่รออ่าน
 */
uint8_t USART_Available(void)
{
    if (!_uart_ready) return 0;
    return (uint8_t)hal_uart_available(_uart_handle);
}

/**
 * @brief   อ่าน 1 ไบต์ (ไม่ blocking — คืน 0 ถ้าไม่มีข้อมูล)
 */
uint8_t USART_Read(void)
{
    if (!_uart_ready) return 0;
    return hal_uart_recv_byte(_uart_handle);
}

/**
 * @brief   ล้าง buffer รับ UART
 */
void USART_Flush(void)
{
    if (!_uart_ready) return;
    hal_uart_flush(_uart_handle);
}

/**
 * @brief   พิมพ์ string ทาง UART
 */
void USART_Print(const char *str)
{
    if (!_uart_ready) return;
    hal_uart_send(_uart_handle, (const uint8_t *)str, strlen(str));
}

/**
 * @brief   พิมพ์ตัวเลขจำนวนเต็ม (int32_t) ทาง UART
 *
 * @note    ใช้ sprintf แปลงเป็น string ก่อนแล้ว USART_Print
 */
void USART_PrintNum(int32_t n)
{
    if (!_uart_ready) return;
    char buf[16];
    sprintf(buf, "%ld", (long)n);
    USART_Print(buf);
}

/**
 * @brief   เริ่มต้น SPI master
 *
 * @note    MODE1 fallback → MODE0, MODE2 fallback → MODE3
 *          pin_cfg ถูกละเว้น — ใช้ขาคงที่ (SCK=PA5, MOSI=PA6, MISO=PA4)
 */
void SPI_SimpleInit(SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg)
{
    (void)pin_cfg;
    hal_spi_mode_t hal_mode;
    switch (mode) {
        case SPI_MODE0: hal_mode = HAL_SPI_MODE0; break;
        case SPI_MODE1: hal_mode = HAL_SPI_MODE0; break;
        case SPI_MODE2: hal_mode = HAL_SPI_MODE3; break;
        case SPI_MODE3: hal_mode = HAL_SPI_MODE3; break;
        default:        hal_mode = HAL_SPI_MODE0; break;
    }
    _spi_handle = hal_spi_init(speed, hal_mode, HAL_SPI_MSB_FIRST);
    _spi_ready = (_spi_handle != NULL) ? 1 : 0;
}

/**
 * @brief   ส่ง + รับ 1 ไบต์ (full-duplex)
 */
uint8_t SPI_Transfer(uint8_t data)
{
    if (!_spi_ready) return 0;
    hal_spi_send_byte(_spi_handle, data);
    return hal_spi_recv_byte(_spi_handle);
}

/**
 * @brief   เซ็ตบิต GPIO (stm32-compat) — เฉพาะ GPIOA
 */
void GPIO_SetBits(GPIO_TypeDef* port, GPIO_Pin pin)
{
    if (port != GPIOA) return;
    uint8_t p = 0;
    while (pin > 1) { pin >>= 1; p++; }
    digitalWrite(p, HIGH);
}

/**
 * @brief   เคลียร์บิต GPIO (stm32-compat) — เฉพาะ GPIOA
 */
void GPIO_ResetBits(GPIO_TypeDef* port, GPIO_Pin pin)
{
    if (port != GPIOA) return;
    uint8_t p = 0;
    while (pin > 1) { pin >>= 1; p++; }
    digitalWrite(p, LOW);
}

/**
 * @brief   ตัวกลาง interrupt timer — เรียก callback ของผู้ใช้
 */
static void _tim_irq_wrapper(void *arg)
{
    (void)arg;
    if (_tim_callback) _tim_callback();
}

/**
 * @brief   เริ่มต้น Timer (periodic mode)
 *
 * @note    tim parameter ถูกละเว้น — มี timer instance เดียว
 */
void TIM_SimpleInit(uint8_t tim, uint32_t period_us)
{
    (void)tim;
    _tim_handle = hal_timer_init(period_us, HAL_TIMER_MODE_PERIODIC);
}

/**
 * @brief   ตั้ง callback timer interrupt
 */
void TIM_AttachInterrupt(uint8_t tim, void (*callback)(void))
{
    (void)tim;
    _tim_callback = callback;
    if (_tim_handle) hal_timer_attach_cb(_tim_handle, _tim_irq_wrapper, NULL);
}

/**
 * @brief   เริ่มนับ timer
 */
void TIM_Start(uint8_t tim)
{
    (void)tim;
    if (_tim_handle) hal_timer_start(_tim_handle);
}

/**
 * @brief   หยุดนับ timer
 */
void TIM_Stop(uint8_t tim)
{
    (void)tim;
    if (_tim_handle) hal_timer_stop(_tim_handle);
}

/**
 * @brief   ยกเลิก timer callback
 */
void TIM_DetachInterrupt(uint8_t tim)
{
    (void)tim;
    _tim_callback = NULL;
    if (_tim_handle) hal_timer_attach_cb(_tim_handle, NULL, NULL);
}

/**
 * @brief   เริ่มต้น ADC หลาย channel แบบ auto-switch
 *
 * @note    resolution hardcode เป็น HAL_ADC_8BIT
 *          ถ้ามี handle เดิมจะ free ก่อน
 */
void ADC_SimpleInitChannels(const ADC_Channel *channels, uint8_t count)
{
    if (!channels || count == 0) return;
    if (_adc_handle) hal_adc_free(_adc_handle);
    _adc_resolution = HAL_ADC_8BIT;
    _adc_current_channel = channels[0];
    _adc_handle = hal_adc_init(_adc_resolution, channels[0]);
}

/**
 * @brief   อ่านค่า ADC — สลับ channel อัตโนมัติถ้าต่างจาก channel ปัจจุบัน
 *
 * @note    ถ้า ch != _adc_current_channel → free handle + re-init channel ใหม่
 */
uint16_t ADC_Read(ADC_Channel ch)
{
    if (ch != _adc_current_channel) {
        if (_adc_handle) hal_adc_free(_adc_handle);
        _adc_handle = hal_adc_init(_adc_resolution, ch);
        _adc_current_channel = ch;
    }
    if (!_adc_handle) return 0;
    return hal_adc_read(_adc_handle);
}

/**
 * @brief   แปลง raw ADC → voltage
 *
 * @note    ใช้ / 1024.0f เสมอ (แม้ ADC จริงจะเป็น 8/9-bit)
 */
float ADC_ToVoltage(uint16_t raw, float vref)
{
    return (float)raw * vref / 1024.0f;
}

/* --- IRQ nesting counter --- */
static uint32_t _irq_saved_state = 0;
static uint8_t  _irq_nest_cnt = 0;

/**
 * @brief   ปิด interrupt (มี nesting counter)
 *
 * @note    เก็บค่า mstatus register เฉพาะครั้งแรก
 *          เรียกซ้อนกันได้ — ครั้งที่ 2+ แค่เพิ่ม cnt
 */
void __disable_irq(void)
{
    if (_irq_nest_cnt++ == 0)
        _irq_saved_state = __risc_v_disable_irq();
}

/**
 * @brief   เปิด interrupt (มี nesting counter)
 *
 * @note    คืนค่า mstatus register เฉพาะเมื่อ cnt กลับเป็น 0
 */
void __enable_irq(void)
{
    if (_irq_nest_cnt > 0 && --_irq_nest_cnt == 0)
        __risc_v_enable_irq(_irq_saved_state);
}

/**
 * @brief   เริ่มต้น time base — เรียกให้ SysTick เริ่มนับ
 */
void Timer_Init(void)
{
    hal_get_sys_tick();
}

/**
 * @brief   Arduino-compat analogRead — init + read + free
 *
 * @note    ถ้ามี adc handle เก่าอยู่ จะ free แล้ว re-init
 */
uint16_t analogRead(uint8_t pin)
{
    if (_adc_handle) {
        hal_adc_free(_adc_handle);
        _adc_handle = NULL;
    }
    _adc_handle = hal_adc_init(HAL_ADC_8BIT, pin);
    if (!_adc_handle) return 0;
    return hal_adc_read(_adc_handle);
}

/**
 * @brief   bit-bang shiftOut (LSBFIRST/MSBFIRST)
 */
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value)
{
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (bitOrder == LSBFIRST) ? (value >> i) & 1 : (value >> (7 - i)) & 1;
        digitalWrite(dataPin, bit);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

/**
 * @brief   bit-bang shiftIn (LSBFIRST/MSBFIRST)
 */
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        uint8_t bit = digitalRead(dataPin);
        digitalWrite(clockPin, LOW);
        if (bitOrder == LSBFIRST) {
            value |= (bit << i);
        } else {
            value = (value << 1) | bit;
        }
    }
    return value;
}

/**
 * @brief   วัดความกว้าง pulse (microsecond)
 *
 * @note    polling mode — disable interrupt ขณะวัด
 *          timeout = 0 หมายถึงไม่จำกัด
 */
uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout)
{
    uint32_t start = Get_CurrentUs();
    while (digitalRead(pin) != state) {
        if (timeout && (Get_CurrentUs() - start) >= timeout) return 0;
    }
    uint32_t pulse_start = Get_CurrentUs();
    while (digitalRead(pin) == state) {
        if (timeout && (Get_CurrentUs() - pulse_start) >= timeout) return 0;
    }
    return Get_CurrentUs() - pulse_start;
}

/* ========== OneWire (1-Wire Protocol) Implementation ========== */

static OneWire_Bus onewire_buses[ONEWIRE_MAX_BUSES];
static uint8_t onewire_bus_count = 0;

static bool OneWire_SearchInternal(OneWire_Bus* bus, uint8_t command);

/**
 * @brief   เริ่มต้นบัส 1-Wire
 *
 * @note    ถ้าขานี้มีบัสอยู่แล้ว → คืน pointer เดิม
 *          สูงสุด ONEWIRE_MAX_BUSES (4) บัส
 */
OneWire_Bus* OneWire_Init(uint8_t pin) {
    if (onewire_bus_count >= ONEWIRE_MAX_BUSES) return NULL;
    for (uint8_t i = 0; i < onewire_bus_count; i++) {
        if (onewire_buses[i].pin == pin) return &onewire_buses[i];
    }
    OneWire_Bus* bus = &onewire_buses[onewire_bus_count++];
    memset(bus, 0, sizeof(OneWire_Bus));
    bus->pin = pin;
    bus->initialized = true;
    pinMode(pin, PIN_MODE_INPUT);
    return bus;
}

/**
 * @brief   ส่ง reset pulse — ตรวจสอบ presence
 *
 * @note   -disable interrupt ขณะส่ง (480µs + 240µs = ~720µs)
 */
bool OneWire_Reset(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return false;
    bool presence;
    __disable_irq();
    pinMode(bus->pin, PIN_MODE_OUTPUT);
    digitalWrite(bus->pin, LOW);
    Delay_Us(ONEWIRE_RESET_PULSE);
    pinMode(bus->pin, PIN_MODE_INPUT);
    Delay_Us(ONEWIRE_PRESENCE_WAIT);
    presence = !digitalRead(bus->pin);
    __enable_irq();
    Delay_Us(ONEWIRE_PRESENCE_TIMEOUT);
    return presence;
}

/**
 * @brief   เขียน 1 bit ตาม time slot 1-Wire
 */
void OneWire_WriteBit(OneWire_Bus* bus, uint8_t bit) {
    if (!bus || !bus->initialized) return;
    __disable_irq();
    if (bit & 1) {
        pinMode(bus->pin, PIN_MODE_OUTPUT);
        digitalWrite(bus->pin, LOW);
        Delay_Us(ONEWIRE_WRITE_1_LOW);
        pinMode(bus->pin, PIN_MODE_INPUT);
        Delay_Us(ONEWIRE_SLOT_TIME - ONEWIRE_WRITE_1_LOW);
    } else {
        pinMode(bus->pin, PIN_MODE_OUTPUT);
        digitalWrite(bus->pin, LOW);
        Delay_Us(ONEWIRE_WRITE_0_LOW);
        pinMode(bus->pin, PIN_MODE_INPUT);
        Delay_Us(ONEWIRE_SLOT_TIME - ONEWIRE_WRITE_0_LOW);
    }
    __enable_irq();
    Delay_Us(ONEWIRE_WRITE_RECOVERY);
}

/**
 * @brief   อ่าน 1 bit ตาม time slot 1-Wire
 */
uint8_t OneWire_ReadBit(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return 0;
    uint8_t bit;
    __disable_irq();
    pinMode(bus->pin, PIN_MODE_OUTPUT);
    digitalWrite(bus->pin, LOW);
    Delay_Us(ONEWIRE_READ_LOW);
    pinMode(bus->pin, PIN_MODE_INPUT);
    Delay_Us(ONEWIRE_READ_WAIT);
    bit = digitalRead(bus->pin);
    __enable_irq();
    Delay_Us(ONEWIRE_READ_RECOVERY);
    return bit;
}

/**
 * @brief   เขียน 1 ไบต์ (LSB first)
 */
void OneWire_WriteByte(OneWire_Bus* bus, uint8_t data) {
    if (!bus || !bus->initialized) return;
    for (uint8_t i = 0; i < 8; i++) {
        OneWire_WriteBit(bus, data & 0x01);
        data >>= 1;
    }
}

/**
 * @brief   อ่าน 1 ไบต์ (LSB first)
 */
uint8_t OneWire_ReadByte(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return 0;
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;
        if (OneWire_ReadBit(bus)) data |= 0x80;
    }
    return data;
}

/**
 * @brief   เขียนหลายไบต์
 */
void OneWire_WriteBytes(OneWire_Bus* bus, const uint8_t* data, uint8_t len) {
    if (!bus || !bus->initialized || !data) return;
    for (uint8_t i = 0; i < len; i++) OneWire_WriteByte(bus, data[i]);
}

/**
 * @brief   อ่านหลายไบต์
 */
void OneWire_ReadBytes(OneWire_Bus* bus, uint8_t* buffer, uint8_t len) {
    if (!bus || !bus->initialized || !buffer) return;
    for (uint8_t i = 0; i < len; i++) buffer[i] = OneWire_ReadByte(bus);
}

/**
 * @brief   ส่งคำสั่ง Skip ROM — เลือกอุปกรณ์ทั้งหมด
 */
void OneWire_SkipROM(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return;
    OneWire_WriteByte(bus, ONEWIRE_CMD_SKIP_ROM);
}

/**
 * @brief   Read ROM — อ่าน ROM address ของอุปกรณ์เดียวบนบัส
 *
 * @return  true ถ้า CRC ถูกต้อง
 */
bool OneWire_ReadROM(OneWire_Bus* bus, uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return false;
    if (!OneWire_Reset(bus)) return false;
    OneWire_WriteByte(bus, ONEWIRE_CMD_READ_ROM);
    OneWire_ReadBytes(bus, rom, 8);
    return OneWire_VerifyCRC(rom, 8);
}

/**
 * @brief   Match ROM — เลือกอุปกรณ์ตาม ROM address
 */
void OneWire_MatchROM(OneWire_Bus* bus, const uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return;
    OneWire_WriteByte(bus, ONEWIRE_CMD_MATCH_ROM);
    OneWire_WriteBytes(bus, rom, 8);
}

/**
 * @brief   Reset + Match ROM — เลือกอุปกรณ์
 *
 * @return  true ถ้า reset สำเร็จ
 */
bool OneWire_Select(OneWire_Bus* bus, const uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return false;
    if (!OneWire_Reset(bus)) return false;
    OneWire_MatchROM(bus, rom);
    return true;
}

/**
 * @brief   รีเซ็ตสถานะค้นหา — พร้อมค้นหาอุปกรณ์ใหม่
 */
void OneWire_ResetSearch(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return;
    bus->last_discrepancy = 0;
    bus->last_family_discrepancy = 0;
    bus->last_device_flag = false;
    memset(bus->rom, 0, 8);
}

/**
 * @brief   ค้นหาอุปกรณ์ถัดไปบนบัส (Search ROM)
 */
bool OneWire_Search(OneWire_Bus* bus) {
    return OneWire_SearchInternal(bus, ONEWIRE_CMD_SEARCH_ROM);
}

/**
 * @brief   ค้นหาอุปกรณ์ในสถานะ alarm
 */
bool OneWire_AlarmSearch(OneWire_Bus* bus) {
    return OneWire_SearchInternal(bus, ONEWIRE_CMD_ALARM_SEARCH);
}

/**
 * @brief   อ่าน ROM address ของอุปกรณ์ล่าสุดที่ค้นพบ
 */
void OneWire_GetAddress(OneWire_Bus* bus, uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return;
    memcpy(rom, bus->rom, 8);
}

/**
 * @brief   CRC8 แบบ Dallas (polynomial 0x8C)
 */
uint8_t OneWire_CRC8(const uint8_t* data, uint8_t len) {
    if (!data) return 0;
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

/**
 * @brief   ตรวจสอบ CRC (คาดว่า CRC ต่อท้ายข้อมูล — รวมแล้วต้องได้ 0)
 */
bool OneWire_VerifyCRC(const uint8_t* data, uint8_t len) {
    if (!data || len == 0) return false;
    return (OneWire_CRC8(data, len) == 0);
}

/**
 * @brief   Depower — ปล่อยขาเป็น input (หยุดดึง HIGH)
 */
void OneWire_Depower(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return;
    pinMode(bus->pin, PIN_MODE_INPUT);
}

/**
 * @brief   ค้นหาบัสจากหมายเลขขา
 */
OneWire_Bus* OneWire_GetBusByPin(uint8_t pin) {
    for (uint8_t i = 0; i < onewire_bus_count; i++) {
        if (onewire_buses[i].pin == pin) return &onewire_buses[i];
    }
    return NULL;
}

/**
 * @brief   ฟังก์ชันภายใน — ค้นหา ROM (ใช้ร่วมกับ Search และ Alarm Search)
 *
 * @note    อัลกอริทึมการค้นหา ROM ตาม spec ของ Dallas/Maxim
 *          ใช้ last_discrepancy, last_family_discrepancy, last_device_flag
 *          เพื่อวนค้นหาอุปกรณ์ถัดไปทีละตัว
 */
static bool OneWire_SearchInternal(OneWire_Bus* bus, uint8_t command) {
    if (!bus || !bus->initialized) return false;
    if (bus->last_device_flag) return false;
    if (!OneWire_Reset(bus)) {
        OneWire_ResetSearch(bus);
        return false;
    }
    OneWire_WriteByte(bus, command);

    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    bool search_result = false;

    /* วนอ่านทีละ bit (id_bit = ค่าของบิต, cmp_id_bit = ค่า complement) */
    while (rom_byte_number < 8) {
        uint8_t id_bit = OneWire_ReadBit(bus);
        uint8_t cmp_id_bit = OneWire_ReadBit(bus);

        /* ถ้าทั้งคู่เป็น 1 → ไม่มีอุปกรณ์ (สายขาด) */
        if (id_bit && cmp_id_bit) break;

        uint8_t search_direction;

        if (id_bit != cmp_id_bit) {
            /* มีค่าเดียว — ทุกอุปกรณ์เหมือนกัน */
            search_direction = id_bit;
        } else {
            /* อุปกรณ์ต่างกัน — ต้องตัดสินใจตาม last_discrepancy */
            if (id_bit_number < bus->last_discrepancy) {
                search_direction = ((bus->rom[rom_byte_number] & rom_byte_mask) > 0);
            } else {
                search_direction = (id_bit_number == bus->last_discrepancy);
            }
            if (search_direction == 0) {
                last_zero = id_bit_number;
                if (last_zero < 9) bus->last_family_discrepancy = last_zero;
            }
        }

        /* เขียนทิศทางที่เลือก และบันทึกไปยัง ROM buffer */
        if (search_direction) {
            bus->rom[rom_byte_number] |= rom_byte_mask;
        } else {
            bus->rom[rom_byte_number] &= ~rom_byte_mask;
        }

        OneWire_WriteBit(bus, search_direction);

        id_bit_number++;
        rom_byte_mask <<= 1;

        if (rom_byte_mask == 0) {
            rom_byte_number++;
            rom_byte_mask = 1;
        }
    }

    /* ถ้าอ่านได้ครบ 64 bit → ตรวจสอบ CRC */
    if (id_bit_number >= 65) {
        if (OneWire_VerifyCRC(bus->rom, 8)) {
            bus->last_discrepancy = last_zero;
            if (bus->last_discrepancy == 0) bus->last_device_flag = true;
            search_result = true;
        }
    }

    /* ROM[0] == 0 → อุปกรณ์แรกไม่ถูกต้อง */
    if (!search_result || !bus->rom[0]) {
        OneWire_ResetSearch(bus);
        search_result = false;
    }

    return search_result;
}
