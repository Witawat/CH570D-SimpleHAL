/**
 * @file nRF24L01.c
 * @brief nRF24L01+ 2.4GHz Wireless Transceiver Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "nRF24L01.h"

/* ========== Private: SPI helpers ========== */

static inline void _csn_low(nRF24_Instance* radio) {
    digitalWrite(radio->pin_csn, 0);
    Delay_Us(1);
}

static inline void _csn_high(nRF24_Instance* radio) {
    Delay_Us(1);
    digitalWrite(radio->pin_csn, 1);
}

static inline void _ce_low(nRF24_Instance* radio) {
    digitalWrite(radio->pin_ce, 0);
}

static inline void _ce_high(nRF24_Instance* radio) {
    digitalWrite(radio->pin_ce, 1);
}

/**
 * @brief อ่าน register 1 byte
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
 */
static void _write_reg(nRF24_Instance* radio, uint8_t reg, uint8_t val) {
    _csn_low(radio);
    SPI_Transfer(NRF24_CMD_W_REGISTER | (reg & 0x1F));
    SPI_Transfer(val);
    _csn_high(radio);
}

/**
 * @brief เขียน register หลาย bytes (สำหรับ address)
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
 * @brief ส่ง command ไม่มีข้อมูล
 */
static uint8_t _command(nRF24_Instance* radio, uint8_t cmd) {
    _csn_low(radio);
    uint8_t status = SPI_Transfer(cmd);
    _csn_high(radio);
    return status;
}

/* ========== Public Functions ========== */

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

nRF24_Status nRF24_SetChannel(nRF24_Instance* radio, uint8_t channel) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    if (channel > 125) channel = 125;
    radio->channel = channel;
    _write_reg(radio, NRF24_REG_RF_CH, channel);
    return NRF24_OK;
}

nRF24_Status nRF24_SetDataRate(nRF24_Instance* radio, nRF24_DataRate dr) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    radio->data_rate = dr;
    uint8_t setup = _read_reg(radio, NRF24_REG_RF_SETUP);
    setup = (setup & ~0x28) | (uint8_t)dr;
    _write_reg(radio, NRF24_REG_RF_SETUP, setup);
    return NRF24_OK;
}

nRF24_Status nRF24_SetTxPower(nRF24_Instance* radio, nRF24_TxPower pwr) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    radio->tx_power = pwr;
    uint8_t setup = _read_reg(radio, NRF24_REG_RF_SETUP);
    setup = (setup & ~0x06) | (uint8_t)pwr;
    _write_reg(radio, NRF24_REG_RF_SETUP, setup);
    return NRF24_OK;
}

nRF24_Status nRF24_SetPayloadSize(nRF24_Instance* radio, uint8_t size) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    if (size == 0 || size > NRF24_MAX_PAYLOAD) return NRF24_ERROR_PARAM;
    radio->payload_size = size;
    _write_reg(radio, NRF24_REG_RX_PW_P0, size);
    return NRF24_OK;
}

nRF24_Status nRF24_SetTxAddr(nRF24_Instance* radio, const uint8_t* addr) {
    if (radio == NULL || !radio->initialized || addr == NULL) return NRF24_ERROR_PARAM;
    _write_reg_multi(radio, NRF24_REG_TX_ADDR, addr, NRF24_ADDR_WIDTH);
    /* ตั้ง RX pipe 0 = TX addr สำหรับ Auto-ACK */
    _write_reg_multi(radio, NRF24_REG_RX_ADDR_P0, addr, NRF24_ADDR_WIDTH);
    return NRF24_OK;
}

nRF24_Status nRF24_SetRxAddr(nRF24_Instance* radio, const uint8_t* addr) {
    if (radio == NULL || !radio->initialized || addr == NULL) return NRF24_ERROR_PARAM;
    _write_reg_multi(radio, NRF24_REG_RX_ADDR_P0, addr, NRF24_ADDR_WIDTH);
    return NRF24_OK;
}

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

nRF24_Status nRF24_StopListening(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    _ce_low(radio);
    Delay_Us(200);
    radio->is_rx_mode = 0;
    return NRF24_OK;
}

uint8_t nRF24_Available(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return 0;
    uint8_t status = _read_reg(radio, NRF24_REG_STATUS);
    return (status & NRF24_STATUS_RX_DR) ? 1 : 0;
}

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

uint8_t nRF24_GetStatus(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return 0;
    return _command(radio, NRF24_CMD_NOP);
}

nRF24_Status nRF24_PowerDown(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    _ce_low(radio);
    uint8_t config = _read_reg(radio, NRF24_REG_CONFIG);
    config &= ~(1 << 1);  /* PWR_UP = 0 */
    _write_reg(radio, NRF24_REG_CONFIG, config);
    return NRF24_OK;
}

nRF24_Status nRF24_PowerUp(nRF24_Instance* radio) {
    if (radio == NULL || !radio->initialized) return NRF24_ERROR_PARAM;
    uint8_t config = _read_reg(radio, NRF24_REG_CONFIG);
    config |= (1 << 1);  /* PWR_UP = 1 */
    _write_reg(radio, NRF24_REG_CONFIG, config);
    Delay_Ms(5);  /* รอ power-up settling */
    return NRF24_OK;
}
