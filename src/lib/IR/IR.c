/**
 * @file SimpleIR.c
 * @brief Simple IR Library Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "IR.h"
#include "debug.h"
#include <string.h>

/* ========== Private Variables ========== */

// Receiver state
static struct {
    uint8_t           pin;              // GPIO pin สำหรับ receiver
    IR_Callback_t     callback;         // User callback function
    IR_RawData_t      raw_data;         // Raw timing buffer
    IR_DecodedData_t  decoded_data;     // Decoded data buffer
    uint32_t          last_edge_time;   // เวลาของ edge ล่าสุด (us)
    uint8_t           edge_count;       // จำนวน edge ที่ตรวจพบ
    bool              receiving;        // กำลังรับข้อมูลอยู่หรือไม่
    bool              data_ready;       // มีข้อมูลใหม่พร้อมอ่านหรือไม่
    bool              enabled;          // Receiver เปิดใช้งานหรือไม่
} ir_rx;

// Transmitter state
static struct {
    uint8_t pin;        // GPIO pin สำหรับ transmitter
    bool    initialized; // Transmitter ถูกเริ่มต้นแล้วหรือไม่
} ir_tx;

/* ========== Private Function Prototypes ========== */

static void IR_EdgeInterrupt(void);
static void IR_Mark(uint16_t us);
static void IR_Space(uint16_t us);
static bool IR_DecodeAuto(IR_RawData_t* raw, IR_DecodedData_t* decoded);

/* ========== IR Receiver Implementation ========== */

/**
 * @brief เริ่มต้น IR receiver
 */
void IR_ReceiverInit(uint8_t pin, IR_Callback_t callback) {
    ir_rx.pin = pin;
    ir_rx.callback = callback;
    ir_rx.enabled = true;
    ir_rx.receiving = false;
    ir_rx.data_ready = false;
    ir_rx.edge_count = 0;
    
    // ตั้งค่า pin เป็น input
    pinMode(pin, PIN_MODE_INPUT);
    
    // ตั้งค่า external interrupt (trigger on change)
    attachInterrupt(pin, IR_EdgeInterrupt, CHANGE);
    
    // เคลียร์ buffer
    memset(&ir_rx.raw_data, 0, sizeof(IR_RawData_t));
    memset(&ir_rx.decoded_data, 0, sizeof(IR_DecodedData_t));
}

/**
 * @brief Interrupt handler สำหรับ edge detection
 */
static void IR_EdgeInterrupt(void) {
    if (!ir_rx.enabled) return;
    
    uint32_t current_time = Get_CurrentUs();
    
    // คำนวณระยะเวลาตั้งแต่ edge ล่าสุด
    uint32_t duration = current_time - ir_rx.last_edge_time;
    
    // ถ้าเป็น edge แรก หรือ timeout (เริ่มข้อมูลใหม่)
    if (!ir_rx.receiving || duration > IR_TIMEOUT_US) {
        ir_rx.receiving = true;
        ir_rx.edge_count = 0;
        ir_rx.raw_data.count = 0;
        ir_rx.raw_data.overflow = false;
    } else {
        // เก็บ timing data
        if (ir_rx.raw_data.count < IR_RAW_BUFFER_SIZE) {
            ir_rx.raw_data.timings[ir_rx.raw_data.count++] = (uint16_t)duration;
        } else {
            ir_rx.raw_data.overflow = true;
        }
    }
    
    ir_rx.last_edge_time = current_time;
    ir_rx.edge_count++;
}

/**
 * @brief ประมวลผลข้อมูล IR
 */
void IR_Process(void) {
    if (!ir_rx.enabled || !ir_rx.receiving) return;
    
    // ตรวจสอบ timeout (จบการรับข้อมูล)
    uint32_t elapsed = Get_CurrentUs() - ir_rx.last_edge_time;
    
    if (elapsed > IR_TIMEOUT_US && ir_rx.raw_data.count > 0) {
        ir_rx.receiving = false;
        
        // พยายาม decode ข้อมูล
        if (IR_DecodeAuto(&ir_rx.raw_data, &ir_rx.decoded_data)) {
            ir_rx.data_ready = true;
            
            // เรียก callback ถ้ามี
            if (ir_rx.callback != NULL) {
                ir_rx.callback(&ir_rx.decoded_data);
            }
        }
    }
}

/**
 * @brief ตรวจสอบว่ามีข้อมูลใหม่หรือไม่
 */
bool IR_Available(void) {
    return ir_rx.data_ready;
}

/**
 * @brief อ่านข้อมูลที่ decode แล้ว
 */
IR_DecodedData_t IR_GetData(void) {
    ir_rx.data_ready = false;
    return ir_rx.decoded_data;
}

/**
 * @brief อ่าน raw timing data
 */
