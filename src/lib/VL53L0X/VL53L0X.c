/**
 * @file VL53L0X.c
 * @brief VL53L0X Time-of-Flight Distance Sensor Implementation
 * @note Based on ST VL53L0X API init sequence (simplified)
 */

#include "VL53L0X.h"

/* ========== Private: Register R/W ========== */

static VL53L0X_Status _write(VL53L0X_Instance* tof, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return (I2C_Write(tof->i2c_addr, buf, 2) == 0) ? VL53L0X_OK : VL53L0X_ERROR_I2C;
}

static VL53L0X_Status _write16(VL53L0X_Instance* tof, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    return (I2C_Write(tof->i2c_addr, buf, 3) == 0) ? VL53L0X_OK : VL53L0X_ERROR_I2C;
}

static uint8_t _read(VL53L0X_Instance* tof, uint8_t reg) {
    uint8_t val = 0;
    I2C_Write(tof->i2c_addr, &reg, 1);
    I2C_Read(tof->i2c_addr, &val, 1);
    return val;
}

static uint16_t _read16(VL53L0X_Instance* tof, uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    I2C_Write(tof->i2c_addr, &reg, 1);
    I2C_Read(tof->i2c_addr, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/* ========== Private: Perform single range measurement ========== */

static uint16_t _perform_single_range(VL53L0X_Instance* tof) {
    _write(tof, 0x80, 0x01);
    _write(tof, 0xFF, 0x01);
    _write(tof, 0x00, 0x00);
    _write(tof, 0x91, tof->stop_variable);
    _write(tof, 0x00, 0x01);
    _write(tof, 0xFF, 0x00);
    _write(tof, 0x80, 0x00);

    /* Start single range */
    _write(tof, VL53L0X_REG_SYSRANGE_START, 0x01);

    /* รอ start bit clear */
    uint32_t start = Get_CurrentMs();
    while (_read(tof, VL53L0X_REG_SYSRANGE_START) & 0x01) {
        if ((Get_CurrentMs() - start) > 500) return VL53L0X_OUT_OF_RANGE;
    }

    /* รอผลลัพธ์ (GPIOFUNCTIONALITY new_sample_ready) */
    start = Get_CurrentMs();
    while ((_read(tof, VL53L0X_REG_RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((Get_CurrentMs() - start) > 500) return VL53L0X_OUT_OF_RANGE;
    }

    uint16_t range = _read16(tof, VL53L0X_REG_RESULT_FINAL_RANGE_MM);

    /* Clear interrupt */
    _write(tof, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    if (range >= VL53L0X_OUT_OF_RANGE) range = VL53L0X_OUT_OF_RANGE;
    return range;
}

/* ========== Public ========== */

/**
 * @brief Initializes VL53L0X ToF sensor with standard init sequence
 *
 * @param tof - pointer to VL53L0X_Instance (NULL check)
 * @param addr - I2C address (e.g. 0x29)
 *
 * @return VL53L0X_OK on success, VL53L0X_ERROR_BOOT if model ID != 0xEE
 *
 * @note ตรวจ model ID (0xEE), I/O 2.8V mode, disable signal rate limit checks,
 *       ตั้ง signal rate limit = 0.25 MCPS, ทำ recalibration 2 ขั้นตอน
 *       เก็บ stop_variable สำหรับใช้ในการวัดครั้งถัดไป
 */
VL53L0X_Status VL53L0X_Init(VL53L0X_Instance* tof, uint8_t addr) {
    if (tof == NULL) return VL53L0X_ERROR_PARAM;

    tof->i2c_addr    = addr;
    tof->stop_variable = 0;
    tof->mode        = VL53L0X_MODE_SINGLE;
    tof->initialized = 0;

    /* ตรวจ WHO_AM_I */
    uint8_t model_id = _read(tof, VL53L0X_REG_IDENTIFICATION_MODEL_ID);
    if (model_id != 0xEE) return VL53L0X_ERROR_BOOT;

    /* I/O 2.8V mode */
    uint8_t vhv = _read(tof, VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV);
    _write(tof, VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, vhv | 0x01);

    /* Standard init sequence (ตาม ST reference) */
    _write(tof, 0x88, 0x00);
    _write(tof, 0x80, 0x01);
    _write(tof, 0xFF, 0x01);
    _write(tof, 0x00, 0x00);

    tof->stop_variable = _read(tof, 0x91);

    _write(tof, 0x00, 0x01);
    _write(tof, 0xFF, 0x00);
    _write(tof, 0x80, 0x00);

    /* Disable SIGNAL_RATE_MSRC + SIGNAL_RATE_PRE_RANGE limit checks */
    uint8_t msrc = _read(tof, VL53L0X_REG_MSRC_CONFIG_CONTROL);
    _write(tof, VL53L0X_REG_MSRC_CONFIG_CONTROL, msrc | 0x12);

    /* Set signal rate limit: 0.25 MCPS */
    _write16(tof, VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 0x0020);

    _write(tof, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xFF);

    /* ตั้ง sequence steps */
    _write(tof, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);

    /* Recalibrate */
    _write(tof, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x01);
    _write(tof, VL53L0X_REG_SYSRANGE_START, 0x01);
    Delay_Ms(50);
    _write(tof, VL53L0X_REG_SYSRANGE_START, 0x00);

    _write(tof, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x02);
    _write(tof, VL53L0X_REG_SYSRANGE_START, 0x01);
    Delay_Ms(50);
    _write(tof, VL53L0X_REG_SYSRANGE_START, 0x00);

    _write(tof, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);

    tof->initialized = 1;
    return VL53L0X_OK;
}

/**
 * @brief Performs a single range measurement in mm
 *
 * @param tof - pointer to VL53L0X_Instance (ต้อง init แล้ว)
 *
 * @return distance in mm, VL53L0X_OUT_OF_RANGE if timeout or error
 *
 * @note ใช้ _perform_single_range ภายใน: start → poll start bit clear → poll interrupt → read result
 *       timeout = 500ms ต่อขั้นตอน
 *       clear interrupt หลังอ่าน
 */
uint16_t VL53L0X_ReadRangeMM(VL53L0X_Instance* tof) {
    if (tof == NULL || !tof->initialized) return VL53L0X_OUT_OF_RANGE;
    return _perform_single_range(tof);
}

/**
 * @brief Starts continuous range measurement mode
 *
 * @param tof - pointer to VL53L0X_Instance (ต้อง init แล้ว)
 * @param period_ms - interval between measurements (0 = free-running)
 *
 * @return VL53L0X_OK on success, error otherwise
 *
 * @note period_ms > 0 → timed continuous (set OSC calibrate + timing registers, start = 0x04)
 *       period_ms = 0 → free-running continuous (start = 0x02)
 *       ต้องเรียก VL53L0X_ReadContinuous เพื่ออ่านผลลัพท์
 */
VL53L0X_Status VL53L0X_StartContinuous(VL53L0X_Instance* tof, uint32_t period_ms) {
    if (tof == NULL || !tof->initialized) return VL53L0X_ERROR_PARAM;

    _write(tof, 0x80, 0x01);
    _write(tof, 0xFF, 0x01);
    _write(tof, 0x00, 0x00);
    _write(tof, 0x91, tof->stop_variable);
    _write(tof, 0x00, 0x01);
    _write(tof, 0xFF, 0x00);
    _write(tof, 0x80, 0x00);

    if (period_ms > 0) {
        /* Timed continuous */
        uint16_t osc = _read16(tof, VL53L0X_REG_OSC_CALIBRATE_VAL);
        if (osc != 0) period_ms *= osc;
        _write16(tof, 0x04, (uint16_t)(period_ms >> 16));
        _write16(tof, 0x06, (uint16_t)(period_ms & 0xFFFF));
        _write(tof, VL53L0X_REG_SYSRANGE_START, 0x04);
    } else {
        _write(tof, VL53L0X_REG_SYSRANGE_START, 0x02);
    }

    tof->mode = VL53L0X_MODE_CONTINUOUS;
    return VL53L0X_OK;
}

/**
 * @brief Stops continuous measurement mode
 *
 * @param tof - pointer to VL53L0X_Instance (ต้อง init แล้ว)
 *
 * @return VL53L0X_OK on success, error otherwise
 *
 * @note หยุดการวัดโดยเขียน SYSRANGE_START = 0x01 แล้ว clear stop_variable
 *       กลับสู่ SINGLE mode
 */
VL53L0X_Status VL53L0X_StopContinuous(VL53L0X_Instance* tof) {
    if (tof == NULL || !tof->initialized) return VL53L0X_ERROR_PARAM;

    _write(tof, VL53L0X_REG_SYSRANGE_START, 0x01);
    _write(tof, 0xFF, 0x01);
    _write(tof, 0x00, 0x00);
    _write(tof, 0x91, 0x00);
    _write(tof, 0x00, 0x01);
    _write(tof, 0xFF, 0x00);

    tof->mode = VL53L0X_MODE_SINGLE;
    return VL53L0X_OK;
}

/**
 * @brief Reads distance from continuous mode (non-blocking)
 *
 * @param tof - pointer to VL53L0X_Instance (ต้อง init แล้ว)
 * @param dist_mm - output distance in mm
 *
 * @return VL53L0X_OK on success, VL53L0X_ERROR_TIMEOUT if data not ready yet
 *
 * @note ตรวจ RESULT_INTERRUPT_STATUS ก่อนอ่าน
 *       clear interrupt หลังอ่านเสมอ
 */
VL53L0X_Status VL53L0X_ReadContinuous(VL53L0X_Instance* tof, uint16_t* dist_mm) {
    if (tof == NULL || !tof->initialized || dist_mm == NULL)
        return VL53L0X_ERROR_PARAM;

    if ((_read(tof, VL53L0X_REG_RESULT_INTERRUPT_STATUS) & 0x07) == 0)
        return VL53L0X_ERROR_TIMEOUT;  /* ยังไม่พร้อม */

    *dist_mm = _read16(tof, VL53L0X_REG_RESULT_FINAL_RANGE_MM);
    if (*dist_mm >= VL53L0X_OUT_OF_RANGE) *dist_mm = VL53L0X_OUT_OF_RANGE;

    _write(tof, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    return VL53L0X_OK;
}

/**
 * @brief Changes I2C address of VL53L0X (non-volatile for session)
 *
 * @param tof - pointer to VL53L0X_Instance (ต้อง init แล้ว)
 * @param new_addr - new I2C address (7-bit, 0x00–0x7F)
 *
 * @return VL53L0X_OK on success, error otherwise
 *
 * @note เขียนไปที่ register 0x8A, mask 0x7F
 *       effect ทันที ไม่ต้อง reboot
 *       อัปเดต tof->i2c_addr
 */
VL53L0X_Status VL53L0X_SetAddress(VL53L0X_Instance* tof, uint8_t new_addr) {
    if (tof == NULL || !tof->initialized) return VL53L0X_ERROR_PARAM;
    if (_write(tof, 0x8A, new_addr & 0x7F) != VL53L0X_OK) return VL53L0X_ERROR_I2C;
    tof->i2c_addr = new_addr & 0x7F;
    return VL53L0X_OK;
}
