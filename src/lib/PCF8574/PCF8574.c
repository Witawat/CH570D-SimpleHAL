/**
 * @file PCF8574.c
 * @brief PCF8574 I2C GPIO Expander Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "PCF8574.h"

/* ========== Private: I2C Helpers ========== */

static PCF8574_Status _write_port(PCF8574_Instance* pcf, uint8_t value) {
    uint8_t buf = value;
    if (I2C_Write(pcf->i2c_addr, &buf, 1) != I2C_OK) return PCF8574_ERROR_I2C;
    pcf->port_state = value;
    return PCF8574_OK;
}

static PCF8574_Status _read_port(PCF8574_Instance* pcf, uint8_t* value) {
    if (I2C_Read(pcf->i2c_addr, value, 1) != I2C_OK) return PCF8574_ERROR_I2C;
    return PCF8574_OK;
}

/* ========== Public Functions ========== */

/**
 * @brief Initializes PCF8574 GPIO expander
 *
 * @param pcf - pointer to PCF8574_Instance (NULL check)
 * @param i2c_addr - I2C address (e.g. 0x20–0x27)
 *
 * @return PCF8574_OK on success, error otherwise
 *
 * @note ตรวจ I2C device ready, เขียน 0xFF (ทุก pin = HIGH = input mode)
 *       port_state เริ่มต้น = 0xFF
 */
PCF8574_Status PCF8574_Init(PCF8574_Instance* pcf, uint8_t i2c_addr) {
    if (pcf == NULL) return PCF8574_ERROR_PARAM;

    pcf->i2c_addr   = i2c_addr;
    pcf->port_state = 0xFF;  /* default: ทุก pin = HIGH (input-ready) */
    pcf->initialized = 0;

    /* ตรวจสอบการเชื่อมต่อ */
    if (!I2C_IsDeviceReady(i2c_addr)) return PCF8574_ERROR_I2C;

    /* เขียน 0xFF เพื่อให้ทุก pin อยู่ใน high-impedance (input mode default) */
    if (_write_port(pcf, 0xFF) != PCF8574_OK) return PCF8574_ERROR_I2C;

    pcf->initialized = 1;
    return PCF8574_OK;
}

/**
 * @brief Sets pin direction (input or output)
 *
 * @param pcf - pointer to PCF8574_Instance (ต้อง init แล้ว)
 * @param pin - pin number 0–7
 * @param mode - PCF8574_INPUT or PCF8574_OUTPUT
 *
 * @return PCF8574_OK on success, error otherwise
 *
 * @note PCF8574 เป็น quasi-bidirectional: input = set bit HIGH, output = set bit LOW
 *       ใช้ cached port_state + I2C write
 */
PCF8574_Status PCF8574_PinMode(PCF8574_Instance* pcf, uint8_t pin, PCF8574_Mode mode) {
    if (pcf == NULL || !pcf->initialized) return PCF8574_ERROR_PARAM;
    if (pin > 7) return PCF8574_ERROR_PARAM;

    if (mode == PCF8574_INPUT) {
        /* Input: ตั้ง bit เป็น HIGH (quasi-bidirectional) */
        pcf->port_state |= (1 << pin);
    } else {
        /* Output: ตั้ง bit เป็น LOW เริ่มต้น */
        pcf->port_state &= ~(1 << pin);
    }
    return _write_port(pcf, pcf->port_state);
}

/**
 * @brief Sets a single output pin HIGH or LOW
 *
 * @param pcf - pointer to PCF8574_Instance (ต้อง init แล้ว)
 * @param pin - pin number 0–7
 * @param value - 0 = LOW, non-zero = HIGH
 *
 * @return PCF8574_OK on success, error otherwise
 *
 * @note แก้ port_state ใน cache แล้วเขียนทั้ง port ผ่าน I2C
 */
PCF8574_Status PCF8574_Write(PCF8574_Instance* pcf, uint8_t pin, uint8_t value) {
    if (pcf == NULL || !pcf->initialized) return PCF8574_ERROR_PARAM;
    if (pin > 7) return PCF8574_ERROR_PARAM;

    if (value) {
        pcf->port_state |= (1 << pin);
    } else {
        pcf->port_state &= ~(1 << pin);
    }
    return _write_port(pcf, pcf->port_state);
}

/**
 * @brief Reads a single input pin state
 *
 * @param pcf - pointer to PCF8574_Instance (ต้อง init แล้ว)
 * @param pin - pin number 0–7
 *
 * @return 1 if HIGH, 0 if LOW (หรือ 0 on error/out of range)
 *
 * @note อ่าน I2C port จริง (ไม่ใช้ cache) แล้ว extract bit
 *       pin ที่เป็น output จะอ่านค่าที่เขียนล่าสุด
 */
uint8_t PCF8574_Read(PCF8574_Instance* pcf, uint8_t pin) {
    if (pcf == NULL || !pcf->initialized) return 0;
    if (pin > 7) return 0;

    uint8_t port = 0;
    if (_read_port(pcf, &port) != PCF8574_OK) return 0;
    return (port >> pin) & 0x01;
}

/**
 * @brief Writes all 8 port pins at once
 *
 * @param pcf - pointer to PCF8574_Instance (ต้อง init แล้ว)
 * @param value - byte value (bit = pin level)
 *
 * @return PCF8574_OK on success, error otherwise
 *
 * @note เขียน I2C 1 byte, อัปเดต port_state cache
 */
PCF8574_Status PCF8574_WritePort(PCF8574_Instance* pcf, uint8_t value) {
    if (pcf == NULL || !pcf->initialized) return PCF8574_ERROR_PARAM;
    return _write_port(pcf, value);
}

/**
 * @brief Reads all 8 port pins at once
 *
 * @param pcf - pointer to PCF8574_Instance (ต้อง init แล้ว)
 * @param value - output byte value (bit = pin level)
 *
 * @return PCF8574_OK on success, error otherwise
 *
 * @note อ่าน I2C 1 byte จาก PCF8574 โดยตรง
 */
PCF8574_Status PCF8574_ReadPort(PCF8574_Instance* pcf, uint8_t* value) {
    if (pcf == NULL || !pcf->initialized) return PCF8574_ERROR_PARAM;
    if (value == NULL) return PCF8574_ERROR_PARAM;
    return _read_port(pcf, value);
}

/**
 * @brief Toggles a single output pin
 *
 * @param pcf - pointer to PCF8574_Instance (ต้อง init แล้ว)
 * @param pin - pin number 0–7
 *
 * @return PCF8574_OK on success, error otherwise
 *
 * @note XOR bit ใน port_state แล้วเขียนทั้ง port
 */
PCF8574_Status PCF8574_Toggle(PCF8574_Instance* pcf, uint8_t pin) {
    if (pcf == NULL || !pcf->initialized) return PCF8574_ERROR_PARAM;
    if (pin > 7) return PCF8574_ERROR_PARAM;

    pcf->port_state ^= (1 << pin);
    return _write_port(pcf, pcf->port_state);
}
