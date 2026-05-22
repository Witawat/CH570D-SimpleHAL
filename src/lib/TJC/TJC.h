/**
 * @file TJC.h
 * @author MAKER WITAWAT (https://www.makerwitawat.com)
 * @brief TJC HMI Display Library for CH57x
 * @version 1.0
 * @date 2025-12-19
 *
 * @details
 * Library สำหรับควบคุม TJC HMI Display ผ่าน UART
 * - รองรับรูปแบบคำสั่ง CMD|PARA1|PARA2|... (ปิดท้ายด้วย ; หรือไม่ก็ได้)
 * - ตรวจจับและแปลความหมายข้อผิดพลาดจาก TJC
 * - รองรับ callback functions สำหรับ event handling
 * - รองรับ UART pin remapping
 */

#ifndef __TJC_H__
#define __TJC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"

/* ========== Configuration ========== */

#define TJC_RX_BUFFER_SIZE 256    // ขนาด receive buffer (รองรับข้อความ 10-60 ตัวอักษร)
#define TJC_MAX_STRING_LENGTH 128  // ความยาวสูงสุดของ string ที่รับได้
#define TJC_MAX_PARAMS 10          // จำนวน parameters สูงสุดที่รองรับ
#define TJC_TERMINATOR_SIZE 3      // ขนาดของ terminator (0xFF 0xFF 0xFF)

/** @brief ขนาด packet สูงสุดที่ยอมรับได้ก่อน reset buffer (string + header + terminator) */
#define TJC_PACKET_MAX_SIZE  (TJC_MAX_STRING_LENGTH + 4)

/* ========== Enumerations ========== */

/**
 * @brief UART Pin Configuration (ตาม CH57x Datasheet)
 */
typedef enum {
  TJC_PINS_DEFAULT = 0, // Default: TX=PD5, RX=PD6
  TJC_PINS_REMAP1 = 1,  // Remap1:  TX=PD0, RX=PD1
  TJC_PINS_REMAP2 = 2   // Remap2:  TX=PD6, RX=PD5
} TJC_PinConfig;

/**
 * @brief TJC Error Codes (ตามเอกสาร TJC - bkcmd非0时的通知格式)
 */
typedef enum {
  TJC_ERR_INVALID_CMD = 0x00,         // คำสั่งไม่ถูกต้อง
  TJC_ERR_SUCCESS = 0x01,             // สำเร็จ
  TJC_ERR_INVALID_COMPONENT = 0x02,   // Component ID ไม่ถูกต้อง
  TJC_ERR_INVALID_PAGE = 0x03,        // Page ID ไม่ถูกต้อง
  TJC_ERR_INVALID_PICTURE = 0x04,     // Picture ID ไม่ถูกต้อง
  TJC_ERR_INVALID_FONT = 0x05,        // Font ID ไม่ถูกต้อง
  TJC_ERR_FILE_OPERATION = 0x06,      // การทำงานกับไฟล์ล้มเหลว
  TJC_ERR_CRC_FAILED = 0x09,          // CRC ไม่ตรงกัน
  TJC_ERR_INVALID_BAUDRATE = 0x11,    // Baud rate ไม่ถูกต้อง
  TJC_ERR_INVALID_CURVE = 0x12,       // Curve control ID/channel ไม่ถูกต้อง
  TJC_ERR_INVALID_VARIABLE = 0x1A,    // ชื่อตัวแปรไม่ถูกต้อง
  TJC_ERR_INVALID_OPERATION = 0x1B,   // การคำนวณไม่ถูกต้อง
  TJC_ERR_ASSIGNMENT_FAILED = 0x1C,   // การกำหนดค่าล้มเหลว
  TJC_ERR_EEPROM_FAILED = 0x1D,       // การทำงานกับ EEPROM ล้มเหลว
  TJC_ERR_INVALID_PARAM_COUNT = 0x1E, // จำนวน parameter ไม่ถูกต้อง
  TJC_ERR_IO_FAILED = 0x1F,           // IO operation ล้มเหลว
  TJC_ERR_ESCAPE_CHAR = 0x20,         // ใช้ escape character ผิด
  TJC_ERR_VARIABLE_NAME_LONG = 0x23   // ชื่อตัวแปรยาวเกินไป
} TJC_ErrorCode;

