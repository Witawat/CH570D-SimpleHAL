/**
 * @file DS18B20.h
 * @brief DS18B20 Digital Temperature Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2025-12-22
 * 
 * @details
 * Library นี้ใช้สำหรับวัดอุณหภูมิด้วย DS18B20 digital temperature sensor
 * ใช้ Simple1Wire library เป็นพื้นฐานสำหรับการสื่อสาร
 * 
 * **คุณสมบัติ:**
 * - วัดอุณหภูมิ -55°C ถึง +125°C
 * - ความละเอียด 9-12 bit (0.5°C - 0.0625°C)
 * - รองรับ single และ multi-sensor บน bus เดียวกัน
 * - Alarm threshold สำหรับตรวจสอบอุณหภูมิ
 * - EEPROM สำหรับบันทึกค่า configuration
 * - CRC validation สำหรับความน่าเชื่อถือ
 * - รองรับทั้ง normal power และ parasite power mode
 * 
 * **Hardware Connection:**
 * ```
 * Normal Power Mode:
 *   VCC (3.3V)
 *      |
 *    [4.7kΩ]
 *      |
 *      +---- DQ (Data) ---- GPIO Pin
 *      |
 *   DS18B20
 *      |
 *     GND
 * 
 * Parasite Power Mode:
 *   VCC (3.3V)
 *      |
 *    [4.7kΩ]
 *      |
 *      +---- DQ (Data) ---- GPIO Pin
 *      |
 *   DS18B20 (VDD connected to GND)
 *      |
 *     GND
 * ```
 * 
 * @example
 * #include "DS18B20.h"
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     
 *     // เริ่มต้น DS18B20 บน PD2
 *     DS18B20_Device* sensor = DS18B20_Init(PD2);
 *     
 *     while(1) {
 *         // อ่านอุณหภูมิแบบ blocking
 *         float temp = DS18B20_ReadTemperatureBlocking(sensor);
 *         printf("Temperature: %.2f C\\r\\n", temp);
 *         
 *         Delay_Ms(1000);
 *     }
 * }
 * 
 * @note ต้องเรียก SystemCoreClockUpdate() ก่อนใช้งาน
 * @note ต้องมี pull-up resistor 4.7kΩ ภายนอก
 */

#ifndef __DS18B20_H
#define __DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/Simple1Wire.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ========== Configuration ========== */

#define DS18B20_MAX_DEVICES  8  /**< จำนวน DS18B20 devices สูงสุดต่อ bus */

/* ========== DS18B20 Commands ========== */

/**
 * @brief DS18B20 Function Commands
 */
#define DS18B20_CMD_CONVERT_T        0x44  /**< Start temperature conversion */
#define DS18B20_CMD_READ_SCRATCHPAD  0xBE  /**< Read scratchpad memory */
#define DS18B20_CMD_WRITE_SCRATCHPAD 0x4E  /**< Write scratchpad memory */
#define DS18B20_CMD_COPY_SCRATCHPAD  0x48  /**< Copy scratchpad to EEPROM */
#define DS18B20_CMD_RECALL_E2        0xB8  /**< Recall EEPROM to scratchpad */
#define DS18B20_CMD_READ_POWER       0xB4  /**< Read power supply mode */

/* ========== DS18B20 Constants ========== */

#define DS18B20_FAMILY_CODE     0x28  /**< DS18B20 family code */
#define DS18B20_SCRATCHPAD_SIZE 9     /**< Scratchpad memory size (bytes) */

/* Temperature limits */
#define DS18B20_TEMP_MIN        -55.0f   /**< Minimum temperature (°C) */
#define DS18B20_TEMP_MAX        125.0f   /**< Maximum temperature (°C) */

/* Error values */
#define DS18B20_ERROR_VALUE     85.0f    /**< Power-on reset value */
#define DS18B20_DISCONNECTED    -127.0f  /**< Disconnected sensor value */

/* ========== Resolution Settings ========== */

/**
 * @brief DS18B20 Resolution Settings
 */
typedef enum {
    DS18B20_RES_9BIT  = 0,  /**< 9-bit resolution (0.5°C, 93.75 ms) */
    DS18B20_RES_10BIT = 1,  /**< 10-bit resolution (0.25°C, 187.5 ms) */
    DS18B20_RES_11BIT = 2,  /**< 11-bit resolution (0.125°C, 375 ms) */
    DS18B20_RES_12BIT = 3   /**< 12-bit resolution (0.0625°C, 750 ms) */
} DS18B20_Resolution;

