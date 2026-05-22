/**
 * @file BMP280.c
 * @brief BMP280 Temperature & Pressure Sensor Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "BMP280.h"
#include <math.h>

/* ========== Private: Register Helpers ========== */

static BMP280_Status _write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    return (I2C_WriteReg(addr, reg, val) == I2C_OK) ? BMP280_OK : BMP280_ERROR_I2C;
}

static BMP280_Status _read_reg(uint8_t addr, uint8_t reg, uint8_t* val) {
    return (I2C_ReadRegMulti(addr, reg, val, 1) == I2C_OK) ? BMP280_OK : BMP280_ERROR_I2C;
}

static BMP280_Status _read_regs(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
    if (I2C_Write(addr, &reg, 1) != I2C_OK)    return BMP280_ERROR_I2C;
    if (I2C_Read(addr, buf, len) != I2C_OK)     return BMP280_ERROR_I2C;
    return BMP280_OK;
}

/* ========== Private: Load Calibration Data ========== */

static BMP280_Status _load_calib(BMP280_Instance* bmp) {
    uint8_t buf[24];
    BMP280_Status st = _read_regs(bmp->i2c_addr, BMP280_REG_CALIB_START, buf, 24);
    if (st != BMP280_OK) return st;

    BMP280_Calib* c = &bmp->calib;
    c->dig_T1 = (uint16_t)((buf[1]  << 8) | buf[0]);
    c->dig_T2 = (int16_t) ((buf[3]  << 8) | buf[2]);
    c->dig_T3 = (int16_t) ((buf[5]  << 8) | buf[4]);
    c->dig_P1 = (uint16_t)((buf[7]  << 8) | buf[6]);
    c->dig_P2 = (int16_t) ((buf[9]  << 8) | buf[8]);
    c->dig_P3 = (int16_t) ((buf[11] << 8) | buf[10]);
    c->dig_P4 = (int16_t) ((buf[13] << 8) | buf[12]);
    c->dig_P5 = (int16_t) ((buf[15] << 8) | buf[14]);
    c->dig_P6 = (int16_t) ((buf[17] << 8) | buf[16]);
    c->dig_P7 = (int16_t) ((buf[19] << 8) | buf[18]);
    c->dig_P8 = (int16_t) ((buf[21] << 8) | buf[20]);
    c->dig_P9 = (int16_t) ((buf[23] << 8) | buf[22]);
    return BMP280_OK;
}

/* ========== Private: Temperature Compensation (Bosch formula) ========== */

static float _compensate_temp(BMP280_Instance* bmp, int32_t adc_T) {
    BMP280_Calib* c = &bmp->calib;

    float var1 = ((float)adc_T / 16384.0f - (float)c->dig_T1 / 1024.0f)
                 * (float)c->dig_T2;
    float var2 = ((float)adc_T / 131072.0f - (float)c->dig_T1 / 8192.0f)
                 * ((float)adc_T / 131072.0f - (float)c->dig_T1 / 8192.0f)
                 * (float)c->dig_T3;

    bmp->t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0f;
}

/* ========== Private: Pressure Compensation (Bosch formula) ========== */

static float _compensate_press(BMP280_Instance* bmp, int32_t adc_P) {
    BMP280_Calib* c = &bmp->calib;

    float var1 = (float)bmp->t_fine / 2.0f - 64000.0f;
    float var2 = var1 * var1 * (float)c->dig_P6 / 32768.0f;
    var2 = var2 + var1 * (float)c->dig_P5 * 2.0f;
    var2 = var2 / 4.0f + (float)c->dig_P4 * 65536.0f;
    var1 = ((float)c->dig_P3 * var1 * var1 / 524288.0f
            + (float)c->dig_P2 * var1) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * (float)c->dig_P1;

    if (var1 == 0.0f) return 0.0f; /* ป้องกัน division by zero */

    float press = 1048576.0f - (float)adc_P;
    press = (press - var2 / 4096.0f) * 6250.0f / var1;
    var1 = (float)c->dig_P9 * press * press / 2147483648.0f;
    var2 = press * (float)c->dig_P8 / 32768.0f;
    press = press + (var1 + var2 + (float)c->dig_P7) / 16.0f;

    return press / 100.0f;  /* Pa → hPa */
}

/* ========== Public Functions ========== */

