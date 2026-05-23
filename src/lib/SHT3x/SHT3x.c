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

/**
 * @brief Initializes SHT3x temperature/humidity sensor
 *
 * @param sht - pointer to SHT3x_Instance (NULL check)
 * @param addr - I2C address (SHT3X_ADDR_LOW = 0x44 or SHT3X_ADDR_HIGH = 0x45)
 *
 * @return SHT3X_OK on success, error otherwise
 *
 * @note ส่ง Soft Reset command, รอ 15ms
 *       default repeatability = HIGH
 */
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

/**
 * @brief Sets measurement repeatability (affects accuracy + conversion time)
 *
 * @param sht - pointer to SHT3x_Instance (ต้อง init แล้ว)
 * @param rep - SHT3X_REP_HIGH / MEDIUM / LOW
 *
 * @note HIGH = 15ms, MEDIUM = 6ms, LOW = 4ms conversion time
 */
void SHT3x_SetRepeatability(SHT3x_Instance* sht, SHT3x_Repeatability rep) {
    if (sht == NULL || !sht->initialized) return;
    sht->repeatability = rep;
}

/**
 * @brief Performs single-shot measurement of temperature and humidity
 *
 * @param sht - pointer to SHT3x_Instance (ต้อง init แล้ว)
 * @param temp - output temperature (°C)
 * @param hum - output relative humidity (0–100%)
 *
 * @return SHT3X_OK on success, SHT3X_ERROR_CRC if data corrupt
 *
 * @note ส่ง measurement command → รอ conversion → อ่าน 6 bytes
 *       CRC-8 check (polynomial 0x31) ต่อ temperature และ humidity
 *       Humidity clamped to 0–100%
 */
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

/**
 * @brief Sends soft reset command to SHT3x
 *
 * @param sht - pointer to SHT3x_Instance (ต้อง init แล้ว)
 *
 * @return SHT3X_OK on success, error otherwise
 *
 * @note รอ 15ms หลัง reset เพื่อให้ sensor พร้อม
 */
SHT3x_Status SHT3x_Reset(SHT3x_Instance* sht) {
    if (sht == NULL || !sht->initialized) return SHT3X_ERROR_PARAM;
    uint8_t cmd[2] = {_CMD_SOFT_RESET[0], _CMD_SOFT_RESET[1]};
    if (I2C_Write(sht->i2c_addr, cmd, 2) != 0) return SHT3X_ERROR_I2C;
    Delay_Ms(15);
    return SHT3X_OK;
}

/**
 * @brief Reads SHT3x status register
 *
 * @param sht - pointer to SHT3x_Instance (ต้อง init แล้ว)
 * @param status - output 16-bit status register value
 *
 * @return SHT3X_OK on success, SHT3X_ERROR_CRC if data corrupt
 *
 * @note CRC-8 verified on returned data
 */
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
