/**
 * @file ADS1115.c
 * @brief ADS1115 16-bit ADC Implementation
 */

#include "ADS1115.h"

/* FSR lookup (V) ตาม PGA bits [11:9] */
static const float _fsr_table[] = {
    6.144f, /* PGA_6144 = 0b000 */
    4.096f, /* PGA_4096 = 0b001 */
    2.048f, /* PGA_2048 = 0b010 */
    1.024f, /* PGA_1024 = 0b011 */
    0.512f, /* PGA_512  = 0b100 */
    0.256f, /* PGA_256  = 0b101 */
    0.256f, /* (reserved) */
    0.256f
};

/* Delay สำหรับ conversion ตาม data rate (ms, บวก margin) */
static const uint16_t _conv_delay[] = {
    130, 70, 40, 20, 10, 5, 4, 3
};  /* DR_8 → DR_860 */

/* ========== Private ========== */

static ADS1115_Status _write_reg(ADS1115_Instance* ads, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    return (I2C_Write(ads->i2c_addr, buf, 3) == 0) ? ADS1115_OK : ADS1115_ERROR_I2C;
}

static ADS1115_Status _read_reg(ADS1115_Instance* ads, uint8_t reg, uint16_t* val) {
    uint8_t buf[2];
    if (I2C_Write(ads->i2c_addr, &reg, 1) != 0) return ADS1115_ERROR_I2C;
    if (I2C_Read(ads->i2c_addr, buf, 2) != 0)  return ADS1115_ERROR_I2C;
    *val = ((uint16_t)buf[0] << 8) | buf[1];
    return ADS1115_OK;
}

/* ========== Public ========== */

ADS1115_Status ADS1115_Init(ADS1115_Instance* ads, uint8_t addr) {
    if (ads == NULL) return ADS1115_ERROR_PARAM;

    ads->i2c_addr   = addr;
    ads->pga        = ADS1115_PGA_2048;
    ads->data_rate  = ADS1115_DR_128;
    ads->fsr        = 2.048f;
    ads->initialized = 0;

    /* ทดสอบ: อ่าน config register */
    uint16_t config = 0;
    if (_read_reg(ads, ADS1115_REG_CONFIG, &config) != ADS1115_OK)
        return ADS1115_ERROR_I2C;

    ads->initialized = 1;
    return ADS1115_OK;
}

void ADS1115_SetGain(ADS1115_Instance* ads, ADS1115_PGA pga) {
    if (ads == NULL || !ads->initialized) return;
    ads->pga = pga;
    uint8_t idx = (uint8_t)((pga >> 9) & 0x07);
    ads->fsr = _fsr_table[idx];
}

void ADS1115_SetDataRate(ADS1115_Instance* ads, ADS1115_DataRate dr) {
    if (ads == NULL || !ads->initialized) return;
    ads->data_rate = dr;
}

int16_t ADS1115_ReadRaw(ADS1115_Instance* ads, ADS1115_Channel ch) {
    if (ads == NULL || !ads->initialized) return 0;

    /* Build CONFIG register:
     * bit 15: OS = 1 (start single-shot)
     * bit 14:12: MUX
     * bit 11:9: PGA
     * bit 8: MODE = 1 (single-shot)
     * bit 7:5: DR
     * bit 4:0: comparator off (0x03)
     */
    uint16_t config = 0x8000 |              /* OS = start */
                      (uint16_t)ch    |      /* MUX */
                      (uint16_t)ads->pga |   /* PGA */
                      0x0100 |              /* MODE = single-shot */
                      (uint16_t)ads->data_rate |
                      0x0003;               /* COMP_QUE = disable */

    if (_write_reg(ads, ADS1115_REG_CONFIG, config) != ADS1115_OK) return 0;

    /* รอ conversion เสร็จ (poll OS bit หรือ delay) */
    uint8_t dr_idx = (uint8_t)(((uint16_t)ads->data_rate >> 5) & 0x07);
    Delay_Ms(_conv_delay[dr_idx]);

    /* Poll ว่า conversion เสร็จ (OS bit = 1) */
    uint32_t start = Get_CurrentMs();
    uint16_t cfg;
    do {
        Delay_Ms(1);
        if (_read_reg(ads, ADS1115_REG_CONFIG, &cfg) != ADS1115_OK) return 0;
    } while (!(cfg & 0x8000) && (Get_CurrentMs() - start) < 500);

    uint16_t raw = 0;
    if (_read_reg(ads, ADS1115_REG_CONVERSION, &raw) != ADS1115_OK) return 0;
    return (int16_t)raw;
}

float ADS1115_ToVoltage(ADS1115_Instance* ads, int16_t raw) {
    if (ads == NULL || !ads->initialized) return 0.0f;
    /* LSB = FSR / 32768 */
    return (float)raw * (ads->fsr / 32768.0f);
}

float ADS1115_ReadVoltage(ADS1115_Instance* ads, ADS1115_Channel ch) {
    int16_t raw = ADS1115_ReadRaw(ads, ch);
    return ADS1115_ToVoltage(ads, raw);
}
