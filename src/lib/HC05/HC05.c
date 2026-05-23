/**
 * @file HC05.c
 * @brief HC-05 Bluetooth Module Implementation
 */

#include "HC05.h"

/* ========== Public ========== */

/**
 * @brief เริ่มต้น HC-05 (USART)
 *
 * @param bt - instance ที่ผ่านการ init แล้ว (NULL → return HC05_ERROR_PARAM)
 * @param baudrate - baud rate (9600 default data mode, 38400 AT mode)
 *
 * @return HC05_OK หรือ HC05_ERROR_PARAM
 *
 * @note ใช้ USART1 polling — blocking จนกว่า USART พร้อม
 *       ตั้งค่า bt->rx_head/bt->rx_tail = 0, initialized = 1
 *       ต้องเรียก SystemCoreClockUpdate() และ Timer_Init() ก่อน
 */
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

/**
 * @brief ส่ง byte เดียว
 *
 * @param bt - instance (NULL หรือ !initialized → return ทันที)
 * @param byte - ข้อมูล 1 ไบต์ที่ต้องการส่ง
 *
 * @note ใช้ USART_WriteByte polling — blocking จนกว่าส่งเสร็จ
 */
void HC05_SendByte(HC05_Instance* bt, uint8_t byte) {
    if (bt == NULL || !bt->initialized) return;
    USART_WriteByte(byte);
}

/**
 * @brief ส่ง buffer
 *
 * @param bt - instance (NULL หรือ !initialized → HC05_ERROR_PARAM)
 * @param data - pointer ข้อมูล (NULL → HC05_ERROR_PARAM)
 * @param len - จำนวนไบต์
 *
 * @return HC05_OK หรือ HC05_ERROR_PARAM
 *
 * @note ใช้ USART_WriteByte polling — blocking จนกว่าส่งครบ
 *       9600 baud → ~1ms/byte
 */
HC05_Status HC05_Send(HC05_Instance* bt, const uint8_t* data, uint16_t len) {
    if (bt == NULL || !bt->initialized || data == NULL) return HC05_ERROR_PARAM;
    for (uint16_t i = 0; i < len; i++) {
        USART_WriteByte(data[i]);
    }
    return HC05_OK;
}

/**
 * @brief ส่ง string (null-terminated)
 *
 * @param bt - instance (NULL หรือ !initialized → HC05_ERROR_PARAM)
 * @param str - string C (NULL → HC05_ERROR_PARAM)
 *
 * @return HC05_OK หรือ HC05_ERROR_PARAM
 *
 * @note ส่งทีละ char จนถึง null terminator
 *       ไม่ได้เพิ่ม "\r\n" อัตโนมัติ
 */
HC05_Status HC05_SendString(HC05_Instance* bt, const char* str) {
    if (bt == NULL || !bt->initialized || str == NULL) return HC05_ERROR_PARAM;
    while (*str) {
        USART_WriteByte((uint8_t)*str++);
    }
    return HC05_OK;
}

/**
 * @brief ตรวจสอบว่ามีข้อมูลรอรับหรือไม่
 *
 * @param bt - instance (NULL หรือ !initialized → return 0)
 *
 * @return จำนวนไบต์ใน RX buffer (0 = ไม่มีข้อมูล)
 *
 * @note เรียก USART_Available() จาก SimpleUSART
 *       ใช้ polling ใน main loop
 */
uint8_t HC05_Available(HC05_Instance* bt) {
    if (bt == NULL || !bt->initialized) return 0;
    return USART_Available();
}

/**
 * @brief อ่าน 1 ไบต์จาก RX buffer
 *
 * @param bt - instance (NULL หรือ !initialized → HC05_ERROR_PARAM)
 * @param byte - output pointer รับค่า (NULL → HC05_ERROR_PARAM)
 *
 * @return HC05_OK, HC05_ERROR_PARAM, หรือ HC05_ERROR_TIMEOUT
 *
 * @note ไม่ได้รอ — ถ้าไม่มีข้อมูลคืน HC05_ERROR_TIMEOUT ทันที
 *       ควรเรียก HC05_Available() ก่อน
 */
HC05_Status HC05_ReadByte(HC05_Instance* bt, uint8_t* byte) {
    if (bt == NULL || !bt->initialized || byte == NULL) return HC05_ERROR_PARAM;
    if (!USART_Available()) return HC05_ERROR_TIMEOUT;
    *byte = USART_Read();
    return HC05_OK;
}

/**
 * @brief อ่านจนถึง newline ('\n') หรือ timeout
 *
 * @param bt - instance (NULL หรือ !initialized → HC05_ERROR_PARAM)
 * @param buf - output buffer (NULL หรือ max_len==0 → HC05_ERROR_PARAM)
 * @param max_len - ขนาด buffer สูงสุด
 * @param timeout_ms - timeout รอแต่ละ byte (ms)
 *
 * @return HC05_OK (ได้ข้อมูล), HC05_ERROR_TIMEOUT, หรือ HC05_ERROR_PARAM
 *
 * @note null-terminate ให้อัตโนมัติ
 *       ตัด '\r' ออก (skip)
 *       หยุดเมื่อเจอ '\n' (ไม่รวมใน output)
 *       ถ้า idx>0 และ timeout คืน HC05_OK พร้อมข้อมูลที่มี
 */
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

/**
 * @brief ส่ง AT command และรอรับ response (สำหรับ AT mode)
 *
 * @param bt - instance (NULL หรือ !initialized → HC05_ERROR_PARAM)
 * @param cmd - AT command เช่น "AT", "AT+NAME=..." (NULL → HC05_ERROR_PARAM)
 * @param resp - output buffer (NULL หรือ resp_len==0 → ข้ามรับ response)
 * @param resp_len - ขนาด buffer response
 * @param timeout_ms - timeout รอ response (ms)
 *
 * @return HC05_OK หรือ error code
 *
 * @note flush RX buffer ก่อนส่งเสมอ
 *       ผนวก "\r\n" ต่อท้าย command อัตโนมัติ
 *       ต้องเข้า AT mode ก่อน (EN pin = HIGH ตอน power-on)
 */
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

/**
 * @brief Flush RX buffer (อ่านทิ้งจนว่าง)
 *
 * @param bt - instance (NULL หรือ !initialized → return ทันที)
 *
 * @note ใช้ polling อ่าน USART จน buffer ว่าง
 *       blocking จนกว่าจะอ่านหมด
 */
void HC05_Flush(HC05_Instance* bt) {
    if (bt == NULL || !bt->initialized) return;
    /* อ่านทิ้งจน buffer ว่าง */
    while (USART_Available()) {
        USART_Read();
    }
}
