/**
 * @file RC522.c
 * @brief RC522 RFID Reader Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "RC522.h"

/* ========== Private: SPI Helpers ========== */

/**
 * @brief set CS pin = LOW (start SPI)
 *
 * @param rfid - instance
 */
static void _cs_low(RC522_Instance* rfid) {
    digitalWrite(rfid->pin_cs, LOW);
}

/**
 * @brief set CS pin = HIGH (end SPI)
 *
 * @param rfid - instance
 */
static void _cs_high(RC522_Instance* rfid) {
    digitalWrite(rfid->pin_cs, HIGH);
}

/* ========== Private: Register R/W ========== */

/**
 * @brief เขียน register ผ่าน SPI
 *
 * @param rfid - instance
 * @param reg - register address (6-bit)
 * @param val - 8-bit value
 *
 * @note SPI frame: [address << 1] with bit7=0 (write), bit0=0
 */
static void _write_reg(RC522_Instance* rfid, uint8_t reg, uint8_t val) {
    _cs_low(rfid);
    SPI_Transfer((reg << 1) & 0x7E);   /* Write: bit7=0, bit0=0, bits[6:1]=reg */
    SPI_Transfer(val);
    _cs_high(rfid);
}

/**
 * @brief อ่าน register ผ่าน SPI
 *
 * @param rfid - instance
 * @param reg - register address (6-bit)
 *
 * @return 8-bit register value
 *
 * @note SPI frame: [address << 1] | 0x80 (read), bit0=0
 *       ส่ง dummy byte 0x00 เพื่อรับ data
 */
static uint8_t _read_reg(RC522_Instance* rfid, uint8_t reg) {
    _cs_low(rfid);
    SPI_Transfer(((reg << 1) & 0x7E) | 0x80);  /* Read: bit7=1, bit0=0, bits[6:1]=reg */
    uint8_t val = SPI_Transfer(0x00);
    _cs_high(rfid);
    return val;
}

/**
 * @brief set bit ใน register (read-modify-write OR)
 *
 * @param rfid - instance
 * @param reg - register address
 * @param mask - bits ที่ต้องการ set
 */
static void _set_bit_mask(RC522_Instance* rfid, uint8_t reg, uint8_t mask) {
    _write_reg(rfid, reg, _read_reg(rfid, reg) | mask);
}

/**
 * @brief clear bit ใน register (read-modify-write AND)
 *
 * @param rfid - instance
 * @param reg - register address
 * @param mask - bits ที่ต้องการ clear
 */
static void _clear_bit_mask(RC522_Instance* rfid, uint8_t reg, uint8_t mask) {
    _write_reg(rfid, reg, _read_reg(rfid, reg) & ~mask);
}

/* ========== Private: Antenna ========== */

/**
 * @brief เปิด antenna (TX control register bits 0-1)
 *
 * @param rfid - instance
 *
 * @note ตรวจสอบ current value ก่อน set 0x03
 *       ต้องเรียกระหว่าง Init เพื่อเปิด RF field
 */
static void _antenna_on(RC522_Instance* rfid) {
    uint8_t val = _read_reg(rfid, RC522_REG_TX_CONTROL);
    if ((val & 0x03) != 0x03) {
        _set_bit_mask(rfid, RC522_REG_TX_CONTROL, 0x03);
    }
}

/* ========== Private: FIFO ========== */

/**
 * @brief ล้าง FIFO buffer
 *
 * @param rfid - instance
 *
 * @note set bit 7 ของ FIFO_LEVEL register
 *       เรียกก่อนทุก transceive
 */
static void _flush_fifo(RC522_Instance* rfid) {
    _set_bit_mask(rfid, RC522_REG_FIFO_LEVEL, 0x80);
}

/* ========== Private: Transceive ========== */

