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

/**
 * @brief เริ่มต้น BH1750
 *
 * @param bh   ตัวแปร instance
 * @param addr BH1750_ADDR_LOW (0x23) หรือ BH1750_ADDR_HIGH (0x5C)
 *
 * @return BH1750_OK หรือ error code
 *
 * @note ส่ง POWER_ON → RESET → delay 10ms → CONT_H_RES → delay 180ms
 *       delay 180ms เพื่อรอ first measurement ใน H-Resolution mode
 *       Default mode: BH1750_CONT_H_RES (1 lux resolution, 120ms cycle)
 */
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

/**
 * @brief ตั้งโหมดการวัด
 *
 * @param bh   ตัวแปร instance
 * @param mode โหมดการวัด (BH1750_CONT_H_RES, BH1750_ONE_L_RES, ฯลฯ)
 *
 * @return BH1750_OK หรือ BH1750_ERROR_I2C
 *
 * @note ส่ง command byte ไปยัง sensor โดยตรง
 *       One-time modes จะ sleep หลังจากอ่านครั้งเดียว ต้องส่ง command ทุกครั้ง
 *       Continuous modes วัดตลอด — อ่านเมื่อไหร่ก็ได้ค่าใหม่ล่าสุด
 */
BH1750_Status BH1750_SetMode(BH1750_Instance* bh, BH1750_Mode mode) {
    if (bh == NULL || !bh->initialized) return BH1750_ERROR_PARAM;
    bh->mode = mode;
    return _send_cmd(bh, (uint8_t)mode);
}

/**
 * @brief อ่านค่า raw (ก่อนแปลงเป็น lux)
 *
 * @param bh  ตัวแปร instance
 * @param raw ตัวแปรรับค่า raw (0-65535)
 *
 * @return BH1750_OK หรือ error code
 *
 * @note One-time mode: ส่ง command ก่อน (H/L res) แล้วรอ 24ms (L) หรือ 180ms (H)
 *       Continuous mode: อ่าน 2 bytes จาก I2C ได้ทันที
 *       ค่า raw = (byte0 << 8) | byte1
 *       ถ้า I2C read ล้มเหลว คืน error
 */
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

/**
 * @brief อ่านค่าความสว่าง
 *
 * @param bh ตัวแปร instance
 *
 * @return ค่า Lux (0.0 = error)
 *
 * @note เรียก BH1750_ReadRaw() แล้วแปลง: lux = raw / 1.2
 *       High-Res2 mode (0.5 lux resolution): ÷ 2 เพิ่มอีก
 *       ถ้า ReadRaw คืน error → return 0.0f
 */
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

/**
 * @brief Power Down (ประหยัดไฟ ~0.01µA)
 *
 * @param bh ตัวแปร instance
 *
 * @return BH1750_OK หรือ BH1750_ERROR_I2C
 *
 * @note ส่ง command 0x00 (Power Down)
 *       Sensor จะไม่ตอบสนองจนกว่าจะ Power Up
 *       ไม่ได้เปลี่ยน bh->mode — Power Up จะใช้ mode เดิม
 */
BH1750_Status BH1750_PowerDown(BH1750_Instance* bh) {
    if (bh == NULL || !bh->initialized) return BH1750_ERROR_PARAM;
    return _send_cmd(bh, BH1750_CMD_POWER_DOWN);
}

/**
 * @brief Power Up + ตั้งโหมดเดิม
 *
 * @param bh ตัวแปร instance
 *
 * @return BH1750_OK หรือ error code
 *
 * @note ส่ง POWER_ON → delay 10ms → ส่ง mode (bh->mode) คืน
 *       ต้องเรียกหลังจาก PowerDown เพื่อกลับมาวัดต่อ
 */
BH1750_Status BH1750_PowerUp(BH1750_Instance* bh) {
    if (bh == NULL || !bh->initialized) return BH1750_ERROR_PARAM;
    if (_send_cmd(bh, BH1750_CMD_POWER_ON) != BH1750_OK) return BH1750_ERROR_I2C;
    Delay_Ms(10);
    return _send_cmd(bh, (uint8_t)bh->mode);
}
