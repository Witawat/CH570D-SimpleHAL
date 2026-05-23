#ifndef __SIMPLE_1WIRE_H
#define __SIMPLE_1WIRE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

#define ONEWIRE_MAX_BUSES  4

/**
 * @brief   ระยะเวลา reset pulse (µs)
 */
#define ONEWIRE_RESET_PULSE       480
/**
 * @brief   รอสัญญาณ presence หลังจาก reset (µs)
 */
#define ONEWIRE_PRESENCE_WAIT     70
/**
 * @brief   timeout รอ presence (µs)
 */
#define ONEWIRE_PRESENCE_TIMEOUT  240
/**
 * @brief   ระยะเวลาดึง LOW เมื่อเขียน bit 0 (µs)
 */
#define ONEWIRE_WRITE_0_LOW       60
/**
 * @brief   ระยะเวลาดึง LOW เมื่อเขียน bit 1 (µs)
 */
#define ONEWIRE_WRITE_1_LOW       10
/**
 * @brief   ระยะพักหลังเขียนแต่ละ bit (µs)
 */
#define ONEWIRE_WRITE_RECOVERY    1
/**
 * @brief   ระยะเวลาดึง LOW ก่อนอ่าน bit (µs)
 */
#define ONEWIRE_READ_LOW          3
/**
 * @brief   รอสัญญาณหลังจากดึง LOW (µs)
 */
#define ONEWIRE_READ_WAIT         10
/**
 * @brief   ระยะพักหลังอ่านแต่ละ bit (µs)
 */
#define ONEWIRE_READ_RECOVERY     55
/**
 * @brief   ระยะเวลารวมของ 1 time slot (µs)
 */
#define ONEWIRE_SLOT_TIME         65

/**
 * @brief   คำสั่ง Skip ROM — เลือกอุปกรณ์ทั้งหมดบนบัส
 */
#define ONEWIRE_CMD_SKIP_ROM      0xCC
/**
 * @brief   คำสั่ง Read ROM — อ่าน ROM ของอุปกรณ์เดียวบนบัส
 */
#define ONEWIRE_CMD_READ_ROM      0x33
/**
 * @brief   คำสั่ง Match ROM — เลือกอุปกรณ์ตาม ROM address
 */
#define ONEWIRE_CMD_MATCH_ROM     0x55
/**
 * @brief   คำสั่ง Search ROM — ค้นหาอุปกรณ์ทั้งหมดบนบัส
 */
#define ONEWIRE_CMD_SEARCH_ROM    0xF0
/**
 * @brief   คำสั่ง Alarm Search — ค้นหาอุปกรณ์ที่อยู่ในสถานะ alarm
 */
#define ONEWIRE_CMD_ALARM_SEARCH  0xEC

/**
 * @brief   โครงสร้างข้อมูลบัส 1-Wire หนึ่งบัส
 *
 * @param   pin - หมายเลขขาที่ใช้
 * @param   rom - ROM address 8 ไบต์ของอุปกรณ์ล่าสุด
 * @param   last_discrepancy - จุดที่พบความแตกต่างในการค้นหาครั้งล่าสุด
 * @param   last_family_discrepancy - จุดที่พบความแตกต่างใน family byte
 * @param   last_device_flag - true เมื่อค้นพบอุปกรณ์สุดท้ายแล้ว
 * @param   initialized - true เมื่อบัสนี้เริ่มต้นแล้ว
 */
typedef struct {
    uint8_t pin;
    uint8_t rom[8];
    uint8_t last_discrepancy;
    uint8_t last_family_discrepancy;
    bool last_device_flag;
    bool initialized;
} OneWire_Bus;

/**
 * @brief   เริ่มต้นบัส 1-Wire บนขาที่กำหนด
 *
 * @param   pin - หมายเลขขาที่ต้องการใช้
 *
 * @return  pointer ไปยัง OneWire_Bus หรือ NULL หากบัสเต็ม
 */
OneWire_Bus* OneWire_Init(uint8_t pin);

/**
 * @brief   ส่ง reset pulse และตรวจสอบ presence
 *
 * @param   bus - pointer ไปยังบัส
 *
 * @return  true ถ้ามีอุปกรณ์ตอบสนอง
 */
bool OneWire_Reset(OneWire_Bus* bus);

/**
 * @brief   เขียน 1 bit
 *
 * @param   bus - pointer ไปยังบัส
 * @param   bit - ค่า 0 หรือ 1
 */
void OneWire_WriteBit(OneWire_Bus* bus, uint8_t bit);

/**
 * @brief   อ่าน 1 bit
 *
 * @param   bus - pointer ไปยังบัส
 *
 * @return  ค่า 0 หรือ 1
 */
uint8_t OneWire_ReadBit(OneWire_Bus* bus);

/**
 * @brief   เขียน 1 ไบต์ (LSB first)
 *
 * @param   bus - pointer ไปยังบัส
 * @param   data - ค่าที่ต้องการเขียน
 */