/* ========== Structures ========== */

/**
 * @brief DS18B20 Device Structure
 */
typedef struct {
    OneWire_Bus* bus;               /**< 1-Wire bus instance */
    uint8_t rom[8];                 /**< 64-bit ROM code */
    DS18B20_Resolution resolution;  /**< Temperature resolution */
    bool use_rom;                   /**< true = ใช้ ROM addressing (multi-sensor) */
    float last_temperature;         /**< Last temperature reading (°C) */
    bool initialized;               /**< Initialization flag */
} DS18B20_Device;

/**
 * @brief DS18B20 Scratchpad Structure
 */
typedef struct {
    int16_t temperature;    /**< Temperature (raw) */
    int8_t th;              /**< High alarm threshold */
    int8_t tl;              /**< Low alarm threshold */
    uint8_t config;         /**< Configuration register */
    uint8_t reserved[3];    /**< Reserved bytes */
    uint8_t crc;            /**< CRC8 checksum */
} DS18B20_Scratchpad;

/* ========== Function Prototypes ========== */

/* === Initialization === */

/**
 * @brief เริ่มต้น DS18B20 sensor แบบ single device
 * @param pin GPIO pin number ที่ต่อกับ DS18B20
 * @return ตัวชี้ไปยัง DS18B20_Device instance หรือ NULL ถ้าผิดพลาด
 * 
 * @note ใช้สำหรับ bus ที่มี sensor เดียว (ไม่ต้องใช้ ROM addressing)
 * @note ความละเอียดเริ่มต้น: 12-bit
 * 
 * @example
 * DS18B20_Device* sensor = DS18B20_Init(PD2);
 * if (sensor == NULL) {
 *     // Error: ไม่พบ sensor
 * }
 */
DS18B20_Device* DS18B20_Init(uint8_t pin);

/**
 * @brief เริ่มต้น DS18B20 sensor ด้วย ROM address
 * @param bus ตัวชี้ไปยัง OneWire_Bus instance
 * @param rom ตัวชี้ไปยัง ROM address 8 bytes
 * @return ตัวชี้ไปยัง DS18B20_Device instance หรือ NULL ถ้าผิดพลาด
 * 
 * @note ใช้สำหรับ multi-sensor bus
 * @note ต้องทราบ ROM address ก่อน (ใช้ DS18B20_SearchDevices)
 * 
 * @example
 * uint8_t rom[8] = {0x28, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE};
 * DS18B20_Device* sensor = DS18B20_InitWithROM(bus, rom);
 */
DS18B20_Device* DS18B20_InitWithROM(OneWire_Bus* bus, const uint8_t* rom);

/* === Temperature Reading === */

/**
 * @brief เริ่มการแปลงสัญญาณอุณหภูมิ
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @note ต้องรอให้ conversion เสร็จก่อนอ่านอุณหภูมิ
 * @note ใช้เวลา 93.75-750 ms ขึ้นกับ resolution
 * 
 * @example
 * DS18B20_StartConversion(sensor);
 * Delay_Ms(750);  // รอให้ conversion เสร็จ (12-bit)
 * float temp = DS18B20_ReadTemperature(sensor);
 */
bool DS18B20_StartConversion(DS18B20_Device* sensor);

/**
 * @brief เริ่มการแปลงสัญญาณอุณหภูมิทุก sensors บน bus
 * @param bus ตัวชี้ไปยัง OneWire_Bus instance
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @note ใช้ Skip ROM command เพื่อส่ง command ไปยังทุก devices
 * 
 * @example
 * DS18B20_StartConversionAll(bus);
 * Delay_Ms(750);
 * // อ่านอุณหภูมิจากแต่ละ sensor
 */
bool DS18B20_StartConversionAll(OneWire_Bus* bus);

/**
 * @brief อ่านอุณหภูมิ (°C)
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return อุณหภูมิ (°C) หรือ NAN ถ้าผิดพลาด
 * 
 * @note ต้องเรียก DS18B20_StartConversion() และรอให้เสร็จก่อน
 * @note ถ้าได้ 85.0°C = power-on reset value (ยังไม่ได้ convert)
 * @note ถ้าได้ -127.0°C = sensor ไม่ตอบสนอง
 * 
 * @example
 * float temp = DS18B20_ReadTemperature(sensor);
 * if (!isnan(temp)) {
 *     printf("Temperature: %.2f C\\r\\n", temp);
 * }
 */
