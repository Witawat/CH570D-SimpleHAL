/**
 * @file INA219.c
 * @brief INA219 Current/Voltage/Power Monitor Implementation
 */

#include "INA219.h"

/* ========== Private: Register R/W ========== */

static INA219_Status _write_reg16(INA219_Instance* ina, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    return (I2C_Write(ina->i2c_addr, buf, 3) == 0) ? INA219_OK : INA219_ERROR_I2C;
}

static INA219_Status _read_reg16(INA219_Instance* ina, uint8_t reg, uint16_t* val) {
    uint8_t buf[2];
    if (I2C_Write(ina->i2c_addr, &reg, 1) != 0) return INA219_ERROR_I2C;
    if (I2C_Read(ina->i2c_addr, buf, 2) != 0)  return INA219_ERROR_I2C;
    *val = ((uint16_t)buf[0] << 8) | buf[1];
    return INA219_OK;
}

/* ========== Public ========== */

INA219_Status INA219_Init(INA219_Instance* ina, uint8_t addr,
                           float r_shunt, float max_amps) {
    if (ina == NULL) return INA219_ERROR_PARAM;
    if (r_shunt <= 0.0f || max_amps <= 0.0f) return INA219_ERROR_PARAM;

    ina->i2c_addr  = addr;
    ina->r_shunt   = r_shunt;
    ina->initialized = 0;

    /* Reset */
    if (_write_reg16(ina, INA219_REG_CONFIG, 0x8000) != INA219_OK)
        return INA219_ERROR_I2C;
    Delay_Ms(5);

    /* คำนวณ calibration:
     * current_lsb = max_amps / 32768
     * Cal = 0.04096 / (current_lsb × r_shunt)
     */
    ina->current_lsb = max_amps / 32768.0f;
    float cal_f = 0.04096f / (ina->current_lsb * r_shunt);
    ina->calibration = (uint16_t)cal_f;

    if (_write_reg16(ina, INA219_REG_CALIBRATION, ina->calibration) != INA219_OK)
        return INA219_ERROR_I2C;

    /* Config: 32V range, ±320mV PGA, 12-bit ADC, Continuous */
    uint16_t config = INA219_BRNG_32V | INA219_PGA_320MV |
                      (0x0F << 7) |  /* BADC: 128 samples avg */
                      (0x0F << 3) |  /* SADC: 128 samples avg */
                      INA219_MODE_CONT;

    if (_write_reg16(ina, INA219_REG_CONFIG, config) != INA219_OK)
        return INA219_ERROR_I2C;

    Delay_Ms(10);
    ina->initialized = 1;
    return INA219_OK;
}

float INA219_GetBusVoltage(INA219_Instance* ina) {
    if (ina == NULL || !ina->initialized) return -1.0f;
    uint16_t raw = 0;
    if (_read_reg16(ina, INA219_REG_BUSVOLTAGE, &raw) != INA219_OK) return -1.0f;
    /* Bits 15:3 = voltage, LSB = 4mV, bit 0 = OVF */
    return (float)(raw >> 3) * 0.004f;
}

float INA219_GetShuntVoltage(INA219_Instance* ina) {
    if (ina == NULL || !ina->initialized) return -999.0f;
    uint16_t raw = 0;
    if (_read_reg16(ina, INA219_REG_SHUNTVOLTAGE, &raw) != INA219_OK) return -999.0f;
    /* LSB = 10µV = 0.01mV, signed */
    return (float)(int16_t)raw * 0.01f;  /* mV */
}

float INA219_GetCurrent(INA219_Instance* ina) {
    if (ina == NULL || !ina->initialized) return -999.0f;
    uint16_t raw = 0;
    if (_read_reg16(ina, INA219_REG_CURRENT, &raw) != INA219_OK) return -999.0f;
    return (float)(int16_t)raw * ina->current_lsb;  /* A */
}

float INA219_GetPower(INA219_Instance* ina) {
    if (ina == NULL || !ina->initialized) return -1.0f;
    uint16_t raw = 0;
    if (_read_reg16(ina, INA219_REG_POWER, &raw) != INA219_OK) return -1.0f;
    /* power_lsb = 20 × current_lsb */
    return (float)raw * (20.0f * ina->current_lsb);  /* W */
}

INA219_Status INA219_GetAll(INA219_Instance* ina,
                             float* voltage, float* current, float* power) {
    if (ina == NULL || !ina->initialized) return INA219_ERROR_PARAM;
    if (voltage == NULL || current == NULL || power == NULL) return INA219_ERROR_PARAM;

    *voltage = INA219_GetBusVoltage(ina);
    *current = INA219_GetCurrent(ina);
    *power   = INA219_GetPower(ina);
    return INA219_OK;
}

INA219_Status INA219_PowerDown(INA219_Instance* ina) {
    if (ina == NULL || !ina->initialized) return INA219_ERROR_PARAM;
    uint16_t config = 0;
    if (_read_reg16(ina, INA219_REG_CONFIG, &config) != INA219_OK) return INA219_ERROR_I2C;
    config &= ~0x07;  /* MODE = 000 = power-down */
    return _write_reg16(ina, INA219_REG_CONFIG, config);
}

INA219_Status INA219_PowerUp(INA219_Instance* ina) {
    if (ina == NULL || !ina->initialized) return INA219_ERROR_PARAM;
    uint16_t config = 0;
    if (_read_reg16(ina, INA219_REG_CONFIG, &config) != INA219_OK) return INA219_ERROR_I2C;
    config = (config & ~0x07) | INA219_MODE_CONT;
    return _write_reg16(ina, INA219_REG_CONFIG, config);
}
