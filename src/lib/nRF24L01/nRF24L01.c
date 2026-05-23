/**
 * @file nRF24L01.c
 * @brief nRF24L01+ 2.4GHz Wireless Transceiver Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "nRF24L01.h"

/* ========== Private: SPI helpers ========== */

/**
 * @brief CSN LOW — เริ่ม SPI transaction
 *
 * @param radio - instance
 *
 * @note ต้องหน่วง 1µs หลัง CSN LOW
 */
static inline void _csn_low(nRF24_Instance* radio) {
    digitalWrite(radio->pin_csn, 0);
    Delay_Us(1);
}

/**
 * @brief CSN HIGH — จบ SPI transaction
 *
 * @param radio - instance
 *
 * @note ต้องหน่วง 1µs ก่อน CSN HIGH
 */
static inline void _csn_high(nRF24_Instance* radio) {
    Delay_Us(1);
    digitalWrite(radio->pin_csn, 1);
}

/**
 * @brief CE LOW — ปิด RX/TX
 *
 * @param radio - instance
 */
static inline void _ce_low(nRF24_Instance* radio) {
    digitalWrite(radio->pin_ce, 0);
}

/**
 * @brief CE HIGH — เปิด RX (start listening)
 *
 * @param radio - instance
 */
static inline void _ce_high(nRF24_Instance* radio) {
    digitalWrite(radio->pin_ce, 1);
}

/**
 * @brief อ่าน register 1 byte
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param reg - register address (0x00-0x1F)
 *
 * @return ค่าใน register
 *
 * @note ใช้ R_REGISTER command (0x00 | reg)
 *       CSN low → send command → read data → CSN high
 */
static uint8_t _read_reg(nRF24_Instance* radio, uint8_t reg) {
    _csn_low(radio);
    SPI_Transfer(NRF24_CMD_R_REGISTER | (reg & 0x1F));
    uint8_t val = SPI_Transfer(NRF24_CMD_NOP);
    _csn_high(radio);
    return val;
}

/**
 * @brief เขียน register 1 byte
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param reg - register address (0x00-0x1F)
 * @param val - ค่าที่จะเขียน
 *
 * @note ใช้ W_REGISTER command (0x20 | reg)
 *       CSN low → send command → send data → CSN high
 */
static void _write_reg(nRF24_Instance* radio, uint8_t reg, uint8_t val) {
    _csn_low(radio);
    SPI_Transfer(NRF24_CMD_W_REGISTER | (reg & 0x1F));
    SPI_Transfer(val);
    _csn_high(radio);
}

/**
 * @brief เขียน register หลาย bytes (สำหรับ address)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param reg - register address
 * @param buf - pointer ไปยังข้อมูลที่จะเขียน
 * @param len - จำนวน bytes
 *
 * @note ใช้สำหรับตั้ง TX_ADDR, RX_ADDR_P0 (5 bytes)
 *       W_REGISTER command แล้วตามด้วย data len bytes
 */
static void _write_reg_multi(nRF24_Instance* radio, uint8_t reg, const uint8_t* buf, uint8_t len) {
    _csn_low(radio);
    SPI_Transfer(NRF24_CMD_W_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) {
        SPI_Transfer(buf[i]);
    }
    _csn_high(radio);
}

/**
 * @brief ส่ง command ไม่มีข้อมูลตาม
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param cmd - command byte (เช่น NRF24_CMD_FLUSH_TX, NRF24_CMD_NOP)
 *
 * @return status register (8-bit)
 *
 * @note ใช้สำหรับ flush FIFO และอ่าน STATUS แบบรวดเร็ว
 */
