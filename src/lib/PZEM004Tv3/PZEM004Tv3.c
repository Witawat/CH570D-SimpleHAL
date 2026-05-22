/**
 * @file PZEM004Tv3.c
 * @brief PZEM-004T v3 AC Power Monitor Implementation (Modbus RTU)
 */

#include "PZEM004Tv3.h"

/* ========== Private: Modbus CRC-16 ========== */

static uint16_t _modbus_crc(uint8_t* buf, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void _flush_rx(void) {
    uint32_t start = Get_CurrentMs();
    while ((Get_CurrentMs() - start) < 10) {
        if (USART_Available()) USART_Read();
    }
}

/**
 * @brief รับ N bytes ด้วย timeout
 */
static PZEM004Tv3_Status _recv_bytes(uint8_t* buf, uint8_t len) {
    uint32_t start = Get_CurrentMs();
    for (uint8_t i = 0; i < len; i++) {
        while (!USART_Available()) {
            if ((Get_CurrentMs() - start) >= PZEM004TV3_TIMEOUT_MS) {
                return PZEM004Tv3_ERROR_TIMEOUT;
            }
        }
        buf[i] = USART_Read();
    }
    return PZEM004Tv3_OK;
}

/**
 * @brief ส่ง Modbus Read Input Registers (FC=0x04)
 * @param addr      slave address
 * @param reg_start register เริ่มต้น
 * @param reg_count จำนวน register
 * @param resp_buf  buffer รับ response (ขนาดอย่างน้อย 5 + reg_count*2 bytes)
 * @param resp_len  [out] จำนวน bytes ที่รับได้
 * @return PZEM004Tv3_Status
 */
static PZEM004Tv3_Status _modbus_read(uint8_t addr, uint16_t reg_start,
                                       uint16_t reg_count, uint8_t* resp_buf,
                                       uint8_t* resp_len) {
    /* สร้าง request frame: [ADDR][0x04][REG_H][REG_L][CNT_H][CNT_L][CRC_L][CRC_H] */
    uint8_t req[8];
    req[0] = addr;
    req[1] = PZEM004TV3_FC_READ;
    req[2] = (uint8_t)(reg_start >> 8);
    req[3] = (uint8_t)(reg_start & 0xFF);
    req[4] = (uint8_t)(reg_count >> 8);
    req[5] = (uint8_t)(reg_count & 0xFF);
    uint16_t crc = _modbus_crc(req, 6);
    req[6] = (uint8_t)(crc & 0xFF);
    req[7] = (uint8_t)(crc >> 8);

    _flush_rx();

    for (uint8_t i = 0; i < 8; i++) USART_WriteByte(req[i]);
    USART_Flush();

    /* Response: [ADDR][FC][BYTE_COUNT][DATA...][CRC_L][CRC_H] */
    /* รับ header 3 bytes ก่อน */
    uint8_t header[3];
    PZEM004Tv3_Status st = _recv_bytes(header, 3);
    if (st != PZEM004Tv3_OK) return st;

    /* ตรวจ address และ function code */
    if (header[0] != addr) return PZEM004Tv3_ERROR_RESP;
    if (header[1] == (PZEM004TV3_FC_READ | 0x80)) return PZEM004Tv3_ERROR_EXCEPT;
    if (header[1] != PZEM004TV3_FC_READ) return PZEM004Tv3_ERROR_RESP;

    uint8_t byte_count = header[2];
    if (byte_count != reg_count * 2) return PZEM004Tv3_ERROR_RESP;

    /* รับ data + CRC (byte_count + 2 bytes) */
    uint8_t total = byte_count + 2;
    if (total > 64) return PZEM004Tv3_ERROR_RESP;  /* sanity check */

    st = _recv_bytes(resp_buf, total);
    if (st != PZEM004Tv3_OK) return st;

    /* ตรวจ CRC ทั้ง frame */
    uint8_t full[3 + 64];
    full[0] = header[0];
    full[1] = header[1];
    full[2] = header[2];
    for (uint8_t i = 0; i < byte_count; i++) full[3 + i] = resp_buf[i];

    uint16_t calc_crc = _modbus_crc(full, 3 + byte_count);
    uint16_t recv_crc = (uint16_t)resp_buf[byte_count] | ((uint16_t)resp_buf[byte_count + 1] << 8);
    if (calc_crc != recv_crc) return PZEM004Tv3_ERROR_CRC;

    *resp_len = byte_count;
    return PZEM004Tv3_OK;
}

/**
 * @brief ส่ง Modbus Write Single Register (FC=0x06)
 */
static PZEM004Tv3_Status _modbus_write(uint8_t addr, uint16_t reg, uint16_t value) {
    uint8_t req[8];
    req[0] = addr;
    req[1] = PZEM004TV3_FC_WRITE;
    req[2] = (uint8_t)(reg >> 8);
    req[3] = (uint8_t)(reg & 0xFF);
    req[4] = (uint8_t)(value >> 8);
    req[5] = (uint8_t)(value & 0xFF);
    uint16_t crc = _modbus_crc(req, 6);
    req[6] = (uint8_t)(crc & 0xFF);
    req[7] = (uint8_t)(crc >> 8);

    _flush_rx();
    for (uint8_t i = 0; i < 8; i++) USART_WriteByte(req[i]);
    USART_Flush();

    /* Response: 8 bytes echo */
    uint8_t resp[8];
    PZEM004Tv3_Status st = _recv_bytes(resp, 8);
    if (st != PZEM004Tv3_OK) return st;

    if (resp[0] != addr || resp[1] != PZEM004TV3_FC_WRITE) return PZEM004Tv3_ERROR_RESP;
    uint16_t resp_crc = (uint16_t)resp[6] | ((uint16_t)resp[7] << 8);
    if (_modbus_crc(resp, 6) != resp_crc) return PZEM004Tv3_ERROR_CRC;

    return PZEM004Tv3_OK;
}

/* ========== Public ========== */

PZEM004Tv3_Status PZEM004Tv3_Init(PZEM004Tv3* pzem, uint8_t modbus_addr,
                                    uint8_t pin_config) {
    if (pzem == NULL) return PZEM004Tv3_ERROR_PARAM;
    if (modbus_addr == 0x00 || modbus_addr > 0xF7) return PZEM004Tv3_ERROR_PARAM;

    pzem->modbus_addr = modbus_addr;
    pzem->pin_config  = pin_config;
    pzem->initialized = 0;

    USART_SimpleInit(PZEM004TV3_BAUD, pin_config);
    pzem->initialized = 1;
    return PZEM004Tv3_OK;
}

PZEM004Tv3_Status PZEM004Tv3_ReadAll(PZEM004Tv3* pzem, PZEM004Tv3_Data* data) {
    if (pzem == NULL || data == NULL) return PZEM004Tv3_ERROR_PARAM;
    if (!pzem->initialized) return PZEM004Tv3_ERROR_PARAM;

    /* อ่าน 10 registers ตั้งแต่ 0x0000 ในครั้งเดียว */
    uint8_t raw[PZEM004TV3_REG_COUNT * 2 + 2];  /* data + CRC */
    uint8_t rlen = 0;

    PZEM004Tv3_Status st = _modbus_read(pzem->modbus_addr,
                                         PZEM004TV3_REG_VOLTAGE,
                                         PZEM004TV3_REG_COUNT,
                                         raw, &rlen);
    if (st != PZEM004Tv3_OK) return st;
    if (rlen < PZEM004TV3_REG_COUNT * 2) return PZEM004Tv3_ERROR_RESP;

    /* แปลง raw bytes → ค่าวัด */
    /* Reg 0: Voltage × 0.1 V */
    data->voltage = (float)((uint16_t)(raw[0] << 8) | raw[1]) * 0.1f;

    /* Reg 1-2: Current 32-bit × 0.001 A */
    uint32_t curr_raw = ((uint32_t)(raw[2] << 8) | raw[3]) |
                        ((uint32_t)((raw[4] << 8) | raw[5]) << 16);
    data->current = (float)curr_raw * 0.001f;

    /* Reg 3-4: Power 32-bit × 0.1 W */
    uint32_t pwr_raw = ((uint32_t)(raw[6] << 8) | raw[7]) |
                       ((uint32_t)((raw[8] << 8) | raw[9]) << 16);
    data->power = (float)pwr_raw * 0.1f;

    /* Reg 5-6: Energy 32-bit × 1 Wh */
    data->energy = ((uint32_t)(raw[10] << 8) | raw[11]) |
                   ((uint32_t)((raw[12] << 8) | raw[13]) << 16);

    /* Reg 7: Frequency × 0.1 Hz */
    data->frequency = (float)((uint16_t)(raw[14] << 8) | raw[15]) * 0.1f;

    /* Reg 8: Power Factor × 0.01 */
    data->power_factor = (float)((uint16_t)(raw[16] << 8) | raw[17]) * 0.01f;

    /* Reg 9: Alarm status */
    data->alarm = (((uint16_t)(raw[18] << 8) | raw[19]) != 0) ? 1 : 0;

    return PZEM004Tv3_OK;
}

float PZEM004Tv3_GetVoltage(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;
    uint8_t raw[4]; uint8_t rlen = 0;
    if (_modbus_read(pzem->modbus_addr, PZEM004TV3_REG_VOLTAGE, 1, raw, &rlen) != PZEM004Tv3_OK)
        return -1.0f;
    return (float)((uint16_t)(raw[0] << 8) | raw[1]) * 0.1f;
}

float PZEM004Tv3_GetCurrent(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;
    uint8_t raw[6]; uint8_t rlen = 0;
    if (_modbus_read(pzem->modbus_addr, PZEM004TV3_REG_CURRENT, 2, raw, &rlen) != PZEM004Tv3_OK)
        return -1.0f;
    uint32_t v = ((uint32_t)(raw[0] << 8) | raw[1]) |
                 ((uint32_t)((raw[2] << 8) | raw[3]) << 16);
    return (float)v * 0.001f;
}

float PZEM004Tv3_GetPower(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;
    uint8_t raw[6]; uint8_t rlen = 0;
    if (_modbus_read(pzem->modbus_addr, PZEM004TV3_REG_POWER, 2, raw, &rlen) != PZEM004Tv3_OK)
        return -1.0f;
    uint32_t v = ((uint32_t)(raw[0] << 8) | raw[1]) |
                 ((uint32_t)((raw[2] << 8) | raw[3]) << 16);
    return (float)v * 0.1f;
}

uint32_t PZEM004Tv3_GetEnergy(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return 0xFFFFFFFF;
    uint8_t raw[6]; uint8_t rlen = 0;
    if (_modbus_read(pzem->modbus_addr, PZEM004TV3_REG_ENERGY, 2, raw, &rlen) != PZEM004Tv3_OK)
        return 0xFFFFFFFF;
    return ((uint32_t)(raw[0] << 8) | raw[1]) |
           ((uint32_t)((raw[2] << 8) | raw[3]) << 16);
}

float PZEM004Tv3_GetFrequency(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;
    uint8_t raw[4]; uint8_t rlen = 0;
    if (_modbus_read(pzem->modbus_addr, PZEM004TV3_REG_FREQ, 1, raw, &rlen) != PZEM004Tv3_OK)
        return -1.0f;
    return (float)((uint16_t)(raw[0] << 8) | raw[1]) * 0.1f;
}

float PZEM004Tv3_GetPowerFactor(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;
    uint8_t raw[4]; uint8_t rlen = 0;
    if (_modbus_read(pzem->modbus_addr, PZEM004TV3_REG_PF, 1, raw, &rlen) != PZEM004Tv3_OK)
        return -1.0f;
    return (float)((uint16_t)(raw[0] << 8) | raw[1]) * 0.01f;
}

PZEM004Tv3_Status PZEM004Tv3_ResetEnergy(PZEM004Tv3* pzem) {
    if (pzem == NULL || !pzem->initialized) return PZEM004Tv3_ERROR_PARAM;

    /* Reset energy: [ADDR][0x42][CRC_L][CRC_H] */
    uint8_t req[4];
    req[0] = pzem->modbus_addr;
    req[1] = PZEM004TV3_FC_RESET;
    uint16_t crc = _modbus_crc(req, 2);
    req[2] = (uint8_t)(crc & 0xFF);
    req[3] = (uint8_t)(crc >> 8);

    _flush_rx();
    for (uint8_t i = 0; i < 4; i++) USART_WriteByte(req[i]);
    USART_Flush();

    /* Response: [ADDR][0x42][CRC_L][CRC_H] echo */
    uint8_t resp[4];
    PZEM004Tv3_Status st = _recv_bytes(resp, 4);
    if (st != PZEM004Tv3_OK) return st;
    if (resp[0] != pzem->modbus_addr || resp[1] != PZEM004TV3_FC_RESET)
        return PZEM004Tv3_ERROR_RESP;
    uint16_t resp_crc = (uint16_t)resp[2] | ((uint16_t)resp[3] << 8);
    if (_modbus_crc(resp, 2) != resp_crc) return PZEM004Tv3_ERROR_CRC;

    return PZEM004Tv3_OK;
}

PZEM004Tv3_Status PZEM004Tv3_SetAddress(PZEM004Tv3* pzem, uint8_t new_addr) {
    if (pzem == NULL || !pzem->initialized) return PZEM004Tv3_ERROR_PARAM;
    if (new_addr == 0x00 || new_addr > 0xF7) return PZEM004Tv3_ERROR_PARAM;

    PZEM004Tv3_Status st = _modbus_write(pzem->modbus_addr, PZEM004TV3_HREG_ADDR,
                                          (uint16_t)new_addr);
    if (st == PZEM004Tv3_OK) {
        pzem->modbus_addr = new_addr;
    }
    return st;
}