float DS18B20_ReadTemperature(DS18B20_Device* sensor);

/**
 * @brief อ่านอุณหภูมิ (°F)
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return อุณหภูมิ (°F) หรือ NAN ถ้าผิดพลาด
 * 
 * @example
 * float temp_f = DS18B20_ReadTemperatureF(sensor);
 */
float DS18B20_ReadTemperatureF(DS18B20_Device* sensor);

/**
 * @brief อ่านอุณหภูมิแบบ blocking (รอ conversion)
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return อุณหภูมิ (°C) หรือ NAN ถ้าผิดพลาด
 * 
 * @note ฟังก์ชันนี้จะเริ่ม conversion และรอจนกว่าจะเสร็จ
 * @note ใช้เวลา 93.75-750 ms ขึ้นกับ resolution
 * 
 * @example
 * float temp = DS18B20_ReadTemperatureBlocking(sensor);
 */
float DS18B20_ReadTemperatureBlocking(DS18B20_Device* sensor);

/**
 * @brief ตรวจสอบว่า conversion เสร็จหรือยัง
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return true = เสร็จแล้ว, false = ยังไม่เสร็จ
 * 
 * @note ใช้สำหรับ non-blocking temperature reading
 * 
 * @example
 * DS18B20_StartConversion(sensor);
 * while (!DS18B20_IsConversionDone(sensor)) {
 *     // Do other work
 * }
 * float temp = DS18B20_ReadTemperature(sensor);
 */
bool DS18B20_IsConversionDone(DS18B20_Device* sensor);

/* === Configuration === */

/**
 * @brief ตั้งค่าความละเอียด
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @param resolution ความละเอียด (9-12 bit)
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @note ค่าจะถูกบันทึกใน scratchpad (ยังไม่ได้บันทึกใน EEPROM)
 * @note ใช้ DS18B20_SaveToEEPROM() เพื่อบันทึกถาวร
 * 
 * @example
 * DS18B20_SetResolution(sensor, DS18B20_RES_12BIT);  // 0.0625°C
 */
bool DS18B20_SetResolution(DS18B20_Device* sensor, DS18B20_Resolution resolution);

/**
 * @brief อ่านความละเอียดปัจจุบัน
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return ความละเอียด (9-12 bit)
 * 
 * @example
 * DS18B20_Resolution res = DS18B20_GetResolution(sensor);
 * printf("Resolution: %d-bit\\r\\n", 9 + res);
 */
DS18B20_Resolution DS18B20_GetResolution(DS18B20_Device* sensor);

/**
 * @brief ตั้งค่า alarm thresholds
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @param th High alarm threshold (°C)
 * @param tl Low alarm threshold (°C)
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @note ค่าจะถูกบันทึกใน scratchpad (ยังไม่ได้บันทึกใน EEPROM)
 * @note ใช้ DS18B20_SaveToEEPROM() เพื่อบันทึกถาวร
 * 
 * @example
 * DS18B20_SetAlarm(sensor, 30, 20);  // Alarm ถ้า >30°C หรือ <20°C
 */
bool DS18B20_SetAlarm(DS18B20_Device* sensor, int8_t th, int8_t tl);

/**
 * @brief อ่านค่า alarm thresholds
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @param th ตัวชี้สำหรับเก็บ high threshold
 * @param tl ตัวชี้สำหรับเก็บ low threshold
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @example
 * int8_t th, tl;
 * DS18B20_GetAlarm(sensor, &th, &tl);
 * printf("Alarm: TH=%d, TL=%d\\r\\n", th, tl);
 */
bool DS18B20_GetAlarm(DS18B20_Device* sensor, int8_t* th, int8_t* tl);

/**
 * @brief บันทึกค่า configuration ลง EEPROM
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @note บันทึก TH, TL, และ resolution ลง EEPROM
 * @note ค่าจะถูกโหลดอัตโนมัติเมื่อ power-on
 * 
 * @example
 * DS18B20_SetResolution(sensor, DS18B20_RES_12BIT);
 * DS18B20_SetAlarm(sensor, 30, 20);
 * DS18B20_SaveToEEPROM(sensor);  // บันทึกถาวร
 */
bool DS18B20_SaveToEEPROM(DS18B20_Device* sensor);

/**
 * @brief โหลดค่า configuration จาก EEPROM
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return true = สำเร็จ, false = ผิดพลาด
 * 
 * @note โหลด TH, TL, และ resolution จาก EEPROM ไปยัง scratchpad
 * 
 * @example
 * DS18B20_LoadFromEEPROM(sensor);
 */
