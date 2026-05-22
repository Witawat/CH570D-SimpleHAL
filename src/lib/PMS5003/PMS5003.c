/**
 * @file PMS5003.c
 * @brief PMS5003 / PMS7003 PM2.5 Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "PMS5003.h"

/* ========== Protocol Constants ========== */

#define PMS5003_HEAD_1              0x42
#define PMS5003_HEAD_2              0x4D
#define PMS5003_FRAME_DATA_LEN      28
#define PMS5003_CMD_LEN             7

/* ========== Private Helpers ========== */

static uint16_t PMS5003_ReadBE16(const uint8_t* buf, uint8_t idx)
{
    return (uint16_t)(((uint16_t)buf[idx] << 8) | buf[idx + 1]);
}

static uint8_t PMS5003_ValidateChecksum(const uint8_t* frame)
{
    uint16_t sum = 0;
    uint8_t i;

    for(i = 0; i < 30; i++) {
        sum = (uint16_t)(sum + frame[i]);
    }

    return (sum == PMS5003_ReadBE16(frame, 30)) ? 1 : 0;
}

static void PMS5003_ParseFrame(PMS5003_Data* out, const uint8_t* frame)
{
    out->frame_len  = PMS5003_ReadBE16(frame, 2);

    out->pm1_0_std  = PMS5003_ReadBE16(frame, 4);
    out->pm2_5_std  = PMS5003_ReadBE16(frame, 6);
    out->pm10_std   = PMS5003_ReadBE16(frame, 8);

    out->pm1_0_atm  = PMS5003_ReadBE16(frame, 10);
    out->pm2_5_atm  = PMS5003_ReadBE16(frame, 12);
    out->pm10_atm   = PMS5003_ReadBE16(frame, 14);

    out->cnt_0_3    = PMS5003_ReadBE16(frame, 16);
    out->cnt_0_5    = PMS5003_ReadBE16(frame, 18);
    out->cnt_1_0    = PMS5003_ReadBE16(frame, 20);
    out->cnt_2_5    = PMS5003_ReadBE16(frame, 22);
    out->cnt_5_0    = PMS5003_ReadBE16(frame, 24);
    out->cnt_10     = PMS5003_ReadBE16(frame, 26);

    out->reserved   = PMS5003_ReadBE16(frame, 28);
    out->checksum   = PMS5003_ReadBE16(frame, 30);
}

static PMS5003_Status PMS5003_SendCommand(PMS5003_Instance* sensor, uint8_t cmd, uint8_t data_h, uint8_t data_l)
{
    uint8_t frame[PMS5003_CMD_LEN];
    uint16_t sum;

    if(sensor == 0 || sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    frame[0] = PMS5003_HEAD_1;
    frame[1] = PMS5003_HEAD_2;
    frame[2] = cmd;
    frame[3] = data_h;
    frame[4] = data_l;

    sum = (uint16_t)(frame[0] + frame[1] + frame[2] + frame[3] + frame[4]);
    frame[5] = (uint8_t)(sum >> 8);
    frame[6] = (uint8_t)(sum & 0xFF);

    USART_WriteByte(frame[0]);
    USART_WriteByte(frame[1]);
    USART_WriteByte(frame[2]);
    USART_WriteByte(frame[3]);
    USART_WriteByte(frame[4]);
    USART_WriteByte(frame[5]);
    USART_WriteByte(frame[6]);

    return PMS5003_OK;
}

/* ========== Public Functions ========== */

PMS5003_Status PMS5003_Init(PMS5003_Instance* sensor, USART_PinConfig pin_config)
{
    if(sensor == 0) {
        return PMS5003_ERROR_PARAM;
    }

    sensor->pin_config = pin_config;
    sensor->rx_idx = 0;
    sensor->frame_ready = 0;
    sensor->initialized = 0;

    USART_SimpleInit(BAUD_9600, pin_config);
    USART_Flush();

    sensor->initialized = 1;
    return PMS5003_OK;
}

PMS5003_Status PMS5003_Update(PMS5003_Instance* sensor)
{
    uint8_t b;

    if(sensor == 0) {
        return PMS5003_ERROR_PARAM;
    }

    if(sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    while(USART_Available()) {
        b = USART_Read();

        if(sensor->rx_idx == 0) {
            if(b == PMS5003_HEAD_1) {
                sensor->rx_buf[0] = b;
                sensor->rx_idx = 1;
            }
            continue;
        }

        if(sensor->rx_idx == 1) {
            if(b == PMS5003_HEAD_2) {
                sensor->rx_buf[1] = b;
                sensor->rx_idx = 2;
            } else if(b == PMS5003_HEAD_1) {
                sensor->rx_buf[0] = b;
                sensor->rx_idx = 1;
            } else {
                sensor->rx_idx = 0;
            }
            continue;
        }

        sensor->rx_buf[sensor->rx_idx++] = b;

        if(sensor->rx_idx >= PMS5003_FRAME_LEN) {
            sensor->rx_idx = 0;

            if(PMS5003_ReadBE16(sensor->rx_buf, 2) != PMS5003_FRAME_DATA_LEN) {
                continue;
            }

            if(!PMS5003_ValidateChecksum(sensor->rx_buf)) {
                continue;
            }

            PMS5003_ParseFrame(&sensor->last_data, sensor->rx_buf);
            sensor->frame_ready = 1;
            return PMS5003_OK;
        }
    }

    return PMS5003_OK;
}

PMS5003_Status PMS5003_Read(PMS5003_Instance* sensor, PMS5003_Data* out, uint32_t timeout_ms)
{
    uint32_t t0;
    PMS5003_Status st;

    if(sensor == 0 || out == 0) {
        return PMS5003_ERROR_PARAM;
    }

    if(sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    if(timeout_ms == 0) {
        timeout_ms = PMS5003_DEFAULT_TIMEOUT_MS;
    }

    sensor->frame_ready = 0;
    sensor->rx_idx = 0;
    USART_Flush();

    t0 = Get_CurrentMs();
    while((Get_CurrentMs() - t0) < timeout_ms) {
        st = PMS5003_Update(sensor);
        if(st != PMS5003_OK) {
            return st;
        }

        if(sensor->frame_ready) {
            *out = sensor->last_data;
            sensor->frame_ready = 0;
            return PMS5003_OK;
        }
    }

    return PMS5003_ERROR_TIMEOUT;
}

uint8_t PMS5003_IsDataReady(PMS5003_Instance* sensor)
{
    if(sensor == 0 || sensor->initialized == 0) {
        return 0;
    }

    return sensor->frame_ready;
}

void PMS5003_ClearReady(PMS5003_Instance* sensor)
{
    if(sensor == 0 || sensor->initialized == 0) {
        return;
    }

    sensor->frame_ready = 0;
}

PMS5003_Status PMS5003_GetData(PMS5003_Instance* sensor, PMS5003_Data* out)
{
    if(sensor == 0 || out == 0) {
        return PMS5003_ERROR_PARAM;
    }

    if(sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    if(sensor->frame_ready == 0) {
        return PMS5003_ERROR_TIMEOUT;
    }

    *out = sensor->last_data;
    return PMS5003_OK;
}

PMS5003_Status PMS5003_Sleep(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE4, 0x00, 0x00);
}

PMS5003_Status PMS5003_Wakeup(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE4, 0x00, 0x01);
}

PMS5003_Status PMS5003_SetPassiveMode(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE1, 0x00, 0x00);
}

PMS5003_Status PMS5003_SetActiveMode(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE1, 0x00, 0x01);
}

PMS5003_Status PMS5003_Request(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE2, 0x00, 0x00);
}
