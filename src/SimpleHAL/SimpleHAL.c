#include "SimpleHAL.h"
#include "Simple1Wire.h"
#include "core_riscv.h"

#define COMPAT_GPIO_MAX_PINS  HAL_GPIO_MAX_PINS

static hal_gpio_handle_t _gpio_handles[COMPAT_GPIO_MAX_PINS] = {0};
static hal_gpio_mode_t   _gpio_modes[COMPAT_GPIO_MAX_PINS] = {0};
static uint8_t           _gpio_initialized[COMPAT_GPIO_MAX_PINS] = {0};

static hal_i2c_handle_t  _i2c_handle = NULL;
static uint8_t           _i2c_ready = 0;

static hal_uart_handle_t _uart_handle = NULL;
static uint8_t           _uart_ready = 0;

#define COMPAT_IRQ_CALLBACKS_MAX  HAL_GPIO_MAX_PINS
static void (*_irq_user_callbacks[COMPAT_IRQ_CALLBACKS_MAX])(void) = {0};
static void _irq_wrapper(void *arg)
{
    uintptr_t pin = (uintptr_t)arg;
    if (pin < COMPAT_IRQ_CALLBACKS_MAX && _irq_user_callbacks[pin])
        _irq_user_callbacks[pin]();
}

#define COMPAT_PWM_CHANNELS 8
static hal_pwm_handle_t _pwm_handles[COMPAT_PWM_CHANNELS] = {0};
static uint8_t          _pwm_inited[COMPAT_PWM_CHANNELS] = {0};
static uint8_t          _pwm_is_16bit[COMPAT_PWM_CHANNELS] = {0};
static uint16_t         _pwm_period[COMPAT_PWM_CHANNELS] = {0};

static uint8_t _hal_inited = 0;

static void (*_tim_callbacks[8])(void) = {0};

static hal_gpio_mode_t _map_pin_mode(GPIO_PinMode mode)
{
    switch (mode) {
        case PIN_MODE_INPUT:        return HAL_GPIO_INPUT_FLOATING;
        case PIN_MODE_OUTPUT:       return HAL_GPIO_OUTPUT_PP_5mA;
        case PIN_MODE_INPUT_PULLUP: return HAL_GPIO_INPUT_PULLUP;
        case PIN_MODE_INPUT_PULLDOWN: return HAL_GPIO_INPUT_PULLDOWN;
        case PIN_MODE_OUTPUT_OD:    return HAL_GPIO_OUTPUT_PP_5mA;
        default:                    return HAL_GPIO_INPUT_FLOATING;
    }
}

static hal_pwm_channel_t _map_pwm_channel(PWM_Channel ch)
{
    switch (ch) {
        case PWM1_CH1: return HAL_PWM_CH1;
        case PWM1_CH2: return HAL_PWM_CH2;
        case PWM1_CH3: return HAL_PWM_CH3;
        case PWM1_CH4: return HAL_PWM_CH4;
        case PWM2_CH1: return HAL_PWM_CH5;
        default:       return HAL_PWM_CH1;
    }
}

void SimpleHAL_Init(void)
{
    if (_hal_inited) return;
    _hal_inited = 1;
}

void pinMode(uint8_t pin, GPIO_PinMode mode)
{
    if (pin >= COMPAT_GPIO_MAX_PINS) return;
    hal_gpio_mode_t hal_mode = _map_pin_mode(mode);
    _gpio_handles[pin] = hal_gpio_init(pin, hal_mode);
    _gpio_modes[pin] = hal_mode;
    _gpio_initialized[pin] = (_gpio_handles[pin] != NULL) ? 1 : 0;
}

void pinModeMultiple(const uint8_t *pins, GPIO_PinMode mode)
{
    if (!pins) return;
    while (*pins != 0xFF) {
        pinMode(*pins, mode);
        pins++;
    }
}

void digitalWrite(uint8_t pin, uint8_t value)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return;
    hal_gpio_write(_gpio_handles[pin], value);
}

