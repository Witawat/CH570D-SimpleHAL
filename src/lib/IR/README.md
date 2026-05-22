# SimpleIR Library - คู่มือการใช้งานฉบับสมบูรณ์

## สารบัญ

1. [บทนำ](#บทนำ)
2. [ทฤษฎี IR Communication](#ทฤษฎี-ir-communication)
3. [Hardware Setup](#hardware-setup)
4. [Quick Start Guide](#quick-start-guide)
5. [API Reference](#api-reference)
6. [Advanced Techniques](#advanced-techniques)
7. [Troubleshooting](#troubleshooting)
8. [Project Examples](#project-examples)

---

## บทนำ

**SimpleIR** เป็น library สำหรับการสื่อสารด้วย Infrared (IR) บน CH57x microcontroller โดยใช้ SimpleHAL เป็นพื้นฐาน

### คุณสมบัติหลัก

✅ **IR Receiver**
- รับสัญญาณ IR ด้วย External Interrupt
- วัดเวลา pulse ด้วยความแม่นยำระดับ microsecond
- Auto-detect โปรโตคอล (NEC, RC5, SIRC)
- Raw timing capture สำหรับวิเคราะห์โปรโตคอลใหม่

✅ **IR Transmitter**
- ส่งสัญญาณ IR ด้วย PWM 38kHz carrier
- รองรับโปรโตคอล NEC, RC5, SIRC
- ส่ง raw timing data ได้
- NEC repeat code support

✅ **Non-blocking Operation**
- ไม่บล็อกการทำงานของโปรแกรม
- ใช้ callback สำหรับแจ้งเตือนเมื่อรับข้อมูล
- เหมาะสำหรับ multitasking

### ข้อกำหนดของระบบ

- **MCU:** CH57x (24MHz)
- **Dependencies:** 
  - SimpleGPIO (GPIO control)
  - SimplePWM (38kHz carrier generation)
  - Timer library (microsecond timing)
- **Memory:** ~2KB Flash, ~200 bytes RAM

---

## ทฤษฎี IR Communication

### หลักการทำงานของ IR

Infrared (IR) communication ใช้แสงอินฟราเรด (ความยาวคลื่น 940nm) ในการส่งข้อมูล โดยมีหลักการดังนี้:

```
Transmitter                    Receiver
┌─────────┐                   ┌─────────┐
│ IR LED  │ ───────────────> │ TSOP    │
│ 940nm   │   IR Light        │ 38238   │
└─────────┘                   └─────────┘
     │                             │
     │ Modulated 38kHz             │ Demodulated
     │                             │
  Digital                       Digital
  Signal                        Signal
```

### Carrier Frequency

IR communication ใช้ **carrier frequency** (โดยทั่วไป 38kHz) เพื่อ:
- แยกสัญญาณ IR จากแสงแดดและแสงอื่นๆ
- ลดการรบกวนจากแหล่งกำเนิดแสงอื่น
- เพิ่มระยะทางการส่ง

**การทำงาน:**
- **Mark (1):** Carrier ON (38kHz PWM)
- **Space (0):** Carrier OFF (no signal)

### IR Protocols ยอดนิยม

#### 1. NEC Protocol

NEC เป็นโปรโตคอลที่ใช้กันมากที่สุด พบใน TV, DVD, Air conditioner

**Format:**
```
┌─────┬─────┬──────────────────────────────────┬───┐
│ 9ms │4.5ms│         32 bits data             │560│
│MARK │SPACE│  (Address + ~Addr + Cmd + ~Cmd)  │us │
└─────┴─────┴──────────────────────────────────┴───┘
```

**Bit Encoding:**
- Bit 0: 560us mark + 560us space
- Bit 1: 560us mark + 1690us space

**ข้อดี:**
- ตรวจสอบความถูกต้องด้วย inverse bits
- รองรับ repeat code (กดค้าง)
- ใช้กันแพร่หลาย

**ข้อเสีย:**
- ใช้เวลานานในการส่ง (~67.5ms)

#### 2. RC5 Protocol (Philips)

RC5 ใช้ Manchester encoding พบใน Philips, Marantz

**Format:**
```
14 bits: [S1][S2][T][Address:5][Command:6]
- S1, S2: Start bits (always 1)
- T: Toggle bit (สลับทุกครั้งที่กดปุ่มใหม่)
- Address: 5 bits (0-31)
- Command: 6 bits (0-63)
```

**Bit Encoding (Manchester):**
- Bit 0: 889us mark + 889us space
- Bit 1: 889us space + 889us mark

**ข้อดี:**
- กะทัดรัด (14 bits)
- Toggle bit ช่วยแยกการกดปุ่มใหม่
- Manchester encoding ทนทานต่อสัญญาณรบกวน

#### 3. SIRC Protocol (Sony)

SIRC เป็นโปรโตคอลของ Sony มีหลายรูปแบบ (12/15/20-bit)

**Format (12-bit):**
```
┌──────┬─────┬────────────────────────┐
│2.4ms │ 600 │    12 bits data        │
│ MARK │SPACE│ (Command:7 + Addr:5)   │
└──────┴─────┴────────────────────────┘
```

**Bit Encoding:**
- Bit 0: 600us mark + 600us space
- Bit 1: 1200us mark + 600us space

**ข้อดี:**
- ส่งเร็ว (~18ms สำหรับ 12-bit)
- LSB first (ง่ายต่อการ decode)

**ข้อเสีย:**
- ไม่มี error checking
- ต้องส่งซ้ำ 3 ครั้งเพื่อความน่าเชื่อถือ

---

## Hardware Setup

### IR Receiver Circuit

**อุปกรณ์:**
- TSOP38238 หรือ VS1838B (IR receiver module 38kHz)
- ตัวต้านทาน 100Ω (optional, สำหรับ filtering)
- ตัวเก็บประจุ 10µF (optional, สำหรับ power filtering)

**Schematic:**
```
         TSOP38238
         ┌───────┐
  3.3V ──┤1    3├── PC1 (Signal)
         │   ▼  │
   GND ──┤2    ─├
         └───────┘
         
Pin 1: VCC (3.3V)
Pin 2: GND
Pin 3: OUT (to MCU)
```

**หมายเหตุ:**
- TSOP38238 output เป็น **active LOW** (ไม่มีสัญญาณ = HIGH, มีสัญญาณ = LOW)
- ระยะรับสัญญาณ: ~5-10 เมตร (ขึ้นกับกำลังส่ง)
- มุมรับสัญญาณ: ±45 องศา

### IR Transmitter Circuit

**อุปกรณ์:**
- IR LED 940nm (5mm)
- NPN Transistor 2N2222 หรือ BC547
- ตัวต้านทาน 100Ω (สำหรับ IR LED)
- ตัวต้านทาน 10kΩ (สำหรับ base transistor)

**Schematic:**
```
                    +3.3V
                      │
                      ├── IR LED (940nm)
                      │   Anode (+)
                     ┌┴┐
                100Ω│ │
                     └┬┘
                      │ Cathode (-)
                      │
                      C (Collector)
                    ┌─┴─┐
                    │2N2│ 2222
  PC6 ──┤10kΩ├──── B   │ (NPN)
                    └─┬─┘
                      E (Emitter)
                      │
                     GND
```

**การคำนวณกระแส:**
```
VCC = 3.3V
V_LED = 1.2V (IR LED forward voltage)
R = 100Ω

I_LED = (VCC - V_LED) / R
      = (3.3 - 1.2) / 100
      = 21mA
```

**หมายเหตุ:**
- กระแส 20-30mA เหมาะสำหรับ IR LED ทั่วไป
- ระยะส่ง: ~5-10 เมตร
- ใช้ transistor เพื่อไม่ให้ MCU รับกระแสมากเกินไป

### Pin Connections Summary

| Component | Pin | Description |
|-----------|-----|-------------|
| IR Receiver | PC1 | Input (EXTI) |
| IR Transmitter | PC6 | Output (TIM1_CH1 PWM) |
| Status LED | PC0 | Output (optional) |

---

## Quick Start Guide

### ขั้นตอนที่ 1: ติดตั้ง Library

1. คัดลอกโฟลเดอร์ `IR/` ไปยัง `User/Lib/`
2. เพิ่ม include path ใน project settings
3. เพิ่มไฟล์ `SimpleIR.c` ใน build

### ขั้นตอนที่ 2: Include Headers

```c
#include "User/Lib/Timer/timer.h"
#include "User/Lib/IR/SimpleIR.h"
```

### ขั้นตอนที่ 3: เริ่มต้นระบบ

```c
int main(void) {
    // เริ่มต้นระบบพื้นฐาน
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    
    // เริ่มต้น Timer (จำเป็น!)
    Timer_Init();
    
    // ... ตั้งค่า IR receiver/transmitter
}
```

### ตัวอย่างที่ 1: รับสัญญาณ IR

```c
#include "debug.h"
#include "User/Lib/Timer/timer.h"
#include "User/Lib/IR/SimpleIR.h"

void ir_callback(IR_DecodedData_t* data) {
    printf("Protocol: %s\n", IR_PROTOCOL_NAME(data->protocol));
    printf("Address: 0x%02X, Command: 0x%02X\n", 
           data->address, data->command);
}

int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    
    Timer_Init();
    IR_ReceiverInit(PC1, ir_callback);
    
    while(1) {
        IR_Process();  // ต้องเรียกใน main loop!
        Delay_Ms(10);
    }
}
```

### ตัวอย่างที่ 2: ส่งสัญญาณ IR

```c
#include "debug.h"
#include "User/Lib/Timer/timer.h"
#include "User/Lib/IR/SimpleIR.h"

int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    
    Timer_Init();
    IR_TransmitterInit(PC6);
    
    while(1) {
        // ส่งคำสั่ง NEC
        IR_Send(IR_PROTOCOL_NEC, 0x00, 0x12);
        Delay_Ms(2000);
    }
}
```

---

## API Reference

### Receiver Functions

#### `IR_ReceiverInit()`
```c
void IR_ReceiverInit(uint8_t pin, IR_Callback_t callback);
```
**คำอธิบาย:** เริ่มต้น IR receiver

**Parameters:**
- `pin`: GPIO pin สำหรับ receiver (แนะนำ PC1)
- `callback`: ฟังก์ชันที่จะถูกเรียกเมื่อรับข้อมูลสำเร็จ

**ตัวอย่าง:**
```c
void my_callback(IR_DecodedData_t* data) {
    // ประมวลผลข้อมูล
}

IR_ReceiverInit(PC1, my_callback);
```

---

#### `IR_Process()`
```c
void IR_Process(void);
```
**คำอธิบาย:** ประมวลผลข้อมูล IR (ต้องเรียกใน main loop)

**หมายเหตุ:** ฟังก์ชันนี้จะตรวจสอบ timeout และ decode ข้อมูล

**ตัวอย่าง:**
```c
while(1) {
    IR_Process();
    // ... งานอื่นๆ
}
```

---

#### `IR_Available()`
```c
bool IR_Available(void);
```
**คำอธิบาย:** ตรวจสอบว่ามีข้อมูลใหม่หรือไม่

**Return:** `true` = มีข้อมูลใหม่, `false` = ไม่มี

**ตัวอย่าง:**
```c
if (IR_Available()) {
    IR_DecodedData_t data = IR_GetData();
    printf("Command: 0x%02X\n", data.command);
}
```

---

#### `IR_GetData()`
```c
IR_DecodedData_t IR_GetData(void);
```
**คำอธิบาย:** อ่านข้อมูลที่ decode แล้ว

**Return:** โครงสร้าง `IR_DecodedData_t`

**ตัวอย่าง:**
```c
IR_DecodedData_t data = IR_GetData();
if (data.valid) {
    printf("Protocol: %s\n", IR_PROTOCOL_NAME(data.protocol));
    printf("Address: 0x%02X\n", data.address);
    printf("Command: 0x%02X\n", data.command);
}
```

---

#### `IR_GetRawData()`
```c
IR_RawData_t* IR_GetRawData(void);
```
**คำอธิบาย:** อ่าน raw timing data

**Return:** ตัวชี้ไปยัง `IR_RawData_t`

**ตัวอย่าง:**
```c
IR_RawData_t* raw = IR_GetRawData();
printf("Timings: %d\n", raw->count);
for (int i = 0; i < raw->count; i++) {
    printf("%d ", raw->timings[i]);
}
```

---

### Transmitter Functions

#### `IR_TransmitterInit()`
```c
void IR_TransmitterInit(uint8_t pin);
```
**คำอธิบาย:** เริ่มต้น IR transmitter

**Parameters:**
- `pin`: GPIO pin สำหรับ transmitter (แนะนำ PC6 สำหรับ TIM1_CH1)

**ตัวอย่าง:**
```c
IR_TransmitterInit(PC6);
```

---

#### `IR_Send()`
```c
bool IR_Send(IR_Protocol_t protocol, uint16_t address, uint16_t command);
```
**คำอธิบาย:** ส่งข้อมูล IR ตามโปรโตคอล

**Parameters:**
- `protocol`: โปรโตคอล (IR_PROTOCOL_NEC, IR_PROTOCOL_RC5, IR_PROTOCOL_SIRC)
- `address`: Address/Device code
- `command`: Command/Function code

**Return:** `true` = ส่งสำเร็จ, `false` = ส่งไม่สำเร็จ

**ตัวอย่าง:**
```c
// ส่งคำสั่ง NEC
IR_Send(IR_PROTOCOL_NEC, 0x00, 0x12);

// ส่งคำสั่ง RC5
IR_Send(IR_PROTOCOL_RC5, 0x05, 0x0C);

// ส่งคำสั่ง SIRC
IR_Send(IR_PROTOCOL_SIRC, 0x01, 0x15);
```

---

#### `IR_SendRaw()`
```c
bool IR_SendRaw(const uint16_t* timings, uint16_t count);
```
**คำอธิบาย:** ส่ง raw timing data

**Parameters:**
- `timings`: array ของ timing (microseconds)
- `count`: จำนวน timing

**Return:** `true` = ส่งสำเร็จ

**ตัวอย่าง:**
```c
uint16_t custom_signal[] = {
    9000, 4500,  // Lead pulse
    560, 560,    // Bit 0
    560, 1690    // Bit 1
};
IR_SendRaw(custom_signal, 6);
```

---

### Utility Functions

#### `IR_PrintRawData()`
```c
void IR_PrintRawData(void);
```
**คำอธิบาย:** แสดง raw timing data ทาง UART

**ตัวอย่าง:**
```c
IR_PrintRawData();
// Output: Raw IR Data (48 timings):
//         9000 4500 560 560 560 1690 ...
```

---

#### `IR_PrintDecodedData()`
```c
void IR_PrintDecodedData(IR_DecodedData_t* data);
```
**คำอธิบาย:** แสดงข้อมูลที่ decode แล้วทาง UART

**ตัวอย่าง:**
```c
IR_DecodedData_t data = IR_GetData();
IR_PrintDecodedData(&data);
// Output: Protocol: NEC, Address: 0x00, Command: 0x12
```

---

## Advanced Techniques

### เทคนิคที่ 1: Raw Data Analysis

การวิเคราะห์ raw timing data เพื่อเข้าใจโปรโตคอลใหม่

```c
void analyze_protocol(void) {
    IR_RawData_t* raw = IR_GetRawData();
    
    printf("\n=== Protocol Analysis ===\n");
    printf("Total timings: %d\n", raw->count);
    
    // หา lead pulse (pulse แรกที่ยาวที่สุด)
    uint16_t max_pulse = 0;
    for (int i = 0; i < raw->count; i += 2) {
        if (raw->timings[i] > max_pulse) {
            max_pulse = raw->timings[i];
        }
    }
    printf("Lead pulse: %d us\n", max_pulse);
    
    // หา bit time (timing ที่พบบ่อยที่สุด)
    uint16_t bit_time = 0;
    int max_count = 0;
    
    for (int i = 2; i < raw->count; i += 2) {
        int count = 0;
        for (int j = 2; j < raw->count; j += 2) {
            if (abs(raw->timings[i] - raw->timings[j]) < 100) {
                count++;
            }
        }
        if (count > max_count) {
            max_count = count;
            bit_time = raw->timings[i];
        }
    }
    printf("Bit time: %d us\n", bit_time);
    
    // คำนวณจำนวน bits
    int bits = (raw->count - 2) / 2;  // ลบ lead pulse
    printf("Estimated bits: %d\n", bits);
}
```

### เทคนิคที่ 2: Custom Protocol Decoder

สร้าง decoder สำหรับโปรโตคอลที่ไม่รองรับ

```c
bool IR_DecodeCustom(IR_RawData_t* raw, IR_DecodedData_t* decoded) {
    // ตรวจสอบจำนวน timing
    if (raw->count < 20) return false;
    
    // ตรวจสอบ lead pulse (ตัวอย่าง: 8ms mark + 4ms space)
    if (!IR_MATCH(raw->timings[0], 8000, 25)) return false;
    if (!IR_MATCH(raw->timings[1], 4000, 25)) return false;
    
    // Decode bits
    uint16_t data = 0;
    for (int i = 0; i < 16; i++) {
        int idx = 2 + (i * 2);
        
        // ตรวจสอบ mark
        if (!IR_MATCH(raw->timings[idx], 500, 25)) return false;
        
        // แยก bit 0/1 จาก space
        if (IR_MATCH(raw->timings[idx + 1], 1500, 25)) {
            data |= (1 << i);  // Bit 1
        } else if (!IR_MATCH(raw->timings[idx + 1], 500, 25)) {
            return false;
        }
    }
    
    // เก็บผลลัพธ์
    decoded->protocol = IR_PROTOCOL_RAW;  // หรือสร้าง enum ใหม่
    decoded->address = (data >> 8) & 0xFF;
    decoded->command = data & 0xFF;
    decoded->bits = 16;
    decoded->valid = true;
    
    return true;
}
```

### เทคนิคที่ 3: IR Repeater

สร้าง IR repeater (รับแล้วส่งต่อ)

```c
#define IR_RX_PIN  PC1
#define IR_TX_PIN  PC6

void ir_repeater_callback(IR_DecodedData_t* data) {
    // รับข้อมูลแล้วส่งต่อทันที
    if (data->valid) {
        printf("Repeating: %s 0x%02X 0x%02X\n",
               IR_PROTOCOL_NAME(data->protocol),
               data->address, data->command);
        
        IR_Send(data->protocol, data->address, data->command);
    } else {
        // ถ้า decode ไม่ได้ ส่ง raw data
        IR_RawData_t* raw = IR_GetRawData();
        IR_SendRaw(raw->timings, raw->count);
    }
}

int main(void) {
    // ... init system
    
    Timer_Init();
    IR_ReceiverInit(IR_RX_PIN, ir_repeater_callback);
    IR_TransmitterInit(IR_TX_PIN);
    
    printf("IR Repeater started\n");
    
    while(1) {
        IR_Process();
        Delay_Ms(10);
    }
}
```

### เทคนิคที่ 4: Power Optimization

ลดการใช้พลังงานเมื่อไม่ใช้งาน

```c
void power_save_mode(void) {
    // ปิด receiver เมื่อไม่ใช้งาน
    IR_DisableReceiver();
    
    // เข้า sleep mode
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
    
    // ตื่นจาก external interrupt
    IR_EnableReceiver();
}

// ตัวอย่างการใช้งาน
void ir_callback(IR_DecodedData_t* data) {
    // ประมวลผลข้อมูล
    process_command(data);
    
    // กลับเข้า sleep mode หลังจาก 5 วินาที
    Delay_Ms(5000);
    power_save_mode();
}
```

### เทคนิคที่ 5: Multi-Button Learning

เรียนรู้หลายปุ่มและบันทึกลง Flash

```c
#include "ch32v00x_flash.h"

#define FLASH_ADDR  0x08001000  // ที่อยู่ Flash สำหรับบันทึก
#define MAX_BUTTONS 10

typedef struct {
    uint16_t timings[IR_RAW_BUFFER_SIZE];
    uint16_t count;
} StoredButton_t;

// บันทึกปุ่มลง Flash
void save_button(uint8_t index, IR_RawData_t* raw) {
    FLASH_Unlock();
    
    uint32_t addr = FLASH_ADDR + (index * sizeof(StoredButton_t));
    
    // ลบ page ก่อน (ถ้าจำเป็น)
    FLASH_ErasePage(addr);
    
    // เขียนข้อมูล
    StoredButton_t button;
    button.count = raw->count;
    memcpy(button.timings, raw->timings, raw->count * sizeof(uint16_t));
    
    FLASH_ProgramPage(addr, (uint32_t*)&button, sizeof(StoredButton_t));
    
    FLASH_Lock();
    
    printf("Button %d saved to Flash\n", index);
}

// อ่านปุ่มจาก Flash
bool load_button(uint8_t index, StoredButton_t* button) {
    uint32_t addr = FLASH_ADDR + (index * sizeof(StoredButton_t));
    memcpy(button, (void*)addr, sizeof(StoredButton_t));
    
    // ตรวจสอบว่าข้อมูลถูกต้อง
    return (button->count > 0 && button->count < IR_RAW_BUFFER_SIZE);
}
```

---

## Troubleshooting

### ปัญหาที่พบบ่อย

#### 1. ไม่สามารถรับสัญญาณ IR ได้

**อาการ:** กดปุ่มบน remote แล้วไม่มีการตอบสนอง

**สาเหตุและวิธีแก้:**

✅ **ตรวจสอบการต่อวงจร**
```c
// ทดสอบ pin ด้วย digitalRead
pinMode(PC1, PIN_MODE_INPUT);
while(1) {
    uint8_t state = digitalRead(PC1);
    printf("Pin state: %d\n", state);
    Delay_Ms(100);
}
// ควรเห็นค่าเปลี่ยนเมื่อกดปุ่ม remote
```

✅ **ตรวจสอบ Timer_Init()**
```c
// ต้องเรียก Timer_Init() ก่อน IR_ReceiverInit()
Timer_Init();  // จำเป็น!
IR_ReceiverInit(PC1, callback);
```

✅ **ตรวจสอบ IR_Process()**
```c
// ต้องเรียก IR_Process() ใน main loop
while(1) {
    IR_Process();  // จำเป็น!
    Delay_Ms(10);
}
```

✅ **ตรวจสอบ TSOP38238**
- ต่อ VCC, GND ถูกต้องหรือไม่
- ใช้ TSOP38238 หรือ VS1838B (38kHz)
- ระยะห่างจาก remote ไม่เกิน 10 เมตร

---

#### 2. Decode ข้อมูลไม่ถูกต้อง

**อาการ:** รับสัญญาณได้แต่ decode ผิด

**วิธีแก้:**

✅ **ตรวจสอบ raw timing**
```c
void ir_callback(IR_DecodedData_t* data) {
    IR_PrintRawData();  // แสดง raw data
}
```

✅ **ปรับ tolerance**
```c
// ใน SimpleIR.h
#define IR_TOLERANCE  30  // เพิ่มจาก 25 เป็น 30
```

✅ **ตรวจสอบโปรโตคอล**
- NEC: Lead pulse ~9000us
- RC5: Bit time ~889us
- SIRC: Lead pulse ~2400us

---

#### 3. ส่งสัญญาณ IR ไม่ออก

**อาการ:** เรียก IR_Send() แล้วไม่มีสัญญาณออก

**วิธีแก้:**

✅ **ตรวจสอบวงจร transmitter**
```c
// ทดสอบ PWM ด้วย LED ธรรมดา
pinMode(PC6, PIN_MODE_OUTPUT);
while(1) {
    digitalWrite(PC6, HIGH);
    Delay_Ms(500);
    digitalWrite(PC6, LOW);
    Delay_Ms(500);
}
// LED ควรกะพริบ
```

✅ **ตรวจสอบ IR LED ด้วยกล้องมือถือ**
- เปิดกล้องมือถือ
- มองที่ IR LED
- ควรเห็นแสงสีม่วง/ขาวเมื่อส่งสัญญาณ

✅ **ตรวจสอบ transistor**
- Base -> 10kΩ -> PC6
- Collector -> IR LED
- Emitter -> GND

---

#### 4. Buffer Overflow

**อาการ:** `raw->overflow == true`

**วิธีแก้:**

✅ **เพิ่มขนาด buffer**
```c
// ใน SimpleIR.h
#define IR_RAW_BUFFER_SIZE  300  // เพิ่มจาก 200
```

✅ **ตรวจสอบสัญญาณรบกวน**
- สัญญาณรบกวนอาจทำให้มี edge มากเกินไป
- ลองเพิ่มตัวเก็บประจุ 10µF ที่ VCC ของ TSOP

---

#### 5. Timing ไม่แม่นยำ

**อาการ:** Timing ผิดเพี้ยนจากค่ามาตรฐาน

**วิธีแก้:**

✅ **ตรวจสอบ SystemCoreClock**
```c
printf("SystemCoreClock: %lu Hz\n", SystemCoreClock);
// ควรเป็น 24000000 (24MHz)
```

✅ **Calibrate Delay_Us()**
```c
// ทดสอบความแม่นยำ
uint32_t start = Get_CurrentUs();
Delay_Us(1000);
uint32_t elapsed = Get_CurrentUs() - start;
printf("Delay 1000us, actual: %lu us\n", elapsed);
// ควรใกล้เคียง 1000us
```

---

## Project Examples

### โปรเจคที่ 1: IR Remote Control

สร้างรีโมทคอนโทรลสำหรับควบคุมอุปกรณ์

```c
// ปุ่มต่างๆ บนรีโมท
#define BTN_POWER   PC2
#define BTN_VOL_UP  PC3
#define BTN_VOL_DN  PC4
#define BTN_CH_UP   PC5
#define BTN_CH_DN   PD2

// คำสั่ง NEC
#define CMD_POWER   0x12
#define CMD_VOL_UP  0x13
#define CMD_VOL_DN  0x14
#define CMD_CH_UP   0x15
#define CMD_CH_DN   0x16

void send_command(uint8_t cmd) {
    IR_Send(IR_PROTOCOL_NEC, 0x00, cmd);
    printf("Sent command: 0x%02X\n", cmd);
}

int main(void) {
    // ... init
    
    pinMode(BTN_POWER, PIN_MODE_INPUT_PULLUP);
    pinMode(BTN_VOL_UP, PIN_MODE_INPUT_PULLUP);
    // ... ตั้งค่าปุ่มอื่นๆ
    
    IR_TransmitterInit(PC6);
    
    while(1) {
        if (digitalRead(BTN_POWER) == LOW) {
            send_command(CMD_POWER);
            while (digitalRead(BTN_POWER) == LOW);  // รอปล่อยปุ่ม
        }
        
        if (digitalRead(BTN_VOL_UP) == LOW) {
            send_command(CMD_VOL_UP);
            while (digitalRead(BTN_VOL_UP) == LOW);
        }
        
        // ... ตรวจสอบปุ่มอื่นๆ
        
        Delay_Ms(50);
    }
}
```

### โปรเจคที่ 2: IR Obstacle Detection

ใช้ IR สำหรับตรวจจับสิ่งกีดขวาง

```c
#define IR_TX_PIN  PC6
#define IR_RX_PIN  PC1

bool detect_obstacle(void) {
    // ส่งสัญญาณ IR burst
    for (int i = 0; i < 10; i++) {
        IR_Mark(100);  // 100us pulse
        Delay_Us(100);
    }
    
    // ตรวจสอบว่ามีสัญญาณสะท้อนกลับหรือไม่
    uint32_t start = Get_CurrentUs();
    while (Get_CurrentUs() - start < 1000) {  // รอ 1ms
        if (digitalRead(IR_RX_PIN) == LOW) {
            return true;  // ตรวจพบสิ่งกีดขวาง
        }
    }
    
    return false;  // ไม่พบสิ่งกีดขวาง
}

int main(void) {
    // ... init
    
    pinMode(IR_RX_PIN, PIN_MODE_INPUT);
    IR_TransmitterInit(IR_TX_PIN);
    
    while(1) {
        if (detect_obstacle()) {
            printf("Obstacle detected!\n");
        } else {
            printf("Clear\n");
        }
        
        Delay_Ms(100);
    }
}
```

### โปรเจคที่ 3: IR Data Transmission

ส่งข้อมูลผ่าน IR (เหมือน IrDA)

```c
// ส่งข้อมูล 1 byte
void ir_send_byte(uint8_t data) {
    // Start bit
    IR_Mark(1000);
    IR_Space(500);
    
    // ส่ง 8 bits
    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) {
            IR_Mark(500);
            IR_Space(1000);  // Bit 1
        } else {
            IR_Mark(500);
            IR_Space(500);   // Bit 0
        }
    }
    
    // Stop bit
    IR_Mark(1000);
    IR_Space(500);
}

// ส่ง string
void ir_send_string(const char* str) {
    while (*str) {
        ir_send_byte(*str);
        str++;
        Delay_Ms(10);  // หน่วงระหว่าง byte
    }
}

// ตัวอย่างการใช้งาน
int main(void) {
    // ... init
    
    IR_TransmitterInit(PC6);
    
    while(1) {
        ir_send_string("Hello IR!");
        Delay_Ms(1000);
    }
}
```

---

## สรุป

SimpleIR library ให้เครื่องมือครบครันสำหรับการทำงานกับ IR บน CH57x:

✅ **Receiver:** รับสัญญาณ IR พร้อม auto-detect โปรโตคอล  
✅ **Transmitter:** ส่งสัญญาณ IR ด้วย 38kHz carrier  
✅ **Protocols:** รองรับ NEC, RC5, SIRC  
✅ **Raw Data:** จับและส่ง raw timing data  
✅ **Non-blocking:** ไม่บล็อกการทำงานของโปรแกรม  

**Tips สำหรับการใช้งาน:**
1. เรียก `Timer_Init()` ก่อนเสมอ
2. เรียก `IR_Process()` ใน main loop
3. ใช้ callback สำหรับประมวลผลข้อมูล
4. ใช้ raw data capture สำหรับวิเคราะห์โปรโตคอลใหม่
5. ทดสอบวงจรด้วย LED และกล้องมือถือ

**เอกสารเพิ่มเติม:**
- [NEC Protocol Specification](https://www.sbprojects.net/knowledge/ir/nec.php)
- [RC5 Protocol Specification](https://www.sbprojects.net/knowledge/ir/rc5.php)
- [SIRC Protocol Specification](https://www.sbprojects.net/knowledge/ir/sirc.php)

---

**Version:** 1.0  
**Date:** 2025-12-21  
**Author:** SimpleIR Library for CH57x  
**License:** MIT

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)