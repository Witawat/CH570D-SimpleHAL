/**
 * @file MPU6050.c
 * @brief MPU6050 6-Axis IMU Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "MPU6050.h"

/* ========== Private: Accel LSB lookup ========== */
static const float _accel_lsb_table[] = { 16384.0f, 8192.0f, 4096.0f, 2048.0f };
static const float _gyro_lsb_table[]  = { 131.0f, 65.5f, 32.8f, 16.4f };

/* ========== Private: Register Helpers ========== */

static MPU6050_Status _write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    return (I2C_WriteReg(addr, reg, val) == I2C_OK) ? MPU6050_OK : MPU6050_ERROR_I2C;
}

static MPU6050_Status _read_reg(uint8_t addr, uint8_t reg, uint8_t* val) {
    return (I2C_ReadRegMulti(addr, reg, val, 1) == I2C_OK) ? MPU6050_OK : MPU6050_ERROR_I2C;
}

/**
 * @brief อ่าน block register (sequential read ผ่าน I2C_Write → I2C_Read)
 */
static MPU6050_Status _read_regs(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
    uint8_t start = reg;
    if (I2C_Write(addr, &start, 1) != I2C_OK) return MPU6050_ERROR_I2C;
    if (I2C_Read(addr, buf, len) != I2C_OK)   return MPU6050_ERROR_I2C;
    return MPU6050_OK;
}

/* ========== Public Functions ========== */

/**
 * @brief Initializes MPU6050 sensor (wake up + register config)
 *
 * @param imu - pointer to MPU6050_Instance (NULL check)
 * @param i2c_addr - I2C address (0x68 or 0x69)
 *
 * @return MPU6050_OK on success, MPU6050_ERROR_I2C / MPU6050_ERROR_WHOAMI / MPU6050_ERROR_PARAM on failure
 *
 * @note ตรวจสอบ WHO_AM_I, wake up (ล้าง SLEEP), DLPF = 44Hz,
 *       sample rate = 1kHz, Accel ±2g, Gyro ±250°/s
 */
MPU6050_Status MPU6050_Init(MPU6050_Instance* imu, uint8_t i2c_addr) {
    if (imu == NULL) return MPU6050_ERROR_PARAM;

    imu->i2c_addr     = i2c_addr;
    imu->accel_range  = MPU6050_ACCEL_2G;
    imu->gyro_range   = MPU6050_GYRO_250DPS;
    imu->accel_lsb    = _accel_lsb_table[0];
    imu->gyro_lsb     = _gyro_lsb_table[0];
    imu->gyro_offset_x = 0;
    imu->gyro_offset_y = 0;
    imu->gyro_offset_z = 0;
    imu->initialized  = 0;

    /* ตรวจสอบ WHO_AM_I */
    uint8_t whoami;
    if (_read_reg(i2c_addr, MPU6050_REG_WHO_AM_I, &whoami) != MPU6050_OK)
        return MPU6050_ERROR_I2C;
    if (whoami != MPU6050_WHO_AM_I_VAL)
        return MPU6050_ERROR_WHOAMI;

    /* Wake up (ล้าง SLEEP bit) */
    MPU6050_Status st = _write_reg(i2c_addr, MPU6050_REG_PWR_MGMT_1, 0x00);
    if (st != MPU6050_OK) return st;
    Delay_Ms(100);

    /* ตั้ง DLPF = 44Hz */
    st = _write_reg(i2c_addr, MPU6050_REG_CONFIG, (uint8_t)MPU6050_DLPF_44HZ);
    if (st != MPU6050_OK) return st;

    /* ตั้ง sample rate = 1kHz / (1+0) = 1kHz */
    st = _write_reg(i2c_addr, MPU6050_REG_SMPLRT_DIV, 0x00);
    if (st != MPU6050_OK) return st;

    /* ตั้ง Accel = ±2g */
    st = _write_reg(i2c_addr, MPU6050_REG_ACCEL_CONFIG,
                    (uint8_t)(MPU6050_ACCEL_2G << 3));
    if (st != MPU6050_OK) return st;

    /* ตั้ง Gyro = ±250°/s */
    st = _write_reg(i2c_addr, MPU6050_REG_GYRO_CONFIG,
                    (uint8_t)(MPU6050_GYRO_250DPS << 3));
    if (st != MPU6050_OK) return st;

    imu->initialized = 1;
    return MPU6050_OK;
}

