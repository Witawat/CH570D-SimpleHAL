/**
 * @file DS18B20.c
 * @brief DS18B20 Digital Temperature Sensor Library Implementation
 * @version 1.0
 * @date 2025-12-22
 */

#include "DS18B20.h"
#include "../../SimpleHAL/SimpleDelay.h"
#include <string.h>

/* ========== Private Variables ========== */

static DS18B20_Device ds18b20_devices[DS18B20_MAX_DEVICES];
static uint8_t ds18b20_device_count = 0;

/* ========== Private Function Prototypes ========== */

static bool DS18B20_ReadScratchpad(DS18B20_Device* sensor, uint8_t* scratchpad);
static bool DS18B20_WriteScratchpad(DS18B20_Device* sensor, int8_t th, int8_t tl, uint8_t config);
static float DS18B20_ConvertRawTemperature(int16_t raw, DS18B20_Resolution resolution);

/* ========== Initialization ========== */

/**
 * @brief เริ่มต้น DS18B20 sensor แบบ single device
 */
DS18B20_Device* DS18B20_Init(uint8_t pin) {
    // ตรวจสอบว่าเต็มหรือไม่
    if (ds18b20_device_count >= DS18B20_MAX_DEVICES) {
        return NULL;
    }
    
    // เริ่มต้น 1-Wire bus
    OneWire_Bus* bus = OneWire_Init(pin);
    if (bus == NULL) {
        return NULL;
    }
    
    // ตรวจสอบว่ามี device หรือไม่
    if (!OneWire_Reset(bus)) {
        return NULL;
    }
    
    // สร้าง device instance
    DS18B20_Device* sensor = &ds18b20_devices[ds18b20_device_count++];
    memset(sensor, 0, sizeof(DS18B20_Device));
    
    sensor->bus = bus;
    sensor->resolution = DS18B20_RES_12BIT;  // Default 12-bit
    sensor->use_rom = false;  // Single device, ไม่ต้องใช้ ROM addressing
    sensor->initialized = true;
    
    // อ่าน ROM address (optional, สำหรับ debug)
    OneWire_ReadROM(bus, sensor->rom);
    
    return sensor;
}

/**
 * @brief เริ่มต้น DS18B20 sensor ด้วย ROM address
 */
DS18B20_Device* DS18B20_InitWithROM(OneWire_Bus* bus, const uint8_t* rom) {
    if (!bus || !rom) return NULL;
    
    // ตรวจสอบว่าเต็มหรือไม่
    if (ds18b20_device_count >= DS18B20_MAX_DEVICES) {
        return NULL;
    }
    
    // ตรวจสอบว่าเป็น DS18B20 หรือไม่
    if (!DS18B20_VerifyDevice(rom)) {
        return NULL;
    }
    
    // สร้าง device instance
    DS18B20_Device* sensor = &ds18b20_devices[ds18b20_device_count++];
    memset(sensor, 0, sizeof(DS18B20_Device));
    
    sensor->bus = bus;
    memcpy(sensor->rom, rom, 8);
    sensor->resolution = DS18B20_RES_12BIT;
    sensor->use_rom = true;  // Multi-device bus
    sensor->initialized = true;
    
    return sensor;
}

/* ========== Temperature Reading ========== */

/**
 * @brief เริ่มการแปลงสัญญาณอุณหภูมิ
 */
bool DS18B20_StartConversion(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized || !sensor->bus) return false;
    
    // Reset
    if (!OneWire_Reset(sensor->bus)) {
        return false;
    }
    
    // Select device
    if (sensor->use_rom) {
        OneWire_MatchROM(sensor->bus, sensor->rom);
    } else {
        OneWire_SkipROM(sensor->bus);
    }
    
    // Send Convert T command
    OneWire_WriteByte(sensor->bus, DS18B20_CMD_CONVERT_T);
    
    return true;
}

/**
 * @brief เริ่มการแปลงสัญญาณอุณหภูมิทุก sensors บน bus
 */
bool DS18B20_StartConversionAll(OneWire_Bus* bus) {
    if (!bus) return false;
    
    // Reset
    if (!OneWire_Reset(bus)) {
        return false;
    }
    
    // Skip ROM (broadcast to all devices)
    OneWire_SkipROM(bus);
    
    // Send Convert T command
    OneWire_WriteByte(bus, DS18B20_CMD_CONVERT_T);
    
    return true;
}

