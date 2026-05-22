/**
 * @file SimpleIR.h
 * @brief Simple IR (Infrared) Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library นี้ให้ API สำหรับการรับและส่งสัญญาณ IR
 * รองรับโปรโตคอล NEC, RC5, SIRC และ raw data capture
 * 
 * **คุณสมบัติ:**
 * - รับสัญญาณ IR ด้วย External Interrupt
 * - วัดเวลา pulse ด้วย microsecond precision
 * - Decode โปรโตคอล NEC, RC5, SIRC
 * - ส่งสัญญาณ IR ด้วย PWM 38kHz carrier
 * - Raw timing capture สำหรับวิเคราะห์โปรโตคอลใหม่
 * - Non-blocking operation
 * 
 * **Hardware Requirements:**
 * - IR Receiver: TSOP38238, VS1838B หรือเทียบเท่า
 * - IR Transmitter: IR LED 940nm + NPN transistor
 * 
 * @example
 * // ตัวอย่างการรับสัญญาณ NEC
 * #include "IR.h"
 * 
 * void ir_callback(IR_DecodedData_t* data) {
 *     printf("Protocol: %d, Address: 0x%02X, Command: 0x%02X\n", 
 *            data->protocol, data->address, data->command);
 * }
 * 
 * int main(void) {
 *     Timer_Init();
 *     IR_ReceiverInit(PC1, ir_callback);
 *     while(1) {
 *         IR_Process();  // ประมวลผลข้อมูล IR
 *     }
 * }
 * 
 * @note ต้องเรียก Timer_Init() ก่อนใช้งาน IR library
 */

#ifndef __IR_H
#define __IR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief ขนาดสูงสุดของ raw timing buffer
 * @note เพิ่มค่านี้หากต้องการรับข้อมูลที่ยาวกว่า
 */
#define IR_RAW_BUFFER_SIZE  200

/**
 * @brief Carrier frequency สำหรับ IR transmitter (Hz)
 * @note ค่ามาตรฐานคือ 38kHz
 */
#define IR_CARRIER_FREQ     38000

/**
 * @brief Timeout สำหรับการรับข้อมูล (microseconds)
 * @note หากไม่มีสัญญาณเกินเวลานี้ จะถือว่าจบข้อมูล
 */
#define IR_TIMEOUT_US       100000  // 100ms

/**
 * @brief Tolerance สำหรับการเปรียบเทียบ timing (%)
 * @note ใช้สำหรับ decode โปรโตคอล
 */
#define IR_TOLERANCE        25

/* ========== IR Protocol Definitions ========== */

/**
 * @brief IR Protocol types
 */
typedef enum {
    IR_PROTOCOL_UNKNOWN = 0,  /**< โปรโตคอลไม่รู้จัก */
    IR_PROTOCOL_NEC,          /**< NEC protocol (32-bit) */
    IR_PROTOCOL_NEC_REPEAT,   /**< NEC repeat code */
    IR_PROTOCOL_RC5,          /**< Philips RC5 (14-bit) */
    IR_PROTOCOL_SIRC,         /**< Sony SIRC (12/15/20-bit) */
    IR_PROTOCOL_RAW           /**< Raw timing data */
} IR_Protocol_t;

/* ========== Data Structures ========== */

/**
 * @brief โครงสร้างข้อมูล Raw IR timing
 * 
 * @details เก็บข้อมูล pulse และ space duration ในหน่วย microseconds
 *          - timings[0] = pulse 1
 *          - timings[1] = space 1
 *          - timings[2] = pulse 2
 *          - ...
 */
typedef struct {
    uint16_t timings[IR_RAW_BUFFER_SIZE];  /**< Raw timing data (us) */
    uint16_t count;                         /**< จำนวน timing ที่เก็บ */
    bool     overflow;                      /**< Buffer overflow flag */
} IR_RawData_t;

/**
 * @brief โครงสร้างข้อมูล IR ที่ decode แล้ว
 */
typedef struct {
    IR_Protocol_t protocol;  /**< โปรโตคอลที่ตรวจพบ */
    uint16_t      address;   /**< Address/Device code */
    uint16_t      command;   /**< Command/Function code */
    uint8_t       bits;      /**< จำนวน bits ของข้อมูล */
    bool          valid;     /**< ข้อมูลถูกต้องหรือไม่ */
} IR_DecodedData_t;

/**
 * @brief Callback function type สำหรับเมื่อรับข้อมูล IR
 * @param data ข้อมูลที่ decode แล้ว
 */
typedef void (*IR_Callback_t)(IR_DecodedData_t* data);

/* ========== IR Receiver Functions ========== */

