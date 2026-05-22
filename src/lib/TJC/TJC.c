/**
 * @file TJC.c
 * @author MAKER WITAWAT (https://www.makerwitawat.com)
 * @brief TJC HMI Display Library Implementation
 * @version 1.0
 * @date 2025-12-19
 */

#include "TJC.h"
#include <stdio.h>
#include <string.h>

/* ========== Private Variables ========== */

static TJC_RxBuffer_t rx_buffer = {0};

/* Response parser state (file-scope เพื่อให้ TJC_ResetResponse() เข้าถึงได้) */
static uint8_t  response_buffer[TJC_PACKET_MAX_SIZE];
static uint16_t response_len = 0;

// Callback function pointers
static TJC_ErrorCallback_t error_callback = NULL;
static TJC_TouchEventCallback_t touch_event_callback = NULL;
static TJC_TouchCoordCallback_t touch_coord_callback = NULL;
static TJC_NumericCallback_t numeric_callback = NULL;
static TJC_StringCallback_t string_callback = NULL;
static TJC_SystemEventCallback_t system_event_callback = NULL;
static TJC_CommandCallback_t command_callback = NULL;

// Error message strings (ภาษาไทย)
static const char *error_strings[] = {
    [TJC_ERR_INVALID_CMD] = "คำสั่งไม่ถูกต้อง",
    [TJC_ERR_SUCCESS] = "สำเร็จ",
    [TJC_ERR_INVALID_COMPONENT] = "Component ID ไม่ถูกต้อง",
    [TJC_ERR_INVALID_PAGE] = "Page ID ไม่ถูกต้อง",
    [TJC_ERR_INVALID_PICTURE] = "Picture ID ไม่ถูกต้อง",
    [TJC_ERR_INVALID_FONT] = "Font ID ไม่ถูกต้อง",
    [TJC_ERR_FILE_OPERATION] = "การทำงานกับไฟล์ล้มเหลว",
    [0x07] = "Unknown Error",
    [0x08] = "Unknown Error",
    [TJC_ERR_CRC_FAILED] = "CRC ไม่ตรงกัน",
    [0x0A ... 0x10] = "Unknown Error",
    [TJC_ERR_INVALID_BAUDRATE] = "Baud rate ไม่ถูกต้อง",
    [TJC_ERR_INVALID_CURVE] = "Curve control ID/channel ไม่ถูกต้อง",
    [0x13 ... 0x19] = "Unknown Error",
    [TJC_ERR_INVALID_VARIABLE] = "ชื่อตัวแปรไม่ถูกต้อง",
    [TJC_ERR_INVALID_OPERATION] = "การคำนวณไม่ถูกต้อง",
    [TJC_ERR_ASSIGNMENT_FAILED] = "การกำหนดค่าล้มเหลว",
    [TJC_ERR_EEPROM_FAILED] = "การทำงานกับ EEPROM ล้มเหลว",
    [TJC_ERR_INVALID_PARAM_COUNT] = "จำนวน parameter ไม่ถูกต้อง",
    [TJC_ERR_IO_FAILED] = "IO operation ล้มเหลว",
    [TJC_ERR_ESCAPE_CHAR] = "ใช้ escape character ผิด",
    [0x21] = "Unknown Error",
    [0x22] = "Unknown Error",
    [TJC_ERR_VARIABLE_NAME_LONG] = "ชื่อตัวแปรยาวเกินไป"};

/* ========== Private Helper Functions ========== */

/**
 * @brief ส่ง 1 byte ผ่าน UART
 */
static void TJC_SendByte(uint8_t data) {
  USART_WriteByte(data);
}

/**
 * @brief ส่ง string ผ่าน UART
 */
static void TJC_SendString(const char *str) {
  while (*str) {
    TJC_SendByte(*str++);
  }
}

/**
 * @brief ส่ง terminator (0xFF 0xFF 0xFF)
 */
static void TJC_SendTerminator(void) {
  TJC_SendByte(0xFF);
  TJC_SendByte(0xFF);
  TJC_SendByte(0xFF);
}

