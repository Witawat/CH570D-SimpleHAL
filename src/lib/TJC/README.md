# TJC — TJC HMI Display Library

Library สำหรับควบคุม **TJC HMI Display** ผ่าน UART บน CH57x  
รองรับการส่งคำสั่ง, รับ event ต่าง ๆ ผ่าน callback และ pin remapping ทุกรูปแบบ

---

## สารบัญ

- [ไฟล์และ Dependencies](#ไฟล์และ-dependencies)
- [การติดตั้งและตั้งค่า Project](#การติดตั้งและตั้งค่า-project)
- [Pin Configuration](#pin-configuration)
- [การส่งคำสั่งไปยัง TJC](#การส่งคำสั่งไปยัง-tjc)
- [การรับข้อมูลจาก TJC (Callbacks)](#การรับข้อมูลจาก-tjc-callbacks)
- [การส่งคำสั่ง Custom จาก TJC มายัง MCU](#การส่งคำสั่ง-custom-จาก-tjc-มายัง-mcu)
- [API Reference ฉบับเต็ม](#api-reference-ฉบับเต็ม)
- [Error Codes](#error-codes)
- [Configuration](#configuration)
- [ตัวอย่างโปรแกรมเต็ม](#ตัวอย่างโปรแกรมเต็ม)
- [หมายเหตุ](#หมายเหตุ)

---

## ไฟล์และ Dependencies

| ไฟล์ | คำอธิบาย |
|------|-----------|
| `TJC.h` | Header file — types, enums, function prototypes |
| `TJC.c` | Implementation |

**Dependencies:**

| Library | หมายเหตุ |
|---------|----------|
| `Peripheral/inc/ch32v00x.h` | CH57x HAL |
| `<string.h>` | Standard C |

> ไม่ขึ้นกับ SimpleHAL — ใช้ Hardware Register โดยตรง

---

## การติดตั้งและตั้งค่า Project

**1. เพิ่มไฟล์เข้า build** (`obj/sources.mk` หรือ makefile):

```
User/Lib/TJC/TJC.c
```

**2. เพิ่ม include path:**

```
User/Lib/TJC
```

**3. เพิ่ม IRQ Handler ใน `User/ch32v00x_it.c`:**

```c
#include "TJC.h"

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) {
    TJC_UART_IRQHandler();
}
```

> ⚠️ ถ้ามี `USART1_IRQHandler` อยู่แล้ว ให้เพิ่มแค่ `TJC_UART_IRQHandler();` ข้างใน

**4. ตั้งค่า TJC HMI ให้ส่ง feedback:**

```
// ใน TJC Editor — Global Initialization Event
bkcmd=3   // ส่ง response ทุกคำสั่ง (แนะนำสำหรับ debug)
bkcmd=1   // ส่งเฉพาะเมื่อสำเร็จ (production)
bkcmd=0   // ปิด response (ใช้เมื่อไม่ต้องการ feedback)
```

---

## Pin Configuration

| Enum | TX | RX | หมายเหตุ |
|------|----|----|----------|
| `TJC_PINS_DEFAULT` | PD5 | PD6 | Default |
| `TJC_PINS_REMAP1` | PD0 | PD1 | Partial Remap 1 |
| `TJC_PINS_REMAP2` | PD6 | PD5 | Partial Remap 2 (สลับ TX/RX) |

---

## การส่งคำสั่งไปยัง TJC

### `TJC_SendCommand(cmd)` — ส่งคำสั่งตรง

```c
TJC_Init(115200, TJC_PINS_DEFAULT);

/* ควบคุม Page */
TJC_SendCommand("page 0");          // เปิด page 0
TJC_SendCommand("page main");       // เปิด page ชื่อ main

/* ควบคุม Text */
TJC_SendCommand("t0.txt=\"Hello\"");
TJC_SendCommand("t0.txt=\"Temp: 25C\"");

/* ควบคุม Number */
TJC_SendCommand("n0.val=100");
TJC_SendCommand("n0.val=0");

/* ควบคุม Progress Bar */
TJC_SendCommand("j0.val=75");       // ตั้ง 75%

/* ควบคุม Visibility */
TJC_SendCommand("vis b0,1");        // แสดง component b0
TJC_SendCommand("vis b0,0");        // ซ่อน component b0

/* จำลองการกด */
TJC_SendCommand("click b0,1");      // กด b0
TJC_SendCommand("click b0,0");      // ปล่อย b0

/* ขอค่ากลับ (ผลลัพธ์มาทาง callback) */
TJC_SendCommand("get t0.txt");      // → OnString() callback
TJC_SendCommand("get n0.val");      // → OnNumeric() callback
TJC_SendCommand("get dp");          // → OnNumeric() ได้หมายเลข page ปัจจุบัน

/* ตั้งค่าระบบ */
TJC_SendCommand("dim=50");          // ความสว่างหน้าจอ 50%
TJC_SendCommand("sleep=0");         // ปลุกหน้าจอ
TJC_SendCommand("sleep=1");         // สั่งให้หน้าจอ sleep
TJC_SendCommand("bkcmd=3");         // เปิด response feedback
```

### `TJC_SendCommandParams(cmd, params, count, semicolon)` — ส่งแบบมี Parameter

ใช้สำหรับ protocol `CMD|PARAM1|PARAM2|...` ที่ออกแบบเองระหว่าง TJC ↔ MCU:

```c
/* ตัวอย่าง: ส่งคำสั่ง update|25|60; ไปที่ TJC */
const char *p[] = {"25", "60"};
TJC_SendCommandParams("update", p, 2, 1);
// ส่ง: "update|25|60;\xFF\xFF\xFF"

/* ส่งโดยไม่มี semicolon */
const char *p2[] = {"on"};
TJC_SendCommandParams("led", p2, 1, 0);
// ส่ง: "led|on\xFF\xFF\xFF"
```

---

## การรับข้อมูลจาก TJC (Callbacks)

ทุก callback ต้องลงทะเบียนก่อน `while(1)` และเรียก `TJC_ProcessResponse()` ใน loop:

```c
TJC_RegisterXxxCallback(MyFunction);   // ลงทะเบียน

while (1) {
    TJC_ProcessResponse();             // ต้องเรียกเสมอ
}
```

---

### Touch Event — กดปุ่ม/component

```c
void OnTouch(TJC_TouchEvent_t *event) {
    /* event->page_id      : หมายเลข page */
    /* event->component_id : หมายเลข component */
    /* event->event_type   : 0x01=Press, 0x00=Release */

    if (event->page_id == 0 && event->component_id == 1) {
        if (event->event_type == 0x01) {
            // b1 บน page 0 ถูกกด
            TJC_SendCommand("n0.val=100");
        } else {
            // b1 ถูกปล่อย
        }
    }
}

TJC_RegisterTouchEventCallback(OnTouch);
```

> ต้องตั้งค่าใน TJC Editor: component ต้องมี `Touch Event` เปิดอยู่

---

### Touch Coordinate — พิกัด XY การสัมผัส

```c
void OnTouchCoord(TJC_TouchCoord_t *coord) {
    /* coord->x          : พิกัด X (pixels) */
    /* coord->y          : พิกัด Y (pixels) */
    /* coord->event_type : 0x01=Press, 0x00=Release */
}

TJC_RegisterTouchCoordCallback(OnTouchCoord);
```

> ต้องส่งคำสั่งเปิดใน TJC: `sendxy=1`

---

### Numeric Data — ค่าตัวเลขจาก `get`

```c
void OnNumeric(uint32_t value) {
    /* value : ค่าตัวเลขที่ได้รับ (little-endian, 32-bit) */
}

TJC_RegisterNumericCallback(OnNumeric);

/* ขอค่ากลับมา */
TJC_SendCommand("get n0.val");   // TJC ตอบกลับ → OnNumeric() ถูกเรียก
TJC_SendCommand("get dp");       // ขอหมายเลข page ปัจจุบัน
```

---

### String Data — ค่า string จาก `get`

```c
void OnString(const char *str, uint16_t len) {
    /* str : string ที่ได้รับ (null-terminated) */
    /* len : ความยาว string */
    
    // ตัวอย่าง: นำไปแสดงผลหรือเปรียบเทียบ
    if (strcmp(str, "ready") == 0) {
        // TJC ส่งสถานะ ready กลับมา
    }
}

TJC_RegisterStringCallback(OnString);

/* ขอค่ากลับมา */
TJC_SendCommand("get t0.txt");   // TJC ตอบกลับ → OnString() ถูกเรียก
```

---

### System Event — Startup / Sleep / Wake

```c
void OnSystemEvent(uint8_t event_type) {
    switch (event_type) {
        case TJC_RET_STARTUP:    /* 0x88 — จอเริ่มทำงาน / reset */
            TJC_SendCommand("page 0");
            TJC_SendCommand("t0.txt=\"Ready\"");
            break;
        case TJC_RET_AUTO_SLEEP: /* 0x86 — จอเข้า sleep อัตโนมัติ */
            break;
        case TJC_RET_AUTO_WAKE:  /* 0x87 — จอออกจาก sleep อัตโนมัติ */
            TJC_SendCommand("page 0");  // refresh หน้าจอ
            break;
        case TJC_RET_SD_UPGRADE: /* 0x89 — เริ่ม SD card upgrade */
            break;
    }
}

TJC_RegisterSystemEventCallback(OnSystemEvent);
```

> ตั้งค่า sleep timer ใน TJC Editor: `thsp=30` (sleep หลัง 30 วินาทีไม่มีการสัมผัส)

---

### Error Callback — ผลลัพธ์คำสั่ง

```c
void OnError(uint8_t error_code) {
    if (error_code == TJC_ERR_SUCCESS) {
        // คำสั่งล่าสุดสำเร็จ
    } else {
        // คำสั่งล่าสุดมีข้อผิดพลาด
        // TJC_GetErrorString(error_code) — แปลงรหัสเป็นข้อความ
    }
}

TJC_RegisterErrorCallback(OnError);
```

---

## การส่งคำสั่ง Custom จาก TJC มายัง MCU

นี่คือวิธีให้ **TJC สั่งงาน MCU** โดยตรง เช่น กดปุ่มบนจอแล้วให้ MCU เปิด relay

### รูปแบบคำสั่ง

```
CMD\xFF\xFF\xFF
CMD;\xFF\xFF\xFF
CMD|PARAM1|PARAM2;\xFF\xFF\xFF
```

### ฝั่ง TJC Editor

เปิด Event ของปุ่มหรือ component ที่ต้องการ แล้วเขียน:

```
// กดปุ่มสั่ง MCU เปิด LED
prints "led|1;"

// กดปุ่มสั่ง MCU เปิด relay
prints "relay|1;"

// ส่งค่าจาก slider ไปยัง MCU
prints "pwm|"
prints h0.val,0
prints ";"

// ส่งข้อความจาก text input
prints "msg|"
printh 70     // 'p'
prints t0.txt,0
prints ";"
```

> ⚠️ `prints` ใน TJC ไม่ได้ส่ง terminator อัตโนมัติ ต้องปิดท้ายเองด้วย `;` หรือใช้ `printh FF FF FF`

### ฝั่ง MCU

```c
#include <string.h>
#include <stdlib.h>

void OnTJCCommand(TJC_ReceivedCommand_t *cmd) {
    /* cmd->command     : คำสั่งหลัก */
    /* cmd->params[i]   : parameter ที่ i (index เริ่มจาก 0) */
    /* cmd->param_count : จำนวน parameters */

    /* --- สั่งงาน GPIO --- */
    if (strcmp(cmd->command, "led") == 0 && cmd->param_count >= 1) {
        uint8_t state = (uint8_t)atoi(cmd->params[0]);
        GPIO_WriteBit(GPIOD, GPIO_Pin_3, state ? Bit_SET : Bit_RESET);
    }

    /* --- ควบคุม Relay --- */
    else if (strcmp(cmd->command, "relay") == 0 && cmd->param_count >= 1) {
        uint8_t ch    = (uint8_t)atoi(cmd->params[0]); // channel
        uint8_t state = (cmd->param_count >= 2) ? (uint8_t)atoi(cmd->params[1]) : 1;
        // relay_set(ch, state);
    }

    /* --- ตั้งค่า PWM --- */
    else if (strcmp(cmd->command, "pwm") == 0 && cmd->param_count >= 1) {
        uint16_t duty = (uint16_t)atoi(cmd->params[0]);
        // pwm_set_duty(duty);
        /* ตอบกลับยืนยัน */
        TJC_SendCommand("t_status.txt=\"PWM OK\"");
    }

    /* --- เปลี่ยน Page --- */
    else if (strcmp(cmd->command, "goto") == 0 && cmd->param_count >= 1) {
        char buf[32];
        snprintf(buf, sizeof(buf), "page %s", cmd->params[0]);
        TJC_SendCommand(buf);
    }

    /* --- ขอสถานะ (MCU ตอบกลับ) --- */
    else if (strcmp(cmd->command, "status") == 0) {
        TJC_SendCommand("t0.txt=\"Running\"");
        TJC_SendCommand("n0.val=42");
    }
}

/* ลงทะเบียน — ถ้าไม่ได้ลงทะเบียน คำสั่งจาก TJC จะถูกทิ้งทันที */
TJC_RegisterCommandCallback(OnTJCCommand);
```

### Flow ของ Custom Command

```
TJC Editor: prints "led|1;"
      ↓ UART
TJC_UART_IRQHandler() → TJC_BufferPut()  [IRQ context]
      ↓
TJC_ProcessResponse()                     [main loop]
  → ตรวจพบ terminator 0xFF 0xFF 0xFF
  → byte แรก = 'l' (0x6C) → ASCII → TJC_ProcessCommand()
  → แยก command = "led", params[0] = "1"
  → OnTJCCommand(&cmd)                    [user callback]
        → GPIO_WriteBit(...)
```

---

## API Reference ฉบับเต็ม

### `void TJC_Init(uint32_t baudrate, TJC_PinConfig pin_config)`

เริ่มต้น UART, GPIO, AFIO, NVIC และ RX interrupt พร้อมใช้งาน

```c
TJC_Init(115200, TJC_PINS_DEFAULT);
TJC_Init(9600,   TJC_PINS_REMAP1);
```

---

### `void TJC_SendCommand(const char *cmd)`

ส่งคำสั่งพร้อม terminator `0xFF 0xFF 0xFF` อัตโนมัติ

---

### `void TJC_SendCommandParams(const char *cmd, const char **params, uint8_t param_count, uint8_t use_semicolon)`

ส่งคำสั่งรูปแบบ `CMD|P1|P2|...;` พร้อม terminator

---

### `void TJC_ProcessResponse(void)`

ประมวลผลข้อมูลใน RX buffer ตรวจ terminator ตรวจความยาว packet และเรียก callback  
**ต้องเรียกใน `while(1)` เสมอ**

---

### `void TJC_ResetResponse(void)`

รีเซ็ต parser state เพื่อ re-sync เมื่อเกิด timeout หรือ bus error  
(ไม่ล้าง RX buffer — ถ้าต้องการล้าง buffer ใช้ `TJC_FlushRxBuffer()` ด้วย)

```c
// ตัวอย่าง: timeout watchdog
if (no_response_for_1_second) {
    TJC_ResetResponse();
    TJC_FlushRxBuffer();  // ล้างข้อมูลค้างใน buffer ด้วย (ถ้าจำเป็น)
}
```

---

### `uint16_t TJC_Available(void)`

คืนจำนวน bytes ที่รออยู่ใน RX buffer

---

### `void TJC_FlushRxBuffer(void)`

ล้างข้อมูลทั้งหมดใน RX buffer

---

### `const char* TJC_GetErrorString(uint8_t error_code)`

แปลง error code เป็นข้อความภาษาไทย

```c
void OnError(uint8_t code) {
    // ใช้กับ SimpleUSART
    // USART_Print(TJC_GetErrorString(code));
}
```

---

### `void TJC_EnableRxInterrupt(void)`

เปิด UART RX interrupt (ถูกเรียกอัตโนมัติใน `TJC_Init()`)

---

### `void TJC_UART_IRQHandler(void)`

ต้องเรียกจาก `USART1_IRQHandler` ใน `ch32v00x_it.c`

---

## Error Codes

| Enum | ค่า | คำอธิบาย |
|------|-----|-----------|
| `TJC_ERR_INVALID_CMD` | `0x00` | คำสั่งไม่ถูกต้อง |
| `TJC_ERR_SUCCESS` | `0x01` | คำสั่งสำเร็จ |
| `TJC_ERR_INVALID_COMPONENT` | `0x02` | Component ID ไม่ถูกต้อง |
| `TJC_ERR_INVALID_PAGE` | `0x03` | Page ID ไม่ถูกต้อง |
| `TJC_ERR_INVALID_PICTURE` | `0x04` | Picture ID ไม่ถูกต้อง |
| `TJC_ERR_INVALID_FONT` | `0x05` | Font ID ไม่ถูกต้อง |
| `TJC_ERR_CRC_FAILED` | `0x09` | CRC ไม่ตรงกัน |
| `TJC_ERR_INVALID_BAUDRATE` | `0x11` | Baud rate ไม่ถูกต้อง |
| `TJC_ERR_INVALID_CURVE` | `0x12` | Curve control ไม่ถูกต้อง |
| `TJC_ERR_INVALID_VARIABLE` | `0x1A` | ชื่อตัวแปรไม่ถูกต้อง |
| `TJC_ERR_INVALID_OPERATION` | `0x1B` | การคำนวณไม่ถูกต้อง |
| `TJC_ERR_ASSIGNMENT_FAILED` | `0x1C` | การกำหนดค่าล้มเหลว |
| `TJC_ERR_EEPROM_FAILED` | `0x1D` | EEPROM ล้มเหลว |
| `TJC_ERR_INVALID_PARAM_COUNT` | `0x1E` | จำนวน parameter ไม่ถูกต้อง |
| `TJC_ERR_IO_FAILED` | `0x1F` | IO ล้มเหลว |
| `TJC_ERR_ESCAPE_CHAR` | `0x20` | Escape character ผิด |
| `TJC_ERR_VARIABLE_NAME_LONG` | `0x23` | ชื่อตัวแปรยาวเกิน |

---

## Configuration

ปรับค่าได้ใน `TJC.h`:

| Define | Default | คำอธิบาย |
|--------|---------|-----------|
| `TJC_RX_BUFFER_SIZE` | `256` | ขนาด circular RX buffer (bytes) |
| `TJC_MAX_STRING_LENGTH` | `128` | ความยาวสูงสุดของ string ที่รับได้ |
| `TJC_MAX_PARAMS` | `10` | จำนวน parameters สูงสุดต่อคำสั่ง |
| `TJC_TERMINATOR_SIZE` | `3` | ขนาด terminator (`0xFF 0xFF 0xFF`) |
| `TJC_PACKET_MAX_SIZE` | `132` | ขนาด packet สูงสุดก่อน auto-reset |

---

## ตัวอย่างโปรแกรมเต็ม

### `main.c`

```c
#include <string.h>
#include <stdlib.h>
#include "TJC.h"

/* ========== Callbacks ========== */

void OnSystemEvent(uint8_t event_type) {
    if (event_type == TJC_RET_STARTUP) {
        /* จอพร้อมใช้งาน — ส่งค่าเริ่มต้น */
        TJC_SendCommand("bkcmd=1");
        TJC_SendCommand("page 0");
        TJC_SendCommand("t0.txt=\"Ready\"");
    }
}

void OnTouch(TJC_TouchEvent_t *event) {
    if (event->event_type != 0x01) return; /* เฉพาะ Press */

    /* Page 0 */
    if (event->page_id == 0) {
        switch (event->component_id) {
            case 1: TJC_SendCommand("page 1");      break; /* ปุ่มไป page 1 */
            case 2: TJC_SendCommand("n0.val=0");    break; /* ปุ่ม reset */
        }
    }
}

void OnNumeric(uint32_t value) {
    /* ได้รับค่าตัวเลขจาก TJC (เช่น get n0.val) */
    (void)value;
}

void OnString(const char *str, uint16_t len) {
    /* ได้รับ string จาก TJC (เช่น get t0.txt) */
    (void)str;
    (void)len;
}

/* MCU รับคำสั่งจาก TJC */
void OnTJCCommand(TJC_ReceivedCommand_t *cmd) {
    if (strcmp(cmd->command, "led") == 0 && cmd->param_count >= 1) {
        uint8_t state = (uint8_t)atoi(cmd->params[0]);
        GPIO_WriteBit(GPIOD, GPIO_Pin_3, state ? Bit_SET : Bit_RESET);
        TJC_SendCommand(state ? "t_led.txt=\"ON\"" : "t_led.txt=\"OFF\"");
    }
    else if (strcmp(cmd->command, "status") == 0) {
        TJC_SendCommand("t0.txt=\"Running\"");
    }
}

void OnError(uint8_t error_code) {
    (void)error_code; /* จัดการ error ตามต้องการ */
}

/* ========== Main ========== */

int main(void) {
    SystemCoreClockUpdate();

    /* Init TJC */
    TJC_Init(115200, TJC_PINS_DEFAULT);

    /* ลงทะเบียน callbacks */
    TJC_RegisterSystemEventCallback(OnSystemEvent);
    TJC_RegisterTouchEventCallback(OnTouch);
    TJC_RegisterNumericCallback(OnNumeric);
    TJC_RegisterStringCallback(OnString);
    TJC_RegisterCommandCallback(OnTJCCommand); /* รับคำสั่งจาก TJC */
    TJC_RegisterErrorCallback(OnError);

    while (1) {
        TJC_ProcessResponse(); /* ต้องเรียกทุก loop */
    }
}
```

### `ch32v00x_it.c`

```c
#include "TJC.h"

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) {
    TJC_UART_IRQHandler();
}
```

### TJC Editor — Event ของปุ่มบนจอ

```
// Touch Press Event ของปุ่ม b0 — สั่ง MCU เปิด LED
prints "led|1;"

// Touch Press Event ของปุ่ม b1 — สั่ง MCU ปิด LED
prints "led|0;"

// Touch Press Event ของปุ่ม b2 — ขอสถานะจาก MCU
prints "status;"
```

---

## หมายเหตุ

- ทุกคำสั่งต้องจบด้วย `0xFF 0xFF 0xFF` — library จัดการให้อัตโนมัติ
- **`TJC_ProcessResponse()` ต้องเรียกสม่ำเสมอ** มิฉะนั้น RX buffer จะเต็มและข้อมูลหาย
- `TJC_RegisterCommandCallback()` ต้องลงทะเบียนเสมอ ถ้าต้องการรับคำสั่งจาก TJC — ถ้าไม่ลงทะเบียน คำสั่งที่ TJC ส่งมาจะถูกทิ้งไปเงียบ ๆ
- ถ้า Baud rate ไม่ตรง TJC จะตอบ `TJC_ERR_INVALID_BAUDRATE (0x11)`
- `TJC_PACKET_MAX_SIZE` ควรตั้งให้ใหญ่กว่าความยาว string สูงสุดที่ส่ง+4 เสมอ
- Numeric data ที่ TJC ส่งคือ **little-endian** 32-bit — library แปลงให้อัตโนมัติแล้ว

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)