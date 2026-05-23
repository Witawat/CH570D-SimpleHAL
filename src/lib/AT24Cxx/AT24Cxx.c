/**
 * @file AT24Cxx.c
 * @brief AT24Cxx I2C EEPROM Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "AT24Cxx.h"

/* ========== Private Variables ========== */

/**
 * @brief ข้อมูลแต่ละรุ่น: {capacity, page_size, addr_bytes}
 */
static const struct {
    uint32_t capacity;
    uint16_t page_size;
    uint8_t  addr_bytes;
} _eeprom_info[] = {
    /* AT24C01  */ { 128,    8,  1 },
    /* AT24C02  */ { 256,    8,  1 },
    /* AT24C04  */ { 512,   16,  1 },
    /* AT24C08  */ { 1024,  16,  1 },
    /* AT24C16  */ { 2048,  16,  1 },
    /* AT24C32  */ { 4096,  32,  2 },
    /* AT24C64  */ { 8192,  32,  2 },
    /* AT24C128 */ { 16384, 64,  2 },
    /* AT24C256 */ { 32768, 64,  2 },
    /* AT24C512 */ { 65536, 128, 2 },
};

/* ========== Private Functions ========== */

/**
 * @brief สร้าง I2C address ที่ถูกต้อง สำหรับ AT24C04/08/16
 *        (บาง chip ใช้บิตใน I2C address เป็นส่วนหนึ่งของ mem address)
 */
static uint8_t _get_dev_addr(AT24Cxx_Instance* eeprom, uint32_t mem_addr) {
    uint8_t dev_addr = eeprom->i2c_addr;

    /* AT24C04 (512B): bit 8 ของ address ใส่ใน A0 ของ I2C device address */
    if (eeprom->type == AT24C04) {
        dev_addr = (eeprom->i2c_addr & 0xFE) | ((mem_addr >> 8) & 0x01);
    }
    /* AT24C08 (1KB): bit 9-8 ของ address ใส่ใน A1-A0 */
    else if (eeprom->type == AT24C08) {
        dev_addr = (eeprom->i2c_addr & 0xFC) | ((mem_addr >> 8) & 0x03);
    }
    /* AT24C16 (2KB): bit 10-8 ของ address ใส่ใน A2-A0 */
    else if (eeprom->type == AT24C16) {
        dev_addr = (eeprom->i2c_addr & 0xF8) | ((mem_addr >> 8) & 0x07);
    }
    return dev_addr;
}

/**
 * @brief ส่ง I2C address bytes ก่อน read/write
 */
static I2C_Status _set_address(AT24Cxx_Instance* eeprom, uint32_t mem_addr) {
    uint8_t addr_buf[2];
    uint8_t dev_addr = _get_dev_addr(eeprom, mem_addr);

    if (eeprom->addr_bytes == 2) {
        addr_buf[0] = (uint8_t)(mem_addr >> 8);
        addr_buf[1] = (uint8_t)(mem_addr & 0xFF);
        return I2C_Write(dev_addr, addr_buf, 2);
    } else {
        addr_buf[0] = (uint8_t)(mem_addr & 0xFF);
        return I2C_Write(dev_addr, addr_buf, 1);
    }
}

/**
 * @brief รอให้ EEPROM write cycle เสร็จสิ้น (Acknowledge Polling)
 */