/**
 * @brief ส่งและรับข้อมูลผ่าน RF
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 * @param cmd - command (TRANSCEIVE / TRANSMIT)
 * @param send_data - data ที่จะส่ง
 * @param send_len - จำนวน bytes ที่ส่ง
 * @param recv_data - buffer รับข้อมูล
 * @param recv_len - ขนาด buffer / จำนวน bytes ที่รับได้จริง
 * @param last_bits - จำนวน bits ใน byte สุดท้าย (0 = ทั้ง byte)
 *
 * @return RC522_OK, RC522_ERROR_TIMEOUT, RC522_ERROR_COLLISION, etc.
 *
 * @note ตั้ง IRQ mask, flush FIFO, เขียนข้อมูลลง FIFO
 *       รอ RxIRq/IdleIRq หรือ timeout ~25ms
 *       ตรวจ error bits: BufferOvfl, CRCErr, ParityErr, CollErr
 *       อ่านข้อมูลจาก FIFO หลังจากส่งเสร็จ
 */
static RC522_Status _transceive(RC522_Instance* rfid,
                                 uint8_t  cmd,
                                 uint8_t* send_data, uint8_t send_len,
                                 uint8_t* recv_data, uint8_t* recv_len,
                                 uint8_t  last_bits) {
    /* ตั้ง IRQ mask */
    uint8_t irq_en   = 0x77;  /* TxIEn, RxIEn, IdleIEn, ErrIEn, TimerIEn */
    uint8_t wait_irq = 0x30;  /* RxIRq | IdleIRq */

    _write_reg(rfid, RC522_REG_COM_IEN, irq_en | 0x80);
    _clear_bit_mask(rfid, RC522_REG_COM_IRQ, 0x80);  /* Clear all IRQ bits */
    _flush_fifo(rfid);
    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_IDLE);

    /* เขียนข้อมูลลง FIFO */
    for (uint8_t i = 0; i < send_len; i++) {
        _write_reg(rfid, RC522_REG_FIFO_DATA, send_data[i]);
    }

    /* ตั้งค่า bit framing */
    _write_reg(rfid, RC522_REG_BIT_FRAMING, last_bits & 0x07);

    /* เริ่มคำสั่ง */
    _write_reg(rfid, RC522_REG_COMMAND, cmd);
    if (cmd == RC522_CMD_TRANSCEIVE) {
        _set_bit_mask(rfid, RC522_REG_BIT_FRAMING, 0x80);  /* StartSend */
    }

    /* รอ IRQ */
    uint32_t start = Get_CurrentMs();
    uint8_t  irq   = 0;
    do {
        irq = _read_reg(rfid, RC522_REG_COM_IRQ);
        if ((Get_CurrentMs() - start) >= RC522_TIMEOUT_MS) {
            return RC522_ERROR_TIMEOUT;
        }
    } while (!(irq & wait_irq) && !(irq & 0x01));  /* RxIRq/IdleIRq or TimerIRq */

    _clear_bit_mask(rfid, RC522_REG_BIT_FRAMING, 0x80);

    if (irq & 0x01) return RC522_ERROR_TIMEOUT;

    /* ตรวจ error */
    uint8_t err = _read_reg(rfid, RC522_REG_ERROR);
    if (err & 0x1B) {  /* BufferOvfl | CRCErr | ParityErr | ProtocolErr */
        return RC522_ERROR_GENERAL;
    }
    if (err & 0x08) {  /* CollErr */
        return RC522_ERROR_COLLISION;
    }

    /* อ่านข้อมูลจาก FIFO */
    if (recv_data != NULL && recv_len != NULL) {
        uint8_t n = _read_reg(rfid, RC522_REG_FIFO_LEVEL);
        if (n > *recv_len) n = *recv_len;
        for (uint8_t i = 0; i < n; i++) {
            recv_data[i] = _read_reg(rfid, RC522_REG_FIFO_DATA);
        }
        *recv_len = n;
    }

    return RC522_OK;
}

/* ========== Private: Anti-collision & Select ========== */

