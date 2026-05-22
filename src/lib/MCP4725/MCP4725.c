/**
 * @file MCP4725.c
 * @brief MCP4725 12-bit DAC Implementation
 */

#include "MCP4725.h"

MCP4725_Status MCP4725_Init(MCP4725_Instance* dac, uint8_t addr) {
    if (dac == NULL) return MCP4725_ERROR_PARAM;

    dac->i2c_addr   = addr;
    dac->value      = 0;
    dac->initialized = 0;

    /* ทดสอบ I2C connection: ส่งค่า 0 */
    uint8_t buf[2] = {0x00, 0x00};
    if (I2C_Write(dac->i2c_addr, buf, 2) != 0)
        return MCP4725_ERROR_I2C;

    dac->initialized = 1;
    return MCP4725_OK;
}

MCP4725_Status MCP4725_SetRaw(MCP4725_Instance* dac, uint16_t value) {
    if (dac == NULL || !dac->initialized) return MCP4725_ERROR_PARAM;
    if (value > 4095) value = 4095;

    /* Fast Write: 2 bytes
     * Byte 0: [PD1][PD0][D11][D10][D9][D8]  (upper nibble + PD=00)
     * Byte 1: [D7][D6][D5][D4][D3][D2][D1][D0]
     */
    uint8_t buf[2];
    buf[0] = (uint8_t)((value >> 8) & 0x0F);  /* D11-D8, PD=00 */
    buf[1] = (uint8_t)(value & 0xFF);           /* D7-D0 */

    if (I2C_Write(dac->i2c_addr, buf, 2) != 0)
        return MCP4725_ERROR_I2C;

    dac->value = value;
    return MCP4725_OK;
}

MCP4725_Status MCP4725_SetVoltage(MCP4725_Instance* dac, float voltage, float vref) {
    if (dac == NULL || !dac->initialized) return MCP4725_ERROR_PARAM;
    if (vref <= 0.0f) return MCP4725_ERROR_PARAM;
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > vref) voltage = vref;

    uint16_t raw = (uint16_t)(voltage / vref * 4095.0f + 0.5f);
    return MCP4725_SetRaw(dac, raw);
}

MCP4725_Status MCP4725_SetRawEEPROM(MCP4725_Instance* dac, uint16_t value) {
    if (dac == NULL || !dac->initialized) return MCP4725_ERROR_PARAM;
    if (value > 4095) value = 4095;

    /* Write DAC Register and EEPROM: 3 bytes
     * Byte 0: 0x60 = [0][1][1][PD1][PD0][X][X][X] → Write DAC+EEPROM, PD=00
     * Byte 1: D11-D4 (upper 8 bits)
     * Byte 2: D3-D0 << 4 (lower 4 bits, LSB nibble)
     */
    uint8_t buf[3];
    buf[0] = 0x60;  /* Write DAC + EEPROM command */
    buf[1] = (uint8_t)((value >> 4) & 0xFF);
    buf[2] = (uint8_t)((value & 0x0F) << 4);

    if (I2C_Write(dac->i2c_addr, buf, 3) != 0)
        return MCP4725_ERROR_I2C;

    dac->value = value;
    Delay_Ms(25);  /* รอ EEPROM write */
    return MCP4725_OK;
}

MCP4725_Status MCP4725_GetRaw(MCP4725_Instance* dac, uint16_t* value) {
    if (dac == NULL || !dac->initialized || value == NULL)
        return MCP4725_ERROR_PARAM;

    /* อ่าน 3 bytes: status, DAC MSB, DAC LSB */
    uint8_t buf[3] = {0, 0, 0};
    if (I2C_Read(dac->i2c_addr, buf, 3) != 0)
        return MCP4725_ERROR_I2C;

    /* Byte 1: D11-D4, Byte 2: D3-D0 (upper nibble) */
    *value = ((uint16_t)buf[1] << 4) | (buf[2] >> 4);
    return MCP4725_OK;
}

MCP4725_Status MCP4725_SetPowerDown(MCP4725_Instance* dac, MCP4725_PowerDown pd) {
    if (dac == NULL || !dac->initialized) return MCP4725_ERROR_PARAM;

    /* Fast Write with PD bits */
    uint8_t buf[2];
    buf[0] = (uint8_t)(((uint8_t)pd & 0x03) << 4) | (uint8_t)((dac->value >> 8) & 0x0F);
    buf[1] = (uint8_t)(dac->value & 0xFF);

    if (I2C_Write(dac->i2c_addr, buf, 2) != 0)
        return MCP4725_ERROR_I2C;

    return MCP4725_OK;
}