static uint8_t _command(nRF24_Instance* radio, uint8_t cmd) {
    _csn_low(radio);
    uint8_t status = SPI_Transfer(cmd);
    _csn_high(radio);
    return status;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น nRF24L01+ module
 *
 * @param radio - instance ที่จะ init
 * @param pin_csn - GPIO pin สำหรับ SPI chip select
 * @param pin_ce - GPIO pin สำหรับ Chip Enable
 *
 * @return NRF24_OK ถ้าสำเร็จ, NRF24_ERROR_PARAM ถ้า radio == NULL
 *
 * @note ตั้งค่า CONFIG: CRC 2 bytes, PWR_UP=1, PRIM_RX=1
 *       Auto-ACK pipe 0, 5-byte address, 250µs retry delay, 3 retries
 *       ต้องรอ 100ms ให้ power-on reset เสร็จก่อน
 *       Channel default = 76, data rate = 1Mbps, power = 0dBm
 */
nRF24_Status nRF24_Init(nRF24_Instance* radio, GPIO_Pin pin_csn, GPIO_Pin pin_ce) {
    if (radio == NULL) return NRF24_ERROR_PARAM;

    radio->pin_csn      = pin_csn;
    radio->pin_ce       = pin_ce;
    radio->payload_size = NRF24_MAX_PAYLOAD;
    radio->channel      = 76;
    radio->data_rate    = NRF24_DR_1MBPS;
    radio->tx_power     = NRF24_PWR_0DBM;
    radio->is_rx_mode   = 0;
    radio->initialized  = 0;

    /* ตั้ง GPIO */
    pinMode(pin_csn, PIN_MODE_OUTPUT);
    pinMode(pin_ce,  PIN_MODE_OUTPUT);
    _csn_high(radio);  /* CSN idle = HIGH */
    _ce_low(radio);    /* CE idle = LOW */

    /* รอ power-on reset (100ms) */
    Delay_Ms(100);

    /* ตั้งค่า register:
     * CONFIG: CRC 2 bytes, PWR_UP, RX mode เริ่มต้น
     * EN_AA:  Auto-ACK pipe 0
     * EN_RXADDR: เปิด pipe 0
     * SETUP_AW: 5 bytes address
     * SETUP_RETR: 250µs delay, 3 retries
     * RF_CH:  channel 76
     * RF_SETUP: 1Mbps, 0dBm
     * STATUS: ล้าง interrupt flags
     */
    _write_reg(radio, NRF24_REG_CONFIG,     0x0B);  /* CRC2=1, EN_CRC=1, PWR_UP=1, PRIM_RX=1 */
    _write_reg(radio, NRF24_REG_EN_AA,      0x01);  /* Auto-ACK pipe 0 */
    _write_reg(radio, NRF24_REG_EN_RXADDR,  0x01);  /* Enable pipe 0 */
    _write_reg(radio, NRF24_REG_SETUP_AW,   0x03);  /* 5-byte address */
    _write_reg(radio, NRF24_REG_SETUP_RETR, 0x03);  /* 250µs delay, 3 retries */
    _write_reg(radio, NRF24_REG_RF_CH,      76);
    _write_reg(radio, NRF24_REG_RF_SETUP,   0x06);  /* 1Mbps, 0dBm */
    _write_reg(radio, NRF24_REG_RX_PW_P0,  NRF24_MAX_PAYLOAD);

    /* ล้าง STATUS flags และ FIFO */
    _write_reg(radio, NRF24_REG_STATUS, NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);
    _command(radio, NRF24_CMD_FLUSH_TX);
    _command(radio, NRF24_CMD_FLUSH_RX);

    Delay_Ms(5);
    radio->initialized = 1;
    return NRF24_OK;
}

/**
 * @brief กำหนด RF channel (0-125)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param channel - channel number (0-125, default = 76)
 *
 * @return NRF24_OK ถ้าสำเร็จ, NRF24_ERROR_PARAM ถ้า param ไม่ถูกต้อง
 *
 * @note channel > 125 จะถูก clamped
 *       ความถี่ = 2400 + channel (MHz)
 *       ต้องใช้ channel เดียวกันทั้ง TX และ RX
 */
nRF24_Status nRF24_SetChannel(nRF24_Instance* radio, uint8_t channel) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    if (channel > 125) channel = 125;
    radio->channel = channel;
    _write_reg(radio, NRF24_REG_RF_CH, channel);
    return NRF24_OK;
}