/**
 * @brief TJC Return Data Types (ตามเอกสาร TJC - 其他数据返回格式)
 */
typedef enum {
  TJC_RET_BUFFER_OVERFLOW = 0x24,  // Serial buffer เต็ม
  TJC_RET_TOUCH_EVENT = 0x65,      // มีการสัมผัสหน้าจอ
  TJC_RET_PAGE_ID = 0x66,          // ส่งหมายเลขหน้าปัจจุบัน
  TJC_RET_TOUCH_COORD = 0x67,      // พิกัดการสัมผัส
  TJC_RET_SLEEP_TOUCH = 0x68,      // สัมผัสหน้าจอในโหมดพักหน้าจอ
  TJC_RET_STRING_DATA = 0x70,      // ข้อมูล string variable
  TJC_RET_NUMERIC_DATA = 0x71,     // ข้อมูล numeric variable
  TJC_RET_AUTO_SLEEP = 0x86,       // เข้าโหมดพักหน้าจออัตโนมัติ
  TJC_RET_AUTO_WAKE = 0x87,        // ปลุกหน้าจออัตโนมัติ
  TJC_RET_STARTUP = 0x88,          // ระบบเริ่มทำงาน
  TJC_RET_SD_UPGRADE = 0x89,       // เริ่ม SD card upgrade
  TJC_RET_TRANSPARENT_DONE = 0xFD, // ส่งข้อมูลแบบ transparent เสร็จ
  TJC_RET_TRANSPARENT_READY = 0xFE // พร้อมรับข้อมูลแบบ transparent
} TJC_ReturnType;

/* ========== Data Structures ========== */

/**
 * @brief โครงสร้างสำหรับเก็บข้อมูล Touch Event (0x65)
 */
typedef struct {
  uint8_t page_id;      // หมายเลขหน้า
  uint8_t component_id; // หมายเลข component
  uint8_t event_type;   // 0x01=Press, 0x00=Release
} TJC_TouchEvent_t;

/**
 * @brief โครงสร้างสำหรับเก็บพิกัดการสัมผัส (0x67)
 */
typedef struct {
  uint16_t x;         // พิกัด X
  uint16_t y;         // พิกัด Y
  uint8_t event_type; // 0x01=Press, 0x00=Release
} TJC_TouchCoord_t;

/**
 * @brief โครงสร้างสำหรับเก็บข้อมูลตัวเลข (0x71)
 */
typedef struct {
  uint32_t value; // ค่าตัวเลข
} TJC_NumericData_t;

/**
 * @brief Circular Buffer สำหรับรับข้อมูล
 */
typedef struct {
  uint8_t buffer[TJC_RX_BUFFER_SIZE];
  volatile uint16_t head;
  volatile uint16_t tail;
} TJC_RxBuffer_t;

/**
 * @brief โครงสร้างสำหรับเก็บคำสั่งที่รับมา (CMD|PARA1|PARA2|...)
 */
typedef struct {
  char command[32];                // คำสั่งหลัก (เช่น "page", "t0.txt")
  char params[TJC_MAX_PARAMS][32]; // Parameters แต่ละตัว
  uint8_t param_count;             // จำนวน parameters
} TJC_ReceivedCommand_t;

/* ========== Callback Function Types ========== */

typedef void (*TJC_ErrorCallback_t)(uint8_t error_code);
typedef void (*TJC_TouchEventCallback_t)(TJC_TouchEvent_t *event);
typedef void (*TJC_TouchCoordCallback_t)(TJC_TouchCoord_t *coord);
typedef void (*TJC_NumericCallback_t)(uint32_t value);
typedef void (*TJC_StringCallback_t)(const char *str, uint16_t len);
typedef void (*TJC_SystemEventCallback_t)(uint8_t event_type);
typedef void (*TJC_CommandCallback_t)(TJC_ReceivedCommand_t *cmd);

/* ========== Public API Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน TJC HMI
 * @param baudrate ความเร็วการสื่อสาร (เช่น 9600, 115200)
 * @param pin_config การตั้งค่า pin (TJC_PINS_DEFAULT, TJC_PINS_REMAP1,
 * TJC_PINS_REMAP2)
 */
void TJC_Init(uint32_t baudrate, TJC_PinConfig pin_config);

