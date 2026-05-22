/**
 * @file BH1750.c
 * @brief BH1750 Ambient Light Sensor Library Implementation
 */

#include "BH1750.h"

/* ========== Private ========== */

static BH1750_Status _send_cmd(BH1750_Instance* bh, uint8_t cmd) {
    return (I2C_Write(bh->i2c_addr, &cmd, 1) == 0) ? BH1750_OK : BH1750_ERROR_I2C;
}

/* ========== Public ========== */

BH1750_Status BH1750_Init(BH1750_Instance* bh, uint8_t addr) {
    if (bh == NULL) return BH1750_ERROR_PARAM;
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH)
        return BH1750_ERROR_PARAM;

    bh->i2c_addr   = addr;
    bh->mode       = BH1750_CONT_H_RES;
    bh->initialized = 0;

    if (_send_cmd(bh, BH1750_CMD_POWER_ON) != BH1750_OK)
        return BH1750_ERROR_I2C;

    _send_cmd(bh, BH1750_CMD_RESET);
    Delay_Ms(10);

    if (_send_cmd(bh, BH1750_CONT_H_RES) != BH1750_OK)
        return BH1750_ERROR_I2C;

    Delay_Ms(180);  /* รอ first measurement */
    bh->initialized = 1;
    return BH1750_OK;
}

BH1750_Status BH1750_SetMode(BH1750_Instance* bh, BH1750_Mode mode) {
    if (bh == NULL || !bh->initialized) return BH1750_ERROR_PARAM;
    bh->mode = mode;
    return _send_cmd(bh, (uint8_t)mode);
}

BH1750_Status BH1750_ReadRaw(BH1750_Instance* bh, uint16_t* raw) {
    if (bh == NULL || !bh->initialized || raw == NULL) return BH1750_ERROR_PARAM;

    /* One-time mode: ส่ง command แล้วรอ */
    if (bh->mode >= BH1750_ONE_H_RES) {
        _send_cmd(bh, (uint8_t)bh->mode);
        uint32_t wait_ms = (bh->mode == BH1750_ONE_L_RES) ? 24 : 180;
        Delay_Ms(wait_ms);
    }

    uint8_t buf[2] = {0, 0};
    if (I2C_Read(bh->i2c_addr, buf, 2) != 0)
        return BH1750_ERROR_I2C;

    *raw = ((uint16_t)buf[0] << 8) | buf[1];
    return BH1750_OK;
}

float BH1750_ReadLux(BH1750_Instance* bh) {
    if (bh == NULL || !bh->initialized) return 0.0f;

    uint16_t raw = 0;
    if (BH1750_ReadRaw(bh, &raw) != BH1750_OK) return 0.0f;

    /* lux = raw / 1.2
     * High-Res2 mode: ÷ 2 เพิ่มอีก (0.5 lux resolution)
     */
    float lux = (float)raw / 1.2f;
    if (bh->mode == BH1750_CONT_H_RES2 || bh->mode == BH1750_ONE_H_RES2)
        lux /= 2.0f;

    return lux;
}

BH1750_Status BH1750_PowerDown(BH1750_Instance* bh) {
    if (bh == NULL || !bh->initialized) return BH1750_ERROR_PARAM;
    return _send_cmd(bh, BH1750_CMD_POWER_DOWN);
}

BH1750_Status BH1750_PowerUp(BH1750_Instance* bh) {
    if (bh == NULL || !bh->initialized) return BH1750_ERROR_PARAM;
    if (_send_cmd(bh, BH1750_CMD_POWER_ON) != BH1750_OK) return BH1750_ERROR_I2C;
    Delay_Ms(10);
    return _send_cmd(bh, (uint8_t)bh->mode);
}
