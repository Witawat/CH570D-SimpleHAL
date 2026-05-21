#include "hal_i2c.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

/*********************************************************************
 * @brief   จำนวนรอบสูงสุดที่รอ event I2C
 */
#define I2C_TIMEOUT 10000

/*********************************************************************
 * @brief   โครงสร้างข้อมูลภายในของ I2C
 */
struct hal_i2c_obj {
    uint8_t  used;
    uint32_t clock_hz;
    uint8_t  role;
    uint16_t own_addr;
};

static struct hal_i2c_obj i2c_instances[HAL_I2C_MAX_INSTANCES];

/*********************************************************************
 * @fn      get_i2c
 *
 * @brief   คืนพอยน์เตอร์ไปยัง instance I2C
 *
 * @return  พอยน์เตอร์ไปยัง instance I2C แรก
 */
static hal_i2c_handle_t get_i2c(void)
{
    return &i2c_instances[0];
}

/*********************************************************************
 * @fn      clock_to_duty
 *
 * @brief   เลือก duty cycle ตามความถี่
 *
 * @param   hz - ความถี่ I2C
 *
 * @return  ค่า duty cycle (I2C_DutyCycle_16_9 หรือ I2C_DutyCycle_2)
 */
static uint32_t clock_to_duty(uint32_t hz)
{
    return (hz > 100000) ? I2C_DutyCycle_16_9 : I2C_DutyCycle_2;
}

/*********************************************************************
 * @fn      wait_event
 *
 * @brief   รอ event I2C จนกว่าจะเกิดหรือ timeout
 *
 * @param   event - event ที่รอ
 *
 * @return  0 หาก event เกิดขึ้น, 1 หาก timeout
 */
static uint8_t wait_event(uint32_t event)
{
    uint32_t to = I2C_TIMEOUT;
    while (!I2C_CheckEvent(event)) {
        if (--to == 0) return 1;
    }
    return 0;
}

/*********************************************************************
 * @fn      hal_i2c_init
 *
 * @brief   เริ่มต้น I2C: กำหนด clock, บทบาท และที่อยู่ (slave)
 *
 * @param   clock_hz - ความถี่ I2C
 * @param   role - บทบาท (MASTER/SLAVE)
 * @param   own_addr - ที่อยู่ตัวเอง (สำหรับ slave)
 *
 * @return  handle ของ I2C
 */
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

/*********************************************************************
 * @fn      hal_i2c_write_raw
 *
 * @brief   เขียนข้อมูลดิบ: START + address + data + STOP
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   data - ข้อมูลที่จะเขียน
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
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

/*********************************************************************
 * @fn      hal_i2c_read_raw
 *
 * @brief   อ่านข้อมูลดิบ: START + address(read) + data + STOP
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   data - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
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

/*********************************************************************
 * @fn      hal_i2c_write
 *
 * @brief   เขียนข้อมูลแบบ register-based: ส่ง reg address แล้วตามด้วย data
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   reg - ที่อยู่ register
 * @param   data - ข้อมูลที่จะเขียน
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
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

/*********************************************************************
 * @fn      hal_i2c_read
 *
 * @brief   อ่านข้อมูลแบบ register-based: ส่ง reg, เริ่มต้นใหม่, อ่าน data
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   reg - ที่อยู่ register
 * @param   data - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
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

/*********************************************************************
 * @fn      hal_i2c_mem_write
 *
 * @brief   เขียนข้อมูลผ่าน memory address (16-bit): ส่ง addr แล้วตามด้วย data
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   mem_addr - ที่อยู่หน่วยความจำ (16-bit)
 * @param   addr_len - ความยาวที่อยู่ (1 หรือ 2 ไบต์)
 * @param   data - ข้อมูลที่จะเขียน
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
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

/*********************************************************************
 * @fn      hal_i2c_mem_read
 *
 * @brief   อ่านข้อมูลผ่าน memory address (16-bit)
 *
 * @param   h - handle ของ I2C
 * @param   dev_addr - ที่อยู่ device
 * @param   mem_addr - ที่อยู่หน่วยความจำ (16-bit)
 * @param   addr_len - ความยาวที่อยู่ (1 หรือ 2 ไบต์)
 * @param   data - บัฟเฟอร์รับข้อมูล
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_TIMEOUT หากหมดเวลา
 */
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

/*********************************************************************
 * @fn      hal_i2c_set_speed
 *
 * @brief   เปลี่ยนความถี่ I2C ขณะทำงาน
 *
 * @param   h - handle ของ I2C
 * @param   clock_hz - ความถี่ใหม่
 */
void hal_i2c_set_speed(hal_i2c_handle_t h, uint32_t clock_hz)
{
    if (!h || !h->used) return;
    h->clock_hz = clock_hz;
    I2C_Init(I2C_Mode_I2C, clock_hz, clock_to_duty(clock_hz),
             I2C_Ack_Enable, I2C_AckAddr_7bit, h->own_addr);
    I2C_Cmd(ENABLE);
}