/**
 * @brief อ่านอุณหภูมิ (°C)
 */
float DS18B20_ReadTemperature(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized || !sensor->bus) return NAN;
    
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
    
    // อ่าน scratchpad
    if (!DS18B20_ReadScratchpad(sensor, scratchpad)) {
        return NAN;
    }
    
    // ตรวจสอบ CRC
    if (!OneWire_VerifyCRC(scratchpad, DS18B20_SCRATCHPAD_SIZE)) {
        return NAN;
    }
    
    // แปลงค่า raw เป็นอุณหภูมิ
    int16_t raw = (scratchpad[1] << 8) | scratchpad[0];
    float temperature = DS18B20_ConvertRawTemperature(raw, sensor->resolution);
    
    // บันทึกค่าล่าสุด
    sensor->last_temperature = temperature;
    
    return temperature;
}

/**
 * @brief อ่านอุณหภูมิ (°F)
 */
float DS18B20_ReadTemperatureF(DS18B20_Device* sensor) {
    float celsius = DS18B20_ReadTemperature(sensor);
    if (isnan(celsius)) return NAN;
    return DS18B20_CelsiusToFahrenheit(celsius);
}

/**
 * @brief อ่านอุณหภูมิแบบ blocking (รอ conversion)
 */
float DS18B20_ReadTemperatureBlocking(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized) return NAN;
    
    // เริ่ม conversion
    if (!DS18B20_StartConversion(sensor)) {
        return NAN;
    }
    
    // รอให้ conversion เสร็จ
    uint16_t delay_ms = DS18B20_GetConversionTime(sensor->resolution);
    Delay_Ms(delay_ms);
    
    // อ่านอุณหภูมิ
    return DS18B20_ReadTemperature(sensor);
}

/**
 * @brief ตรวจสอบว่า conversion เสร็จหรือยัง
 */
bool DS18B20_IsConversionDone(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized || !sensor->bus) return false;
    
    // อ่าน bit จาก bus
    // ถ้า conversion เสร็จ bus จะเป็น 1, ถ้ายังไม่เสร็จจะเป็น 0
    return OneWire_ReadBit(sensor->bus);
}

/* ========== Configuration ========== */

/**
 * @brief ตั้งค่าความละเอียด
 */
bool DS18B20_SetResolution(DS18B20_Device* sensor, DS18B20_Resolution resolution) {
    if (!sensor || !sensor->initialized) return false;
    
    // อ่าน scratchpad ปัจจุบัน
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
    if (!DS18B20_ReadScratchpad(sensor, scratchpad)) {
        return false;
    }
    
    // สร้าง config byte
    uint8_t config = (resolution << 5) | 0x1F;
    
    // เขียน scratchpad (TH, TL, Config)
    if (!DS18B20_WriteScratchpad(sensor, scratchpad[2], scratchpad[3], config)) {
        return false;
    }
    
    // อัพเดท resolution ใน structure
    sensor->resolution = resolution;
    
    return true;
}

/**
 * @brief อ่านความละเอียดปัจจุบัน
 */
DS18B20_Resolution DS18B20_GetResolution(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized) return DS18B20_RES_12BIT;
    
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
    if (!DS18B20_ReadScratchpad(sensor, scratchpad)) {
        return sensor->resolution;  // Return cached value
    }
    
    // แยก resolution จาก config byte
    uint8_t config = scratchpad[4];
    DS18B20_Resolution resolution = (DS18B20_Resolution)((config >> 5) & 0x03);
    
    // อัพเดท cached value
    sensor->resolution = resolution;
    
    return resolution;
}

/**
 * @brief ตั้งค่า alarm thresholds
 */
bool DS18B20_SetAlarm(DS18B20_Device* sensor, int8_t th, int8_t tl) {
    if (!sensor || !sensor->initialized) return false;
    
    // อ่าน config ปัจจุบัน
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
    if (!DS18B20_ReadScratchpad(sensor, scratchpad)) {
        return false;
    }
    
    // เขียน scratchpad (TH, TL, Config)
    return DS18B20_WriteScratchpad(sensor, th, tl, scratchpad[4]);
}

/**
 * @brief อ่านค่า alarm thresholds
 */
