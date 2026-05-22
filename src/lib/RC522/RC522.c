/**
 * @file RC522.c
 * @brief RC522 RFID Reader Library Implementation
 * @version 1.0
 * @date 2026-04-30
 */

#include "RC522.h"

/* ========== Private: SPI Helpers ========== */

static void _cs_low(RC522_Instance* rfid) {
    digitalWrite(rfid->pin_cs, LOW);
}

static void _cs_high(RC522_Instance* rfid) {
    digitalWrite(rfid->pin_cs, HIGH);
}

/* ========== Private: Register R/W ========== */

static void _write_reg(RC522_Instance* rfid, uint8_t reg, uint8_t val) {
    _cs_low(rfid);
    SPI_Transfer((reg << 1) & 0x7E);   /* Write: bit7=0, bit0=0, bits[6:1]=reg */
    SPI_Transfer(val);
    _cs_high(rfid);
}

static uint8_t _read_reg(RC522_Instance* rfid, uint8_t reg) {
    _cs_low(rfid);
    SPI_Transfer(((reg << 1) & 0x7E) | 0x80);  /* Read: bit7=1, bit0=0, bits[6:1]=reg */
    uint8_t val = SPI_Transfer(0x00);
    _cs_high(rfid);
    return val;
}

static void _set_bit_mask(RC522_Instance* rfid, uint8_t reg, uint8_t mask) {
    _write_reg(rfid, reg, _read_reg(rfid, reg) | mask);
}

static void _clear_bit_mask(RC522_Instance* rfid, uint8_t reg, uint8_t mask) {
    _write_reg(rfid, reg, _read_reg(rfid, reg) & ~mask);
}

/* ========== Private: Antenna ========== */

static void _antenna_on(RC522_Instance* rfid) {
    uint8_t val = _read_reg(rfid, RC522_REG_TX_CONTROL);
    if ((val & 0x03) != 0x03) {
        _set_bit_mask(rfid, RC522_REG_TX_CONTROL, 0x03);
    }
}

/* ========== Private: FIFO ========== */

static void _flush_fifo(RC522_Instance* rfid) {
    _set_bit_mask(rfid, RC522_REG_FIFO_LEVEL, 0x80);
}

/* ========== Private: Transceive ========== */

/**
 * @brief ส่งและรับข้อมูลผ่าน RF
 * @param send_data   data ที่จะส่ง
 * @param send_len    จำนวน bytes ที่ส่ง
 * @param recv_data   buffer รับข้อมูล
 * @param recv_len    ขนาด buffer / จำนวน bytes ที่รับได้จริง
 * @param last_bits   จำนวน bits ใน byte สุดท้าย (0 = ทั้ง byte)
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

void RC522_Reset(RC522_Instance* rfid) {
    if (rfid == NULL || !rfid->initialized) return;
    _write_reg(rfid, RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    Delay_Ms(10);
    _antenna_on(rfid);
}

uint8_t RC522_GetVersion(RC522_Instance* rfid) {
    if (rfid == NULL || !rfid->initialized) return 0x00;
    return _read_reg(rfid, RC522_REG_VERSION);
}
