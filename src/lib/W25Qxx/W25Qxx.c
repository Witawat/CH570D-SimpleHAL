/**
 * @file W25Qxx.c
 * @brief W25Qxx SPI Flash Memory Implementation
 */

#include "W25Qxx.h"

/* ========== Private: CS helpers ========== */

/**
 * @brief CS LOW — เริ่ม SPI transaction
 *
 * @param f - instance flash
 */
static inline void _cs_low(W25Qxx_Instance* f) {
    digitalWrite(f->pin_cs, 0);
}

/**
 * @brief CS HIGH — จบ SPI transaction
 *
 * @param f - instance flash
 */
static inline void _cs_high(W25Qxx_Instance* f) {
    digitalWrite(f->pin_cs, 1);
}

/* ========== Private: Basic operations ========== */

/**
 * @brief อ่าน status register 1
 *
 * @param f - instance flash
 *
 * @return 8-bit status register value
 *
 * @note bit 0 = BUSY, bit 1 = WEL (Write Enable Latch)
 *       ใช้สำหรับตรวจสอบ busy ก่อนดำเนินการใด ๆ
 */
static uint8_t _read_status1(W25Qxx_Instance* f) {
    _cs_low(f);
    SPI_Transfer(W25Q_CMD_READ_STATUS1);
    uint8_t s = SPI_Transfer(0xFF);
    _cs_high(f);
    return s;
}

/**
 * @brief ส่ง Write Enable command
 *
 * @param f - instance flash
 *
 * @note ต้องเรียกก่อน PAGE_PROGRAM, SECTOR_ERASE, CHIP_ERASE
 *       ตั้ง WEL bit ใน status register
 */
static void _write_enable(W25Qxx_Instance* f) {
    _cs_low(f);
    SPI_Transfer(W25Q_CMD_WRITE_ENABLE);
    _cs_high(f);
}

/**
 * @brief รอจนกว่า flash จะไม่ busy
 *
 * @param f - instance flash
 * @param timeout_ms - timeout (ms)
 *
 * @return W25Q_OK ถ้าไม่ busy, W25Q_ERROR_BUSY ถ้า timeout
 *
 * @note วน loop อ่าน status register จน BUSY bit = 0
 *       polling rate = 1ms
 */
static W25Qxx_Status _wait_busy(W25Qxx_Instance* f, uint32_t timeout_ms) {
    uint32_t start = Get_CurrentMs();
    while (_read_status1(f) & W25Q_STATUS_BUSY) {
        if ((Get_CurrentMs() - start) > timeout_ms)
            return W25Q_ERROR_BUSY;
        Delay_Ms(1);
    }
    return W25Q_OK;
}

/**
 * @brief ส่ง 24-bit address ผ่าน SPI
 *
 * @param f - instance flash
 * @param addr - 24-bit address (byte 2, 1, 0 ตามลำดับ)
 *
 * @note ส่ง MSB byte ก่อน (big-endian)
 *       รองรับ address สูงสุด 16MB (24-bit)
 */
static void _send_addr(W25Qxx_Instance* f, uint32_t addr) {
    SPI_Transfer((uint8_t)(addr >> 16));
    SPI_Transfer((uint8_t)(addr >> 8));
    SPI_Transfer((uint8_t)(addr & 0xFF));
}

/* ========== Public ========== */

/**
 * @brief เริ่มต้น W25Qxx SPI Flash
 *
 * @param flash - instance ที่จะ init
 * @param pin_cs - GPIO pin สำหรับ chip select
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM, W25Q_ERROR_SPI (ถ้า JEDEC ID อ่านไม่ได้)
 *
 * @note Wake up จาก power down (RELEASE_PD command)
 *       อ่าน JEDEC ID เพื่อระบุรุ่นและความจุ
 *       capacity byte 0x15=16M, 0x16=32M, 0x17=64M, 0x18=128M
 */