/**
 * @brief กำหนด data rate
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param dr - data rate (NRF24_DR_250KBPS, NRF24_DR_1MBPS, NRF24_DR_2MBPS)
 *
 * @return NRF24_OK ถ้าสำเร็จ, NRF24_ERROR_PARAM ถ้า param ไม่ถูกต้อง
 *
 * @note 250kbps มี sensitivity สูงสุดแต่ latency สูงขึ้น
 *       ใช้ bit RF_SETUP[4:3] register
 */
nRF24_Status nRF24_SetDataRate(nRF24_Instance* radio, nRF24_DataRate dr) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    radio->data_rate = dr;
    uint8_t setup = _read_reg(radio, NRF24_REG_RF_SETUP);
    setup = (setup & ~0x28) | (uint8_t)dr;
    _write_reg(radio, NRF24_REG_RF_SETUP, setup);
    return NRF24_OK;
}

/**
 * @brief กำหนด RF output power
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param pwr - ระดับกำลังส่ง (NRF24_PWR_M18DBM ถึง NRF24_PWR_0DBM)
 *
 * @return NRF24_OK ถ้าสำเร็จ, NRF24_ERROR_PARAM ถ้า param ไม่ถูกต้อง
 *
 * @note ใช้ RF_SETUP[2:1] bits
 *       0dBm = ~11.3mA, -18dBm = ~7.5mA
 *       power สูง = ระยะทางไกล แต่กินกระแสมาก
 */
nRF24_Status nRF24_SetTxPower(nRF24_Instance* radio, nRF24_TxPower pwr) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    radio->tx_power = pwr;
    uint8_t setup = _read_reg(radio, NRF24_REG_RF_SETUP);
    setup = (setup & ~0x06) | (uint8_t)pwr;
    _write_reg(radio, NRF24_REG_RF_SETUP, setup);
    return NRF24_OK;
}

/**
 * @brief กำหนดขนาด payload (1-32 bytes)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param size - ขนาด payload เป็น bytes (1-32)
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM ถ้า invalid
 *
 * @note เขียน RX_PW_P0 register
 *       ต้องตรงกันทั้ง TX และ RX side
 */
nRF24_Status nRF24_SetPayloadSize(nRF24_Instance* radio, uint8_t size) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    if (size == 0 || size > NRF24_MAX_PAYLOAD) return NRF24_ERROR_PARAM;
    radio->payload_size = size;
    _write_reg(radio, NRF24_REG_RX_PW_P0, size);
    return NRF24_OK;
}

/**
 * @brief ตั้งค่า TX address (และ RX pipe 0 สำหรับ Auto-ACK)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param addr - pointer ไปยัง address 5 bytes
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM ถ้า param ไม่ถูกต้อง
 *
 * @note เขียน TX_ADDR และ RX_ADDR_P0 พร้อมกันสำหรับ Auto-ACK
 *       Enhanced ShockBurst: RX pipe 0 ต้องมี address = TX_ADDR
 */
nRF24_Status nRF24_SetTxAddr(nRF24_Instance* radio, const uint8_t* addr) {
    if (radio == NULL || !radio->initialized || addr == NULL) return NRF24_ERROR_PARAM;
    _write_reg_multi(radio, NRF24_REG_TX_ADDR, addr, NRF24_ADDR_WIDTH);
    /* ตั้ง RX pipe 0 = TX addr สำหรับ Auto-ACK */
    _write_reg_multi(radio, NRF24_REG_RX_ADDR_P0, addr, NRF24_ADDR_WIDTH);
    return NRF24_OK;
}

/**
 * @brief ตั้งค่า RX address สำหรับ pipe 0
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param addr - pointer ไปยัง address 5 bytes
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM ถ้า param ไม่ถูกต้อง
 *
 * @note เขียนเฉพาะ RX_ADDR_P0
 *       สำหรับรับข้อมูลทั่วไป (Enhanced ShockBurst pipe 0)
 */
nRF24_Status nRF24_SetRxAddr(nRF24_Instance* radio, const uint8_t* addr) {
    if (radio == NULL || !radio->initialized || addr == NULL) return NRF24_ERROR_PARAM;
    _write_reg_multi(radio, NRF24_REG_RX_ADDR_P0, addr, NRF24_ADDR_WIDTH);
    return NRF24_OK;
}

