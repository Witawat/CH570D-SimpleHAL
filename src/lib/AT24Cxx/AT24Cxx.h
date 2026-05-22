/**
 * @file AT24Cxx.h
 * @brief AT24Cxx I2C EEPROM Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่าน/เขียน AT24Cxx EEPROM ผ่าน I2C
 * รองรับหลายขนาด: AT24C01 ถึง AT24C512
 *
 * **รุ่นที่รองรับ:**
 * | รุ่น       | ความจุ | Page Size | Address Bits |
 * |-----------|--------|-----------|--------------|
 * | AT24C01   | 128B   | 8B        | 8-bit        |
 * | AT24C02   | 256B   | 8B        | 8-bit        |
 * | AT24C04   | 512B   | 16B       | 9-bit (A0)   |
 * | AT24C08   | 1KB    | 16B       | 10-bit (A1)  |
 * | AT24C16   | 2KB    | 16B       | 11-bit (A2)  |
 * | AT24C32   | 4KB    | 32B       | 12-bit       |
 * | AT24C64   | 8KB    | 32B       | 13-bit       |
 * | AT24C128  | 16KB   | 64B       | 14-bit       |
 * | AT24C256  | 32KB   | 64B       | 15-bit       |
 * | AT24C512  | 64KB   | 128B      | 16-bit       |
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          AT24C02 (DIP-8)
 *                      +------+
 *   GND  -----------> | A0  VCC| <----- 3.3V
 *   GND  -----------> | A1  WP | <----- GND (disable write protect)
 *   GND  -----------> | A2 SDA | <--+-- PC1 (SDA)
 *   GND  -----------> | GND SCL| <--+-- PC2 (SCL)
 *                      +------+     |
 *                                  [4.7kΩ pull-up ทั้ง 2 เส้น]
 *                                   |
 *                                  3.3V
 *
 * I2C Address: 0x50 | (A2<<2) | (A1<<1) | A0
 *   A0=A1=A2=GND → address = 0x50
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "AT24Cxx.h"
 *
 * AT24Cxx_Instance eeprom;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     AT24Cxx_Init(&eeprom, AT24C02, 0x50);
 *
 *     AT24Cxx_WriteByte(&eeprom, 0x00, 0xAB);   // เขียนที่ address 0
 *     uint8_t val = AT24Cxx_ReadByte(&eeprom, 0x00);  // อ่านกลับ
 *     printf("Read: 0x%02X\r\n", val);  // ควรได้ 0xAB
 *
 *     while (1) {}
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __AT24CXX_H
#define __AT24CXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ========== Configuration ========== */

/** @brief Write cycle time (ms) — ต้องรอหลังจาก write ก่อนจะ write ใหม่ */
#ifndef AT24CXX_WRITE_CYCLE_MS
#define AT24CXX_WRITE_CYCLE_MS      5
#endif

/** @brief จำนวนครั้งสูงสุดที่ retry เมื่อ EEPROM ยัง busy */
#ifndef AT24CXX_WRITE_RETRY
#define AT24CXX_WRITE_RETRY         10
#endif

/* ========== Type Definitions ========== */

/**
 * @brief ประเภท EEPROM
 */
typedef enum {
    AT24C01  = 0, /**< 128B, page 8B, 8-bit address */
    AT24C02  = 1, /**< 256B, page 8B, 8-bit address */
    AT24C04  = 2, /**< 512B, page 16B, 9-bit address */
    AT24C08  = 3, /**< 1KB, page 16B, 10-bit address */
    AT24C16  = 4, /**< 2KB, page 16B, 11-bit address */
    AT24C32  = 5, /**< 4KB, page 32B, 12-bit address */
    AT24C64  = 6, /**< 8KB, page 32B, 13-bit address */
    AT24C128 = 7, /**< 16KB, page 64B, 14-bit address */
    AT24C256 = 8, /**< 32KB, page 64B, 15-bit address */
    AT24C512 = 9  /**< 64KB, page 128B, 16-bit address */
} AT24Cxx_Type;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    AT24CXX_OK             = 0, /**< สำเร็จ */
    AT24CXX_ERROR_PARAM    = 1, /**< Parameter ผิด */
    AT24CXX_ERROR_I2C      = 2, /**< I2C error */
    AT24CXX_ERROR_ADDR_OOB = 3, /**< Address เกินขนาด EEPROM */
    AT24CXX_ERROR_TIMEOUT  = 4  /**< Timeout รอ write cycle */
} AT24Cxx_Status;

/**
 * @brief AT24Cxx Instance
 */
typedef struct {
    AT24Cxx_Type type;      /**< ประเภท EEPROM */
    uint8_t  i2c_addr;      /**< I2C address (7-bit, เช่น 0x50) */

    /* ข้อมูลที่คำนวณจาก type */
    uint32_t capacity;      /**< ความจุทั้งหมด (bytes) */
    uint16_t page_size;     /**< ขนาด page สำหรับ page write (bytes) */
    uint8_t  addr_bytes;    /**< จำนวน bytes ของ address (1 หรือ 2) */

    uint8_t  initialized;   /**< flag บอกว่า Init แล้ว */
} AT24Cxx_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น AT24Cxx EEPROM
 * @param eeprom    ตัวแปร instance
 * @param type      ประเภท EEPROM เช่น AT24C02, AT24C32
 * @param i2c_addr  I2C address (7-bit, default 0x50)
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * AT24Cxx_Init(&eeprom, AT24C02, 0x50);
 */