W25Qxx_Status W25Qxx_Init(W25Qxx_Instance* flash, GPIO_Pin pin_cs) {
    if (flash == NULL) return W25Q_ERROR_PARAM;

    flash->pin_cs     = pin_cs;
    flash->jedec_id   = 0;
    flash->capacity   = 0;
    flash->initialized = 0;

    pinMode(pin_cs, PIN_MODE_OUTPUT);
    _cs_high(flash);
    Delay_Ms(5);

    /* Wake up (ถ้าเคย power down) */
    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_RELEASE_PD);
    _cs_high(flash);
    Delay_Us(30);

    /* อ่าน JEDEC ID */
    flash->jedec_id = W25Qxx_ReadJedecID(flash);
    if (flash->jedec_id == 0 || flash->jedec_id == 0xFFFFFF)
        return W25Q_ERROR_SPI;

    /* คำนวณ capacity จาก capacity byte (byte 2 ของ JEDEC ID)
     * W25Q16=0x15, W25Q32=0x16, W25Q64=0x17, W25Q128=0x18
     */
    uint8_t cap_byte = (uint8_t)(flash->jedec_id & 0xFF);
    if (cap_byte >= 0x11 && cap_byte <= 0x19) {
        flash->capacity = 1UL << cap_byte;  /* bytes */
    }

    flash->initialized = 1;
    return W25Q_OK;
}

/**
 * @brief อ่าน JEDEC ID (Manufacturer + Memory Type + Capacity)
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 *
 * @return 24-bit JEDEC ID, 0 ถ้า flash == NULL
 *
 * @note byte layout: [manufacturer][memory_type][capacity]
 *       W25Q series: manufacturer = 0xEF (Winbond)
 *       capacity byte: 0x15=16Mbit, 0x16=32Mbit, 0x17=64Mbit, 0x18=128Mbit
 */
uint32_t W25Qxx_ReadJedecID(W25Qxx_Instance* flash) {
    if (flash == NULL) return 0;
    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_JEDEC_ID);
    uint8_t m  = SPI_Transfer(0xFF);
    uint8_t t  = SPI_Transfer(0xFF);
    uint8_t c  = SPI_Transfer(0xFF);
    _cs_high(flash);
    return ((uint32_t)m << 16) | ((uint32_t)t << 8) | c;
}

/**
 * @brief รอจน flash ไม่ busy (public API)
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 * @param timeout_ms - timeout (ms)
 *
 * @return W25Q_OK, W25Q_ERROR_BUSY ถ้า timeout
 *
 * @note ใช้ _wait_busy ภายใน
 *       เรียกก่อนอ่าน/เขียนเพื่อให้มั่นใจว่า flash พร้อม
 */
W25Qxx_Status W25Qxx_WaitBusy(W25Qxx_Instance* flash, uint32_t timeout_ms) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;
    return _wait_busy(flash, timeout_ms);
}

/**
 * @brief อ่านข้อมูลจาก flash
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 * @param addr - address เริ่มต้น (24-bit)
 * @param buf - buffer สำหรับรับข้อมูล
 * @param len - จำนวน bytes ที่จะอ่าน
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM, W25Q_ERROR_BUSY
 *
 * @note ใช้ READ_DATA command (0x03)
 *       รอ busy ก่อนอ่าน
 *       รองรับการอ่านข้าม page/sector โดยอัตโนมัติ
 */
W25Qxx_Status W25Qxx_Read(W25Qxx_Instance* flash, uint32_t addr,
                            uint8_t* buf, uint32_t len) {
    if (flash == NULL || !flash->initialized || buf == NULL || len == 0)
        return W25Q_ERROR_PARAM;

    if (_wait_busy(flash, W25Q_TIMEOUT_WRITE_MS) != W25Q_OK) return W25Q_ERROR_BUSY;

    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_READ_DATA);
    _send_addr(flash, addr);
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = SPI_Transfer(0xFF);
    }
    _cs_high(flash);
    return W25Q_OK;
}

/**
 * @brief Page program — เขียนข้อมูลภายใน 1 page
 *
 * @param f - instance flash
 * @param addr - address เริ่มต้น
 * @param data - ข้อมูลที่จะเขียน
 * @param len - จำนวน bytes (สูงสุด 256)
 *
 * @return W25Q_OK, W25Q_ERROR_BUSY
 *
 * @note เรียก _write_enable ก่อนเสมอ
 *       ใช้ PAGE_PROGRAM command (0x02)
 *       ไม่ควรเขียนข้าม page boundary (การันตีโดย W25Qxx_Write)
 *       รอ busy หลังเขียน
 */
static W25Qxx_Status _page_write(W25Qxx_Instance* f, uint32_t addr,
                                  const uint8_t* data, uint16_t len) {
    _write_enable(f);

    _cs_low(f);
    SPI_Transfer(W25Q_CMD_PAGE_PROGRAM);
    _send_addr(f, addr);
    for (uint16_t i = 0; i < len; i++) {
        SPI_Transfer(data[i]);
    }
    _cs_high(f);

    return _wait_busy(f, W25Q_TIMEOUT_WRITE_MS);
}