uint8_t digitalRead(uint8_t pin)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return 0;
    return hal_gpio_read(_gpio_handles[pin]);
}

void digitalToggle(uint8_t pin)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return;
    hal_gpio_toggle(_gpio_handles[pin]);
}

void attachInterrupt(uint8_t pin, void (*callback)(void), GPIO_InterruptMode mode)
{
    if (pin >= COMPAT_GPIO_MAX_PINS) return;
    if (!_gpio_initialized[pin]) {
        _gpio_handles[pin] = hal_gpio_init(pin, HAL_GPIO_INPUT_FLOATING);
        _gpio_initialized[pin] = (_gpio_handles[pin] != NULL) ? 1 : 0;
    }
    if (!_gpio_initialized[pin]) return;
    hal_gpio_irq_mode_t irq_mode;
    switch (mode) {
        case RISING:  irq_mode = HAL_GPIO_IRQ_RISING; break;
        case FALLING: irq_mode = HAL_GPIO_IRQ_FALLING; break;
        default:      irq_mode = HAL_GPIO_IRQ_FALLING; break;
    }
    _irq_user_callbacks[pin] = callback;
    hal_gpio_attach_irq(_gpio_handles[pin], irq_mode, _irq_wrapper, (void*)(uintptr_t)pin);
}

void detachInterrupt(uint8_t pin)
{
    if (pin >= COMPAT_GPIO_MAX_PINS || !_gpio_initialized[pin]) return;
    _irq_user_callbacks[pin] = NULL;
    hal_gpio_detach_irq(_gpio_handles[pin]);
}

uint32_t Get_CurrentUs(void)
{
    uint32_t ms = hal_get_sys_tick();
    uint32_t cnt = SysTick->CNT;
    uint32_t cmp = SysTick->CMP;
    uint32_t us_in_tick = (cmp - cnt) * 1000 / (cmp + 1);
    return ms * 1000 + us_in_tick;
}

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

void PWM_SetDutyCycle(PWM_Channel channel, uint8_t duty_percent)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    hal_pwm_set_duty(_pwm_handles[channel], duty_percent);
}

void PWM_SetDutyCycleRaw(PWM_Channel channel, uint16_t duty_value)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    if (_pwm_is_16bit[channel]) {
        hal_pwm_set_duty_16bit(_pwm_handles[channel], duty_value);
    } else {
        hal_pwm_set_duty(_pwm_handles[channel], (uint8_t)(duty_value & 0xFF));
    }
}

uint16_t PWM_GetPeriod(PWM_Channel channel)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return 0;
    return _pwm_period[channel];
}

void PWM_Start(PWM_Channel channel)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    hal_pwm_start(_pwm_handles[channel]);
}

void PWM_Stop(PWM_Channel channel)
{
    if (channel >= COMPAT_PWM_CHANNELS || !_pwm_inited[channel]) return;
    hal_pwm_stop(_pwm_handles[channel]);
}

void PWM_Write(PWM_Channel channel, uint8_t value)
{
    uint8_t pct = (uint8_t)(((uint16_t)value * 100) / 255);
    PWM_SetDutyCycle(channel, pct);
    if (!_pwm_inited[channel]) PWM_Start(channel);
}

void I2C_SimpleInit(uint32_t speed_hz, uint8_t pins)
{
    (void)pins;
    _i2c_handle = hal_i2c_init(speed_hz, HAL_I2C_MASTER, 0);
    _i2c_ready = (_i2c_handle != NULL) ? 1 : 0;
}

uint8_t I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (!_i2c_ready) return I2C_ERROR;
    return (hal_i2c_write_raw(_i2c_handle, dev_addr, data, len) == HAL_OK) ? I2C_OK : I2C_ERROR;
}

uint8_t I2C_Read(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (!_i2c_ready) return I2C_ERROR;
    return (hal_i2c_read_raw(_i2c_handle, dev_addr, data, len) == HAL_OK) ? I2C_OK : I2C_ERROR;
}

uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return I2C_Write(dev_addr, buf, 2);
}