/**
 * @brief Anti-collision ระดับ 1 — อ่าน UID 4 bytes แรก
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 * @param uid_buf - buffer รับ UID 4 bytes
 *
 * @return RC522_OK, RC522_ERROR_CRC, RC522_ERROR_COLLISION, etc.
 *
 * @note ส่ง SEL_CL1 + cascade level command
 *       ตรวจสอบ BCC (XOR ของ 4 bytes UID)
 *       ใช้ transceive() กับ RECV 5 bytes (4 UID + 1 BCC)
 */
static RC522_Status _anticoll(RC522_Instance* rfid, uint8_t* uid_buf) {
    uint8_t send[2] = { PICC_CMD_SEL_CL1, 0x20 };
    uint8_t recv[5] = {0};
    uint8_t recv_len = 5;

    RC522_Status st = _transceive(rfid, RC522_CMD_TRANSCEIVE,
                                   send, 2, recv, &recv_len, 0);
    if (st != RC522_OK) return st;
    if (recv_len < 5)   return RC522_ERROR_GENERAL;

    /* ตรวจ BCC (XOR ของ 4 bytes) */
    uint8_t bcc = recv[0] ^ recv[1] ^ recv[2] ^ recv[3];
    if (bcc != recv[4]) return RC522_ERROR_CRC;

    uid_buf[0] = recv[0];
    uid_buf[1] = recv[1];
    uid_buf[2] = recv[2];
    uid_buf[3] = recv[3];
    return RC522_OK;
}

/**
 * @brief Select card — ยืนยัน UID และรับ SAK
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 * @param uid_buf - UID 4 bytes ที่ได้จาก anticoll
 *
 * @return RC522_OK, RC522_ERROR_TIMEOUT, error codes
 *
 * @note ส่ง SEL_CL1 + NVB=0x70 + UID 4 bytes + BCC + CRC16_A
 *       คำนวณ CRC ด้วย hardware CRC calculator
 *       รับ SAK 3 bytes ตอบกลับ
 */
static RC522_Status _select(RC522_Instance* rfid, uint8_t* uid_buf) {
    uint8_t send[9];
    send[0] = PICC_CMD_SEL_CL1;
    send[1] = 0x70;  /* NVB: 7 bytes follow */
    send[2] = uid_buf[0];
    send[3] = uid_buf[1];
    send[4] = uid_buf[2];
    send[5] = uid_buf[3];
    send[6] = uid_buf[0] ^ uid_buf[1] ^ uid_buf[2] ^ uid_buf[3];  /* BCC */

    /* คำนวณ CRC */
    _flush_fifo(rfid);
    for (uint8_t i = 0; i < 7; i++) {
        _write_reg(rfid, RC522_REG_FIFO_DATA, send[i]);
    }
    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_CALC_CRC);
    uint32_t start = Get_CurrentMs();
    uint8_t n;
    do {
        n = _read_reg(rfid, RC522_REG_DIV_IRQ);
        if ((Get_CurrentMs() - start) >= RC522_TIMEOUT_MS) return RC522_ERROR_TIMEOUT;
    } while (!(n & 0x04));  /* CRCIRq */
    send[7] = _read_reg(rfid, RC522_REG_CRC_RESULT_L);
    send[8] = _read_reg(rfid, RC522_REG_CRC_RESULT_H);

    uint8_t recv[3] = {0};
    uint8_t recv_len = 3;
    RC522_Status st = _transceive(rfid, RC522_CMD_TRANSCEIVE,
                                   send, 9, recv, &recv_len, 0);
    if (st != RC522_OK) return st;
    return RC522_OK;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น RC522 RFID module
 *
 * @param rfid - instance ที่จะ init
 * @param pin_cs - GPIO pin สำหรับ SPI chip select
 * @param pin_rst - GPIO pin สำหรับ reset
 *
 * @return RC522_OK, RC522_ERROR_PARAM ถ้า rfid == NULL
 *
 * @note hardware reset: RST LOW 10ms → HIGH 50ms
 *       soft reset: command SOFT_RESET
 *       ตั้ง timer: ~25µs per tick, ~25ms timeout
 *       TX: 100% ASK modulation, CRC_A preset (0x6363)
 *       เปิด antenna หลัง init
 */