/**
 * @brief เขียนข้อมูลลง flash (รองรับข้าม page)
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 * @param addr - address เริ่มต้น
 * @param data - ข้อมูลที่จะเขียน
 * @param len - จำนวน bytes
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM, W25Q_ERROR_BUSY
 *
 * @note มี write enable ก่อนทุก page program
 *       คำนวณ page boundary อัตโนมัติ (256 bytes/page)
 *       ต้องลบ sector ก่อนเขียน (ไม่มีการ erase อัตโนมัติ)
 */
W25Qxx_Status W25Qxx_Write(W25Qxx_Instance* flash, uint32_t addr,
                             const uint8_t* data, uint32_t len) {
    if (flash == NULL || !flash->initialized || data == NULL || len == 0)
        return W25Q_ERROR_PARAM;

    W25Qxx_Status ret = W25Q_OK;
    uint32_t remaining = len;
    uint32_t cur_addr  = addr;
    const uint8_t* ptr = data;

    while (remaining > 0) {
        /* คำนวณว่าเขียนได้กี่ bytes ใน page นี้ */
        uint16_t page_offset = (uint16_t)(cur_addr % W25Q_PAGE_SIZE);
        uint16_t page_remain = (uint16_t)(W25Q_PAGE_SIZE - page_offset);
        uint16_t write_len   = (remaining < page_remain) ? (uint16_t)remaining : page_remain;

        ret = _page_write(flash, cur_addr, ptr, write_len);
        if (ret != W25Q_OK) return ret;

        cur_addr  += write_len;
        ptr       += write_len;
        remaining -= write_len;
    }
    return W25Q_OK;
}

/**
 * @brief ลบ sector (4KB)
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 * @param addr - address ใด ๆ ใน sector (จะ auto-align)
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM, W25Q_ERROR_BUSY
 *
 * @note SECTOR_ERASE command (0x20)
 *       address ถูก align ให้เป็น 4KB boundary โดยอัตโนมัติ
 *       ต้องมี write enable ก่อน
 *       รอ busy ประมาณ ~400ms (ขึ้นอยู่กับรุ่น)
 */
W25Qxx_Status W25Qxx_EraseSector(W25Qxx_Instance* flash, uint32_t addr) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;

    addr &= ~(W25Q_SECTOR_SIZE - 1);  /* align to sector */

    if (_wait_busy(flash, W25Q_TIMEOUT_WRITE_MS) != W25Q_OK) return W25Q_ERROR_BUSY;
    _write_enable(flash);

    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_SECTOR_ERASE);
    _send_addr(flash, addr);
    _cs_high(flash);

    return _wait_busy(flash, W25Q_TIMEOUT_ERASE_MS);
}

/**
 * @brief ลบทั้ง chip (all sectors to 0xFF)
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM, W25Q_ERROR_BUSY
 *
 * @note CHIP_ERASE command (0xC7)
 *       ต้องมี write enable ก่อน
 *       รอ busy ประมาณ ~10-30s (ขึ้นอยู่กับความจุ)
 */
W25Qxx_Status W25Qxx_EraseChip(W25Qxx_Instance* flash) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;

    if (_wait_busy(flash, W25Q_TIMEOUT_WRITE_MS) != W25Q_OK) return W25Q_ERROR_BUSY;
    _write_enable(flash);

    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_CHIP_ERASE);
    _cs_high(flash);

    return _wait_busy(flash, W25Q_TIMEOUT_CHIP_MS);
}

/**
 * @brief เข้า Power Down mode
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM
 *
 * @note POWER_DOWN command (0xB9)
 *       กระแสไฟเหลือ ~1µA
 *       ต้องใช้ W25Qxx_WakeUp() ก่อนใช้งานอีกครั้ง
 */
W25Qxx_Status W25Qxx_PowerDown(W25Qxx_Instance* flash) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;
    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_POWER_DOWN);
    _cs_high(flash);
    Delay_Us(5);
    return W25Q_OK;
}

/**
 * @brief ออกจาก Power Down mode
 *
 * @param flash - instance ที่ผ่านการ init แล้ว
 *
 * @return W25Q_OK, W25Q_ERROR_PARAM
 *
 * @note RELEASE_PD command (0xAB)
 *       รอ 30µs ก่อนส่งคำสั่งถัดไป
 */
W25Qxx_Status W25Qxx_WakeUp(W25Qxx_Instance* flash) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;
    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_RELEASE_PD);
    _cs_high(flash);
    Delay_Us(30);
    return W25Q_OK;
}
