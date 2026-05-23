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

/**
 * @brief เริ่มต้น BMP280
 *
 * @param bmp      ตัวแปร instance
 * @param i2c_addr I2C address (BMP280_ADDR_LOW หรือ BMP280_ADDR_HIGH)
 *
 * @return BMP280_OK, BMP280_ERROR_I2C, BMP280_ERROR_CHIPID, หรือ BMP280_ERROR_PARAM
 *
 * @note ตรวจสอบ CHIP_ID (=0x60) → soft reset → delay 10ms → โหลด calibration
 *       ตั้ง default: Normal mode, Temp x2, Press x16, Filter x4, Standby 125ms
 *       Calibration data 24 bytes จาก register 0x88
 *       t_fine ถูก initialize เป็น 0
 */
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

/**
 * @brief ตั้งค่า mode และ oversampling
 *
 * @param bmp      ตัวแปร instance
 * @param mode     BMP280_MODE_SLEEP / FORCED / NORMAL
 * @param os_temp  Oversampling สำหรับอุณหภูมิ
 * @param os_press Oversampling สำหรับความดัน
 *
 * @return BMP280_OK หรือ BMP280_ERROR_I2C
 *
 * @note เขียน ctrl_meas register โดยตรง
 *       Forced mode: วัด 1 ครั้งแล้วกลับ sleep — ต้องตั้งใหม่ทุกครั้งที่ต้องการอ่าน
 *       Oversampling สูง = noise น้อยลง แต่ใช้เวลามากขึ้น
 */
BMP280_Status BMP280_SetMode(BMP280_Instance* bmp, BMP280_Mode mode,
                              BMP280_Oversampling os_temp, BMP280_Oversampling os_press) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;

    uint8_t ctrl = ((uint8_t)os_temp << 5) | ((uint8_t)os_press << 2) | (uint8_t)mode;
    return _write_reg(bmp->i2c_addr, BMP280_REG_CTRL_MEAS, ctrl);
}

/**
 * @brief ตั้ง IIR Filter
 *
 * @param bmp    ตัวแปร instance
 * @param filter BMP280_FILTER_OFF ถึง BMP280_FILTER_X16
 *
 * @return BMP280_OK หรือ BMP280_ERROR_I2C
 *
 * @note อ่าน config register → modify bits [4:2] → เขียนกลับ
 *       ไม่เปลี่ยน bits อื่น (standby time, spi3w_en)
 */
BMP280_Status BMP280_SetFilter(BMP280_Instance* bmp, BMP280_Filter filter) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;

    uint8_t cfg;
    BMP280_Status st = _read_reg(bmp->i2c_addr, BMP280_REG_CONFIG, &cfg);
    if (st != BMP280_OK) return st;

    cfg = (cfg & ~(0x07 << 2)) | ((uint8_t)filter << 2);
    return _write_reg(bmp->i2c_addr, BMP280_REG_CONFIG, cfg);
}

/**
 * @brief อ่านอุณหภูมิและความดัน
 *
 * @param bmp   ตัวแปร instance
 * @param temp  pointer รับอุณหภูมิ (°C) — NULL ถ้าไม่ต้องการ
 * @param press pointer รับความดัน (hPa) — NULL ถ้าไม่ต้องการ
 *
 * @return BMP280_OK หรือ error code
 *
 * @note อ่าน 6 bytes จาก REG_PRESS_MSB (press MSB → temp LSB)
 *       adc_P = buf[0:2] 20-bit, adc_T = buf[3:5] 20-bit
 *       ต้อง compensate อุณหภูมิก่อนเสมอ (t_fine ใช้ใน pressure compensation)
 *       ใช้ Bosch formula แบบ float
 *       t_fine ถูกเซ็ตใน _compensate_temp() และใช้ใน _compensate_press()
 */
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

/**
 * @brief คำนวณความสูงโดยประมาณจากความดัน
 *
 * @param bmp         ตัวแปร instance (ไม่ใช้ค่า, แค่ตรวจ initialized)
 * @param pressure    ความดัน ณ จุดนั้น (hPa) จาก BMP280_Read()
 * @param sea_level_pa ความดันที่ระดับน้ำทะเล (hPa) — ปกติ 1013.25
 *
 * @return ความสูง (เมตร), 0.0 ถ้า sea_level_pa <= 0
 *
 * @note สูตร: alt = 44330 × (1 - (P/P0)^(1/5.255)) = 44330 × (1 - (P/P0)^0.1903)
 *       ใช้ powf() จาก math.h
 *       ถ้า sea_level_pa <= 0 ให้ใช้ default 1013.25
 */
float BMP280_GetAltitude(BMP280_Instance* bmp, float pressure, float sea_level_pa) {
    (void)bmp;
    if (sea_level_pa <= 0.0f) sea_level_pa = 1013.25f;
    /* สูตร: alt = 44330 × (1 - (P/P0)^(1/5.255)) */
    return 44330.0f * (1.0f - powf(pressure / sea_level_pa, 0.1903f));
}

/**
 * @brief Soft reset BMP280
 *
 * @param bmp ตัวแปร instance
 *
 * @return BMP280_OK หรือ error code
 *
 * @note เขียน 0xB6 ไปยัง REG_RESET → delay 10ms → โหลด calibration ใหม่
 *       หลังจาก reset ค่ากลับเป็น default register values
 *       ต้องเรียก BMP280_SetMode() ซ้ำถ้าต้องการปรับ
 */
BMP280_Status BMP280_Reset(BMP280_Instance* bmp) {
    if (bmp == NULL || !bmp->initialized) return BMP280_ERROR_PARAM;
    BMP280_Status st = _write_reg(bmp->i2c_addr, BMP280_REG_RESET, BMP280_RESET_VALUE);
    if (st != BMP280_OK) return st;
    Delay_Ms(10);
    return _load_calib(bmp);
}