/**
 * @brief เริ่มต้น IR receiver
 * @param pin GPIO pin สำหรับต่อ IR receiver (แนะนำ PC1)
 * @param callback ฟังก์ชันที่จะถูกเรียกเมื่อรับข้อมูลสำเร็จ
 * 
 * @note Pin จะถูกตั้งเป็น INPUT_PULLUP mode
 * @note ต้องเรียก Timer_Init() ก่อนใช้งานฟังก์ชันนี้
 * 
 * @example
 * void my_callback(IR_DecodedData_t* data) {
 *     printf("Received: 0x%02X\n", data->command);
 * }
 * IR_ReceiverInit(PC1, my_callback);
 */
void IR_ReceiverInit(uint8_t pin, IR_Callback_t callback);

/**
 * @brief ประมวลผลข้อมูล IR ที่รับมา
 * 
 * @note ต้องเรียกฟังก์ชันนี้ใน main loop เป็นประจำ
 * @note ฟังก์ชันนี้จะ decode ข้อมูลและเรียก callback หากพบข้อมูลใหม่
 * 
 * @example
 * while(1) {
 *     IR_Process();
 *     // ... other tasks
 * }
 */
void IR_Process(void);

/**
 * @brief ตรวจสอบว่ามีข้อมูล IR ใหม่หรือไม่
 * @return true = มีข้อมูลใหม่, false = ไม่มี
 * 
 * @example
 * if(IR_Available()) {
 *     IR_DecodedData_t data = IR_GetData();
 *     printf("Command: 0x%02X\n", data.command);
 * }
 */
bool IR_Available(void);

/**
 * @brief อ่านข้อมูล IR ที่ decode แล้ว
 * @return ข้อมูล IR ที่ decode แล้ว
 * 
 * @note ควรเรียก IR_Available() ก่อนเพื่อตรวจสอบว่ามีข้อมูลใหม่
 * 
 * @example
 * if(IR_Available()) {
 *     IR_DecodedData_t data = IR_GetData();
 * }
 */
IR_DecodedData_t IR_GetData(void);

/**
 * @brief อ่าน raw timing data
 * @return ตัวชี้ไปยัง raw data structure
 * 
 * @note ใช้สำหรับการวิเคราะห์โปรโตคอลใหม่
 * @note ข้อมูลจะถูกเขียนทับเมื่อมีสัญญาณใหม่
 * 
 * @example
 * IR_RawData_t* raw = IR_GetRawData();
 * for(int i = 0; i < raw->count; i++) {
 *     printf("%d ", raw->timings[i]);
 * }
 */
IR_RawData_t* IR_GetRawData(void);

/**
 * @brief เปิดการทำงานของ IR receiver
 * 
 * @note Receiver จะเปิดอัตโนมัติหลัง IR_ReceiverInit()
 * 
 * @example
 * IR_EnableReceiver();
 */
void IR_EnableReceiver(void);

/**
 * @brief ปิดการทำงานของ IR receiver
 * 
 * @note ใช้เพื่อประหยัดพลังงานหรือหยุดรับข้อมูลชั่วคราว
 * 
 * @example
 * IR_DisableReceiver();  // หยุดรับสัญญาณ
 */
void IR_DisableReceiver(void);

/* ========== IR Transmitter Functions ========== */

/**
 * @brief เริ่มต้น IR transmitter
 * @param pin GPIO pin สำหรับต่อ IR LED (แนะนำ PC6 สำหรับ TIM1_CH1)
 * 
 * @note Pin จะถูกตั้งเป็น PWM output ที่ 38kHz
 * @note ใช้ SimplePWM สำหรับสร้าง carrier frequency
 * 
 * @example
 * IR_TransmitterInit(PC6);
 */
void IR_TransmitterInit(uint8_t pin);

/**
 * @brief ส่งข้อมูล IR ตามโปรโตคอลที่กำหนด
 * @param protocol โปรโตคอลที่ต้องการส่ง
 * @param address Address/Device code
 * @param command Command/Function code
 * @return true = ส่งสำเร็จ, false = ส่งไม่สำเร็จ
 * 
 * @note ฟังก์ชันนี้จะบล็อกการทำงานจนกว่าจะส่งเสร็จ
 * 
 * @example
 * // ส่งคำสั่ง NEC: address=0x00, command=0x12
 * IR_Send(IR_PROTOCOL_NEC, 0x00, 0x12);
 */
bool IR_Send(IR_Protocol_t protocol, uint16_t address, uint16_t command);