/**
 * @brief Sets accelerometer full-scale range
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param range - MPU6050_AccelRange (2g/4g/8g/16g)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note เขียน REG_ACCEL_CONFIG, อัปเดต accel_lsb สำหรับแปลงค่าใน GetAccel
 */
MPU6050_Status MPU6050_SetAccelRange(MPU6050_Instance* imu, MPU6050_AccelRange range) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;

    MPU6050_Status st = _write_reg(imu->i2c_addr, MPU6050_REG_ACCEL_CONFIG,
                                    (uint8_t)(range << 3));
    if (st != MPU6050_OK) return st;

    imu->accel_range = range;
    imu->accel_lsb   = _accel_lsb_table[range];
    return MPU6050_OK;
}

/**
 * @brief Sets gyroscope full-scale range
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param range - MPU6050_GyroRange (250/500/1000/2000 °/s)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note เขียน REG_GYRO_CONFIG, อัปเดต gyro_lsb สำหรับแปลงค่าใน GetGyro
 */
MPU6050_Status MPU6050_SetGyroRange(MPU6050_Instance* imu, MPU6050_GyroRange range) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;

    MPU6050_Status st = _write_reg(imu->i2c_addr, MPU6050_REG_GYRO_CONFIG,
                                    (uint8_t)(range << 3));
    if (st != MPU6050_OK) return st;

    imu->gyro_range = range;
    imu->gyro_lsb   = _gyro_lsb_table[range];
    return MPU6050_OK;
}

/**
 * @brief Sets Digital Low-Pass Filter bandwidth
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param dlpf - MPU6050_DLPF value (e.g. MPU6050_DLPF_44HZ)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note เขียน REG_CONFIG โดยตรง ไม่มีผลกับ accel_lsb / gyro_lsb
 */
MPU6050_Status MPU6050_SetDLPF(MPU6050_Instance* imu, MPU6050_DLPF dlpf) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;
    return _write_reg(imu->i2c_addr, MPU6050_REG_CONFIG, (uint8_t)dlpf);
}

/**
 * @brief Reads raw sensor data (accel + temp + gyro) in one I2C burst
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param raw - output buffer for raw values (ax/ay/az, temp, gx/gy/gz)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note อ่าน 14 bytes ต่อเนื่องตั้งแต่ REG_ACCEL_XOUT_H ถึง REG_GYRO_ZOUT_L
 *       ใช้ combined transaction (I2C_Write + I2C_Read)
 */
MPU6050_Status MPU6050_GetRaw(MPU6050_Instance* imu, MPU6050_RawData* raw) {
    if (imu == NULL || !imu->initialized || raw == NULL) return MPU6050_ERROR_PARAM;

    /* อ่าน 14 bytes: ACCEL(6) + TEMP(2) + GYRO(6) ต่อเนื่องกัน */
    uint8_t buf[14];
    MPU6050_Status st = _read_regs(imu->i2c_addr, MPU6050_REG_ACCEL_XOUT_H, buf, 14);
    if (st != MPU6050_OK) return st;

    raw->ax_raw   = (int16_t)((buf[0]  << 8) | buf[1]);
    raw->ay_raw   = (int16_t)((buf[2]  << 8) | buf[3]);
    raw->az_raw   = (int16_t)((buf[4]  << 8) | buf[5]);
    raw->temp_raw = (int16_t)((buf[6]  << 8) | buf[7]);
    raw->gx_raw   = (int16_t)((buf[8]  << 8) | buf[9]);
    raw->gy_raw   = (int16_t)((buf[10] << 8) | buf[11]);
    raw->gz_raw   = (int16_t)((buf[12] << 8) | buf[13]);

    return MPU6050_OK;
}