BMP280_Status BMP280_Init(BMP280_Instance* bmp, uint8_t i2c_addr) {
    if (bmp == NULL) return BMP280_ERROR_PARAM;

    bmp->i2c_addr   = i2c_addr;
    bmp->t_fine     = 0;
    bmp->initialized = 0;

    /* ตรวจสอบ CHIP_ID */
    uint8_t chip_id;
    if (_read_reg(i2c_addr, BMP280_REG_CHIP_ID, &chip_id) != BMP280_OK)
        return BMP280_ERROR_I2C;
    if (chip_id != BMP280_CHIP_ID)
        return BMP280_ERROR_CHIPID;

    /* Soft reset */
    _write_reg(i2c_addr, BMP280_REG_RESET, BMP280_RESET_VALUE);
    Delay_Ms(10);

    /* โหลด calibration data จาก OTP */
    BMP280_Status st = _load_calib(bmp);
    if (st != BMP280_OK) return st;

    /* ตั้งค่า default:
     *  ctrl_meas: osrs_t=x2 (010), osrs_p=x16 (101), mode=Normal (11)
     *  config:    t_sb=125ms (010), filter=x4 (010), spi3w_en=0
     */
    uint8_t ctrl = ((uint8_t)BMP280_OS_X2 << 5) | ((uint8_t)BMP280_OS_X16 << 2) | (uint8_t)BMP280_MODE_NORMAL;
    st = _write_reg(i2c_addr, BMP280_REG_CTRL_MEAS, ctrl);
    if (st != BMP280_OK) return st;

    uint8_t cfg = ((uint8_t)BMP280_STANDBY_125MS << 5) | ((uint8_t)BMP280_FILTER_X4 << 2);
    st = _write_reg(i2c_addr, BMP280_REG_CONFIG, cfg);
    if (st != BMP280_OK) return st;

    bmp->initialized = 1;
    return BMP280_OK;
}

BMP280_Status BMP280_SetMode(BMP280_Instance* bmp, BMP280_Mode mode,
                              BMP280_Oversampling os_temp, BMP280_Oversampling os_press) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;

    uint8_t ctrl = ((uint8_t)os_temp << 5) | ((uint8_t)os_press << 2) | (uint8_t)mode;
    return _write_reg(bmp->i2c_addr, BMP280_REG_CTRL_MEAS, ctrl);
}

BMP280_Status BMP280_SetFilter(BMP280_Instance* bmp, BMP280_Filter filter) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;

    uint8_t cfg;
    BMP280_Status st = _read_reg(bmp->i2c_addr, BMP280_REG_CONFIG, &cfg);
    if (st != BMP280_OK) return st;

    cfg = (cfg & ~(0x07 << 2)) | ((uint8_t)filter << 2);
    return _write_reg(bmp->i2c_addr, BMP280_REG_CONFIG, cfg);
}

BMP280_Status BMP280_Read(BMP280_Instance* bmp, float* temp, float* press) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;

    /* อ่าน 6 bytes: press(3) + temp(3) */
    uint8_t buf[6];
    BMP280_Status st = _read_regs(bmp->i2c_addr, BMP280_REG_PRESS_MSB, buf, 6);
    if (st != BMP280_OK) return st;

    int32_t adc_P = (int32_t)(((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | (buf[2] >> 4));
    int32_t adc_T = (int32_t)(((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | (buf[5] >> 4));

    /* อุณหภูมิก่อน (จำเป็นเพราะ t_fine ใช้ใน pressure) */
    float t = _compensate_temp(bmp, adc_T);
    if (temp  != NULL) *temp  = t;
    if (press != NULL) *press = _compensate_press(bmp, adc_P);

    return BMP280_OK;
}

float BMP280_GetAltitude(BMP280_Instance* bmp, float pressure, float sea_level_pa) {
    (void)bmp;
    if (sea_level_pa <= 0.0f) sea_level_pa = 1013.25f;
    /* สูตร: alt = 44330 × (1 - (P/P0)^(1/5.255)) */
    return 44330.0f * (1.0f - powf(pressure / sea_level_pa, 0.1903f));
}

BMP280_Status BMP280_Reset(BMP280_Instance* bmp) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;
    BMP280_Status st = _write_reg(bmp->i2c_addr, BMP280_REG_RESET, BMP280_RESET_VALUE);
    if (st != BMP280_OK) return st;
    Delay_Ms(10);
    return _load_calib(bmp);
}