/**
 * @brief ส่งข้อมูล wireless ผ่าน Enhanced ShockBurst
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param data - pointer ไปยังข้อมูลที่จะส่ง
 * @param len - จำนวน bytes ที่จะส่ง (สูงสุด payload_size)
 *
 * @return NRF24_OK ถ้าสำเร็จ, NRF24_ERROR_TX_FAIL ถ้า MAX_RT, NRF24_ERROR_TIMEOUT ถ้าเกิน 500ms
 *
 * @note สลับจาก RX mode → TX mode ชั่วคราว (PRIM_RX = 0)
 *       เขียน payload ผ่าน W_TX_PAYLOAD, pulse CE ≥ 10µs
 *       รอ TX_DS (success) หรือ MAX_RT (fail) หรือ timeout 500ms
 *       Auto-ACK จะ retry ตาม SETUP_RETR ที่ตั้งไว้
 */
nRF24_Status nRF24_Transmit(nRF24_Instance* radio, const uint8_t* data, uint8_t len) {
    if (radio == NULL || !radio->initialized || data == NULL) return NRF24_ERROR_PARAM;

    /* ออกจาก RX mode → TX mode */
    _ce_low(radio);
    uint8_t config = _read_reg(radio, NRF24_REG_CONFIG);
    config &= ~(1 << 0);  /* PRIM_RX = 0 → TX mode */
    _write_reg(radio, NRF24_REG_CONFIG, config);

    /* ล้าง STATUS flags */
    _write_reg(radio, NRF24_REG_STATUS,
               NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);
    _command(radio, NRF24_CMD_FLUSH_TX);

    /* เขียน payload */
    uint8_t buf[NRF24_MAX_PAYLOAD];
    memset(buf, 0, NRF24_MAX_PAYLOAD);
    uint8_t copy_len = (len > radio->payload_size) ? radio->payload_size : len;
    memcpy(buf, data, copy_len);

    _csn_low(radio);
    SPI_Transfer(NRF24_CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < radio->payload_size; i++) {
        SPI_Transfer(buf[i]);
    }
    _csn_high(radio);

    /* Pulse CE ≥ 10µs เพื่อเริ่ม TX */
    _ce_high(radio);
    Delay_Us(15);
    _ce_low(radio);

    /* รอ TX_DS หรือ MAX_RT */
    uint32_t start = Get_CurrentMs();
    uint8_t status;
    while (1) {
        status = _read_reg(radio, NRF24_REG_STATUS);
        if (status & NRF24_STATUS_TX_DS) {
            _write_reg(radio, NRF24_REG_STATUS, NRF24_STATUS_TX_DS);
            radio->is_rx_mode = 0;
            return NRF24_OK;
        }
        if (status & NRF24_STATUS_MAX_RT) {
            _write_reg(radio, NRF24_REG_STATUS, NRF24_STATUS_MAX_RT);
            _command(radio, NRF24_CMD_FLUSH_TX);
            radio->is_rx_mode = 0;
            return NRF24_ERROR_TX_FAIL;
        }
        if ((Get_CurrentMs() - start) > 500) {
            radio->is_rx_mode = 0;
            return NRF24_ERROR_TIMEOUT;
        }
    }
}

/**
 * @brief เริ่มรับข้อมูล (RX mode)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM ถ้า radio == NULL
 *
 * @note ล้าง STATUS flags และ flush RX FIFO
 *       ตั้ง PRIM_RX = 1, CE HIGH
 *       รอ 130µs ให้ RX settling ก่อนรับข้อมูล
 */
nRF24_Status nRF24_StartListening(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;

    /* ล้าง STATUS flags */
    _write_reg(radio, NRF24_REG_STATUS,
               NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);
    _command(radio, NRF24_CMD_FLUSH_RX);

    /* ตั้ง PRIM_RX = 1 → RX mode */
    uint8_t config = _read_reg(radio, NRF24_REG_CONFIG);
    config |= (1 << 0);
    _write_reg(radio, NRF24_REG_CONFIG, config);

    /* CE HIGH = เริ่ม listen */
    _ce_high(radio);
    Delay_Us(130);  /* รอ RX settling */

    radio->is_rx_mode = 1;
    return NRF24_OK;
}

