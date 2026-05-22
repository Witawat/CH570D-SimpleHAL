/**
 * @file W25Qxx.c
 * @brief W25Qxx SPI Flash Memory Implementation
 */

#include "W25Qxx.h"

/* ========== Private: CS helpers ========== */

static inline void _cs_low(W25Qxx_Instance* f) {
    digitalWrite(f->pin_cs, 0);
}

static inline void _cs_high(W25Qxx_Instance* f) {
    digitalWrite(f->pin_cs, 1);
}

/* ========== Private: Basic operations ========== */

static uint8_t _read_status1(W25Qxx_Instance* f) {
    _cs_low(f);
    SPI_Transfer(W25Q_CMD_READ_STATUS1);
    uint8_t s = SPI_Transfer(0xFF);
    _cs_high(f);
    return s;
}

static void _write_enable(W25Qxx_Instance* f) {
    _cs_low(f);
    SPI_Transfer(W25Q_CMD_WRITE_ENABLE);
    _cs_high(f);
}

static W25Qxx_Status _wait_busy(W25Qxx_Instance* f, uint32_t timeout_ms) {
    uint32_t start = Get_CurrentMs();
    while (_read_status1(f) & W25Q_STATUS_BUSY) {
        if ((Get_CurrentMs() - start) > timeout_ms)
            return W25Q_ERROR_BUSY;
        Delay_Ms(1);
    }
    return W25Q_OK;
}

static void _send_addr(W25Qxx_Instance* f, uint32_t addr) {
    SPI_Transfer((uint8_t)(addr >> 16));
    SPI_Transfer((uint8_t)(addr >> 8));
    SPI_Transfer((uint8_t)(addr & 0xFF));
}

/* ========== Public ========== */

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

W25Qxx_Status W25Qxx_WaitBusy(W25Qxx_Instance* flash, uint32_t timeout_ms) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;
    return _wait_busy(flash, timeout_ms);
}

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

/** Page program ภายใน 1 page */
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

W25Qxx_Status W25Qxx_EraseChip(W25Qxx_Instance* flash) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;

    if (_wait_busy(flash, W25Q_TIMEOUT_WRITE_MS) != W25Q_OK) return W25Q_ERROR_BUSY;
    _write_enable(flash);

    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_CHIP_ERASE);
    _cs_high(flash);

    return _wait_busy(flash, W25Q_TIMEOUT_CHIP_MS);
}

W25Qxx_Status W25Qxx_PowerDown(W25Qxx_Instance* flash) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;
    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_POWER_DOWN);
    _cs_high(flash);
    Delay_Us(5);
    return W25Q_OK;
}

W25Qxx_Status W25Qxx_WakeUp(W25Qxx_Instance* flash) {
    if (flash == NULL || !flash->initialized) return W25Q_ERROR_PARAM;
    _cs_low(flash);
    SPI_Transfer(W25Q_CMD_RELEASE_PD);
    _cs_high(flash);
    Delay_Us(30);
    return W25Q_OK;
}