bool DS18B20_LoadFromEEPROM(DS18B20_Device* sensor);

/* === Multi-Sensor Functions === */

/**
 * @brief ค้นหา DS18B20 devices ทั้งหมดบน bus
 * @param bus ตัวชี้ไปยัง OneWire_Bus instance
 * @param rom_list ตัวชี้ไปยัง array สำหรับเก็บ ROM addresses
 * @param max_devices จำนวน devices สูงสุดที่ต้องการค้นหา
 * @return จำนวน devices ที่พบ
 * 
 * @note rom_list ต้องมีขนาดอย่างน้อย max_devices * 8 bytes
 * @note จะค้นหาเฉพาะ devices ที่มี family code = 0x28 (DS18B20)
 * 
 * @example
 * uint8_t rom_list[DS18B20_MAX_DEVICES][8];
 * uint8_t count = DS18B20_SearchDevices(bus, (uint8_t*)rom_list, DS18B20_MAX_DEVICES);
 * printf("Found %d DS18B20 sensors\\r\\n", count);
 */
uint8_t DS18B20_SearchDevices(OneWire_Bus* bus, uint8_t* rom_list, uint8_t max_devices);

/**
 * @brief ค้นหา DS18B20 devices ที่มี alarm condition
 * @param bus ตัวชี้ไปยัง OneWire_Bus instance
 * @param rom_list ตัวชี้ไปยัง array สำหรับเก็บ ROM addresses
 * @param max_devices จำนวน devices สูงสุดที่ต้องการค้นหา
 * @return จำนวน devices ที่มี alarm
 * 
 * @example
 * uint8_t alarm_list[DS18B20_MAX_DEVICES][8];
 * uint8_t count = DS18B20_SearchAlarm(bus, (uint8_t*)alarm_list, DS18B20_MAX_DEVICES);
 * printf("Found %d sensors with alarm\\r\\n", count);
 */
uint8_t DS18B20_SearchAlarm(OneWire_Bus* bus, uint8_t* rom_list, uint8_t max_devices);

/* === Utility Functions === */

/**
 * @brief ตรวจสอบ power supply mode
 * @param sensor ตัวชี้ไปยัง DS18B20_Device instance
 * @return true = normal power, false = parasite power
 * 
 * @example
 * if (DS18B20_ReadPowerSupply(sensor)) {
 *     printf("Normal power mode\\r\\n");
 * } else {
 *     printf("Parasite power mode\\r\\n");
 * }
 */
bool DS18B20_ReadPowerSupply(DS18B20_Device* sensor);

/**
 * @brief คำนวณเวลา conversion ตาม resolution
 * @param resolution ความละเอียด (9-12 bit)
 * @return เวลา conversion (milliseconds)
 * 
 * @example
 * uint16_t time_ms = DS18B20_GetConversionTime(DS18B20_RES_12BIT);  // 750 ms
 */
uint16_t DS18B20_GetConversionTime(DS18B20_Resolution resolution);

/**
 * @brief ตรวจสอบว่าเป็น DS18B20 จริงหรือไม่
 * @param rom ตัวชี้ไปยัง ROM address 8 bytes
 * @return true = เป็น DS18B20, false = ไม่ใช่
 * 
 * @note ตรวจสอบ family code (byte 0) ว่าเป็น 0x28 หรือไม่
 * 
 * @example
 * uint8_t rom[8];
 * OneWire_ReadROM(bus, rom);
 * if (DS18B20_VerifyDevice(rom)) {
 *     printf("DS18B20 detected\\r\\n");
 * }
 */
bool DS18B20_VerifyDevice(const uint8_t* rom);

/**
 * @brief แปลง °C เป็น °F
 * @param celsius อุณหภูมิ (°C)
 * @return อุณหภูมิ (°F)
 * 
 * @example
 * float f = DS18B20_CelsiusToFahrenheit(25.0f);  // 77.0°F
 */
float DS18B20_CelsiusToFahrenheit(float celsius);

/**
 * @brief แปลง °F เป็น °C
 * @param fahrenheit อุณหภูมิ (°F)
 * @return อุณหภูมิ (°C)
 * 
 * @example
 * float c = DS18B20_FahrenheitToCelsius(77.0f);  // 25.0°C
 */
float DS18B20_FahrenheitToCelsius(float fahrenheit);

#ifdef __cplusplus
}
#endif

#endif  // __DS18B20_H
