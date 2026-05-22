/**
 * @file PZEM004T.c
 * @brief PZEM-004T (v1/v2) AC Power Monitor Implementation
 */

#include "PZEM004T.h"

/* ========== Private ========== */

static uint8_t _checksum(uint8_t* buf, uint8_t len) {
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++) sum += buf[i];
    return (uint8_t)(sum & 0xFF);
}

static void _flush_rx(void) {
    /* อ่างข้อมูลค้างออกจาก RX buffer */
    uint32_t start = Get_CurrentMs();
    while ((Get_CurrentMs() - start) < 10) {
        if (USART_Available()) USART_Read();
    }
}

/**
 * @brief ส่ง command และรับ response
 * @param cmd_byte  command byte (0xB0..0xB5)
 * @param addr      device address 4 bytes
 * @param extra     bytes เพิ่มเติมหลัง address (NULL ถ้าไม่มี)
 * @param extra_len จำนวน extra bytes
 * @param resp_buf  buffer รับ response (7 bytes)
 * @param expected  expected response byte แรก (0xA0..0xA5)
 * @return PZEM004T_Status
 */
static PZEM004T_Status _send_cmd(uint8_t cmd_byte, uint8_t* addr,
                                  uint8_t* extra, uint8_t extra_len,
                                  uint8_t* resp_buf, uint8_t expected) {
    /* สร้าง command frame */
    uint8_t frame[8];
    uint8_t flen = 0;
    frame[flen++] = cmd_byte;
    frame[flen++] = addr[0];
    frame[flen++] = addr[1];
    frame[flen++] = addr[2];
    frame[flen++] = addr[3];
    for (uint8_t i = 0; i < extra_len && flen < 7; i++) {
        frame[flen++] = extra[i];
    }
    frame[flen++] = _checksum(frame, flen);

    _flush_rx();

    /* ส่ง frame */
    for (uint8_t i = 0; i < flen; i++) {
        USART_WriteByte(frame[i]);
    }
    USART_Flush();

    /* รอ response 7 bytes */
    uint32_t start = Get_CurrentMs();
    for (uint8_t i = 0; i < PZEM004T_RESP_LEN; i++) {
        while (!USART_Available()) {
            if ((Get_CurrentMs() - start) >= PZEM004T_TIMEOUT_MS) {
                return PZEM004T_ERROR_TIMEOUT;
            }
        }
        resp_buf[i] = USART_Read();
    }

    /* ตรวจ response byte แรก */
    if (resp_buf[0] != expected) return PZEM004T_ERROR_RESP;

    /* ตรวจ checksum */
    if (resp_buf[PZEM004T_RESP_LEN - 1] != _checksum(resp_buf, PZEM004T_RESP_LEN - 1)) {
        return PZEM004T_ERROR_CHECKSUM;
    }

    return PZEM004T_OK;
}

/* ========== Public ========== */

PZEM004T_Status PZEM004T_Init(PZEM004T* pzem, uint8_t pin_config) {
    if (pzem == NULL) return PZEM004T_ERROR_PARAM;

    uint8_t def[] = PZEM004T_DEFAULT_ADDR;
    pzem->addr[0]    = def[0];
    pzem->addr[1]    = def[1];
    pzem->addr[2]    = def[2];
    pzem->addr[3]    = def[3];
    pzem->pin_config = pin_config;
    pzem->initialized = 0;

    USART_SimpleInit(PZEM004T_BAUD, pin_config);
    pzem->initialized = 1;
    return PZEM004T_OK;
}

void PZEM004T_SetAddress(PZEM004T* pzem, uint8_t addr[4]) {
    if (pzem == NULL) return;
    pzem->addr[0] = addr[0];
    pzem->addr[1] = addr[1];
    pzem->addr[2] = addr[2];
    pzem->addr[3] = addr[3];
}

float PZEM004T_GetVoltage(PZEM004T* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;

    uint8_t resp[PZEM004T_RESP_LEN];
    if (_send_cmd(PZEM004T_CMD_VOLTAGE, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_VOLTAGE) != PZEM004T_OK) {
        return -1.0f;
    }
    /* resp[1]=high byte integer, resp[2]=low byte integer, resp[3]=decimal×0.1 */
    return (float)((resp[1] << 8) | resp[2]) + resp[3] * 0.1f;
}