/**
 * @brief Reads accelerometer as g values
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param ax - output X-axis acceleration (g)
 * @param ay - output Y-axis acceleration (g)
 * @param az - output Z-axis acceleration (g)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note แปลง raw value / accel_lsb (ขึ้นอยู่กับ AccelRange ที่ตั้งไว้)
 */
MPU6050_Status MPU6050_GetAccel(MPU6050_Instance* imu, float* ax, float* ay, float* az) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;
    if (ax == NULL || ay == NULL || az == NULL) return MPU6050_ERROR_PARAM;

    uint8_t buf[6];
    MPU6050_Status st = _read_regs(imu->i2c_addr, MPU6050_REG_ACCEL_XOUT_H, buf, 6);
    if (st != MPU6050_OK) return st;

    *ax = (float)(int16_t)((buf[0] << 8) | buf[1]) / imu->accel_lsb;
    *ay = (float)(int16_t)((buf[2] << 8) | buf[3]) / imu->accel_lsb;
    *az = (float)(int16_t)((buf[4] << 8) | buf[5]) / imu->accel_lsb;
    return MPU6050_OK;
}

/**
 * @brief Reads gyroscope as °/s values (with calibration offset correction)
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param gx - output X-axis angular rate (°/s)
 * @param gy - output Y-axis angular rate (°/s)
 * @param gz - output Z-axis angular rate (°/s)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note หัก offset จาก CalibrateGyro ก่อนหาร gyro_lsb
 */
MPU6050_Status MPU6050_GetGyro(MPU6050_Instance* imu, float* gx, float* gy, float* gz) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;
    if (gx == NULL || gy == NULL || gz == NULL) return MPU6050_ERROR_PARAM;

    uint8_t buf[6];
    MPU6050_Status st = _read_regs(imu->i2c_addr, MPU6050_REG_GYRO_XOUT_H, buf, 6);
    if (st != MPU6050_OK) return st;

    int16_t rx = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ry = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t rz = (int16_t)((buf[4] << 8) | buf[5]);

    /* หัก calibration offset */
    *gx = (float)(rx - imu->gyro_offset_x) / imu->gyro_lsb;
    *gy = (float)(ry - imu->gyro_offset_y) / imu->gyro_lsb;
    *gz = (float)(rz - imu->gyro_offset_z) / imu->gyro_lsb;
    return MPU6050_OK;
}

/**
 * @brief Reads temperature sensor
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param temp - output temperature (°C)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note สูตรจาก datasheet: Temp(°C) = raw/340 + 36.53
 */
MPU6050_Status MPU6050_GetTemp(MPU6050_Instance* imu, float* temp) {
    if (imu == NULL || !imu->initialized || temp == NULL) return MPU6050_ERROR_PARAM;

    uint8_t buf[2];
    MPU6050_Status st = _read_regs(imu->i2c_addr, MPU6050_REG_TEMP_OUT_H, buf, 2);
    if (st != MPU6050_OK) return st;

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    /* สูตรจาก MPU6050 datasheet: Temp(°C) = raw/340 + 36.53 */
    *temp = (float)raw / 340.0f + 36.53f;
    return MPU6050_OK;
}

/**
 * @brief Reads all measurements (accel + gyro + temp) in one burst
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param data - output struct with ax/ay/az (g), gx/gy/gz (°/s), temp (°C)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note เรียก GetRaw ภายใน แล้วแปลงค่าทั้งหมดพร้อมหัก gyro offset
 */