/**
 * @brief เพิ่มข้อมูลเข้า circular buffer
 */
static void TJC_BufferPut(uint8_t data) {
  uint16_t next_head = (rx_buffer.head + 1) % TJC_RX_BUFFER_SIZE;

  // ถ้า buffer เต็ม ให้เลื่อน tail ไปด้วย (overwrite old data)
  if (next_head == rx_buffer.tail) {
    rx_buffer.tail = (rx_buffer.tail + 1) % TJC_RX_BUFFER_SIZE;
  }

  rx_buffer.buffer[rx_buffer.head] = data;
  rx_buffer.head = next_head;
}

/**
 * @brief อ่านข้อมูลจาก circular buffer
 */
static uint8_t TJC_BufferGet(void) {
  if (rx_buffer.head == rx_buffer.tail) {
    return 0; // Buffer empty
  }

  uint8_t data = rx_buffer.buffer[rx_buffer.tail];
  rx_buffer.tail = (rx_buffer.tail + 1) % TJC_RX_BUFFER_SIZE;
  return data;
}

/**
 * @brief ตรวจสอบว่ามี terminator (0xFF 0xFF 0xFF) หรือไม่
 */
static uint8_t TJC_CheckTerminator(uint8_t *buffer, uint16_t len) {
  if (len < 3)
    return 0;

  return (buffer[len - 3] == 0xFF && buffer[len - 2] == 0xFF &&
          buffer[len - 1] == 0xFF);
}

/**
 * @brief ประมวลผล error response
 */
static void TJC_ProcessError(uint8_t error_code) {
  if (error_callback != NULL) {
    error_callback(error_code);
  }
}

/**
 * @brief ประมวลผล touch event (0x65)
 */
static void TJC_ProcessTouchEvent(uint8_t *data) {
  if (touch_event_callback != NULL) {
    TJC_TouchEvent_t event;
    event.page_id = data[1];
    event.component_id = data[2];
    event.event_type = data[3];
    touch_event_callback(&event);
  }
}

/**
 * @brief ประมวลผล touch coordinate (0x67)
 */
static void TJC_ProcessTouchCoord(uint8_t *data) {
  if (touch_coord_callback != NULL) {
    TJC_TouchCoord_t coord;
    coord.x = (data[1] << 8) | data[2];
    coord.y = (data[3] << 8) | data[4];
    coord.event_type = data[5];
    touch_coord_callback(&coord);
  }
}

/**
 * @brief ประมวลผล numeric data (0x71)
 */
static void TJC_ProcessNumericData(uint8_t *data) {
  if (numeric_callback != NULL) {
    /* TJC ส่ง numeric data แบบ little-endian: [v0=LSB][v1][v2][v3=MSB] */
    uint32_t value = (uint32_t)data[1]
                   | ((uint32_t)data[2] << 8)
                   | ((uint32_t)data[3] << 16)
                   | ((uint32_t)data[4] << 24);
    numeric_callback(value);
  }
}

/**
 * @brief ประมวลผล string data (0x70)
 */
static void TJC_ProcessStringData(uint8_t *data, uint16_t len) {
  if (string_callback != NULL && len > 4) {
    // String data อยู่ระหว่าง byte 1 ถึง len-4 (ไม่รวม header และ terminator)
    uint16_t str_len = len - 4;
    if (str_len > TJC_MAX_STRING_LENGTH) {
      str_len = TJC_MAX_STRING_LENGTH;
    }

    static char str_buffer[TJC_MAX_STRING_LENGTH + 1];
    memcpy(str_buffer, &data[1], str_len);
    str_buffer[str_len] = '\0';

    string_callback(str_buffer, str_len);
  }
}

/**
 * @brief ประมวลผล system events
 */
static void TJC_ProcessSystemEvent(uint8_t event_type) {
  if (system_event_callback != NULL) {
    system_event_callback(event_type);
  }
}

/**
 * @brief ประมวลผลคำสั่งที่รับมา (รูปแบบ CMD|PARA1|PARA2|...)
 */
