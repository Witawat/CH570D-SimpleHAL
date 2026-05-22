/**
 * @file ESP01.h
 * @brief ESP-01 / ESP-01S Wi-Fi AT Library for CH57x series
 * @version 1.0
 * @date 2026-04-29
 *
 * @author CH57x Library Team
 */

#ifndef __ESP01_H
#define __ESP01_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

#ifndef ESP01_RX_LINE_MAX
#define ESP01_RX_LINE_MAX          192
#endif

#ifndef ESP01_DEFAULT_TIMEOUT_MS
#define ESP01_DEFAULT_TIMEOUT_MS   3000
#endif

#ifndef ESP01_DEFAULT_RETRY
#define ESP01_DEFAULT_RETRY        2
#endif

/* ========== Types ========== */

typedef enum {
    ESP01_OK = 0,
    ESP01_ERROR_PARAM,
    ESP01_ERROR_TIMEOUT,
    ESP01_ERROR_RESPONSE,
    ESP01_ERROR_NOT_READY,
    ESP01_ERROR_OVERFLOW     /**< buffer ไม่พอ / ข้อมูลถูกตัด */
} ESP01_Status;

typedef enum {
    ESP01_MODE_STA = 1,
    ESP01_MODE_AP = 2,
    ESP01_MODE_STA_AP = 3
} ESP01_WiFiMode;

typedef enum {
    ESP01_SLEEP_NONE = 0,
    ESP01_SLEEP_LIGHT = 1,
    ESP01_SLEEP_MODEM = 2
} ESP01_SleepMode;

typedef enum {
    ESP01_PWR_DEEP_SLEEP = 0,
    ESP01_PWR_SLEEP,
    ESP01_PWR_IDLE_CONNECTED,
    ESP01_PWR_RX_ACTIVE,
    ESP01_PWR_TX_PEAK
} ESP01_PowerState;

typedef struct {
    uint16_t current_typ_ma;
    uint16_t current_peak_ma;
    const char* note;
} ESP01_CurrentProfile;

typedef struct {
    uint32_t baudrate;
    USART_PinConfig pin_config;
    uint8_t retry;
    uint8_t initialized;
} ESP01_Instance;

/* ========== Core ========== */
ESP01_Status ESP01_Init(ESP01_Instance* esp, uint32_t baudrate, USART_PinConfig pin_config);
ESP01_Status ESP01_TestAT(ESP01_Instance* esp);
ESP01_Status ESP01_Reset(ESP01_Instance* esp, uint32_t timeout_ms);
ESP01_Status ESP01_SetEcho(ESP01_Instance* esp, uint8_t enable);
ESP01_Status ESP01_GetVersion(ESP01_Instance* esp, char* out, uint16_t out_len, uint32_t timeout_ms);

/* ========== Wi-Fi ========== */
ESP01_Status ESP01_SetMode(ESP01_Instance* esp, ESP01_WiFiMode mode);
ESP01_Status ESP01_JoinAP(ESP01_Instance* esp, const char* ssid, const char* pass, uint32_t timeout_ms);
ESP01_Status ESP01_IsConnected(ESP01_Instance* esp, uint8_t* connected);
ESP01_Status ESP01_GetIP(ESP01_Instance* esp, char* ip_buf, uint16_t ip_buf_len, uint32_t timeout_ms);

/* ========== TCP / HTTP ========== */
ESP01_Status ESP01_TCPConnect(ESP01_Instance* esp, const char* host, uint16_t port, uint32_t timeout_ms);
ESP01_Status ESP01_TCPSend(ESP01_Instance* esp, const uint8_t* data, uint16_t len, uint32_t timeout_ms);
ESP01_Status ESP01_TCPClose(ESP01_Instance* esp, uint32_t timeout_ms);
ESP01_Status ESP01_HTTPGet(ESP01_Instance* esp,
                           const char* host,
                           const char* path,
                           char* response,
                           uint16_t response_len,
                           uint32_t timeout_ms);
ESP01_Status ESP01_HTTPPost(ESP01_Instance* esp,
                            const char* host,
                            const char* path,
                            const char* content_type,
                            const char* body,
                            char* response,
                            uint16_t response_len,
                            uint32_t timeout_ms);

/**
 * @brief ตรวจสอบว่ามีข้อมูล TCP รอรับอยู่ไหม (จาก +IPD ที่รับมา)
 * @return จำนวน bytes ที่รอรับ (0 = ไม่มี)
 */
uint16_t ESP01_TCPReadAvailable(ESP01_Instance* esp);

/**
 * @brief อ่านข้อมูล TCP ที่รับมาผ่าน +IPD
 * @param buf        buffer รับข้อมูล
 * @param buf_len    ขนาด buffer
 * @param timeout_ms รอรับข้อมูลสูงสุดกี่ ms
 * @return จำนวน bytes ที่อ่านได้จริง
 *
 * @note รูปแบบ ESP response: +IPD,<len>:<data>
 */
uint16_t ESP01_TCPRead(ESP01_Instance* esp,
                       uint8_t* buf,
                       uint16_t buf_len,
                       uint32_t timeout_ms);

/**
 * @brief ตัดการเชื่อมต่อจาก AP (AT+CWQAP)
 */
ESP01_Status ESP01_DisconnectAP(ESP01_Instance* esp);

/**
 * @brief เปลี่ยน baud rate ของ ESP-01 (AT+UART_DEF) และ reinit UART ฝั่ง MCU
 * @param new_baud baud rate ใหม่
 * @note บันทึกลง Flash ของ ESP ด้วย (UART_DEF) ทุก power cycle ใช้ค่าใหม่
 */
ESP01_Status ESP01_SetBaud(ESP01_Instance* esp, uint32_t new_baud);

/* ========== Power ========== */
ESP01_Status ESP01_SetSleepMode(ESP01_Instance* esp, ESP01_SleepMode mode);
ESP01_Status ESP01_SetTxPowerQuarterDbm(ESP01_Instance* esp, uint8_t quarter_dbm);
ESP01_Status ESP01_EnterDeepSleepUs(ESP01_Instance* esp, uint32_t sleep_us);
ESP01_Status ESP01_GetCurrentProfile(ESP01_PowerState state, ESP01_CurrentProfile* profile);

#ifdef __cplusplus
}
#endif

#endif /* __ESP01_H */
