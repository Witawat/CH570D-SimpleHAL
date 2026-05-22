/**
 * @file HC05.c
 * @brief HC-05 Bluetooth Module Implementation
 */

#include "HC05.h"

/* ========== Public ========== */

HC05_Status HC05_Init(HC05_Instance* bt, uint32_t baudrate) {
    if (bt == NULL) return HC05_ERROR_PARAM;

    bt->baudrate    = baudrate;
    bt->rx_head     = 0;
    bt->rx_tail     = 0;
    bt->initialized = 0;

    /* Init USART ผ่าน SimpleUSART */
    USART_SimpleInit((USART_BaudRate)baudrate, USART_PINS_DEFAULT);

    bt->initialized = 1;
    return HC05_OK;
}

void HC05_SendByte(HC05_Instance* bt, uint8_t byte) {
    if (bt == NULL || !bt->initialized) return;
    USART_WriteByte(byte);
}

HC05_Status HC05_Send(HC05_Instance* bt, const uint8_t* data, uint16_t len) {
    if (bt == NULL || !bt->initialized || data == NULL) return HC05_ERROR_PARAM;
    for (uint16_t i = 0; i < len; i++) {
        USART_WriteByte(data[i]);
    }
    return HC05_OK;
}

HC05_Status HC05_SendString(HC05_Instance* bt, const char* str) {
    if (bt == NULL || !bt->initialized || str == NULL) return HC05_ERROR_PARAM;
    while (*str) {
        USART_WriteByte((uint8_t)*str++);
    }
    return HC05_OK;
}

uint8_t HC05_Available(HC05_Instance* bt) {
    if (bt == NULL || !bt->initialized) return 0;
    return USART_Available();
}

HC05_Status HC05_ReadByte(HC05_Instance* bt, uint8_t* byte) {
    if (bt == NULL || !bt->initialized || byte == NULL) return HC05_ERROR_PARAM;
    if (!USART_Available()) return HC05_ERROR_TIMEOUT;
    *byte = USART_Read();
    return HC05_OK;
}

HC05_Status HC05_ReadLine(HC05_Instance* bt, char* buf,
                           uint16_t max_len, uint32_t timeout_ms) {
    if (bt == NULL || !bt->initialized || buf == NULL || max_len == 0)
        return HC05_ERROR_PARAM;

    uint16_t idx   = 0;
    uint32_t start = Get_CurrentMs();

    while (idx < max_len - 1) {
        /* รอ byte */
        while (!USART_Available()) {
            if ((Get_CurrentMs() - start) > timeout_ms) {
                buf[idx] = '\0';
                return (idx > 0) ? HC05_OK : HC05_ERROR_TIMEOUT;
            }
        }

        uint8_t c = USART_Read();
        if (c == '\n') break;
        if (c == '\r') continue;  /* ตัด CR */
        buf[idx++] = (char)c;
    }

    buf[idx] = '\0';
    return HC05_OK;
}

HC05_Status HC05_ATCommand(HC05_Instance* bt, const char* cmd,
                            char* resp, uint16_t resp_len, uint32_t timeout_ms) {
    if (bt == NULL || !bt->initialized || cmd == NULL) return HC05_ERROR_PARAM;

    HC05_Flush(bt);

    /* ส่ง command + "\r\n" */
    HC05_SendString(bt, cmd);
    HC05_SendString(bt, "\r\n");

    if (resp == NULL || resp_len == 0) return HC05_OK;

    return HC05_ReadLine(bt, resp, resp_len, timeout_ms);
}

void HC05_Flush(HC05_Instance* bt) {
    if (bt == NULL || !bt->initialized) return;
    /* อ่านทิ้งจน buffer ว่าง */
    while (USART_Available()) {
        USART_Read();
    }
}
