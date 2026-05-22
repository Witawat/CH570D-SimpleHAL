# AT24Cxx I2C EEPROM Library

> **Library สำหรับอ่าน/เขียน AT24Cxx EEPROM ผ่าน I2C บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [คุณสมบัติ](#คุณสมบัติ)
3. [Hardware Setup](#hardware-setup)
4. [หลักการทำงาน](#หลักการทำงาน)
5. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
6. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
7. [Troubleshooting](#troubleshooting)
8. [API Reference](#api-reference)

---

## ภาพรวม

AT24Cxx คือ EEPROM (Electrically Erasable Programmable Read-Only Memory) ที่สื่อสารผ่าน I2C  
ใช้สำหรับ **เก็บข้อมูลถาวร** ที่ไม่หายแม้ไฟดับ เช่น การตั้งค่า, calibration data, log

| รุ่น | ความจุ | Page Size | I2C Address |
|------|--------|-----------|------------|
| AT24C01 | 128B | 8B | 0x50-0x57 |
| AT24C02 | 256B | 8B | 0x50-0x57 |
| AT24C04 | 512B | 16B | 0x50-0x53 |
| AT24C08 | 1KB | 16B | 0x50-0x51 |
| AT24C16 | 2KB | 16B | 0x50 only |
| AT24C32 | 4KB | 32B | 0x50-0x57 |
| AT24C64 | 8KB | 32B | 0x50-0x57 |
| AT24C128 | 16KB | 64B | 0x50-0x57 |
| AT24C256 | 32KB | 64B | 0x50-0x57 |
| AT24C512 | 64KB | 128B | 0x50-0x53 |

---

## คุณสมบัติ

- ✅ รองรับทุกรุ่น AT24C01 ถึง AT24C512
- ✅ WriteByte, ReadByte
- ✅ WriteArray, ReadArray (Page Write — เร็วกว่า byte-by-byte)
- ✅ WriteString, ReadString (null-terminated)
- ✅ WriteUint32, ReadUint32 (Little Endian)
- ✅ EraseAll (เขียน 0xFF ทุก address)
- ✅ ข้าม page boundary อัตโนมัติ
- ✅ Write cycle polling (รอให้ EEPROM พร้อมก่อน)
- ✅ เอกสารภาษาไทยครบถ้วน

---

## Hardware Setup

### วงจร AT24C02

```
CH57x              AT24C02 (DIP-8)
                      +------+
GND  ------------> 1 | A0  VCC| 8 <----- 3.3V
GND  ------------> 2 | A1  WP | 7 <----- GND  ← WP=LOW (เปิด write)
GND  ------------> 3 | A2 SCL | 6 <----- PC2 (SCL) + [4.7kΩ → 3.3V]
GND  ------------> 4 | GND SDA| 5 <----- PC1 (SDA) + [4.7kΩ → 3.3V]
                      +------+
```

> ⚠️ **สำคัญ**: ต้องต่อ **pull-up 4.7kΩ** ที่ SDA และ SCL ทุกครั้ง

### การตั้ง I2C Address

```
I2C Address = 0x50 | (A2 << 2) | (A1 << 1) | A0

ตัวอย่าง:
  A2=0, A1=0, A0=0 → address = 0x50  (default)
  A2=0, A1=0, A0=1 → address = 0x51
  A2=1, A1=1, A0=1 → address = 0x57
```

ต่อ Module หลาย IC บน bus เดียวกัน โดยเปลี่ยน A0-A2:
```
Bus I2C:
  MCU ────────────────────────────────────── SDA
        |               |               |
      AT24C02(0x50)  AT24C02(0x51)  AT24C32(0x52)
      A0=GND         A0=VCC         A0=VCC
```

---

## หลักการทำงาน

### Write Cycle Time

AT24Cxx ต้องการเวลา **5ms** หลังจาก write ก่อนที่จะ write ใหม่ได้  
Library ใช้ **Acknowledge Polling** เพื่อตรวจสอบว่า EEPROM พร้อมแล้ว

```
Write Sequence:
  MCU → [START] [DEV_ADDR+W] [MEM_ADDR_H] [MEM_ADDR_L] [DATA] [STOP]
                                                                   ↓
                                                          EEPROM เริ่ม write (5ms)
  
  Polling:
  MCU → [START] [DEV_ADDR+W] → EEPROM ตอบ NACK ถ้ายัง busy
                              → EEPROM ตอบ ACK ถ้าพร้อมแล้ว ✓
```

### Page Write

การเขียนครั้งละหลาย byte ใน **page เดียวกัน** จะเร็วกว่าการเขียนทีละ byte  
เช่น AT24C02 (page 8 bytes) — เขียน 8 bytes ใช้เวลา 5ms แทนที่จะเป็น 8×5ms = 40ms

```
Page Write ตัวอย่าง (page size = 8):
  Address 0x00-0x07 → page 0
  Address 0x08-0x0F → page 1

  เขียน 20 bytes จาก address 0x05:
    Chunk 1: address 0x05-0x07 (3 bytes, จนจบ page 0)
    Chunk 2: address 0x08-0x0F (8 bytes, page 1 เต็ม)
    Chunk 3: address 0x10-0x12 (3 bytes, เริ่ม page 2)
```

---

## การใช้งานพื้นฐาน

### ขั้นตอนที่ 1: Include และประกาศตัวแปร

```c
#include "SimpleHAL.h"
#include "AT24Cxx.h"

AT24Cxx_Instance eeprom;
```

### ขั้นตอนที่ 2: Init ใน main()

```c
int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);  // SCL=PC2, SDA=PC1

    AT24Cxx_Init(&eeprom, AT24C02, 0x50);  // AT24C02, address 0x50
```

### ขั้นตอนที่ 3: อ่าน-เขียน

```c
    // เขียน 1 byte ที่ address 0x00
    AT24Cxx_WriteByte(&eeprom, 0x00, 0xAB);

    // อ่าน 1 byte กลับ
    uint8_t val;
    AT24Cxx_ReadByte(&eeprom, 0x00, &val);
    printf("Read: 0x%02X\r\n", val);  // ควรได้ 0xAB

    // เขียน array
    uint8_t data_w[] = {10, 20, 30, 40, 50};
    AT24Cxx_WriteArray(&eeprom, 0x10, data_w, 5);

    // อ่าน array
    uint8_t data_r[5] = {0};
    AT24Cxx_ReadArray(&eeprom, 0x10, data_r, 5);

    while (1) {}
}
```

---

## การใช้งานขั้นสูง

### บันทึกและโหลด Configuration

```c
typedef struct {
    uint16_t version;
    float    temp_offset;
    uint8_t  brightness;
    char     device_name[16];
} AppConfig_t;

AppConfig_t config;

// บันทึก config
void save_config(void) {
    AT24Cxx_WriteArray(&eeprom, 0x00, (uint8_t*)&config, sizeof(AppConfig_t));
    printf("Config saved.\r\n");
}

// โหลด config
void load_config(void) {
    AT24Cxx_ReadArray(&eeprom, 0x00, (uint8_t*)&config, sizeof(AppConfig_t));
    if (config.version != 0x0100) {
        // EEPROM ใหม่หรือ version ไม่ตรง — ใช้ค่า default
        config.version    = 0x0100;
        config.temp_offset = 0.0f;
        config.brightness  = 50;
        strcpy(config.device_name, "CH57x");
        save_config();
    }
    printf("Config loaded: %s\r\n", config.device_name);
}
```

### บันทึก uint32_t (Counter)

```c
uint32_t boot_count = 0;

// โหลด counter
AT24Cxx_ReadUint32(&eeprom, 0xF0, &boot_count);
boot_count++;
printf("Boot count: %lu\r\n", boot_count);

// บันทึกกลับ
AT24Cxx_WriteUint32(&eeprom, 0xF0, boot_count);
```

### เขียน/อ่าน String

```c
AT24Cxx_WriteString(&eeprom, 0x00, "Hello EEPROM!");

char buf[20];
AT24Cxx_ReadString(&eeprom, 0x00, buf, 20);
printf("String: %s\r\n", buf);  // Hello EEPROM!
```

### Error Handling

```c
AT24Cxx_Status st = AT24Cxx_WriteByte(&eeprom, 0x00, 0x42);
if (st != AT24CXX_OK) {
    printf("Write error: %s\r\n", AT24Cxx_StatusStr(st));
}
```

---

## Troubleshooting

### ปัญหา: อ่านค่าได้ 0xFF ตลอด

| สาเหตุ | วิธีแก้ |
|--------|---------|
| EEPROM ยังไม่เคย write | ค่าเริ่มต้นของ EEPROM ใหม่คือ 0xFF ถือว่าปกติ |
| I2C address ผิด | ใช้ I2CScan เพื่อหา address จริง |
| Pull-up ขาด | ต่อ 4.7kΩ ที่ SDA และ SCL → 3.3V |

### ปัญหา: Write แล้วข้อมูลไม่คงอยู่

| สาเหตุ | วิธีแก้ |
|--------|---------|
| WP (pin 7) ต่อ HIGH | ต่อ WP → GND |
| ไม่รอ write cycle | ตรวจว่า `_wait_write_done()` ทำงาน (เพิ่ม retry) |
| I2C error ระหว่าง write | ตรวจ error code ของ `AT24Cxx_WriteByte()` |

### ปัญหา: เขียนข้อมูล array แล้ว address แรกถูกต้องแต่ที่เหลือผิด

| สาเหตุ | วิธีแก้ |
|--------|---------|
| Page boundary overflow | library ข้าม page อัตโนมัติ แต่ตรวจว่า page_size ถูกต้อง |
| ใช้ type ผิด | ตรวจว่า AT24Cxx_Init ใส่ type ถูกต้อง (เช่น AT24C02 ≠ AT24C32) |

---

## API Reference

### `AT24Cxx_Init(eeprom, type, i2c_addr)` → `AT24Cxx_Status`
เริ่มต้น EEPROM — ต้องเรียก I2C_SimpleInit() ก่อน

| Parameter | Type | คำอธิบาย |
|-----------|------|----------|
| `eeprom` | `AT24Cxx_Instance*` | ตัวแปร instance |
| `type` | `AT24Cxx_Type` | เช่น `AT24C02`, `AT24C32` |
| `i2c_addr` | `uint8_t` | I2C address 7-bit (default `0x50`) |

---

### `AT24Cxx_WriteByte(eeprom, address, data)` → `AT24Cxx_Status`
เขียน 1 byte — รอ write cycle อัตโนมัติ (5ms)

---

### `AT24Cxx_ReadByte(eeprom, address, &data)` → `AT24Cxx_Status`
อ่าน 1 byte

---

### `AT24Cxx_WriteArray(eeprom, address, data[], len)` → `AT24Cxx_Status`
เขียน array (page write — ข้าม page boundary อัตโนมัติ)

---

### `AT24Cxx_ReadArray(eeprom, address, buf[], len)` → `AT24Cxx_Status`
อ่าน array (sequential read)

---

### `AT24Cxx_WriteString(eeprom, address, str)` → `AT24Cxx_Status`
เขียน string พร้อม null terminator

---

### `AT24Cxx_ReadString(eeprom, address, buf, max_len)` → `AT24Cxx_Status`
อ่าน string

---

### `AT24Cxx_WriteUint32(eeprom, address, value)` → `AT24Cxx_Status`
เขียน 32-bit integer (4 bytes, Little Endian)

---

### `AT24Cxx_ReadUint32(eeprom, address, &value)` → `AT24Cxx_Status`
อ่าน 32-bit integer

---

### `AT24Cxx_EraseAll(eeprom)` → `AT24Cxx_Status`
ลบ EEPROM ทั้งหมด (เขียน 0xFF) — ใช้เวลานาน

---

### `AT24Cxx_GetCapacity(eeprom)` → `uint32_t`
คืนขนาดความจุ (bytes)

---

### `AT24Cxx_StatusStr(status)` → `const char*`
แปลง error code เป็น string

---

### AT24Cxx_Status Values

| ค่า | ความหมาย |
|-----|----------|
| `AT24CXX_OK` | สำเร็จ |
| `AT24CXX_ERROR_PARAM` | Parameter ผิด (NULL pointer หรือ type ไม่ถูกต้อง) |
| `AT24CXX_ERROR_I2C` | I2C communication error |
| `AT24CXX_ERROR_ADDR_OOB` | Address เกินขนาด EEPROM |
| `AT24CXX_ERROR_TIMEOUT` | รอ write cycle นานเกินกำหนด |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)