RC522_Status RC522_Init(RC522_Instance* rfid, uint8_t pin_cs, uint8_t pin_rst) {
    if (rfid == NULL) return RC522_ERROR_PARAM;

    rfid->pin_cs      = pin_cs;
    rfid->pin_rst     = pin_rst;
    rfid->initialized = 0;

    /* ตั้ง GPIO */
    pinMode(pin_cs,  PIN_MODE_OUTPUT);
    pinMode(pin_rst, PIN_MODE_OUTPUT);
    _cs_high(rfid);

    /* Hardware reset */
    digitalWrite(pin_rst, LOW);
    Delay_Ms(10);
    digitalWrite(pin_rst, HIGH);
    Delay_Ms(50);

    /* Soft reset */
    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    Delay_Ms(10);

    /* ตั้งค่า timer: TPrescaler=169 → ~25µs per tick, TReload=0x03E8=1000 → ~25ms timeout */
    _write_reg(rfid, RC522_REG_T_MODE,      0x8D);   /* TAuto=1, TGated=00, TAutoRestart=0, TPrescalerHi=0xD */
    _write_reg(rfid, RC522_REG_T_PRESCALER, 0x3E);   /* TPrescalerLo=0x3E → TPrescaler=0xD3E=3390 */
    _write_reg(rfid, RC522_REG_T_RELOAD_H,  0x03);
    _write_reg(rfid, RC522_REG_T_RELOAD_L,  0xE8);

    /* ตั้งค่า TX: 100% ASK modulation */
    _write_reg(rfid, RC522_REG_TX_ASK, 0x40);

    /* ตั้งค่า CRC preset: 0x6363 (ISO 14443-3 CRC_A) */
    _write_reg(rfid, RC522_REG_MODE, 0x3D);

    /* เปิด antenna */
    _antenna_on(rfid);

    rfid->initialized = 1;
    return RC522_OK;
}

/**
 * @brief ตรวจสอบว่ามีการ์ดอยู่ใกล้ antenna หรือไม่
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 *
 * @return 1 = มีการ์ด, 0 = ไม่มี
 *
 * @note ส่ง REQA command (7 bits) รอ ATQA ตอบกลับ 2 bytes
 *       bit framing: 7 bits สำหรับ REQA ตาม ISO 14443-3
 *       ถ้าได้ ATQA ครบ 2 bytes = มีการ์ด
 */
uint8_t RC522_IsCardPresent(RC522_Instance* rfid) {
    if (rfid == NULL || !rfid->initialized) return 0;

    /* ส่ง REQA */
    _write_reg(rfid, RC522_REG_BIT_FRAMING, 0x07);  /* 7 bits */
    uint8_t cmd    = PICC_CMD_REQA;
    uint8_t atqa[2] = {0};
    uint8_t len    = 2;

    RC522_Status st = _transceive(rfid, RC522_CMD_TRANSCEIVE,
                                   &cmd, 1, atqa, &len, 7);
    return (st == RC522_OK && len == 2) ? 1 : 0;
}

/**
 * @brief อ่าน UID ของการ์ด (REQA + Anti-collision + Select)
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 * @param uid - buffer สำหรับ UID (ต้องมีขนาด ≥ 4 bytes)
 * @param uid_len - ขนาด buffer / จำนวน bytes UID ที่อ่านได้
 *
 * @return RC522_OK, error codes ถ้า card ไม่มีหรือล้มเหลว
 *
 * @note ขั้นตอน MIFARE: REQA → Anti-collision → Select
 *       anticoll อ่าน 4 bytes UID, select ยืนยันและรับ SAK
 *       ใช้ cascade level 1 (4-byte UID เท่านั้น)
 */