MPU6050_Status MPU6050_GetAll(MPU6050_Instance* imu, MPU6050_Data* data) {
    if (imu == NULL || !imu->initialized || data == NULL) return MPU6050_ERROR_PARAM;

    MPU6050_RawData raw;
    MPU6050_Status st = MPU6050_GetRaw(imu, &raw);
    if (st != MPU6050_OK) return st;

    data->ax   = (float)raw.ax_raw / imu->accel_lsb;
    data->ay   = (float)raw.ay_raw / imu->accel_lsb;
    data->az   = (float)raw.az_raw / imu->accel_lsb;
    data->gx   = (float)(raw.gx_raw - imu->gyro_offset_x) / imu->gyro_lsb;
    data->gy   = (float)(raw.gy_raw - imu->gyro_offset_y) / imu->gyro_lsb;
    data->gz   = (float)(raw.gz_raw - imu->gyro_offset_z) / imu->gyro_lsb;
    data->temp = (float)raw.temp_raw / 340.0f + 36.53f;
    return MPU6050_OK;
}

/**
 * @brief Calibrates gyroscope zero-rate offset
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 * @param samples - number of samples to average (0 = 200 default)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note อ่านค่า gyro samples ครั้ง หาค่าเฉลี่ยเก็บใน gyro_offset_x/y/z
 *       ควรวาง sensor นิ่งระหว่าง calibrate, หน่วง 2ms ระหว่าง samples
 */
MPU6050_Status MPU6050_CalibrateGyro(MPU6050_Instance* imu, uint16_t samples) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;
    if (samples == 0) samples = 200;

    /* Reset offset ก่อน */
    imu->gyro_offset_x = 0;
    imu->gyro_offset_y = 0;
    imu->gyro_offset_z = 0;

    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    for (uint16_t i = 0; i < samples; i++) {
        uint8_t buf[6];
        MPU6050_Status st = _read_regs(imu->i2c_addr, MPU6050_REG_GYRO_XOUT_H, buf, 6);
        if (st != MPU6050_OK) return st;

        sum_x += (int16_t)((buf[0] << 8) | buf[1]);
        sum_y += (int16_t)((buf[2] << 8) | buf[3]);
        sum_z += (int16_t)((buf[4] << 8) | buf[5]);
        Delay_Ms(2);
    }

    imu->gyro_offset_x = (int16_t)(sum_x / samples);
    imu->gyro_offset_y = (int16_t)(sum_y / samples);
    imu->gyro_offset_z = (int16_t)(sum_z / samples);
    return MPU6050_OK;
}

/**
 * @brief Wakes MPU6050 from sleep mode
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note ล้าง SLEEP bit (bit 6) ใน REG_PWR_MGMT_1
 */
MPU6050_Status MPU6050_WakeUp(MPU6050_Instance* imu) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;
    uint8_t val;
    MPU6050_Status st = _read_reg(imu->i2c_addr, MPU6050_REG_PWR_MGMT_1, &val);
    if (st != MPU6050_OK) return st;
    val &= ~(1 << 6);  /* ล้าง SLEEP bit */
    return _write_reg(imu->i2c_addr, MPU6050_REG_PWR_MGMT_1, val);
}

/**
 * @brief Puts MPU6050 into sleep mode (low power)
 *
 * @param imu - pointer to MPU6050_Instance (ต้อง init แล้ว)
 *
 * @return MPU6050_OK on success, error otherwise
 *
 * @note ตั้ง SLEEP bit (bit 6) ใน REG_PWR_MGMT_1
 */
MPU6050_Status MPU6050_Sleep(MPU6050_Instance* imu) {
    if (imu == NULL || !imu->initialized) return MPU6050_ERROR_PARAM;
    uint8_t val;
    MPU6050_Status st = _read_reg(imu->i2c_addr, MPU6050_REG_PWR_MGMT_1, &val);
    if (st != MPU6050_OK) return st;
    val |= (1 << 6);  /* ตั้ง SLEEP bit */
    return _write_reg(imu->i2c_addr, MPU6050_REG_PWR_MGMT_1, val);
}