bool DS18B20_GetAlarm(DS18B20_Device* sensor, int8_t* th, int8_t* tl) {
    if (!sensor || !sensor->initialized || !th || !tl) return false;
    
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
    if (!DS18B20_ReadScratchpad(sensor, scratchpad)) {
        return false;
    }
    
    *th = (int8_t)scratchpad[2];
    *tl = (int8_t)scratchpad[3];
    
    return true;
}

/**
 * @brief บันทึกค่า configuration ลง EEPROM
 */
bool DS18B20_SaveToEEPROM(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized || !sensor->bus) return false;
    
    // Reset
    if (!OneWire_Reset(sensor->bus)) {
        return false;
    }
    
    // Select device
    if (sensor->use_rom) {
        OneWire_MatchROM(sensor->bus, sensor->rom);
    } else {
        OneWire_SkipROM(sensor->bus);
    }
    
    // Send Copy Scratchpad command
    OneWire_WriteByte(sensor->bus, DS18B20_CMD_COPY_SCRATCHPAD);
    
    // รอให้ EEPROM write เสร็จ (10 ms typical)
    Delay_Ms(10);
    
    return true;
}

/**
 * @brief โหลดค่า configuration จาก EEPROM
 */
bool DS18B20_LoadFromEEPROM(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized || !sensor->bus) return false;
    
    // Reset
    if (!OneWire_Reset(sensor->bus)) {
        return false;
    }
    
    // Select device
    if (sensor->use_rom) {
        OneWire_MatchROM(sensor->bus, sensor->rom);
    } else {
        OneWire_SkipROM(sensor->bus);
    }
    
    // Send Recall E2 command
    OneWire_WriteByte(sensor->bus, DS18B20_CMD_RECALL_E2);
    
    // รอให้ recall เสร็จ (< 1 ms)
    Delay_Us(500);
    
    // อ่าน resolution ใหม่
    DS18B20_GetResolution(sensor);
    
    return true;
}

/* ========== Multi-Sensor Functions ========== */

/**
 * @brief ค้นหา DS18B20 devices ทั้งหมดบน bus
 */
uint8_t DS18B20_SearchDevices(OneWire_Bus* bus, uint8_t* rom_list, uint8_t max_devices) {
    if (!bus || !rom_list || max_devices == 0) return 0;
    
    uint8_t count = 0;
    uint8_t rom[8];
    
    // Reset search
    OneWire_ResetSearch(bus);
    
    // ค้นหาทุก devices
    while (OneWire_Search(bus) && count < max_devices) {
        OneWire_GetAddress(bus, rom);
        
        // ตรวจสอบว่าเป็น DS18B20 หรือไม่
        if (DS18B20_VerifyDevice(rom)) {
            // คัดลอก ROM address ไปยัง list
            memcpy(&rom_list[count * 8], rom, 8);
            count++;
        }
    }
    
    return count;
}

/**
 * @brief ค้นหา DS18B20 devices ที่มี alarm condition
 */
uint8_t DS18B20_SearchAlarm(OneWire_Bus* bus, uint8_t* rom_list, uint8_t max_devices) {
    if (!bus || !rom_list || max_devices == 0) return 0;
    
    uint8_t count = 0;
    uint8_t rom[8];
    
    // Reset search
    OneWire_ResetSearch(bus);
    
    // ค้นหา devices ที่มี alarm
    while (OneWire_AlarmSearch(bus) && count < max_devices) {
        OneWire_GetAddress(bus, rom);
        
        // ตรวจสอบว่าเป็น DS18B20 หรือไม่
        if (DS18B20_VerifyDevice(rom)) {
            // คัดลอก ROM address ไปยัง list
            memcpy(&rom_list[count * 8], rom, 8);
            count++;
        }
    }
    
    return count;
}

/* ========== Utility Functions ========== */

/**
 * @brief ตรวจสอบ power supply mode
 */
bool DS18B20_ReadPowerSupply(DS18B20_Device* sensor) {
    if (!sensor || !sensor->initialized || !sensor->bus) return false;
    
    // Reset
    if (!OneWire_Reset(sensor->bus)) {
        return false;
    }
    
    // Select device
    if (sensor->use_rom) {
        OneWire_MatchROM(sensor->bus, sensor->rom);
    } else {
        OneWire_SkipROM(sensor->bus);
    }
    
    // Send Read Power Supply command
    OneWire_WriteByte(sensor->bus, DS18B20_CMD_READ_POWER);
    
    // อ่านผลลัพธ์
    // 1 = normal power, 0 = parasite power
    return OneWire_ReadBit(sensor->bus);
}