RC522_Status RC522_ReadUID(RC522_Instance* rfid, uint8_t* uid, uint8_t* uid_len) {
    if (rfid == NULL || !rfid->initialized) return RC522_ERROR_PARAM;
    if (uid == NULL || uid_len == NULL)     return RC522_ERROR_PARAM;

    /* REQA */
    _write_reg(rfid, RC522_REG_BIT_FRAMING, 0x07);
    uint8_t cmd    = PICC_CMD_REQA;
    uint8_t atqa[2] = {0};
    uint8_t rlen   = 2;
    RC522_Status st = _transceive(rfid, RC522_CMD_TRANSCEIVE,
                                   &cmd, 1, atqa, &rlen, 7);
    if (st != RC522_OK) return RC522_ERROR_NOCARD;

    /* Anti-collision */
    uint8_t uid_buf[4] = {0};
    st = _anticoll(rfid, uid_buf);
    if (st != RC522_OK) return st;

    /* Select */
    st = _select(rfid, uid_buf);
    if (st != RC522_OK) return st;

    uid[0] = uid_buf[0];
    uid[1] = uid_buf[1];
    uid[2] = uid_buf[2];
    uid[3] = uid_buf[3];
    *uid_len = 4;
    return RC522_OK;
}

/**
 * @brief สั่งหยุดการ์ด (Halt command)
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 *
 * @note ส่ง PICC_CMD_HLTA + CRC16_A ผ่าน TRANSMIT command
 *       คำนวณ CRC ด้วย hardware ก่อนส่ง
 *       กลับสู่ IDLE state หลังจาก halt
 */
void RC522_Halt(RC522_Instance* rfid) {
    if (rfid == NULL || !rfid->initialized) return;

    uint8_t buf[4];
    buf[0] = PICC_CMD_HLTA;
    buf[1] = 0x00;

    /* คำนวณ CRC */
    _flush_fifo(rfid);
    _write_reg(rfid, RC522_REG_FIFO_DATA, buf[0]);
    _write_reg(rfid, RC522_REG_FIFO_DATA, buf[1]);
    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_CALC_CRC);
    uint32_t start = Get_CurrentMs();
    uint8_t n;
    do {
        n = _read_reg(rfid, RC522_REG_DIV_IRQ);
        if ((Get_CurrentMs() - start) >= RC522_TIMEOUT_MS) return;
    } while (!(n & 0x04));
    buf[2] = _read_reg(rfid, RC522_REG_CRC_RESULT_L);
    buf[3] = _read_reg(rfid, RC522_REG_CRC_RESULT_H);

    _transceive(rfid, RC522_CMD_TRANSMIT, buf, 4, NULL, NULL, 0);

    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_IDLE);
}

/**
 * @brief Soft reset module
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 *
 * @note ส่ง SOFT_RESET command, รอ 10ms
 *       เปิด antenna หลัง reset (ต้องเปิดใหม่ทุกครั้ง)
 */
void RC522_Reset(RC522_Instance* rfid) {
    if (rfid == NULL || !rfid->initialized) return;
    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    Delay_Ms(10);
    _antenna_on(rfid);
}

/**
 * @brief อ่าน version register ของ RC522
 *
 * @param rfid - instance ที่ผ่านการ init แล้ว
 *
 * @return version byte (0x00 ถ้า rfid == NULL), เช่น 0x12 (FM17522), 0x22 (MFRC522 v2.0)
 *
 * @note อ่าน RC522_REG_VERSION register
 *       ใช้ตรวจสอบว่า module ตอบสนอง SPI ถูกต้อง
 */
uint8_t RC522_GetVersion(RC522_Instance* rfid) {
    if (rfid == NULL || !rfid->initialized) return 0x00;
    return _read_reg(rfid, RC522_REG_VERSION);
}