uint8_t I2C_ReadRegMulti(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (I2C_Write(dev_addr, &reg, 1) != I2C_OK) return I2C_ERROR;
    return I2C_Read(dev_addr, data, len);
}

uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg)
{
    uint8_t val = 0;
    if (I2C_ReadRegMulti(dev_addr, reg, &val, 1) != I2C_OK) return 0;
    return val;
}

uint8_t I2C_IsDeviceReady(uint8_t dev_addr)
{
    if (!_i2c_ready) return 0;
    uint8_t dummy = 0;
    return (hal_i2c_write_raw(_i2c_handle, dev_addr, &dummy, 0) == HAL_OK) ? 1 : 0;
}

void USART_SimpleInit(uint32_t baudrate, uint8_t pins)
{
    (void)pins;
    _uart_handle = hal_uart_init(baudrate, UART_DEFAULT_TX_PIN, UART_DEFAULT_RX_PIN);
    _uart_ready = (_uart_handle != NULL) ? 1 : 0;
}

void USART_WriteByte(uint8_t data)
{
    if (!_uart_ready) return;
    hal_uart_send_byte(_uart_handle, data);
}

uint8_t USART_Available(void)
{
    if (!_uart_ready) return 0;
    return (uint8_t)hal_uart_available(_uart_handle);
}

uint8_t USART_Read(void)
{
    if (!_uart_ready) return 0;
    return hal_uart_recv_byte(_uart_handle);
}

void USART_Flush(void)
{
    if (!_uart_ready) return;
    hal_uart_flush(_uart_handle);
}

void USART_Print(const char *str)
{
    if (!_uart_ready) return;
    hal_uart_send(_uart_handle, (const uint8_t *)str, strlen(str));
}

void SPI_SimpleInit(SPI_Mode mode, SPI_Speed speed, SPI_PinConfig pin_cfg)
{
    (void)mode;
    (void)speed;
    (void)pin_cfg;
}

uint8_t SPI_Transfer(uint8_t data)
{
    (void)data;
    return 0;
}

void GPIO_SetBits(GPIO_TypeDef* port, GPIO_Pin pin)
{
    if (port == GPIOA) {
        uint8_t p = 0;
        while (pin > 1) { pin >>= 1; p++; }
        digitalWrite(p, HIGH);
    }
}

void GPIO_ResetBits(GPIO_TypeDef* port, GPIO_Pin pin)
{
    if (port == GPIOA) {
        uint8_t p = 0;
        while (pin > 1) { pin >>= 1; p++; }
        digitalWrite(p, LOW);
    }
}

void TIM_SimpleInit(uint8_t tim, uint32_t period_us)
{
    (void)tim;
    (void)period_us;
}

void TIM_AttachInterrupt(uint8_t tim, void (*callback)(void))
{
    if (tim < 8) _tim_callbacks[tim] = callback;
}

void TIM_Start(uint8_t tim)
{
    (void)tim;
}

void TIM_Stop(uint8_t tim)
{
    (void)tim;
}

void TIM_DetachInterrupt(uint8_t tim)
{
    if (tim < 8) _tim_callbacks[tim] = NULL;
}

void ADC_SimpleInitChannels(const ADC_Channel *channels, uint8_t count)
{
    (void)channels;
    (void)count;
}

uint16_t ADC_Read(ADC_Channel ch)
{
    (void)ch;
    return 0;
}

float ADC_ToVoltage(uint16_t raw, float vref)
{
    (void)raw;
    (void)vref;
    return 0.0f;
}

void __disable_irq(void)
{
    __risc_v_disable_irq();
}

void __enable_irq(void)
{
    PFIC_EnableAllIRQ();
}

void Timer_Init(void)
{
    hal_get_sys_tick();
}

