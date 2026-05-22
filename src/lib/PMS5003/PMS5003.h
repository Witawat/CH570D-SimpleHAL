/**
 * @file PMS5003.h
 * @brief PMS5003 / PMS7003 PM2.5 Sensor Library for CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @author CH57x Library Team
 */

#ifndef __PMS5003_H
#define __PMS5003_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

#ifndef PMS5003_FRAME_LEN
#define PMS5003_FRAME_LEN          32
#endif

#ifndef PMS5003_DEFAULT_TIMEOUT_MS
#define PMS5003_DEFAULT_TIMEOUT_MS 2000
#endif

/* ========== Types ========== */

typedef enum {
    PMS5003_OK = 0,
    PMS5003_ERROR_PARAM,
    PMS5003_ERROR_TIMEOUT,
    PMS5003_ERROR_CHECKSUM,
    PMS5003_ERROR_NOT_INIT
} PMS5003_Status;

typedef struct {
    uint16_t frame_len;

    uint16_t pm1_0_std;
    uint16_t pm2_5_std;
    uint16_t pm10_std;

    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;
    uint16_t pm10_atm;

    uint16_t cnt_0_3;
    uint16_t cnt_0_5;
    uint16_t cnt_1_0;
    uint16_t cnt_2_5;
    uint16_t cnt_5_0;
    uint16_t cnt_10;

    uint16_t reserved;
    uint16_t checksum;
} PMS5003_Data;

typedef struct {
    USART_PinConfig pin_config;

    uint8_t rx_buf[PMS5003_FRAME_LEN];
    uint8_t rx_idx;
    uint8_t frame_ready;

    PMS5003_Data last_data;

    uint8_t initialized;
} PMS5003_Instance;

/* ========== Core ========== */
PMS5003_Status PMS5003_Init(PMS5003_Instance* sensor, USART_PinConfig pin_config);
PMS5003_Status PMS5003_Update(PMS5003_Instance* sensor);
PMS5003_Status PMS5003_Read(PMS5003_Instance* sensor, PMS5003_Data* out, uint32_t timeout_ms);

/* ========== Data Access ========== */
uint8_t PMS5003_IsDataReady(PMS5003_Instance* sensor);
void PMS5003_ClearReady(PMS5003_Instance* sensor);
PMS5003_Status PMS5003_GetData(PMS5003_Instance* sensor, PMS5003_Data* out);

/* ========== Sensor Command ========== */
PMS5003_Status PMS5003_Sleep(PMS5003_Instance* sensor);
PMS5003_Status PMS5003_Wakeup(PMS5003_Instance* sensor);
PMS5003_Status PMS5003_SetPassiveMode(PMS5003_Instance* sensor);
PMS5003_Status PMS5003_SetActiveMode(PMS5003_Instance* sensor);
PMS5003_Status PMS5003_Request(PMS5003_Instance* sensor);

#ifdef __cplusplus
}
#endif

#endif