/**
 * @brief คำนวณเวลา conversion ตาม resolution
 */
uint16_t DS18B20_GetConversionTime(DS18B20_Resolution resolution) {
    switch (resolution) {
        case DS18B20_RES_9BIT:  return 94;   // 93.75 ms
        case DS18B20_RES_10BIT: return 188;  // 187.5 ms
        case DS18B20_RES_11BIT: return 375;  // 375 ms
        case DS18B20_RES_12BIT: return 750;  // 750 ms
        default: return 750;
    }
}

/**
 * @brief ตรวจสอบว่าเป็น DS18B20 จริงหรือไม่
 */
bool DS18B20_VerifyDevice(const uint8_t* rom) {
    if (!rom) return false;
    
    // ตรวจสอบ family code
    return (rom[0] == DS18B20_FAMILY_CODE);
}

/**
 * @brief แปลง °C เป็น °F
 */
float DS18B20_CelsiusToFahrenheit(float celsius) {
    return celsius * 1.8f + 32.0f;
}

/**
 * @brief แปลง °F เป็น °C
 */
float DS18B20_FahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) / 1.8f;
}

/* ========== Private Functions ========== */

/**
 * @brief อ่าน scratchpad memory
 */
static bool DS18B20_ReadScratchpad(DS18B20_Device* sensor, uint8_t* scratchpad) {
    if (!sensor || !sensor->bus || !scratchpad) return false;
    
    // Reset
    if (!OneWire_Reset(sensor->bus)) {
        return false;
    }
    
    // Select device
    if (sensor->use_rom) {
        OneWire_MatchROM(sensor->bus, sensor->rom);
    } else {
        OneWire_SkipROM(sensor->bus);
    }
    
    // Send Read Scratchpad command
    OneWire_WriteByte(sensor->bus, DS18B20_CMD_READ_SCRATCHPAD);
    
    // อ่าน scratchpad (9 bytes)
    OneWire_ReadBytes(sensor->bus, scratchpad, DS18B20_SCRATCHPAD_SIZE);
    
    return true;
}

/**
 * @brief เขียน scratchpad memory
 */
static bool DS18B20_WriteScratchpad(DS18B20_Device* sensor, int8_t th, int8_t tl, uint8_t config) {
    if (!sensor || !sensor->bus) return false;
    
    // Reset
    if (!OneWire_Reset(sensor->bus)) {
        return false;
    }
    
    // Select device
    if (sensor->use_rom) {
        OneWire_MatchROM(sensor->bus, sensor->rom);
    } else {
        OneWire_SkipROM(sensor->bus);
    }
    
    // Send Write Scratchpad command
    OneWire_WriteByte(sensor->bus, DS18B20_CMD_WRITE_SCRATCHPAD);
    
    // เขียน TH, TL, Config
    OneWire_WriteByte(sensor->bus, (uint8_t)th);
    OneWire_WriteByte(sensor->bus, (uint8_t)tl);
    OneWire_WriteByte(sensor->bus, config);
    
    return true;
}

/**
 * @brief แปลงค่า raw temperature เป็น °C
 */
static float DS18B20_ConvertRawTemperature(int16_t raw, DS18B20_Resolution resolution) {
    // DS18B20 ใช้ two's complement format
    // 12-bit: 0.0625°C per bit
    // 11-bit: 0.125°C per bit
    // 10-bit: 0.25°C per bit
    // 9-bit:  0.5°C per bit
    
    // Mask ตาม resolution
    switch (resolution) {
        case DS18B20_RES_9BIT:
            raw &= 0xFFF8;  // Mask 3 LSBs
            break;
        case DS18B20_RES_10BIT:
            raw &= 0xFFFC;  // Mask 2 LSBs
            break;
        case DS18B20_RES_11BIT:
            raw &= 0xFFFE;  // Mask 1 LSB
            break;
        case DS18B20_RES_12BIT:
        default:
            // ไม่ต้อง mask
            break;
    }
    
    // แปลงเป็น °C
    float temperature = (float)raw / 16.0f;
    
    return temperature;
}
