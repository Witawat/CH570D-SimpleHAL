/**
 * @file PCA9685.c
 * @brief PCA9685 16-Channel PWM Expander Implementation
 */

#include "PCA9685.h"

/* ========== Private ========== */

static PCA9685_Status _write_reg(PCA9685_Instance* pca, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return (I2C_Write(pca->i2c_addr, buf, 2) == 0) ? PCA9685_OK : PCA9685_ERROR_I2C;
}

static PCA9685_Status _read_reg(PCA9685_Instance* pca, uint8_t reg, uint8_t* val) {
    if (I2C_Write(pca->i2c_addr, &reg, 1) != 0) return PCA9685_ERROR_I2C;
    if (I2C_Read(pca->i2c_addr, val, 1) != 0)  return PCA9685_ERROR_I2C;
    return PCA9685_OK;
}

/* ========== Public ========== */

PCA9685_Status PCA9685_Init(PCA9685_Instance* pca, uint8_t addr, uint16_t freq) {
    if (pca == NULL) return PCA9685_ERROR_PARAM;
    if (freq < 24 || freq > 1526) return PCA9685_ERROR_PARAM;

    pca->i2c_addr   = addr;
    pca->frequency  = freq;
    pca->period_us  = 1000000UL / freq;
    pca->initialized = 0;

    /* Reset */
    if (_write_reg(pca, PCA9685_REG_MODE1, 0x00) != PCA9685_OK)
        return PCA9685_ERROR_I2C;

    /* Sleep (ต้อง sleep ก่อนตั้ง prescaler) */
    uint8_t mode1 = 0;
    _read_reg(pca, PCA9685_REG_MODE1, &mode1);
    mode1 = (mode1 & ~PCA9685_MODE1_RESTART) | PCA9685_MODE1_SLEEP;
    if (_write_reg(pca, PCA9685_REG_MODE1, mode1) != PCA9685_OK)
        return PCA9685_ERROR_I2C;

    /* ตั้ง Prescaler:
     * prescale = round(OSC_CLOCK / (4096 × freq)) - 1
     */
    uint8_t prescale = (uint8_t)((PCA9685_OSC_CLOCK + (uint32_t)freq * 2048UL)
                                  / ((uint32_t)freq * 4096UL)) - 1;
    if (_write_reg(pca, PCA9685_REG_PRE_SCALE, prescale) != PCA9685_OK)
        return PCA9685_ERROR_I2C;

    /* Wake up */
    _read_reg(pca, PCA9685_REG_MODE1, &mode1);
    mode1 &= ~PCA9685_MODE1_SLEEP;
    if (_write_reg(pca, PCA9685_REG_MODE1, mode1) != PCA9685_OK)
        return PCA9685_ERROR_I2C;

    Delay_Ms(5);

    /* Restart */
    mode1 |= PCA9685_MODE1_RESTART;
    _write_reg(pca, PCA9685_REG_MODE1, mode1);

    pca->initialized = 1;
    return PCA9685_OK;
}

PCA9685_Status PCA9685_SetPWM(PCA9685_Instance* pca, uint8_t channel,
                               uint16_t on, uint16_t off) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    if (channel > 15) return PCA9685_ERROR_PARAM;

    uint8_t reg  = (uint8_t)(PCA9685_REG_LED0_ON_L + channel * 4);
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = (uint8_t)(on & 0xFF);
    buf[2] = (uint8_t)(on >> 8);
    buf[3] = (uint8_t)(off & 0xFF);
    buf[4] = (uint8_t)(off >> 8);
    return (I2C_Write(pca->i2c_addr, buf, 5) == 0) ? PCA9685_OK : PCA9685_ERROR_I2C;
}

PCA9685_Status PCA9685_SetDuty(PCA9685_Instance* pca, uint8_t channel, float duty) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    if (duty < 0.0f)   duty = 0.0f;
    if (duty > 100.0f) duty = 100.0f;

    if (duty == 0.0f)   return PCA9685_Off(pca, channel);
    if (duty == 100.0f) return PCA9685_FullOn(pca, channel);

    uint16_t off = (uint16_t)(duty / 100.0f * 4096.0f);
    if (off > 4095) off = 4095;
    return PCA9685_SetPWM(pca, channel, 0, off);
}

PCA9685_Status PCA9685_SetPulse(PCA9685_Instance* pca, uint8_t channel, uint16_t us) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    /* off_count = (us / period_us) × 4096 */
    uint32_t off = ((uint32_t)us * 4096UL) / pca->period_us;
    if (off > 4095) off = 4095;
    return PCA9685_SetPWM(pca, channel, 0, (uint16_t)off);
}

PCA9685_Status PCA9685_SetServoAngle(PCA9685_Instance* pca, uint8_t channel, uint8_t angle) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    if (angle > 180) angle = 180;

    uint16_t us = (uint16_t)(PCA9685_SERVO_MIN_US +
                  ((uint32_t)angle * (PCA9685_SERVO_MAX_US - PCA9685_SERVO_MIN_US)) / 180);
    return PCA9685_SetPulse(pca, channel, us);
}

PCA9685_Status PCA9685_Off(PCA9685_Instance* pca, uint8_t channel) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    if (channel == 255) {
        /* ทุก channel */
        uint8_t buf[5] = {PCA9685_REG_ALL_OFF_L, 0x00, 0x00, 0x00, 0x10};
        return (I2C_Write(pca->i2c_addr, buf, 5) == 0) ? PCA9685_OK : PCA9685_ERROR_I2C;
    }
    return PCA9685_SetPWM(pca, channel, 0, 0x1000);  /* bit 12 = full OFF */
}

PCA9685_Status PCA9685_FullOn(PCA9685_Instance* pca, uint8_t channel) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    return PCA9685_SetPWM(pca, channel, 0x1000, 0);  /* bit 12 = full ON */
}

PCA9685_Status PCA9685_Sleep(PCA9685_Instance* pca) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    uint8_t mode1 = 0;
    _read_reg(pca, PCA9685_REG_MODE1, &mode1);
    return _write_reg(pca, PCA9685_REG_MODE1, mode1 | PCA9685_MODE1_SLEEP);
}

PCA9685_Status PCA9685_WakeUp(PCA9685_Instance* pca) {
    if (pca == NULL || !pca->initialized) return PCA9685_ERROR_PARAM;
    uint8_t mode1 = 0;
    _read_reg(pca, PCA9685_REG_MODE1, &mode1);
    mode1 &= ~PCA9685_MODE1_SLEEP;
    _write_reg(pca, PCA9685_REG_MODE1, mode1);
    Delay_Ms(5);
    mode1 |= PCA9685_MODE1_RESTART;
    return _write_reg(pca, PCA9685_REG_MODE1, mode1);
}
