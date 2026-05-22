/**
 * @file SHT3x.c
 * @brief SHT30/SHT31/SHT35 Temperature & Humidity Sensor Implementation
 */

#include "SHT3x.h"

/* ========== Commands (2-byte, MSB first) ========== */

/* Single Shot, No Clock Stretching */
static const uint8_t _cmd_ss[3][2] = {
    {0x24, 0x00},  /* HIGH repeatability   */
    {0x24, 0x0B},  /* MEDIUM repeatability */
    {0x24, 0x16},  /* LOW repeatability    */
};

/* Measurement delay (ms) per repeatability */
static const uint8_t _delay_ms[3] = {15, 6, 4};

static const uint8_t _CMD_SOFT_RESET[2]     = {0x30, 0xA2};
static const uint8_t _CMD_READ_STATUS[2]    = {0xF3, 0x2D};

/* ========== Private: CRC-8 ========== */
/* Polynomial: 0x31, Init: 0xFF, No reflect */
static uint8_t _crc8(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
    }
    return crc;
}

/* ========== Private: send 2-byte command ========== */
static SHT3x_Status _send_cmd(SHT3x_Instance* sht, const uint8_t cmd[2]) {
    uint8_t buf[2] = {cmd[0], cmd[1]};
    return (I2C_Write(sht->i2c_addr, buf, 2) == 0) ? SHT3X_OK : SHT3X_ERROR_I2C;
}

/* ========== Public ========== */

SHT3x_Status SHT3x_Init(SHT3x_Instance* sht, uint8_t addr) {
    if (sht == NULL) return SHT3X_ERROR_PARAM;
    if (addr != SHT3X_ADDR_LOW && addr != SHT3X_ADDR_HIGH)
        return SHT3X_ERROR_PARAM;

    sht->i2c_addr    = addr;
    sht->repeatability = SHT3X_REP_HIGH;
    sht->initialized = 0;

    /* Soft reset */
    uint8_t cmd[2] = {_CMD_SOFT_RESET[0], _CMD_SOFT_RESET[1]};
    if (I2C_Write(sht->i2c_addr, cmd, 2) != 0)
        return SHT3X_ERROR_I2C;

    Delay_Ms(15);
    sht->initialized = 1;
    return SHT3X_OK;
}

void SHT3x_SetRepeatability(SHT3x_Instance* sht, SHT3x_Repeatability rep) {
    if (sht == NULL || !sht->initialized) return;
    sht->repeatability = rep;
}

SHT3x_Status SHT3x_Read(SHT3x_Instance* sht, float* temp, float* hum) {
    if (sht == NULL || !sht->initialized) return SHT3X_ERROR_PARAM;
    if (temp == NULL || hum == NULL) return SHT3X_ERROR_PARAM;

    uint8_t rep = (uint8_t)sht->repeatability;

    /* ส่ง measurement command */
    if (_send_cmd(sht, _cmd_ss[rep]) != SHT3X_OK)
        return SHT3X_ERROR_I2C;

    Delay_Ms(_delay_ms[rep]);

    /* อ่าน 6 bytes: T_MSB, T_LSB, T_CRC, H_MSB, H_LSB, H_CRC */
    uint8_t buf[6];
    if (I2C_Read(sht->i2c_addr, buf, 6) != 0)
        return SHT3X_ERROR_I2C;

    /* ตรวจ CRC */
    if (_crc8(buf, 2) != buf[2]) return SHT3X_ERROR_CRC;
    if (_crc8(buf + 3, 2) != buf[5]) return SHT3X_ERROR_CRC;

    uint16_t raw_t = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_h = ((uint16_t)buf[3] << 8) | buf[4];

    /* แปลงตาม datasheet:
     * Temp = -45 + 175 × raw_T / 65535
     * Hum  = 100 × raw_H / 65535
     */
    *temp = -45.0f + 175.0f * ((float)raw_t / 65535.0f);
    *hum  = 100.0f * ((float)raw_h / 65535.0f);

    /* Clamp humidity */
    if (*hum > 100.0f) *hum = 100.0f;
    if (*hum < 0.0f)   *hum = 0.0f;

    return SHT3X_OK;
}

SHT3x_Status SHT3x_Reset(SHT3x_Instance* sht) {
    if (sht == NULL || !sht->initialized) return SHT3X_ERROR_PARAM;
    uint8_t cmd[2] = {_CMD_SOFT_RESET[0], _CMD_SOFT_RESET[1]};
    if (I2C_Write(sht->i2c_addr, cmd, 2) != 0) return SHT3X_ERROR_I2C;
    Delay_Ms(15);
    return SHT3X_OK;
}

SHT3x_Status SHT3x_GetStatus(SHT3x_Instance* sht, uint16_t* status) {
    if (sht == NULL || !sht->initialized || status == NULL)
        return SHT3X_ERROR_PARAM;

    uint8_t cmd[2] = {_CMD_READ_STATUS[0], _CMD_READ_STATUS[1]};
    if (I2C_Write(sht->i2c_addr, cmd, 2) != 0) return SHT3X_ERROR_I2C;

    uint8_t buf[3];
    if (I2C_Read(sht->i2c_addr, buf, 3) != 0) return SHT3X_ERROR_I2C;

    if (_crc8(buf, 2) != buf[2]) return SHT3X_ERROR_CRC;
    *status = ((uint16_t)buf[0] << 8) | buf[1];
    return SHT3X_OK;
}