IR_RawData_t* IR_GetRawData(void) {
    return &ir_rx.raw_data;
}

/**
 * @brief เปิดการทำงานของ receiver
 */
void IR_EnableReceiver(void) {
    ir_rx.enabled = true;
}

/**
 * @brief ปิดการทำงานของ receiver
 */
void IR_DisableReceiver(void) {
    ir_rx.enabled = false;
}

/**
 * @brief รีเซ็ต receiver state
 */
void IR_Reset(void) {
    ir_rx.receiving = false;
    ir_rx.data_ready = false;
    ir_rx.edge_count = 0;
    ir_rx.raw_data.count = 0;
    ir_rx.raw_data.overflow = false;
}

/* ========== IR Transmitter Implementation ========== */

/**
 * @brief เริ่มต้น IR transmitter
 */
void IR_TransmitterInit(uint8_t pin) {
    ir_tx.pin = pin;
    ir_tx.initialized = true;
    
    // ตั้งค่า PWM สำหรับ 38kHz carrier
    // Note: SimplePWM uses channel enum, not pin directly
    // For now, we'll use PWM1_CH1 (PD2) as default
    // TODO: Map pin to appropriate PWM channel
    PWM_Channel pwm_ch = PWM1_CH1;  // Default channel
    
    PWM_Init(pwm_ch, IR_CARRIER_FREQ);
    PWM_SetDutyCycle(pwm_ch, 33);  // Duty cycle 33%
    PWM_Stop(pwm_ch);  // ปิด carrier ก่อน
}

/**
 * @brief ส่ง mark (carrier on)
 */
static void IR_Mark(uint16_t us) {
    PWM_Start(ir_tx.pin);
    Delay_Us(us);
    PWM_Stop(ir_tx.pin);
}

/**
 * @brief ส่ง space (carrier off)
 */
static void IR_Space(uint16_t us) {
    Delay_Us(us);
}

/**
 * @brief ส่งข้อมูล IR ตามโปรโตคอล
 */
bool IR_Send(IR_Protocol_t protocol, uint16_t address, uint16_t command) {
    if (!ir_tx.initialized) return false;
    
    switch (protocol) {
        case IR_PROTOCOL_NEC: {
            // NEC Protocol:
            // - Lead: 9ms mark + 4.5ms space
            // - Data: 32 bits (address + ~address + command + ~command)
            // - Bit 0: 560us mark + 560us space
            // - Bit 1: 560us mark + 1690us space
            
            // Lead pulse
            IR_Mark(9000);
            IR_Space(4500);
            
            // ส่ง address (8 bits)
            for (int i = 0; i < 8; i++) {
                IR_Mark(560);
                if (address & (1 << i)) {
                    IR_Space(1690);  // Bit 1
                } else {
                    IR_Space(560);   // Bit 0
                }
            }
            
            // ส่ง ~address (8 bits)
            uint8_t inv_address = ~address;
            for (int i = 0; i < 8; i++) {
                IR_Mark(560);
                if (inv_address & (1 << i)) {
                    IR_Space(1690);
                } else {
                    IR_Space(560);
                }
            }
            
            // ส่ง command (8 bits)
            for (int i = 0; i < 8; i++) {
                IR_Mark(560);
                if (command & (1 << i)) {
                    IR_Space(1690);
                } else {
                    IR_Space(560);
                }
            }
            
            // ส่ง ~command (8 bits)
            uint8_t inv_command = ~command;
            for (int i = 0; i < 8; i++) {
                IR_Mark(560);
                if (inv_command & (1 << i)) {
                    IR_Space(1690);
                } else {
                    IR_Space(560);
                }
            }
            
            // Stop bit
            IR_Mark(560);
            
            return true;
        }
        
        case IR_PROTOCOL_RC5: {
            // RC5 Protocol (Manchester encoding)
            // - 14 bits total
            // - Bit time: 1778us (889us + 889us)
            // - Start bits: 2 bits (always 1)
            // - Toggle bit: 1 bit
            // - Address: 5 bits
            // - Command: 6 bits
            
            static uint8_t toggle = 0;
            uint16_t data = 0;
            
            // สร้าง 14-bit data
            data |= (1 << 13);              // Start bit 1
            data |= (1 << 12);              // Start bit 2
            data |= (toggle << 11);         // Toggle bit
            data |= ((address & 0x1F) << 6); // Address (5 bits)
            data |= (command & 0x3F);       // Command (6 bits)
            
            // ส่งข้อมูลแบบ Manchester encoding
            for (int i = 13; i >= 0; i--) {
                if (data & (1 << i)) {
                    // Bit 1: space then mark
                    IR_Space(889);
                    IR_Mark(889);
                } else {
                    // Bit 0: mark then space
                    IR_Mark(889);
                    IR_Space(889);
                }
            }
            
            toggle ^= 1;  // Toggle bit สำหรับครั้งถัดไป
            return true;
        }
        
        case IR_PROTOCOL_SIRC: {
            // Sony SIRC Protocol (12-bit version)
            // - Lead: 2400us mark + 600us space
            // - Bit 0: 600us mark + 600us space
            // - Bit 1: 1200us mark + 600us space
            // - Command: 7 bits
            // - Address: 5 bits
            
            // Lead pulse
            IR_Mark(2400);
            IR_Space(600);
            
            // ส่ง command (7 bits, LSB first)
            for (int i = 0; i < 7; i++) {
                if (command & (1 << i)) {
                    IR_Mark(1200);  // Bit 1
                } else {
                    IR_Mark(600);   // Bit 0
                }
                IR_Space(600);
            }
            
            // ส่ง address (5 bits, LSB first)
            for (int i = 0; i < 5; i++) {
                if (address & (1 << i)) {
                    IR_Mark(1200);
                } else {
                    IR_Mark(600);
                }
                IR_Space(600);
            }
            
            return true;
        }
        
        default:
            return false;
    }
}