/**
 * @brief ส่ง raw timing data
 * @param timings array ของ timing data (microseconds)
 * @param count จำนวน timing ใน array
 * @return true = ส่งสำเร็จ, false = ส่งไม่สำเร็จ
 * 
 * @note timings[0] = pulse 1, timings[1] = space 1, ...
 * @note ใช้สำหรับส่งโปรโตคอลที่ไม่ได้รองรับโดยตรง
 * 
 * @example
 * uint16_t custom_signal[] = {9000, 4500, 560, 560, 560, 1690};
 * IR_SendRaw(custom_signal, 6);
 */
bool IR_SendRaw(const uint16_t* timings, uint16_t count);

/**
 * @brief ส่ง NEC repeat code
 * 
 * @note ใช้สำหรับส่งสัญญาณซ้ำเมื่อกดปุ่มค้าง
 * 
 * @example
 * IR_Send(IR_PROTOCOL_NEC, 0x00, 0x12);  // ส่งครั้งแรก
 * Delay_Ms(110);
 * IR_SendRepeat();  // ส่งซ้ำ
 */
void IR_SendRepeat(void);

/* ========== Protocol Decode Functions ========== */

/**
 * @brief Decode NEC protocol
 * @param raw ข้อมูล raw timing
 * @param decoded ตัวชี้สำหรับเก็บข้อมูลที่ decode แล้ว
 * @return true = decode สำเร็จ, false = ไม่สำเร็จ
 * 
 * @note ใช้ภายใน library, ผู้ใช้ไม่จำเป็นต้องเรียกโดยตรง
 */
bool IR_DecodeNEC(IR_RawData_t* raw, IR_DecodedData_t* decoded);

/**
 * @brief Decode RC5 protocol
 * @param raw ข้อมูล raw timing
 * @param decoded ตัวชี้สำหรับเก็บข้อมูลที่ decode แล้ว
 * @return true = decode สำเร็จ, false = ไม่สำเร็จ
 */
bool IR_DecodeRC5(IR_RawData_t* raw, IR_DecodedData_t* decoded);

/**
 * @brief Decode SIRC (Sony) protocol
 * @param raw ข้อมูล raw timing
 * @param decoded ตัวชี้สำหรับเก็บข้อมูลที่ decode แล้ว
 * @return true = decode สำเร็จ, false = ไม่สำเร็จ
 */
bool IR_DecodeSIRC(IR_RawData_t* raw, IR_DecodedData_t* decoded);

/* ========== Utility Functions ========== */

/**
 * @brief แสดง raw timing data ทาง UART
 * 
 * @note ใช้สำหรับ debug และวิเคราะห์โปรโตคอล
 * @note ต้องเปิดใช้งาน UART ก่อน
 * 
 * @example
 * IR_PrintRawData();
 * // Output: Raw IR Data (48 timings):
 * //         9000 4500 560 560 560 1690 ...
 */
void IR_PrintRawData(void);

/**
 * @brief แสดงข้อมูลที่ decode แล้วทาง UART
 * @param data ข้อมูลที่ต้องการแสดง
 * 
 * @example
 * IR_DecodedData_t data = IR_GetData();
 * IR_PrintDecodedData(&data);
 * // Output: Protocol: NEC, Address: 0x00, Command: 0x12
 */
void IR_PrintDecodedData(IR_DecodedData_t* data);

/**
 * @brief รีเซ็ต IR receiver state
 * 
 * @note ใช้เมื่อต้องการล้างข้อมูลและเริ่มรับใหม่
 * 
 * @example
 * IR_Reset();
 */
void IR_Reset(void);

/* ========== Helper Macros ========== */

/**
 * @brief ตรวจสอบว่า timing อยู่ในช่วงที่กำหนดหรือไม่
 * @param value ค่าที่ต้องการตรวจสอบ
 * @param target ค่าเป้าหมาย
 * @param tolerance ความคลาดเคลื่อนที่ยอมรับได้ (%)
 */
#define IR_MATCH(value, target, tolerance) \
    ((value) >= ((target) * (100 - (tolerance)) / 100) && \
     (value) <= ((target) * (100 + (tolerance)) / 100))

/**
 * @brief แปลง protocol enum เป็น string
 */
#define IR_PROTOCOL_NAME(protocol) \
    ((protocol) == IR_PROTOCOL_NEC ? "NEC" : \
     (protocol) == IR_PROTOCOL_NEC_REPEAT ? "NEC_REPEAT" : \
     (protocol) == IR_PROTOCOL_RC5 ? "RC5" : \
     (protocol) == IR_PROTOCOL_SIRC ? "SIRC" : \
     (protocol) == IR_PROTOCOL_RAW ? "RAW" : "UNKNOWN")

#ifdef __cplusplus
}
#endif

#endif  // __IR_H