/**
 * @brief ส่งคำสั่งแบบธรรมดาไปยัง TJC
 * @param cmd คำสั่ง (เช่น "page 0", "t0.txt=\"Hello\"")
 * @note ฟังก์ชันจะเพิ่ม terminator 0xFF 0xFF 0xFF ให้อัตโนมัติ
 */
void TJC_SendCommand(const char *cmd);

/**
 * @brief ส่งคำสั่งแบบมี parameters (รูปแบบ CMD|PARA1|PARA2|...)
 * @param cmd คำสั่งหลัก
 * @param params array ของ parameters
 * @param param_count จำนวน parameters
 * @param use_semicolon ใช้ ; ปิดท้ายหรือไม่ (1=ใช้, 0=ไม่ใช้)
 * @note ตัวอย่าง: TJC_SendCommandParams("t0.txt", {"Hello", "World"}, 2, 1)
 *       จะส่งเป็น "t0.txt|Hello|World;" + 0xFF 0xFF 0xFF
 */
void TJC_SendCommandParams(const char *cmd, const char **params,
                           uint8_t param_count, uint8_t use_semicolon);

/**
 * @brief ประมวลผลข้อมูลที่รับจาก TJC
 * @note ควรเรียกฟังก์ชันนี้ใน main loop หรือ timer interrupt
 */
void TJC_ProcessResponse(void);

/**
 * @brief ลงทะเบียน callback สำหรับ error events
 */
void TJC_RegisterErrorCallback(TJC_ErrorCallback_t callback);

/**
 * @brief ลงทะเบียน callback สำหรับ touch events
 */
void TJC_RegisterTouchEventCallback(TJC_TouchEventCallback_t callback);

/**
 * @brief ลงทะเบียน callback สำหรับ touch coordinate events
 */
void TJC_RegisterTouchCoordCallback(TJC_TouchCoordCallback_t callback);

/**
 * @brief ลงทะเบียน callback สำหรับ numeric data
 */
void TJC_RegisterNumericCallback(TJC_NumericCallback_t callback);

/**
 * @brief ลงทะเบียน callback สำหรับ string data
 */
void TJC_RegisterStringCallback(TJC_StringCallback_t callback);

/**
 * @brief ลงทะเบียน callback สำหรับ system events (startup, sleep, wake)
 */
void TJC_RegisterSystemEventCallback(TJC_SystemEventCallback_t callback);

/**
 * @brief ลงทะเบียน callback สำหรับรับคำสั่งจาก TJC
 * @note คำสั่งที่รับมาจะอยู่ในรูปแบบ CMD หรือ CMD|PARA1|PARA2|... (มี ; ปิดท้ายหรือไม่ก็ได้)
 */
void TJC_RegisterCommandCallback(TJC_CommandCallback_t callback);

/**
 * @brief แปลง error code เป็นข้อความภาษาไทย
 * @param error_code รหัส error
 * @return ข้อความอธิบาย error
 */
const char *TJC_GetErrorString(uint8_t error_code);

/**
 * @brief ตรวจสอบว่ามีข้อมูลใน receive buffer หรือไม่
 * @return จำนวน bytes ที่มีอยู่
 */
uint16_t TJC_Available(void);

/**
 * @brief ล้างข้อมูลใน receive buffer
 */
void TJC_FlushRxBuffer(void);

/**
 * @brief รีเซ็ต response parser state
 * @details ใช้เมื่อ packet ค้างหรือต้องการ re-sync เช่น หลัง timeout หรือ bus error
 * @note ฟังก์ชันนี้ไม่ล้าง RX buffer แค่ reset ตัว parser
 *
 * @example
 * // ถ้าไม่ได้รับ response ใน 1 วินาที ให้ reset
 * TJC_ResetResponse();
 */
void TJC_ResetResponse(void);

/**
 * @brief เปิดใช้งาน UART interrupt สำหรับรับข้อมูล
 * @note ฟังก์ชันนี้จะถูกเรียกอัตโนมัติใน TJC_Init()
 */
void TJC_EnableRxInterrupt(void);

/**
 * @brief UART Interrupt Handler (ควรเรียกใน USART1_IRQHandler)
 * @note ต้องเพิ่ม TJC_UART_IRQHandler() ใน USART1_IRQHandler ของคุณ
 */
void TJC_UART_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __TJC_H__ */
