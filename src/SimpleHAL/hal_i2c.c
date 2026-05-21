#include "hal_i2c.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

#define I2C_TIMEOUT 10000

struct hal_i2c_obj {
    uint8_t  used;
    uint32_t clock_hz;
    uint8_t  role;
    uint16_t own_addr;
};

static struct hal_i2c_obj i2c_instances[HAL_I2C_MAX_INSTANCES];

static hal_i2c_handle_t get_i2c(void)
{
    return &i2c_instances[0];
}

static uint32_t clock_to_duty(uint32_t hz)
{
    return (hz > 100000) ? I2C_DutyCycle_16_9 : I2C_DutyCycle_2;
}

static uint8_t wait_event(uint32_t event)
{
    uint32_t to = I2C_TIMEOUT;
    while (!I2C_CheckEvent(event)) {
        if (--to == 0) return 1;
    }
    return 0;
}

hal_i2c_handle_t hal_i2c_init(uint32_t clock_hz, hal_i2c_role_t role, uint16_t own_addr)
{
    hal_i2c_handle_t h = get_i2c();
    if (h->used) return h;

    h->used      = 1;
    h->clock_hz  = clock_hz;
    h->role      = (uint8_t)role;
    h->own_addr  = own_addr;

    if (role == HAL_I2C_SLAVE) {
        I2C_Init(I2C_Mode_I2C, clock_hz, clock_to_duty(clock_hz),
                 I2C_Ack_Enable, I2C_AckAddr_7bit, own_addr);
    } else {
        I2C_Init(I2C_Mode_I2C, clock_hz, clock_to_duty(clock_hz),
                 I2C_Ack_Enable, I2C_AckAddr_7bit, 0);
    }
    I2C_Cmd(ENABLE);
    return h;
}

hal_status_t hal_i2c_write_raw(hal_i2c_handle_t h, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !len) return HAL_INVALID;

    I2C_GenerateSTART(ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT)) return HAL_TIMEOUT;

    I2C_Send7bitAddress(dev_addr, I2C_Direction_Transmitter);
    if (wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }

    for (uint16_t i = 0; i < len; i++) {
        I2C_SendData(data[i]);
        if (wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            I2C_GenerateSTOP(ENABLE);
            return HAL_TIMEOUT;
        }
    }

    I2C_GenerateSTOP(ENABLE);
    return HAL_OK;
}

hal_status_t hal_i2c_read_raw(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !len) return HAL_INVALID;

    I2C_GenerateSTART(ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT)) return HAL_TIMEOUT;

    I2C_Send7bitAddress(dev_addr, I2C_Direction_Receiver);
    if (wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }

    if (len == 1) {
        I2C_AcknowledgeConfig(DISABLE);
    }

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1 && len > 1) {
            I2C_AcknowledgeConfig(DISABLE);
        }
        if (wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            I2C_GenerateSTOP(ENABLE);
            I2C_AcknowledgeConfig(ENABLE);
            return HAL_TIMEOUT;
        }
        data[i] = I2C_ReceiveData();
    }

    I2C_GenerateSTOP(ENABLE);
    I2C_AcknowledgeConfig(ENABLE);
    return HAL_OK;
}

hal_status_t hal_i2c_write(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    uint8_t buf[256];
    uint16_t total = 1 + len;
    buf[0] = reg;
    for (uint16_t i = 0; i < len && i < 255; i++) {
        buf[1 + i] = data[i];
    }
    return hal_i2c_write_raw(h, dev_addr, buf, total);
}

hal_status_t hal_i2c_read(hal_i2c_handle_t h, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;

    I2C_GenerateSTART(ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT)) return HAL_TIMEOUT;

    I2C_Send7bitAddress(dev_addr, I2C_Direction_Transmitter);
    if (wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }

    I2C_SendData(reg);
    if (wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }

    I2C_GenerateSTART(ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT)) return HAL_TIMEOUT;

    I2C_Send7bitAddress(dev_addr, I2C_Direction_Receiver);
    if (wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }

    if (len == 1) {
        I2C_AcknowledgeConfig(DISABLE);
    }

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1 && len > 1) {
            I2C_AcknowledgeConfig(DISABLE);
        }
        if (wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            I2C_GenerateSTOP(ENABLE);
            I2C_AcknowledgeConfig(ENABLE);
            return HAL_TIMEOUT;
        }
        data[i] = I2C_ReceiveData();
    }

    I2C_GenerateSTOP(ENABLE);
    I2C_AcknowledgeConfig(ENABLE);
    return HAL_OK;
}

hal_status_t hal_i2c_mem_write(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, const uint8_t *data, uint16_t len)
{
    uint8_t buf[258];
    uint8_t pos = 0;
    if (addr_len == 2) {
        buf[pos++] = (mem_addr >> 8) & 0xFF;
        buf[pos++] = mem_addr & 0xFF;
    } else {
        buf[pos++] = mem_addr & 0xFF;
    }
    for (uint16_t i = 0; i < len && pos < 256; i++) {
        buf[pos++] = data[i];
    }
    return hal_i2c_write_raw(h, dev_addr, buf, pos);
}

hal_status_t hal_i2c_mem_read(hal_i2c_handle_t h, uint8_t dev_addr, uint16_t mem_addr, uint8_t addr_len, uint8_t *data, uint16_t len)
{
    uint8_t addr_buf[2];
    uint8_t addr_bytes = (addr_len == 2) ? 2 : 1;

    if (addr_bytes == 2) {
        addr_buf[0] = (mem_addr >> 8) & 0xFF;
        addr_buf[1] = mem_addr & 0xFF;
    } else {
        addr_buf[0] = mem_addr & 0xFF;
    }

    I2C_GenerateSTART(ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT)) return HAL_TIMEOUT;
    I2C_Send7bitAddress(dev_addr, I2C_Direction_Transmitter);
    if (wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }
    for (uint8_t i = 0; i < addr_bytes; i++) {
        I2C_SendData(addr_buf[i]);
        if (wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            I2C_GenerateSTOP(ENABLE);
            return HAL_TIMEOUT;
        }
    }

    I2C_GenerateSTART(ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT)) return HAL_TIMEOUT;

    I2C_Send7bitAddress(dev_addr, I2C_Direction_Receiver);
    if (wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        I2C_GenerateSTOP(ENABLE);
        return HAL_TIMEOUT;
    }

    if (len == 1) {
        I2C_AcknowledgeConfig(DISABLE);
    }

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1 && len > 1) {
            I2C_AcknowledgeConfig(DISABLE);
        }
        if (wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            I2C_GenerateSTOP(ENABLE);
            I2C_AcknowledgeConfig(ENABLE);
            return HAL_TIMEOUT;
        }
        data[i] = I2C_ReceiveData();
    }

    I2C_GenerateSTOP(ENABLE);
    I2C_AcknowledgeConfig(ENABLE);
    return HAL_OK;
}

void hal_i2c_set_speed(hal_i2c_handle_t h, uint32_t clock_hz)
{
    if (!h || !h->used) return;
    h->clock_hz = clock_hz;
    I2C_Init(I2C_Mode_I2C, clock_hz, clock_to_duty(clock_hz),
             I2C_Ack_Enable, I2C_AckAddr_7bit, h->own_addr);
    I2C_Cmd(ENABLE);
}