/**
 * @brief หยุดรับข้อมูล (ออกจาก RX mode)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM ถ้า radio == NULL
 *
 * @note set CE LOW, รอ 200µs ก่อนเปลี่ยนเป็น TX mode
 */
nRF24_Status nRF24_StopListening(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    _ce_low(radio);
    Delay_Us(200);
    radio->is_rx_mode = 0;
    return NRF24_OK;
}

/**
 * @brief ตรวจสอบว่ามีข้อมูลเข้ามาใน RX FIFO หรือไม่
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 *
 * @return 1 = มีข้อมูล, 0 = ไม่มีหรือ radio == NULL
 *
 * @note อ่าน STATUS register ตรวจ RX_DR flag
 *       flag นี้จะถูก clear โดย nRF24_Receive()
 */
uint8_t nRF24_Available(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return 0;
    uint8_t status = _read_reg(radio, NRF24_REG_STATUS);
    return (status & NRF24_STATUS_RX_DR) ? 1 : 0;
}

/**
 * @brief อ่านข้อมูลจาก RX FIFO
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 * @param buf - buffer สำหรับรับข้อมูล (ขนาดต้อง ≥ payload_size)
 *
 * @return NRF24_OK, NRF24_NO_DATA ถ้าไม่มีข้อมูล, NRF24_ERROR_PARAM
 *
 * @note ใช้ R_RX_PAYLOAD command อ่าน payload_size bytes
 *       clear RX_DR flag หลังจากอ่านเสร็จ
 *       ต้องเรียก nRF24_Available() ก่อนเพื่อเช็คว่ามีข้อมูล
 */
nRF24_Status nRF24_Receive(nRF24_Instance* radio, uint8_t* buf) {
    if (radio == NULL || !radio->initialized || buf == NULL) return NRF24_ERROR_PARAM;

    if (!nRF24_Available(radio)) return NRF24_NO_DATA;

    /* อ่าน payload */
    _csn_low(radio);
    SPI_Transfer(NRF24_CMD_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < radio->payload_size; i++) {
        buf[i] = SPI_Transfer(NRF24_CMD_NOP);
    }
    _csn_high(radio);

    /* ล้าง RX_DR flag */
    _write_reg(radio, NRF24_REG_STATUS, NRF24_STATUS_RX_DR);
    return NRF24_OK;
}

/**
 * @brief อ่าน STATUS register
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 *
 * @return 8-bit status value, 0 ถ้า radio == NULL
 *
 * @note ใช้ NOP command ซึ่งส่งกลับ status
 *       bit 7: RX_DR, bit 6: TX_DS, bit 5: MAX_RT
 *       bit 4-1: RX_P_NO, bit 0: TX_FULL
 */
uint8_t nRF24_GetStatus(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return 0;
    return _command(radio, NRF24_CMD_NOP);
}

/**
 * @brief เข้า Power Down mode (ประหยัดไฟ)
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM
 *
 * @note set CE LOW, PWR_UP = 0
 *       กระแสไฟเหลือ ~1.5µA ในโหมดนี้
 *       register contents ยังคงอยู่
 */
nRF24_Status nRF24_PowerDown(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    _ce_low(radio);
    uint8_t config = _read_reg(radio, NRF24_REG_CONFIG);
    config &= ~(1 << 1);  /* PWR_UP = 0 */
    _write_reg(radio, NRF24_REG_CONFIG, config);
    return NRF24_OK;
}

/**
 * @brief ออกจาก Power Down mode
 *
 * @param radio - instance ที่ผ่านการ init แล้ว
 *
 * @return NRF24_OK, NRF24_ERROR_PARAM
 *
 * @note set PWR_UP = 1, รอ 5ms ให้ oscillator stable
 *       ก่อนที่จะใช้ module ได้
 */
nRF24_Status nRF24_PowerUp(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    uint8_t config = _read_reg(radio, NRF24_REG_CONFIG);
    config |= (1 << 1);  /* PWR_UP = 1 */
    _write_reg(radio, NRF24_REG_CONFIG, config);
    Delay_Ms(5);  /* รอ power-up settling */
    return NRF24_OK;
}