/**
 * @brief ส่ง raw timing data
 */
bool IR_SendRaw(const uint16_t* timings, uint16_t count) {
    if (!ir_tx.initialized || timings == NULL) return false;
    
    for (uint16_t i = 0; i < count; i++) {
        if (i % 2 == 0) {
            IR_Mark(timings[i]);   // Mark (even index)
        } else {
            IR_Space(timings[i]);  // Space (odd index)
        }
    }
    
    return true;
}

/**
 * @brief ส่ง NEC repeat code
 */
void IR_SendRepeat(void) {
    // NEC Repeat: 9ms mark + 2.25ms space + 560us mark
    IR_Mark(9000);
    IR_Space(2250);
    IR_Mark(560);
}

/* ========== Protocol Decode Functions ========== */

/**
 * @brief Auto-detect และ decode โปรโตคอล
 */
static bool IR_DecodeAuto(IR_RawData_t* raw, IR_DecodedData_t* decoded) {
    // ลองแต่ละโปรโตคอล
    if (IR_DecodeNEC(raw, decoded)) return true;
    if (IR_DecodeRC5(raw, decoded)) return true;
    if (IR_DecodeSIRC(raw, decoded)) return true;
    
    // ถ้า decode ไม่ได้ เก็บเป็น raw
    decoded->protocol = IR_PROTOCOL_RAW;
    decoded->valid = false;
    return false;
}

/**
 * @brief Decode NEC protocol
 */
bool IR_DecodeNEC(IR_RawData_t* raw, IR_DecodedData_t* decoded) {
    // NEC ต้องมีอย่างน้อย 67 timings (lead + 32 bits + stop)
    if (raw->count < 67) return false;
    
    // ตรวจสอบ lead pulse (9ms mark + 4.5ms space)
    if (!IR_MATCH(raw->timings[0], 9000, IR_TOLERANCE)) return false;
    
    // ตรวจสอบว่าเป็น repeat code หรือไม่
    if (IR_MATCH(raw->timings[1], 2250, IR_TOLERANCE)) {
        decoded->protocol = IR_PROTOCOL_NEC_REPEAT;
        decoded->valid = true;
        return true;
    }
    
    // ตรวจสอบ space หลัง lead
    if (!IR_MATCH(raw->timings[1], 4500, IR_TOLERANCE)) return false;
    
    // Decode 32 bits
    uint32_t data = 0;
    for (int i = 0; i < 32; i++) {
        int idx = 2 + (i * 2);
        
        // ตรวจสอบ mark (ควรเป็น 560us)
        if (!IR_MATCH(raw->timings[idx], 560, IR_TOLERANCE)) return false;
        
        // ตรวจสอบ space เพื่อแยก bit 0/1
        if (IR_MATCH(raw->timings[idx + 1], 1690, IR_TOLERANCE)) {
            data |= (1UL << i);  // Bit 1
        } else if (!IR_MATCH(raw->timings[idx + 1], 560, IR_TOLERANCE)) {
            return false;  // Space ไม่ตรงทั้ง bit 0 และ bit 1
        }
    }
    
    // แยกข้อมูล
    uint8_t address = data & 0xFF;
    uint8_t inv_address = (data >> 8) & 0xFF;
    uint8_t command = (data >> 16) & 0xFF;
    uint8_t inv_command = (data >> 24) & 0xFF;
    
    // ตรวจสอบ inverse bits
    if ((address != (uint8_t)~inv_address) || (command != (uint8_t)~inv_command)) {
        return false;  // ข้อมูลไม่ถูกต้อง
    }
    
    // เก็บผลลัพธ์
    decoded->protocol = IR_PROTOCOL_NEC;
    decoded->address = address;
    decoded->command = command;
    decoded->bits = 32;
    decoded->valid = true;
    
    return true;
}