static AT24Cxx_Status _wait_write_done(AT24Cxx_Instance* eeprom) {
    for (uint8_t i = 0; i < AT24CXX_WRITE_RETRY; i++) {
        Delay_Ms(AT24CXX_WRITE_CYCLE_MS);
        uint8_t dummy;
        I2C_Status st = I2C_Read(eeprom->i2c_addr, &dummy, 1);
        if (st == I2C_OK) return AT24CXX_OK;
    }
    return AT24CXX_ERROR_TIMEOUT;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น AT24Cxx EEPROM
 *
 * @param eeprom    ตัวแปร instance
 * @param type      ประเภท EEPROM เช่น AT24C02, AT24C32
 * @param i2c_addr  I2C address (7-bit, default 0x50)
 *
 * @return AT24CXX_OK หรือ AT24CXX_ERROR_PARAM ถ้า type ผิด
 *
 * @note คำนวณ capacity, page_size, addr_bytes จาก _eeprom_info[] ตาม type
 *       ไม่ได้ทดสอบ I2C connection — แค่เก็บค่า instance
 *       AT24C04/08/16 ใช้ bit ใน I2C address เป็นส่วนของ memory address
 */
AT24Cxx_Status AT24Cxx_Init(AT24Cxx_Instance* eeprom, AT24Cxx_Type type, uint8_t i2c_addr) {
    if (eeprom == NULL) return AT24CXX_ERROR_PARAM;
    if (type > AT24C512)  return AT24CXX_ERROR_PARAM;

    eeprom->type        = type;
    eeprom->i2c_addr    = i2c_addr;
    eeprom->capacity    = _eeprom_info[type].capacity;
    eeprom->page_size   = _eeprom_info[type].page_size;
    eeprom->addr_bytes  = _eeprom_info[type].addr_bytes;
    eeprom->initialized = 1;

    return AT24CXX_OK;
}

/**
 * @brief เขียน 1 byte
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งใน EEPROM (0-based)
 * @param data    ค่าที่ต้องการเขียน
 *
 * @return AT24CXX_OK, AT24CXX_ERROR_ADDR_OOB, AT24CXX_ERROR_I2C, หรือ AT24CXX_ERROR_TIMEOUT
 *
 * @note สร้าง I2C write packet: [addr_hi(opt)] [addr_lo] [data]
 *       รอ write cycle (~5ms) ด้วย acknowledge polling หลัง write เสร็จ
 *       AT24C04/08/16 ใช้ _get_dev_addr() เพื่อรวม bit address ใน slave address
 *       address ต้องไม่เกิน capacity-1
 */
AT24Cxx_Status AT24Cxx_WriteByte(AT24Cxx_Instance* eeprom, uint32_t address, uint8_t data) {
    if (eeprom == NULL || !eeprom->initialized) return AT24CXX_ERROR_PARAM;
    if (address >= eeprom->capacity)             return AT24CXX_ERROR_ADDR_OOB;

    uint8_t  dev_addr = _get_dev_addr(eeprom, address);
    I2C_Status st;

    if (eeprom->addr_bytes == 2) {
        uint8_t buf[3];
        buf[0] = (uint8_t)(address >> 8);
        buf[1] = (uint8_t)(address & 0xFF);
        buf[2] = data;
        st = I2C_Write(dev_addr, buf, 3);
    } else {
        uint8_t buf[2];
        buf[0] = (uint8_t)(address & 0xFF);
        buf[1] = data;
        st = I2C_Write(dev_addr, buf, 2);
    }

    if (st != I2C_OK) return AT24CXX_ERROR_I2C;
    return _wait_write_done(eeprom);
}

/**
 * @brief อ่าน 1 byte
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งใน EEPROM
 * @param data    pointer สำหรับรับค่า
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note ส่ง dummy write (_set_address) เพื่อตั้ง internal address pointer
 *       แล้วจึง I2C read 1 byte
 *       AT24C04/08/16 ใช้ _get_dev_addr() สำหรับ slave address
 *       address ต้องไม่เกิน capacity-1
 */
AT24Cxx_Status AT24Cxx_ReadByte(AT24Cxx_Instance* eeprom, uint32_t address, uint8_t* data) {
    if (eeprom == NULL || !eeprom->initialized || data == NULL) return AT24CXX_ERROR_PARAM;
    if (address >= eeprom->capacity) return AT24CXX_ERROR_ADDR_OOB;

    uint8_t dev_addr = _get_dev_addr(eeprom, address);
    I2C_Status st    = _set_address(eeprom, address);
    if (st != I2C_OK) return AT24CXX_ERROR_I2C;

    st = I2C_Read(dev_addr, data, 1);
    return (st == I2C_OK) ? AT24CXX_OK : AT24CXX_ERROR_I2C;
}

/**
 * @brief เขียน array ของ bytes (page write — เร็วกว่า byte-by-byte)
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param data    pointer ของข้อมูล
 * @param len     จำนวน bytes
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note แบ่งข้อมูลเป็น chunk ตาม page boundary — ข้าม page boundary อัตโนมัติ
 *       แต่ละ chunk: [addr_hi(opt)] [addr_lo] [data...]
 *       รอ write cycle (acknowledge polling) หลังแต่ละ chunk
 *       len=0 → return OK ทันที
 *       address+len ต้องไม่เกิน capacity
 */
AT24Cxx_Status AT24Cxx_WriteArray(AT24Cxx_Instance* eeprom, uint32_t address,
                                   const uint8_t* data, uint16_t len) {
    if (eeprom == NULL || !eeprom->initialized || data == NULL) return AT24CXX_ERROR_PARAM;
    if (len == 0) return AT24CXX_OK;
    if (address + len > eeprom->capacity) return AT24CXX_ERROR_ADDR_OOB;

    uint32_t  cur_addr  = address;
    uint16_t  remaining = len;
    const uint8_t* ptr  = data;

    while (remaining > 0) {
        /* คำนวณขนาด chunk ที่ fit ใน page เดียว */
        uint16_t page_offset  = (uint16_t)(cur_addr % eeprom->page_size);
        uint16_t space_in_page = eeprom->page_size - page_offset;
        uint16_t chunk = (remaining < space_in_page) ? remaining : space_in_page;

        /* สร้าง buffer: [addr_hi(opt)] [addr_lo] [data...] */
        uint8_t buf[2 + 128]; /* max page 128B */
        uint8_t buf_len = 0;

        uint8_t dev_addr = _get_dev_addr(eeprom, cur_addr);

        if (eeprom->addr_bytes == 2) {
            buf[buf_len++] = (uint8_t)(cur_addr >> 8);
        }
        buf[buf_len++] = (uint8_t)(cur_addr & 0xFF);

        for (uint16_t i = 0; i < chunk; i++) {
            buf[buf_len++] = ptr[i];
        }

        I2C_Status st = I2C_Write(dev_addr, buf, buf_len);
        if (st != I2C_OK) return AT24CXX_ERROR_I2C;

        AT24Cxx_Status ws = _wait_write_done(eeprom);
        if (ws != AT24CXX_OK) return ws;

        cur_addr  += chunk;
        ptr       += chunk;
        remaining -= chunk;
    }

    return AT24CXX_OK;
}

/**
 * @brief อ่าน array ของ bytes (sequential read)
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param data    buffer สำหรับรับค่า
 * @param len     จำนวน bytes
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note ใช้ I2C sequential read หลังจาก _set_address() ตั้ง internal address
 *       EEPROM จะ increment address pointer อัตโนมัติระหว่างอ่าน
 *       len=0 → return OK ทันที
 *       address+len ต้องไม่เกิน capacity
 */
AT24Cxx_Status AT24Cxx_ReadArray(AT24Cxx_Instance* eeprom, uint32_t address,
                                  uint8_t* data, uint16_t len) {
    if (eeprom == NULL || !eeprom->initialized || data == NULL) return AT24CXX_ERROR_PARAM;
    if (len == 0) return AT24CXX_OK;
    if (address + len > eeprom->capacity) return AT24CXX_ERROR_ADDR_OOB;

    uint8_t dev_addr = _get_dev_addr(eeprom, address);
    I2C_Status st    = _set_address(eeprom, address);
    if (st != I2C_OK) return AT24CXX_ERROR_I2C;

    st = I2C_Read(dev_addr, data, len);
    return (st == I2C_OK) ? AT24CXX_OK : AT24CXX_ERROR_I2C;
}

/**
 * @brief เขียน string (null-terminated)
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param str     string ที่ต้องการเขียน (บันทึก null terminator ด้วย)
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note ใช้ AT24Cxx_WriteArray() กับ length = strlen(str) + 1
 *       address+len ต้องไม่เกิน capacity
 */
AT24Cxx_Status AT24Cxx_WriteString(AT24Cxx_Instance* eeprom, uint32_t address,
                                    const char* str) {
    if (str == NULL) return AT24CXX_ERROR_PARAM;
    uint16_t len = (uint16_t)(strlen(str) + 1); /* รวม null terminator */
    return AT24Cxx_WriteArray(eeprom, address, (const uint8_t*)str, len);
}

/**
 * @brief อ่าน string
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่งเริ่มต้น
 * @param buf     buffer สำหรับรับ string
 * @param max_len ขนาด buffer สูงสุด (รวม null terminator)
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note ใช้ AT24Cxx_ReadArray() แล้วปิดท้ายด้วย null terminator
 *       buf[max_len-1] = '\0' ป้องกัน buffer overflow
 */
AT24Cxx_Status AT24Cxx_ReadString(AT24Cxx_Instance* eeprom, uint32_t address,
                                   char* buf, uint16_t max_len) {
    if (buf == NULL || max_len == 0) return AT24CXX_ERROR_PARAM;
    AT24Cxx_Status st = AT24Cxx_ReadArray(eeprom, address, (uint8_t*)buf, max_len);
    buf[max_len - 1] = '\0'; /* ป้องกัน buffer overflow */
    return st;
}

/**
 * @brief เขียน uint32_t (4 bytes, Little Endian)
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่ง (ต้องเผื่อ 4 bytes)
 * @param value   ค่าที่ต้องการเขียน
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note Little Endian: byte0 = LSB, byte3 = MSB
 *       ใช้ AT24Cxx_WriteArray() ขนาด 4 bytes
 */
AT24Cxx_Status AT24Cxx_WriteUint32(AT24Cxx_Instance* eeprom, uint32_t address, uint32_t value) {
    uint8_t buf[4];
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
    buf[2] = (uint8_t)((value >> 16) & 0xFF);
    buf[3] = (uint8_t)((value >> 24) & 0xFF);
    return AT24Cxx_WriteArray(eeprom, address, buf, 4);
}

/**
 * @brief อ่าน uint32_t
 *
 * @param eeprom  ตัวแปร instance
 * @param address ตำแหน่ง
 * @param value   pointer สำหรับรับค่า
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note Little Endian: รวม 4 bytes จาก AT24Cxx_ReadArray()
 *       value ถูกเขียนเมื่อ status == OK เท่านั้น
 */
AT24Cxx_Status AT24Cxx_ReadUint32(AT24Cxx_Instance* eeprom, uint32_t address, uint32_t* value) {
    if (value == NULL) return AT24CXX_ERROR_PARAM;
    uint8_t buf[4];
    AT24Cxx_Status st = AT24Cxx_ReadArray(eeprom, address, buf, 4);
    if (st == AT24CXX_OK) {
        *value = (uint32_t)buf[0]
               | ((uint32_t)buf[1] << 8)
               | ((uint32_t)buf[2] << 16)
               | ((uint32_t)buf[3] << 24);
    }
    return st;
}

/**
 * @brief ลบ EEPROM ทั้งหมด (เขียน 0xFF ทุก address)
 *
 * @param eeprom ตัวแปร instance
 *
 * @return AT24CXX_OK หรือ error code
 *
 * @note วนลูปเขียน 0xFF ทีละ page จนเต็ม capacity
 *       ใช้ AT24Cxx_WriteArray() ซึ่งจัดการ page boundary + acknowledge polling
 *       ใช้เวลานาน (capacity / page_size × write_cycle_time)
 */
AT24Cxx_Status AT24Cxx_EraseAll(AT24Cxx_Instance* eeprom) {
    if (eeprom == NULL || !eeprom->initialized) return AT24CXX_ERROR_PARAM;

    uint8_t page_buf[128];
    memset(page_buf, 0xFF, sizeof(page_buf));

    for (uint32_t addr = 0; addr < eeprom->capacity; addr += eeprom->page_size) {
        AT24Cxx_Status st = AT24Cxx_WriteArray(eeprom, addr, page_buf, eeprom->page_size);
        if (st != AT24CXX_OK) return st;
    }
    return AT24CXX_OK;
}

/**
 * @brief ดูขนาดความจุของ EEPROM (bytes)
 *
 * @param eeprom ตัวแปร instance
 *
 * @return ขนาด bytes หรือ 0 ถ้าไม่ได้ init
 *
 * @note คืนค่า eeprom->capacity ซึ่งถูกตั้งใน AT24Cxx_Init()
 */
uint32_t AT24Cxx_GetCapacity(AT24Cxx_Instance* eeprom) {
    if (eeprom == NULL || !eeprom->initialized) return 0;
    return eeprom->capacity;
}

/**
 * @brief แปลง error code เป็น string
 *
 * @param status error code
 *
 * @return ชื่อ error เช่น "OK", "ERROR_I2C"
 *
 * @note switch-case แบบตรงตัว — คืน "UNKNOWN" ถ้าไม่ตรง
 */
const char* AT24Cxx_StatusStr(AT24Cxx_Status status) {
    switch (status) {
        case AT24CXX_OK:             return "OK";
        case AT24CXX_ERROR_PARAM:    return "ERROR_PARAM";
        case AT24CXX_ERROR_I2C:      return "ERROR_I2C";
        case AT24CXX_ERROR_ADDR_OOB: return "ERROR_ADDR_OOB";
        case AT24CXX_ERROR_TIMEOUT:  return "ERROR_TIMEOUT";
        default:                     return "UNKNOWN";
    }
}