float PZEM004T_GetCurrent(PZEM004T* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;

    uint8_t resp[PZEM004T_RESP_LEN];
    if (_send_cmd(PZEM004T_CMD_CURRENT, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_CURRENT) != PZEM004T_OK) {
        return -1.0f;
    }
    /* resp[1]=integer (A), resp[2]=decimal (×0.01A), resp[3]=sub-decimal (×0.001A) */
    return (float)resp[1] + resp[2] * 0.01f + resp[3] * 0.001f;
}

float PZEM004T_GetPower(PZEM004T* pzem) {
    if (pzem == NULL || !pzem->initialized) return -1.0f;

    uint8_t resp[PZEM004T_RESP_LEN];
    if (_send_cmd(PZEM004T_CMD_POWER, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_POWER) != PZEM004T_OK) {
        return -1.0f;
    }
    /* resp[1]=high byte integer, resp[2]=low byte integer, resp[3]=decimal×0.1 */
    return (float)((resp[1] << 8) | resp[2]) + resp[3] * 0.1f;
}

uint32_t PZEM004T_GetEnergy(PZEM004T* pzem) {
    if (pzem == NULL || !pzem->initialized) return 0xFFFFFFFF;

    uint8_t resp[PZEM004T_RESP_LEN];
    if (_send_cmd(PZEM004T_CMD_ENERGY, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_ENERGY) != PZEM004T_OK) {
        return 0xFFFFFFFF;
    }
    /* resp[1-3]: 3 bytes = ((resp[1]<<8)|resp[2])*100 + resp[3] Wh */
    return (uint32_t)(((uint32_t)((resp[1] << 8) | resp[2]) * 100UL) + resp[3]);
}

PZEM004T_Status PZEM004T_ReadAll(PZEM004T* pzem, PZEM004T_Data* data) {
    if (pzem == NULL || data == NULL) return PZEM004T_ERROR_PARAM;

    data->voltage = -1.0f;
    data->current = -1.0f;
    data->power   = -1.0f;
    data->energy  = 0xFFFFFFFF;
    data->alarm   = 0;

    uint8_t resp[PZEM004T_RESP_LEN];
    uint8_t any_ok = 0;

    /* Voltage */
    if (_send_cmd(PZEM004T_CMD_VOLTAGE, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_VOLTAGE) == PZEM004T_OK) {
        data->voltage = (float)((resp[1] << 8) | resp[2]) + resp[3] * 0.1f;
        data->alarm   = resp[4] & 0x01;
        any_ok = 1;
    }

    /* Current */
    if (_send_cmd(PZEM004T_CMD_CURRENT, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_CURRENT) == PZEM004T_OK) {
        data->current = (float)resp[1] + resp[2] * 0.01f + resp[3] * 0.001f;
        any_ok = 1;
    }

    /* Power */
    if (_send_cmd(PZEM004T_CMD_POWER, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_POWER) == PZEM004T_OK) {
        data->power = (float)((resp[1] << 8) | resp[2]) + resp[3] * 0.1f;
        any_ok = 1;
    }

    /* Energy */
    if (_send_cmd(PZEM004T_CMD_ENERGY, pzem->addr, NULL, 0,
                  resp, PZEM004T_RESP_ENERGY) == PZEM004T_OK) {
        data->energy = (uint32_t)(((uint32_t)((resp[1] << 8) | resp[2]) * 100UL) + resp[3]);
        any_ok = 1;
    }

    return any_ok ? PZEM004T_OK : PZEM004T_ERROR_TIMEOUT;
}

PZEM004T_Status PZEM004T_SetAlarm(PZEM004T* pzem, uint16_t watts) {
    if (pzem == NULL || !pzem->initialized) return PZEM004T_ERROR_PARAM;

    /* Alarm command: [0xB5][A0][A1][A2][A3][W_H][W_L][CS] → 8 bytes (extra=2) */
    uint8_t extra[2];
    extra[0] = (uint8_t)(watts >> 8);
    extra[1] = (uint8_t)(watts & 0xFF);

    uint8_t resp[PZEM004T_RESP_LEN];
    return _send_cmd(PZEM004T_CMD_ALARM, pzem->addr, extra, 2,
                     resp, PZEM004T_RESP_ALARM);
}