void OneWire_WriteByte(OneWire_Bus* bus, uint8_t data);

/**
 * @brief   อ่าน 1 ไบต์ (LSB first)
 *
 * @param   bus - pointer ไปยังบัส
 *
 * @return  ค่าที่อ่านได้
 */
uint8_t OneWire_ReadByte(OneWire_Bus* bus);

/**
 * @brief   เขียนหลายไบต์
 *
 * @param   bus - pointer ไปยังบัส
 * @param   data - pointer ข้อมูล
 * @param   len - จำนวนไบต์
 */
void OneWire_WriteBytes(OneWire_Bus* bus, const uint8_t* data, uint8_t len);

/**
 * @brief   อ่านหลายไบต์
 *
 * @param   bus - pointer ไปยังบัส
 * @param   buffer - buffer สำหรับรับข้อมูล
 * @param   len - จำนวนไบต์ที่ต้องการอ่าน
 */
void OneWire_ReadBytes(OneWire_Bus* bus, uint8_t* buffer, uint8_t len);

/**
 * @brief   ส่งคำสั่ง Skip ROM — เลือกอุปกรณ์ทั้งหมด
 *
 * @param   bus - pointer ไปยังบัส
 */
void OneWire_SkipROM(OneWire_Bus* bus);

/**
 * @brief   ส่งคำสั่ง Read ROM — อ่าน ROM ของอุปกรณ์เดียวบนบัส
 *
 * @param   bus - pointer ไปยังบัส
 * @param   rom - buffer ขนาด 8 ไบต์สำหรับรับ ROM
 *
 * @return  true ถ้า CRC ถูกต้อง
 */
bool OneWire_ReadROM(OneWire_Bus* bus, uint8_t* rom);

/**
 * @brief   ส่งคำสั่ง Match ROM — เลือกอุปกรณ์เฉพาะ
 *
 * @param   bus - pointer ไปยังบัส
 * @param   rom - ROM address 8 ไบต์ของอุปกรณ์เป้าหมาย
 */
void OneWire_MatchROM(OneWire_Bus* bus, const uint8_t* rom);

/**
 * @brief   Reset + Match ROM — เลือกอุปกรณ์
 *
 * @param   bus - pointer ไปยังบัส
 * @param   rom - ROM address 8 ไบต์
 *
 * @return  true ถ้า reset สำเร็จ
 */
bool OneWire_Select(OneWire_Bus* bus, const uint8_t* rom);

/**
 * @brief   รีเซ็ตสถานะการค้นหา — พร้อมค้นหาใหม่
 *
 * @param   bus - pointer ไปยังบัส
 */
void OneWire_ResetSearch(OneWire_Bus* bus);

/**
 * @brief   ค้นหาอุปกรณ์ถัดไปบนบัส (Search ROM)
 *
 * @param   bus - pointer ไปยังบัส
 *
 * @return  true เมื่อพบอุปกรณ์, false หากหมดแล้ว
 */
bool OneWire_Search(OneWire_Bus* bus);

/**
 * @brief   ค้นหาอุปกรณ์ที่อยู่ในสถานะ alarm
 *
 * @param   bus - pointer ไปยังบัส
 *
 * @return  true เมื่อพบ, false หากหมดแล้ว
 */
bool OneWire_AlarmSearch(OneWire_Bus* bus);

/**
 * @brief   อ่าน ROM address ของอุปกรณ์ล่าสุดที่ค้นพบ
 *
 * @param   bus - pointer ไปยังบัส
 * @param   rom - buffer ขนาด 8 ไบต์
 */
void OneWire_GetAddress(OneWire_Bus* bus, uint8_t* rom);

/**
 * @brief   คำนวณ CRC8 แบบ Dallas (polynomial 0x8C)
 *
 * @param   data - pointer ข้อมูล
 * @param   len - ความยาวข้อมูล
 *
 * @return  ค่า CRC8
 */
uint8_t OneWire_CRC8(const uint8_t* data, uint8_t len);

/**
 * @brief   ตรวจสอบ CRC ของข้อมูล (คาดว่า CRC ต่อท้าย)
 *
 * @param   data - pointer ข้อมูลรวม CRC
 * @param   len - ความยาวรวม CRC
 *
 * @return  true ถ้า CRC ถูกต้อง
 */
bool OneWire_VerifyCRC(const uint8_t* data, uint8_t len);

/**
 * @brief   Depower บัส — ปล่อยขาเป็น input (ไม่ดึง HIGH ผ่าน pull-up ภายใน)
 *
 * @param   bus - pointer ไปยังบัส
 */
void OneWire_Depower(OneWire_Bus* bus);

/**
 * @brief   ค้นหาบัสจากหมายเลขขา
 *
 * @param   pin - หมายเลขขา
 *
 * @return  pointer ไปยัง OneWire_Bus หรือ NULL หากไม่พบ
 */
OneWire_Bus* OneWire_GetBusByPin(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif
