/**
 * @file MCP4725.c
 * @brief MCP4725 12-bit DAC Implementation
 */

#include "MCP4725.h"

/**
 * @brief เริ่มต้น MCP4725
 *
 * @param dac  ตัวแปร instance
 * @param addr MCP4725_ADDR_A0_GND หรือ MCP4725_ADDR_A0_VCC
 *
 * @return MCP4725_OK หรือ error code
 *
 * @note ทดสอบ I2C connection โดย Fast Write ค่า 0
 *       ไม่ได้อ่าน EEPROM กลับ — instance.value เริ่มต้นที่ 0
 *       ต้องเรียก I2C_SimpleInit() ก่อน
 */
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

/**
 * @brief ตั้งค่า DAC แบบ raw (0-4095)
 *
 * @param dac   ตัวแปร instance
 * @param value ค่า 12-bit (0-4095)
 *
 * @return MCP4725_OK หรือ error code
 *
 * @note Fast Write mode: 2 bytes
 *       Byte 0: [PD1 PD0 D11 D10 D9 D8] (2-bit PD=00, upper 4 data bits)
 *       Byte 1: [D7 D6 D5 D4 D3 D2 D1 D0] (lower 8 data bits)
 *       ไม่บันทึกลง EEPROM — ค่าหายเมื่อไฟดับ
 *       ถ้า value > 4095 จะ clamp เป็น 4095
 */
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

/**
 * @brief ตั้งค่าแรงดัน output
 *
 * @param dac     ตัวแปร instance
 * @param voltage แรงดันที่ต้องการ (V)
 * @param vref    แรงดัน reference = VDD ของ module (V) เช่น 3.3
 *
 * @return MCP4725_OK หรือ error code
 *
 * @note แปลง voltage → raw: raw = voltage / vref * 4095 + 0.5 (round)
 *       clamp voltage ให้อยู่ใน [0, vref]
 *       ใช้ MCP4725_SetRaw() — ไม่บันทึก EEPROM
 *       vref ต้อง > 0 ถ้า <= 0 คืน error
 */
MCP4725_Status MCP4725_SetVoltage(MCP4725_Instance* dac, float voltage, float vref) {
    if (dac == NULL || !dac->initialized) return MCP4725_ERROR_PARAM;
    if (vref <= 0.0f) return MCP4725_ERROR_PARAM;
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > vref) voltage = vref;

    uint16_t raw = (uint16_t)(voltage / vref * 4095.0f + 0.5f);
    return MCP4725_SetRaw(dac, raw);
}

/**
 * @brief ตั้งค่า DAC และบันทึกลง EEPROM
 *
 * @param dac   ตัวแปร instance
 * @param value ค่า 12-bit (0-4095)
 *
 * @return MCP4725_OK หรือ error code
 *
 * @note Write DAC Register and EEPROM: 3 bytes
 *       Byte 0: 0x60 (command: write DAC+EEPROM, PD=00)
 *       Byte 1: D11-D4 (upper 8 bits)
 *       Byte 2: D3-D0 << 4 (lower 4 bits ใน upper nibble)
 *       delay 25ms รอ EEPROM write cycle
 *       EEPROM เขียนได้ประมาณ 1 ล้านครั้ง
 */
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

/**
 * @brief อ่านค่าปัจจุบันจาก DAC
 *
 * @param dac   ตัวแปร instance
 * @param value ตัวแปรรับค่า (0-4095)
 *
 * @return MCP4725_OK หรือ error code
 *
 * @note อ่าน 3 bytes จาก I2C: [status] [DAC_MSB] [DAC_LSB]
 *       Byte 1: D11-D4, Byte 2: D3-D0 (upper nibble)
 *       value = (buf[1] << 4) | (buf[2] >> 4)
 *       byte 0 = status register (por, pd)
 */
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

/**
 * @brief ตั้ง Power-Down mode
 *
 * @param dac ตัวแปร instance
 * @param pd  MCP4725_PD_NORMAL / PD_1K / PD_100K / PD_500K
 *
 * @return MCP4725_OK หรือ error code
 *
 * @note Fast Write with PD bits (PD1-PD0 ใน upper nibble ของ byte 0)
 *       รักษาค่า DAC output ไว้เท่าเดิม — แค่เปลี่ยน load resistor ต่อลง GND
 *       PD_NORMAL: output buffer ปกติ
 *       PD_1K/100K/500K: VOUT ต่อ resistor ไป GND เพื่อ discharge load
 */
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