static void TJC_ProcessCommand(uint8_t *data, uint16_t len) {
  if (command_callback == NULL || len < 4) {
    return; // ไม่มี callback หรือข้อมูลสั้นเกินไป
  }

  // แปลงข้อมูลเป็น string (ไม่รวม terminator 3 bytes)
  uint16_t str_len = len - 3;
  if (str_len > TJC_MAX_STRING_LENGTH) {
    str_len = TJC_MAX_STRING_LENGTH;
  }

  static char cmd_buffer[TJC_MAX_STRING_LENGTH + 1];
  memcpy(cmd_buffer, data, str_len);
  cmd_buffer[str_len] = '\0';

  // ลบ semicolon ปิดท้าย (ถ้ามี)
  if (str_len > 0 && cmd_buffer[str_len - 1] == ';') {
    cmd_buffer[str_len - 1] = '\0';
    str_len--;
  }

  // สร้าง structure สำหรับเก็บคำสั่งที่แยกแล้ว
  TJC_ReceivedCommand_t received_cmd = {0};

  // แยกคำสั่งและ parameters
  char *token = cmd_buffer;
  char *next_pipe = NULL;
  uint8_t param_idx = 0;

  // หาคำสั่งหลัก (ก่อน | แรก)
  next_pipe = strchr(token, '|');
  if (next_pipe != NULL) {
    // มี parameters
    *next_pipe = '\0'; // ตัดที่ |
    strncpy(received_cmd.command, token, sizeof(received_cmd.command) - 1);

    // แยก parameters
    token = next_pipe + 1;
    while (token != NULL && param_idx < TJC_MAX_PARAMS) {
      next_pipe = strchr(token, '|');
      if (next_pipe != NULL) {
        *next_pipe = '\0';
      }

      strncpy(received_cmd.params[param_idx], token, 31);
      received_cmd.params[param_idx][31] = '\0';
      param_idx++;

      if (next_pipe != NULL) {
        token = next_pipe + 1;
      } else {
        break;
      }
    }
    received_cmd.param_count = param_idx;
  } else {
    // ไม่มี parameters
    strncpy(received_cmd.command, token, sizeof(received_cmd.command) - 1);
    received_cmd.param_count = 0;
  }

  // เรียก callback
  command_callback(&received_cmd);
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้นการใช้งาน TJC HMI
 */
void TJC_Init(uint32_t baudrate, TJC_PinConfig pin_config) {
  (void)pin_config;
  USART_SimpleInit(baudrate, 0);
  rx_buffer.head = 0;
  rx_buffer.tail = 0;
}

/**
 * @brief ส่งคำสั่งแบบธรรมดา
 */
void TJC_SendCommand(const char *cmd) {
  TJC_SendString(cmd);
  TJC_SendTerminator();
}

/**
 * @brief ส่งคำสั่งแบบมี parameters
 */
void TJC_SendCommandParams(const char *cmd, const char **params,
                           uint8_t param_count, uint8_t use_semicolon) {
  // ส่งคำสั่งหลัก
  TJC_SendString(cmd);

  // ส่ง parameters
  for (uint8_t i = 0; i < param_count; i++) {
    TJC_SendByte('|');
    TJC_SendString(params[i]);
  }

  // ส่ง semicolon ถ้าต้องการ
  if (use_semicolon) {
    TJC_SendByte(';');
  }

  // ส่ง terminator
  TJC_SendTerminator();
}

/**
 * @brief คืนค่าความยาว packet ที่คาดหวัง สำหรับ packet type ที่รู้จัก
 * @return จำนวน bytes รวม terminator, หรือ -1 ถ้าความยาวไม่แน่นอน (เช่น string)
 */
static int16_t _GetExpectedPacketLen(uint8_t cmd_type) {
  switch (cmd_type) {
    /* Error / status codes — [code][0xFF][0xFF][0xFF] = 4 bytes */
    case TJC_ERR_INVALID_CMD:
    case TJC_ERR_SUCCESS:
    case TJC_ERR_INVALID_COMPONENT:
    case TJC_ERR_INVALID_PAGE:
    case TJC_ERR_INVALID_PICTURE:
    case TJC_ERR_INVALID_FONT:
    case TJC_ERR_FILE_OPERATION:
    case TJC_ERR_CRC_FAILED:
    case TJC_ERR_INVALID_BAUDRATE:
    case TJC_ERR_INVALID_CURVE:
    case TJC_ERR_INVALID_VARIABLE:
    case TJC_ERR_INVALID_OPERATION:
    case TJC_ERR_ASSIGNMENT_FAILED:
    case TJC_ERR_EEPROM_FAILED:
    case TJC_ERR_INVALID_PARAM_COUNT:
    case TJC_ERR_IO_FAILED:
    case TJC_ERR_ESCAPE_CHAR:
    case TJC_ERR_VARIABLE_NAME_LONG:
    case TJC_RET_BUFFER_OVERFLOW:
      return 4;

    case TJC_RET_TOUCH_EVENT: return 7; /* [0x65][page][comp][ev][0xFF×3] */
    case TJC_RET_PAGE_ID:     return 5; /* [0x66][page][0xFF×3] */
    case TJC_RET_TOUCH_COORD: return 9; /* [0x67][xh][xl][yh][yl][ev][0xFF×3] */
    case TJC_RET_SLEEP_TOUCH: return 9; /* [0x68][xh][xl][yh][yl][ev][0xFF×3] */
    case TJC_RET_NUMERIC_DATA: return 8; /* [0x71][v0][v1][v2][v3][0xFF×3] */

    /* System events — [code][0xFF×3] = 4 bytes */
    case TJC_RET_AUTO_SLEEP:
    case TJC_RET_AUTO_WAKE:
    case TJC_RET_STARTUP:
    case TJC_RET_SD_UPGRADE:
    case TJC_RET_TRANSPARENT_DONE:
    case TJC_RET_TRANSPARENT_READY:
      return 4;

    /* ความยาวไม่แน่นอน */
    case TJC_RET_STRING_DATA:
    default:
      return -1;
  }
}

/**
 * @brief ประมวลผลข้อมูลที่รับจาก TJC
 */
void TJC_ProcessResponse(void) {
  while (TJC_Available() > 0) {
    uint8_t data = TJC_BufferGet();

    /* ป้องกัน buffer overflow — ถ้าเกิน TJC_PACKET_MAX_SIZE
       แสดงว่า packet เสีย/ขาดตกหาย ให้ reset แล้วเริ่มใหม่ */
    if (response_len >= TJC_PACKET_MAX_SIZE) {
      response_len = 0;
    }

    response_buffer[response_len++] = data;

    /* ยังไม่พบ terminator — รอข้อมูลเพิ่ม */
    if (!TJC_CheckTerminator(response_buffer, response_len)) {
      continue;
    }

    /* ได้รับ packet ครบ — ตรวจสอบความยาวก่อนประมวลผล */
    uint8_t cmd_type = response_buffer[0];
    int16_t expected = _GetExpectedPacketLen(cmd_type);

    if (expected > 0 && response_len != (uint16_t)expected) {
      /* ความยาวไม่ตรง — packet เสียหรือ noise ทิ้งไป */
      response_len = 0;
      continue;
    }

    /* ประมวลผลตามประเภท */
    switch (cmd_type) {
    /* Error codes */
    case TJC_ERR_INVALID_CMD:
    case TJC_ERR_SUCCESS:
    case TJC_ERR_INVALID_COMPONENT:
    case TJC_ERR_INVALID_PAGE:
    case TJC_ERR_INVALID_PICTURE:
    case TJC_ERR_INVALID_FONT:
    case TJC_ERR_FILE_OPERATION:
    case TJC_ERR_CRC_FAILED:
    case TJC_ERR_INVALID_BAUDRATE:
    case TJC_ERR_INVALID_CURVE:
    case TJC_ERR_INVALID_VARIABLE:
    case TJC_ERR_INVALID_OPERATION:
    case TJC_ERR_ASSIGNMENT_FAILED:
    case TJC_ERR_EEPROM_FAILED:
    case TJC_ERR_INVALID_PARAM_COUNT:
    case TJC_ERR_IO_FAILED:
    case TJC_ERR_ESCAPE_CHAR:
    case TJC_ERR_VARIABLE_NAME_LONG:
    case TJC_RET_BUFFER_OVERFLOW:
      TJC_ProcessError(cmd_type);
      break;

    case TJC_RET_TOUCH_EVENT:
      TJC_ProcessTouchEvent(response_buffer);
      break;

    case TJC_RET_PAGE_ID:
      /* Page ID = response_buffer[1] */
      break;

    case TJC_RET_TOUCH_COORD:
    case TJC_RET_SLEEP_TOUCH:
      TJC_ProcessTouchCoord(response_buffer);
      break;

    case TJC_RET_AUTO_SLEEP:
    case TJC_RET_AUTO_WAKE:
    case TJC_RET_STARTUP:
    case TJC_RET_SD_UPGRADE:
    case TJC_RET_TRANSPARENT_DONE:
    case TJC_RET_TRANSPARENT_READY:
      TJC_ProcessSystemEvent(cmd_type);
      break;

    case TJC_RET_STRING_DATA:
      TJC_ProcessStringData(response_buffer, response_len);
      break;

    case TJC_RET_NUMERIC_DATA:
      TJC_ProcessNumericData(response_buffer);
      break;

    default:
      /* ตรวจสอบว่าเป็น ASCII text command หรือไม่ */
      if (response_buffer[0] >= 0x20 && response_buffer[0] <= 0x7E) {
        TJC_ProcessCommand(response_buffer, response_len);
      }
      break;
    }

    /* รีเซ็ต parser พร้อมรับ packet ถัดไป */
    response_len = 0;
  }
}

/**
 * @brief Callback registration functions
 */
void TJC_RegisterErrorCallback(TJC_ErrorCallback_t callback) {
  error_callback = callback;
}

void TJC_RegisterTouchEventCallback(TJC_TouchEventCallback_t callback) {
  touch_event_callback = callback;
}

void TJC_RegisterTouchCoordCallback(TJC_TouchCoordCallback_t callback) {
  touch_coord_callback = callback;
}

void TJC_RegisterNumericCallback(TJC_NumericCallback_t callback) {
  numeric_callback = callback;
}

void TJC_RegisterStringCallback(TJC_StringCallback_t callback) {
  string_callback = callback;
}

void TJC_RegisterSystemEventCallback(TJC_SystemEventCallback_t callback) {
  system_event_callback = callback;
}

void TJC_RegisterCommandCallback(TJC_CommandCallback_t callback) {
  command_callback = callback;
}

/**
 * @brief แปลง error code เป็นข้อความ
 */
const char *TJC_GetErrorString(uint8_t error_code) {
  if (error_code < sizeof(error_strings) / sizeof(error_strings[0])) {
    return error_strings[error_code];
  }
  return "Unknown Error";
}

/**
 * @brief ตรวจสอบจำนวนข้อมูลใน buffer
 */
uint16_t TJC_Available(void) {
  if (rx_buffer.head >= rx_buffer.tail) {
    return rx_buffer.head - rx_buffer.tail;
  } else {
    return TJC_RX_BUFFER_SIZE - rx_buffer.tail + rx_buffer.head;
  }
}

/**
 * @brief ล้าง receive buffer
 */
void TJC_FlushRxBuffer(void) {
  rx_buffer.head = 0;
  rx_buffer.tail = 0;
}

/**
 * @brief รีเซ็ต response parser state
 */
void TJC_ResetResponse(void) {
  response_len = 0;
}

/**
 * @brief Open RX (no-op, uses polling mode)
 */
void TJC_EnableRxInterrupt(void) {
}

/**
 * @brief UART receive handler (polling mode via SimpleHAL)
 */
void TJC_UART_IRQHandler(void) {
  while (USART_Available() > 0) {
    TJC_BufferPut(USART_Read());
  }
}