uint16_t analogRead(uint8_t pin)
{
    (void)pin;
    return 0;
}

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t value)
{
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (bitOrder == LSBFIRST) ? (value >> i) & 1 : (value >> (7 - i)) & 1;
        digitalWrite(dataPin, bit);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

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

void OneWire_WriteByte(OneWire_Bus* bus, uint8_t data) {
    if (!bus || !bus->initialized) return;
    for (uint8_t i = 0; i < 8; i++) {
        OneWire_WriteBit(bus, data & 0x01);
        data >>= 1;
    }
}

uint8_t OneWire_ReadByte(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return 0;
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;
        if (OneWire_ReadBit(bus)) data |= 0x80;
    }
    return data;
}

void OneWire_WriteBytes(OneWire_Bus* bus, const uint8_t* data, uint8_t len) {
    if (!bus || !bus->initialized || !data) return;
    for (uint8_t i = 0; i < len; i++) OneWire_WriteByte(bus, data[i]);
}

void OneWire_ReadBytes(OneWire_Bus* bus, uint8_t* buffer, uint8_t len) {
    if (!bus || !bus->initialized || !buffer) return;
    for (uint8_t i = 0; i < len; i++) buffer[i] = OneWire_ReadByte(bus);
}

void OneWire_SkipROM(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return;
    OneWire_WriteByte(bus, ONEWIRE_CMD_SKIP_ROM);
}

bool OneWire_ReadROM(OneWire_Bus* bus, uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return false;
    if (!OneWire_Reset(bus)) return false;
    OneWire_WriteByte(bus, ONEWIRE_CMD_READ_ROM);
    OneWire_ReadBytes(bus, rom, 8);
    return OneWire_VerifyCRC(rom, 8);
}

void OneWire_MatchROM(OneWire_Bus* bus, const uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return;
    OneWire_WriteByte(bus, ONEWIRE_CMD_MATCH_ROM);
    OneWire_WriteBytes(bus, rom, 8);
}

bool OneWire_Select(OneWire_Bus* bus, const uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return false;
    if (!OneWire_Reset(bus)) return false;
    OneWire_MatchROM(bus, rom);
    return true;
}

void OneWire_ResetSearch(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return;
    bus->last_discrepancy = 0;
    bus->last_family_discrepancy = 0;
    bus->last_device_flag = false;
    memset(bus->rom, 0, 8);
}

bool OneWire_Search(OneWire_Bus* bus) {
    return OneWire_SearchInternal(bus, ONEWIRE_CMD_SEARCH_ROM);
}

bool OneWire_AlarmSearch(OneWire_Bus* bus) {
    return OneWire_SearchInternal(bus, ONEWIRE_CMD_ALARM_SEARCH);
}

void OneWire_GetAddress(OneWire_Bus* bus, uint8_t* rom) {
    if (!bus || !bus->initialized || !rom) return;
    memcpy(rom, bus->rom, 8);
}

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

bool OneWire_VerifyCRC(const uint8_t* data, uint8_t len) {
    if (!data || len == 0) return false;
    return (OneWire_CRC8(data, len) == 0);
}

void OneWire_Depower(OneWire_Bus* bus) {
    if (!bus || !bus->initialized) return;
    pinMode(bus->pin, PIN_MODE_INPUT);
}

OneWire_Bus* OneWire_GetBusByPin(uint8_t pin) {
    for (uint8_t i = 0; i < onewire_bus_count; i++) {
        if (onewire_buses[i].pin == pin) return &onewire_buses[i];
    }
    return NULL;
}

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

    while (rom_byte_number < 8) {
        uint8_t id_bit = OneWire_ReadBit(bus);
        uint8_t cmp_id_bit = OneWire_ReadBit(bus);

        if (id_bit && cmp_id_bit) break;

        uint8_t search_direction;

        if (id_bit != cmp_id_bit) {
            search_direction = id_bit;
        } else {
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

    if (id_bit_number >= 65) {
        if (OneWire_VerifyCRC(bus->rom, 8)) {
            bus->last_discrepancy = last_zero;
            if (bus->last_discrepancy == 0) bus->last_device_flag = true;
            search_result = true;
        }
    }

    if (!search_result || !bus->rom[0]) {
        OneWire_ResetSearch(bus);
        search_result = false;
    }

    return search_result;
}