/**
 * @brief Decode RC5 protocol
 */
bool IR_DecodeRC5(IR_RawData_t* raw, IR_DecodedData_t* decoded) {
    // RC5 ใช้ Manchester encoding, มี 14 bits
    // แต่ละ bit มี 2 transitions = 28 timings
    if (raw->count < 26 || raw->count > 30) return false;
    
    // ตรวจสอบ bit time (~889us)
    if (!IR_MATCH(raw->timings[0], 889, IR_TOLERANCE * 2)) return false;
    
    uint16_t data = 0;
    int bit_count = 0;
    
    // Decode Manchester encoding
    for (int i = 0; i < raw->count - 1 && bit_count < 14; i += 2) {
        uint16_t t1 = raw->timings[i];
        uint16_t t2 = raw->timings[i + 1];
        
        // ตรวจสอบว่าเป็น half-bit time หรือไม่
        bool half1 = IR_MATCH(t1, 889, IR_TOLERANCE * 2);
        bool half2 = IR_MATCH(t2, 889, IR_TOLERANCE * 2);
        
        if (half1 && half2) {
            // Short-Long = Bit 0
            // Long-Short = Bit 1
            if (t1 < t2) {
                data |= (1 << (13 - bit_count));
            }
            bit_count++;
        }
    }
    
    if (bit_count != 14) return false;
    
    // แยกข้อมูล RC5
    // Bit 13-12: Start bits (ควรเป็น 11)
    // Bit 11: Toggle bit
    // Bit 10-6: Address (5 bits)
    // Bit 5-0: Command (6 bits)
    
    if (((data >> 12) & 0x3) != 0x3) return false;  // Start bits ไม่ถูกต้อง
    
    decoded->protocol = IR_PROTOCOL_RC5;
    decoded->address = (data >> 6) & 0x1F;
    decoded->command = data & 0x3F;
    decoded->bits = 14;
    decoded->valid = true;
    
    return true;
}

/**
 * @brief Decode SIRC (Sony) protocol
 */
bool IR_DecodeSIRC(IR_RawData_t* raw, IR_DecodedData_t* decoded) {
    // SIRC 12-bit: lead + 12 bits = 25 timings
    if (raw->count < 24 || raw->count > 26) return false;
    
    // ตรวจสอบ lead pulse (2400us)
    if (!IR_MATCH(raw->timings[0], 2400, IR_TOLERANCE)) return false;
    if (!IR_MATCH(raw->timings[1], 600, IR_TOLERANCE)) return false;
    
    uint16_t data = 0;
    int bits = (raw->count - 2) / 2;
    
    // Decode bits (LSB first)
    for (int i = 0; i < bits; i++) {
        int idx = 2 + (i * 2);
        
        // ตรวจสอบ mark
        if (IR_MATCH(raw->timings[idx], 1200, IR_TOLERANCE)) {
            data |= (1 << i);  // Bit 1
        } else if (!IR_MATCH(raw->timings[idx], 600, IR_TOLERANCE)) {
            return false;
        }
        
        // ตรวจสอบ space (600us)
        if (!IR_MATCH(raw->timings[idx + 1], 600, IR_TOLERANCE)) return false;
    }
    
    // แยกข้อมูล (12-bit version)
    decoded->protocol = IR_PROTOCOL_SIRC;
    decoded->command = data & 0x7F;        // 7 bits
    decoded->address = (data >> 7) & 0x1F; // 5 bits
    decoded->bits = bits;
    decoded->valid = true;
    
    return true;
}

/* ========== Utility Functions ========== */

/**
 * @brief แสดง raw timing data
 */
void IR_PrintRawData(void) {
    printf("Raw IR Data (%d timings):\n", ir_rx.raw_data.count);
    
    if (ir_rx.raw_data.overflow) {
        printf("WARNING: Buffer overflow!\n");
    }
    
    for (int i = 0; i < ir_rx.raw_data.count; i++) {
        printf("%d ", ir_rx.raw_data.timings[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

/**
 * @brief แสดงข้อมูลที่ decode แล้ว
 */
void IR_PrintDecodedData(IR_DecodedData_t* data) {
    if (data == NULL) return;
    
    printf("Protocol: %s\n", IR_PROTOCOL_NAME(data->protocol));
    
    if (data->valid) {
        printf("Address: 0x%02X\n", data->address);
        printf("Command: 0x%02X\n", data->command);
        printf("Bits: %d\n", data->bits);
    } else {
        printf("Invalid data\n");
    }
}
