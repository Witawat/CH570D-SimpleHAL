# DS18B20 Digital Temperature Sensor Library

> **Library สำหรับวัดอุณหภูมิด้วย DS18B20 digital temperature sensor บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [1-Wire Protocol](#1-wire-protocol)
4. [Hardware Setup](#hardware-setup)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [ROM Addressing](#rom-addressing)
8. [Parasite Power Mode](#parasite-power-mode)
9. [Troubleshooting](#troubleshooting)
10. [API Reference](#api-reference)

---

## ภาพรวม

DS18B20 เป็น digital temperature sensor ที่ใช้ 1-Wire protocol สำหรับการสื่อสาร มีความแม่นยำสูงและใช้งานง่าย

### ข้อดีของ DS18B20

- **ความแม่นยำสูง**: ±0.5°C (-10°C ถึง +85°C)
- **ช่วงการวัด**: -55°C ถึง +125°C
- **ความละเอียดปรับได้**: 9-12 bit (0.5°C - 0.0625°C)
- **ใช้สายเดียว**: สื่อสารผ่าน 1-Wire protocol
- **Multi-sensor**: ต่อหลาย sensors บน bus เดียวกันได้
- **Unique ROM**: แต่ละตัวมี 64-bit ROM address ไม่ซ้ำกัน
- **Alarm Function**: ตั้งค่า threshold ได้
- **EEPROM**: บันทึกค่า configuration
- **Parasite Power**: ทำงานได้โดยไม่ต้องต่อ VDD

### เปรียบเทียบกับ Sensor อื่น

| Feature | DS18B20 | NTC 10K | DHT22 |
|---------|---------|---------|-------|
| ความแม่นยำ | ±0.5°C | ±1-2°C | ±0.5°C |
| ความละเอียด | 0.0625°C | ขึ้นกับ ADC | 0.1°C |
| Interface | 1-Wire | Analog | Digital |
| Multi-sensor | ✓ | ✓ | ✗ |
| Waterproof | ✓ | ✓ | ✗ |
| ราคา | ปานกลาง | ถูก | ปานกลาง |

---

## คุณสมบัติ

### คุณสมบัติของ Library

- ✅ รองรับ single และ multi-sensor
- ✅ ความละเอียด 9-12 bit
- ✅ Blocking และ non-blocking reading
- ✅ ROM search algorithm
- ✅ Alarm threshold
- ✅ EEPROM configuration
- ✅ CRC validation
- ✅ Parasite power support
- ✅ ใช้ Simple1Wire library เป็นพื้นฐาน
- ✅ เอกสารภาษาไทยครบถ้วน

### ข้อมูลทางเทคนิค

- **ช่วงอุณหภูมิ**: -55°C ถึง +125°C
- **ความแม่นยำ**: ±0.5°C (-10°C ถึง +85°C)
- **Resolution**: 9, 10, 11, หรือ 12 bit
- **Conversion Time**:
  - 9-bit: 93.75 ms
  - 10-bit: 187.5 ms
  - 11-bit: 375 ms
  - 12-bit: 750 ms
- **Power Supply**: 3.0V - 5.5V (หรือ parasite power)
- **Current**: 1 mA (active), 750 nA (standby)

---

## 1-Wire Protocol

### หลักการทำงาน

1-Wire protocol เป็น serial communication protocol ที่ใช้สายเดียวสำหรับทั้งข้อมูลและ power

**ลักษณะสำคัญ:**
- ใช้สายเดียว (+ GND)
- Half-duplex communication
- Master-slave architecture
- Open-drain/tri-state
- ต้องมี pull-up resistor

### Timing Specifications

#### Reset และ Presence Pulse

```
Master TX Reset Pulse:
  ___________          _______________
             |________|
             480-960µs  60-240µs

Slave TX Presence Pulse:
  ___________     ___     ___________
             |___|   |___|
             60-75µs 60-240µs
```

#### Write Bit

```
Write 1:
  ___     _______________________
     |___|
     1-15µs

Write 0:
  _______________     ___________
                 |___|
                 60-120µs
```

#### Read Bit

```
Master initiates:
  ___   _________________________
     |_|
     1-15µs

Slave responds (bit = 0):
  ___   _____________________
     |_______|
     60µs

Slave responds (bit = 1):
  ___   ___
     |_|   |___________________
     <15µs
```

### ROM Commands

| Command | Code | Description |
|---------|------|-------------|
| Skip ROM | 0xCC | ข้าม ROM addressing (single device) |
| Read ROM | 0x33 | อ่าน ROM address (single device only) |
| Match ROM | 0x55 | เลือก device ด้วย ROM address |
| Search ROM | 0xF0 | ค้นหา devices ทั้งหมด |
| Alarm Search | 0xEC | ค้นหา devices ที่มี alarm |

### Function Commands

| Command | Code | Description |
|---------|------|-------------|
| Convert T | 0x44 | เริ่มการวัดอุณหภูมิ |
| Read Scratchpad | 0xBE | อ่าน scratchpad memory |
| Write Scratchpad | 0x4E | เขียน scratchpad memory |
| Copy Scratchpad | 0x48 | คัดลอก scratchpad ไป EEPROM |
| Recall E² | 0xB8 | โหลดค่าจาก EEPROM |
| Read Power Supply | 0xB4 | ตรวจสอบ power mode |

---

## Hardware Setup

### Normal Power Mode

```
VCC (3.3V)
    |
  [4.7kΩ]
    |
    +---- DQ (Data) ---- GPIO Pin (PD2)
    |
 DS18B20
    |
   GND
```

**การต่อ:**
1. VDD (pin 3) → VCC (3.3V)
2. DQ (pin 2) → GPIO Pin + Pull-up 4.7kΩ
3. GND (pin 1) → GND

### Parasite Power Mode

```
VCC (3.3V)
    |
  [4.7kΩ]
    |
    +---- DQ (Data) ---- GPIO Pin (PD2)
    |
 DS18B20 (VDD → GND)
    |
   GND
```

**การต่อ:**
1. VDD (pin 3) → GND
2. DQ (pin 2) → GPIO Pin + Pull-up 4.7kΩ
3. GND (pin 1) → GND

### Multi-Sensor Setup

```
VCC (3.3V)
    |
  [4.7kΩ]
    |
    +---- DQ ----+---- DS18B20 #1
    |            |
    |            +---- DS18B20 #2
    |            |
    |            +---- DS18B20 #N
   GND
```

**หมายเหตุ:**
- ใช้ pull-up resistor ตัวเดียวสำหรับทุก sensors
- แต่ละ sensor ต้องมี ROM address ไม่ซ้ำกัน
- รองรับได้หลายสิบตัวบน bus เดียวกัน

### การเลือก Pull-up Resistor

| ความยาวสาย | Resistor | หมายเหตุ |
|-----------|----------|----------|
| < 10 m | 4.7kΩ | Standard |
| 10-50 m | 3.3kΩ | ลดค่าลง |
| > 50 m | 2.2kΩ | ใช้สายคุณภาพดี |

---

## การใช้งานพื้นฐาน

### 1. Single Sensor

```c
#include "DS18B20.h"

int main(void) {
    SystemCoreClockUpdate();
    
    // เริ่มต้น DS18B20 บน PD2
    DS18B20_Device* sensor = DS18B20_Init(PD2);
    
    while(1) {
        // อ่านอุณหภูมิแบบ blocking
        float temp = DS18B20_ReadTemperatureBlocking(sensor);
        printf("Temperature: %.2f C\\r\\n", temp);
        
        Delay_Ms(1000);
    }
}
```

### 2. Non-Blocking Reading

```c
// เริ่ม conversion
DS18B20_StartConversion(sensor);

// ทำงานอื่นๆ...

// รอให้ conversion เสร็จ
uint16_t time_ms = DS18B20_GetConversionTime(sensor->resolution);
Delay_Ms(time_ms);

// อ่านอุณหภูมิ
float temp = DS18B20_ReadTemperature(sensor);
```

### 3. ตั้งค่า Resolution

```c
// ตั้งค่าความละเอียด 12-bit (0.0625°C)
DS18B20_SetResolution(sensor, DS18B20_RES_12BIT);

// บันทึกลง EEPROM
DS18B20_SaveToEEPROM(sensor);
```

### 4. Alarm Threshold

```c
// ตั้งค่า alarm: TH=30°C, TL=20°C
DS18B20_SetAlarm(sensor, 30, 20);

// บันทึกลง EEPROM
DS18B20_SaveToEEPROM(sensor);

// ตรวจสอบ alarm
float temp = DS18B20_ReadTemperatureBlocking(sensor);
if (temp > 30 || temp < 20) {
    printf("ALARM!\\r\\n");
}
```

---

## การใช้งานขั้นสูง

### Multi-Sensor

```c
#define MAX_SENSORS  8

int main(void) {
    // เริ่มต้น 1-Wire bus
    OneWire_Bus* bus = OneWire_Init(PD2);
    
    // ค้นหา DS18B20 sensors ทั้งหมด
    uint8_t rom_list[MAX_SENSORS][8];
    uint8_t count = DS18B20_SearchDevices(bus, (uint8_t*)rom_list, MAX_SENSORS);
    
    printf("Found %d sensors\\r\\n", count);
    
    // สร้าง device instances
    DS18B20_Device* sensors[MAX_SENSORS];
    for (uint8_t i = 0; i < count; i++) {
        sensors[i] = DS18B20_InitWithROM(bus, rom_list[i]);
    }
    
    while(1) {
        // เริ่ม conversion ทุก sensors พร้อมกัน
        DS18B20_StartConversionAll(bus);
        Delay_Ms(750);
        
        // อ่านอุณหภูมิจากทุก sensors
        for (uint8_t i = 0; i < count; i++) {
            float temp = DS18B20_ReadTemperature(sensors[i]);
            printf("Sensor %d: %.2f C\\r\\n", i + 1, temp);
        }
        
        Delay_Ms(1000);
    }
}
```

### Alarm Search

```c
// ค้นหา sensors ที่มี alarm condition
uint8_t alarm_list[MAX_SENSORS][8];
uint8_t alarm_count = DS18B20_SearchAlarm(bus, (uint8_t*)alarm_list, MAX_SENSORS);

printf("Found %d sensors with alarm\\r\\n", alarm_count);

for (uint8_t i = 0; i < alarm_count; i++) {
    // สร้าง device instance
    DS18B20_Device* sensor = DS18B20_InitWithROM(bus, alarm_list[i]);
    
    // อ่านอุณหภูมิ
    float temp = DS18B20_ReadTemperatureBlocking(sensor);
    printf("Alarm sensor: %.2f C\\r\\n", temp);
}
```

### State Machine (Non-Blocking)

```c
typedef enum {
    STATE_IDLE,
    STATE_CONVERTING,
    STATE_READING
} TempState;

TempState state = STATE_IDLE;
uint32_t start_time;

while(1) {
    switch (state) {
        case STATE_IDLE:
            DS18B20_StartConversion(sensor);
            start_time = Get_CurrentMs();
            state = STATE_CONVERTING;
            break;
            
        case STATE_CONVERTING:
            if (Get_CurrentMs() - start_time >= 750) {
                state = STATE_READING;
            }
            // ทำงานอื่นๆ ระหว่างรอ
            break;
            
        case STATE_READING:
            float temp = DS18B20_ReadTemperature(sensor);
            printf("%.2f C\\r\\n", temp);
            Delay_Ms(1000);
            state = STATE_IDLE;
            break;
    }
}
```

---

## ROM Addressing

### โครงสร้าง 64-bit ROM

```
Byte 0: Family Code (0x28 สำหรับ DS18B20)
Byte 1-6: Serial Number (48-bit unique)
Byte 7: CRC8
```

### การอ่าน ROM Address

```c
uint8_t rom[8];

// Single device: ใช้ Read ROM
OneWire_ReadROM(bus, rom);

// Multi-device: ใช้ Search ROM
OneWire_ResetSearch(bus);
while (OneWire_Search(bus)) {
    OneWire_GetAddress(bus, rom);
    
    // ตรวจสอบว่าเป็น DS18B20
    if (DS18B20_VerifyDevice(rom)) {
        printf("DS18B20 found\\r\\n");
    }
}
```

### การแยก ROM Address

```c
uint8_t family_code = rom[0];  // 0x28
uint8_t serial[6];
memcpy(serial, &rom[1], 6);
uint8_t crc = rom[7];

// ตรวจสอบ CRC
if (OneWire_VerifyCRC(rom, 8)) {
    printf("CRC valid\\r\\n");
}
```

---

## Parasite Power Mode

### หลักการทำงาน

ในโหมด parasite power, DS18B20 ดึง power จาก data line แทนที่จะใช้ VDD pin

**ข้อดี:**
- ใช้สายน้อยลง (2 สาย แทน 3 สาย)
- เหมาะสำหรับระยะไกล

**ข้อเสีย:**
- ต้องใช้ strong pull-up ระหว่าง conversion
- จำกัดจำนวน sensors บน bus

### การตรวจสอบ Power Mode

```c
bool normal_power = DS18B20_ReadPowerSupply(sensor);

if (normal_power) {
    printf("Normal power mode\\r\\n");
} else {
    printf("Parasite power mode\\r\\n");
}
```

### การใช้งาน

```c
// เริ่ม conversion
DS18B20_StartConversion(sensor);

// ในโหมด parasite power ต้องใช้ strong pull-up
// (ใช้ pull-up resistor ภายนอกหรือวงจร MOSFET)

// รอให้ conversion เสร็จ
Delay_Ms(750);

// อ่านอุณหภูมิ
float temp = DS18B20_ReadTemperature(sensor);
```

---

## Troubleshooting

### ปัญหาที่พบบ่อย

#### 1. ไม่พบ Sensor (No Presence Pulse)

**สาเหตุ:**
- ต่อสายผิด
- ไม่มี pull-up resistor
- Sensor เสีย
- ระยะทางไกลเกินไป

**วิธีแก้:**
- ตรวจสอบการต่อสาย
- เพิ่ม pull-up resistor 4.7kΩ
- ลดค่า pull-up resistor (3.3kΩ, 2.2kΩ)
- ใช้สายคุณภาพดี

#### 2. อ่านค่าผิดพลาด (CRC Error)

**สาเหตุ:**
- Noise บนสาย
- Pull-up resistor ไม่เหมาะสม
- Timing ไม่แม่นยำ

**วิธีแก้:**
- ใช้สาย shielded
- ปรับค่า pull-up resistor
- ตรวจสอบ system clock
- อ่านซ้ำถ้า CRC ผิด

#### 3. อุณหภูมิผิดปกติ

**85.0°C:**
- Power-on reset value
- ยังไม่ได้ทำ conversion
- รอให้ conversion เสร็จก่อนอ่าน

**-127.0°C:**
- Sensor ไม่ตอบสนอง
- ตรวจสอบการต่อสาย

#### 4. Timing Issues

**วิธีแก้:**
- ตรวจสอบว่า `SystemCoreClockUpdate()` ถูกเรียก
- ตรวจสอบ `Delay_Us()` ทำงานถูกต้อง
- ปิด interrupts ระหว่าง critical timing

---

## API Reference

### Initialization

#### `DS18B20_Init(pin)`
เริ่มต้น DS18B20 แบบ single device

**Parameters:**
- `pin`: GPIO pin number

**Returns:** `DS18B20_Device*` หรือ NULL

#### `DS18B20_InitWithROM(bus, rom)`
เริ่มต้น DS18B20 ด้วย ROM address

**Parameters:**
- `bus`: OneWire_Bus instance
- `rom`: ROM address 8 bytes

**Returns:** `DS18B20_Device*` หรือ NULL

### Temperature Reading

#### `DS18B20_ReadTemperatureBlocking(sensor)`
อ่านอุณหภูมิแบบ blocking

**Returns:** อุณหภูมิ (°C) หรือ NAN

#### `DS18B20_StartConversion(sensor)`
เริ่มการแปลงสัญญาณ

**Returns:** true/false

#### `DS18B20_ReadTemperature(sensor)`
อ่านอุณหภูมิ (ต้อง start conversion ก่อน)

**Returns:** อุณหภูมิ (°C) หรือ NAN

### Configuration

#### `DS18B20_SetResolution(sensor, resolution)`
ตั้งค่าความละเอียด

**Parameters:**
- `resolution`: DS18B20_RES_9BIT - DS18B20_RES_12BIT

#### `DS18B20_SetAlarm(sensor, th, tl)`
ตั้งค่า alarm thresholds

**Parameters:**
- `th`: High threshold (°C)
- `tl`: Low threshold (°C)

#### `DS18B20_SaveToEEPROM(sensor)`
บันทึกค่าลง EEPROM

### Multi-Sensor

#### `DS18B20_SearchDevices(bus, rom_list, max_devices)`
ค้นหา DS18B20 sensors ทั้งหมด

**Returns:** จำนวน devices ที่พบ

#### `DS18B20_SearchAlarm(bus, rom_list, max_devices)`
ค้นหา devices ที่มี alarm

**Returns:** จำนวน devices ที่มี alarm

---

## Examples

Library มี 7 ตัวอย่างการใช้งาน:

1. **01_BasicReading.c** - การอ่านอุณหภูมิพื้นฐาน
2. **02_MultiSensor.c** - การใช้งานหลาย sensors
3. **03_AlarmThreshold.c** - การตั้งค่า alarm
4. **04_HighResolution.c** - การเปรียบเทียบความละเอียด
5. **05_ParasitePower.c** - Parasite power mode
6. **06_ROMSearch.c** - ROM search algorithm
7. **07_Advanced.c** - เทคนิคขั้นสูง

---

## License

MIT License

## Author

CH57x Simple HAL Team

## Version

1.0 (2025-12-22)

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)