AT24Cxx_Status AT24Cxx_Init(AT24Cxx_Instance* eeprom, AT24Cxx_Type type, uint8_t i2c_addr);

/**
 * @brief เขียน 1 byte
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งใน EEPROM (0-based)
 * @param data    ค่าที่ต้องการเขียน
 * @return AT24CXX_OK หรือ error code
 *
 * @note ต้องรอ 5ms หลัง write (write cycle time) — library จัดการให้อัตโนมัติ
 *
 * @example
 * AT24Cxx_WriteByte(&eeprom, 0x00, 0xAB);
 */
AT24Cxx_Status AT24Cxx_WriteByte(AT24Cxx_Instance* eeprom, uint32_t address, uint8_t data);

/**
 * @brief อ่าน 1 byte
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งใน EEPROM
 * @param data    pointer สำหรับรับค่า
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * uint8_t val;
 * AT24Cxx_ReadByte(&eeprom, 0x00, &val);
 */
AT24Cxx_Status AT24Cxx_ReadByte(AT24Cxx_Instance* eeprom, uint32_t address, uint8_t* data);

/**
 * @brief เขียน array ของ bytes (page write — เร็วกว่า byte-by-byte)
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param data    pointer ของข้อมูล
 * @param len     จำนวน bytes
 * @return AT24CXX_OK หรือ error code
 *
 * @note ข้าม page boundary อัตโนมัติ
 *
 * @example
 * uint8_t buf[] = {1, 2, 3, 4, 5};
 * AT24Cxx_WriteArray(&eeprom, 0x10, buf, 5);
 */
AT24Cxx_Status AT24Cxx_WriteArray(AT24Cxx_Instance* eeprom, uint32_t address,
                                   const uint8_t* data, uint16_t len);

/**
 * @brief อ่าน array ของ bytes (sequential read)
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param data    buffer สำหรับรับค่า
 * @param len     จำนวน bytes
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * uint8_t buf[5];
 * AT24Cxx_ReadArray(&eeprom, 0x10, buf, 5);
 */
AT24Cxx_Status AT24Cxx_ReadArray(AT24Cxx_Instance* eeprom, uint32_t address,
                                  uint8_t* data, uint16_t len);

/**
 * @brief เขียน string (null-terminated)
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param str     string ที่ต้องการเขียน (บันทึก null terminator ด้วย)
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * AT24Cxx_WriteString(&eeprom, 0x00, "Hello EEPROM");
 */
AT24Cxx_Status AT24Cxx_WriteString(AT24Cxx_Instance* eeprom, uint32_t address,
                                    const char* str);

/**
 * @brief อ่าน string
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param buf     buffer สำหรับรับ string
 * @param max_len ขนาด buffer สูงสุด (รวม null terminator)
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * char buf[20];
 * AT24Cxx_ReadString(&eeprom, 0x00, buf, 20);
 */
AT24Cxx_Status AT24Cxx_ReadString(AT24Cxx_Instance* eeprom, uint32_t address,
                                   char* buf, uint16_t max_len);

/**
 * @brief เขียน uint32_t (4 bytes, Little Endian)
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่ง (ต้องเผื่อ 4 bytes)
 * @param value   ค่าที่ต้องการเขียน
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * AT24Cxx_WriteUint32(&eeprom, 0x00, 123456789UL);
 */
AT24Cxx_Status AT24Cxx_WriteUint32(AT24Cxx_Instance* eeprom, uint32_t address, uint32_t value);

/**
 * @brief อ่าน uint32_t
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่ง
 * @param value   pointer สำหรับรับค่า
 * @return AT24CXX_OK หรือ error code
 *
 * @example
 * uint32_t v;
 * AT24Cxx_ReadUint32(&eeprom, 0x00, &v);
 */
AT24Cxx_Status AT24Cxx_ReadUint32(AT24Cxx_Instance* eeprom, uint32_t address, uint32_t* value);

/**
 * @brief ลบ EEPROM ทั้งหมด (เขียน 0xFF ทุก address)
 * @param eeprom ตัวแปร instance
 * @return AT24CXX_OK หรือ error code
 * @note ใช้เวลานาน (ขึ้นอยู่กับขนาด EEPROM)
 */
AT24Cxx_Status AT24Cxx_EraseAll(AT24Cxx_Instance* eeprom);

/**
 * @brief ดูขนาดความจุของ EEPROM (bytes)
 * @param eeprom ตัวแปร instance
 * @return ขนาด bytes หรือ 0 ถ้าไม่ได้ init
 */
uint32_t AT24Cxx_GetCapacity(AT24Cxx_Instance* eeprom);

/**
 * @brief แปลง error code เป็น string
 * @param status error code
 * @return ชื่อ error เช่น "OK", "I2C_ERROR"
 */
const char* AT24Cxx_StatusStr(AT24Cxx_Status status);

#ifdef __cplusplus
}
#endif

#endif /* __AT24CXX_H */
