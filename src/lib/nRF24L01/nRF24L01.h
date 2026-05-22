/**
 * @file nRF24L01.h
 * @brief nRF24L01+ 2.4GHz Wireless Transceiver Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับรับ/ส่งข้อมูลไร้สายผ่าน nRF24L01+ 2.4GHz transceiver
 * ใช้ SPI สำหรับสื่อสารกับ module พร้อม GPIO pins CE และ CSN
 *
 * **คุณสมบัติ:**
 * - ความถี่ 2.4GHz, 126 channels (2400-2525 MHz)
 * - Data rate: 250kbps / 1Mbps / 2Mbps
 * - TX Power: -18, -12, -6, 0 dBm
 * - Payload size สูงสุด 32 bytes
 * - Auto-Acknowledgment (Enhanced ShockBurst)
 * - Auto-Retransmit
 * - 6 RX pipes (รับจากหลาย node พร้อมกัน)
 *
 * **Hardware Connection:**
 * ```
 *   CH57x (SPI default pins)    nRF24L01+
 *   PC5 (SCK)  ─────────────────> SCK
 *   PC6 (MOSI) ─────────────────> MOSI
 *   PC7 (MISO) <─────────────────MISO
 *   GPIO (CSN) ─────────────────> CSN  (เช่น PC4)
 *   GPIO (CE)  ─────────────────> CE   (เช่น PC3)
 *   3.3V ───────────────────────> VCC  (⚠️ 3.3V เท่านั้น, ไม่ใช่ 5V)
 *   GND ────────────────────────> GND
 *   100µF + 100nF capacitor ระหว่าง VCC และ GND (ลด noise)
 * ```
 *
 * @example
 * // ===== Transmitter =====
 * #include "SimpleHAL.h"
 * #include "nRF24L01.h"
 *
 * nRF24_Instance radio;
 * const uint8_t tx_addr[] = {0xE7,0xE7,0xE7,0xE7,0xE7};
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     SPI_SimpleInit(SPI_MODE0, SPI_1MHZ, SPI_PINS_DEFAULT);
 *
 *     nRF24_Init(&radio, PIN_PC4, PIN_PC3);  // CSN, CE
 *     nRF24_SetTxAddr(&radio, tx_addr);
 *     nRF24_SetChannel(&radio, 76);
 *     nRF24_SetDataRate(&radio, NRF24_DR_1MBPS);
 *
 *     while (1) {
 *         uint8_t msg[] = "Hello!";
 *         nRF24_Transmit(&radio, msg, 6);
 *         Delay_Ms(1000);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __NRF24L01_H
#define __NRF24L01_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ========== Configuration ========== */

/** @brief Address width (3, 4, หรือ 5 bytes) */
#ifndef NRF24_ADDR_WIDTH
#define NRF24_ADDR_WIDTH    5
#endif

/** @brief Payload size สูงสุด (1-32 bytes) */
#ifndef NRF24_MAX_PAYLOAD
#define NRF24_MAX_PAYLOAD   32
#endif

/* ========== nRF24L01 Registers ========== */
#define NRF24_REG_CONFIG      0x00
#define NRF24_REG_EN_AA       0x01
#define NRF24_REG_EN_RXADDR   0x02
#define NRF24_REG_SETUP_AW    0x03
#define NRF24_REG_SETUP_RETR  0x04
#define NRF24_REG_RF_CH       0x05
#define NRF24_REG_RF_SETUP    0x06
#define NRF24_REG_STATUS      0x07
#define NRF24_REG_OBSERVE_TX  0x08
#define NRF24_REG_RPD         0x09
#define NRF24_REG_RX_ADDR_P0  0x0A
#define NRF24_REG_RX_ADDR_P1  0x0B
#define NRF24_REG_RX_ADDR_P2  0x0C
#define NRF24_REG_RX_ADDR_P3  0x0D
#define NRF24_REG_RX_ADDR_P4  0x0E
#define NRF24_REG_RX_ADDR_P5  0x0F
#define NRF24_REG_TX_ADDR     0x10
#define NRF24_REG_RX_PW_P0   0x11
#define NRF24_REG_RX_PW_P1   0x12
#define NRF24_REG_FIFO_STATUS 0x17
#define NRF24_REG_DYNPD       0x1C
#define NRF24_REG_FEATURE     0x1D

/* ========== Commands ========== */
#define NRF24_CMD_R_REGISTER    0x00
#define NRF24_CMD_W_REGISTER    0x20
#define NRF24_CMD_R_RX_PAYLOAD  0x61
#define NRF24_CMD_W_TX_PAYLOAD  0xA0
#define NRF24_CMD_FLUSH_TX      0xE1
#define NRF24_CMD_FLUSH_RX      0xE2
#define NRF24_CMD_REUSE_TX_PL   0xE3
#define NRF24_CMD_NOP           0xFF

/* ========== STATUS register bits ========== */
#define NRF24_STATUS_RX_DR      (1 << 6) /**< RX Data Ready */
#define NRF24_STATUS_TX_DS      (1 << 5) /**< TX Data Sent */
#define NRF24_STATUS_MAX_RT     (1 << 4) /**< Max Retransmits reached */
#define NRF24_STATUS_TX_FULL    (1 << 0) /**< TX FIFO Full */

/* ========== Type Definitions ========== */

/**
 * @brief Data Rate
 */
typedef enum {
    NRF24_DR_1MBPS   = 0x00, /**< 1 Mbps (default) */
    NRF24_DR_2MBPS   = 0x08, /**< 2 Mbps */
    NRF24_DR_250KBPS = 0x20  /**< 250 kbps (ระยะไกลกว่า) */
} nRF24_DataRate;

/**
 * @brief TX Power
 */
typedef enum {
    NRF24_PWR_M18DBM = 0x00, /**< -18 dBm (ประหยัดไฟสุด) */
    NRF24_PWR_M12DBM = 0x02, /**< -12 dBm */
    NRF24_PWR_M6DBM  = 0x04, /**< -6 dBm */
    NRF24_PWR_0DBM   = 0x06  /**< 0 dBm (แรงสุด) */
} nRF24_TxPower;

/**
 * @brief CRC Length
 */
typedef enum {
    NRF24_CRC_1BYTE = 0, /**< CRC 1 byte */
    NRF24_CRC_2BYTE = 1  /**< CRC 2 bytes (แนะนำ) */
} nRF24_CRCLength;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    NRF24_OK            = 0, /**< สำเร็จ */
    NRF24_ERROR_PARAM   = 1, /**< Parameter ผิด */
    NRF24_ERROR_TX_FAIL = 2, /**< ส่งไม่สำเร็จ (MAX_RT) */
    NRF24_ERROR_TIMEOUT = 3, /**< Timeout */
    NRF24_NO_DATA       = 4  /**< ไม่มีข้อมูลรอรับ */
} nRF24_Status;

/**
 * @brief nRF24L01 Instance
 */
typedef struct {
    GPIO_Pin   pin_csn;            /**< GPIO pin สำหรับ CSN (Chip Select, active LOW) */
    GPIO_Pin   pin_ce;             /**< GPIO pin สำหรับ CE (Chip Enable) */
    uint8_t    payload_size;       /**< ขนาด payload (1-32 bytes) */
    uint8_t    channel;            /**< RF channel (0-125) */
    nRF24_DataRate data_rate;      /**< Data rate */
    nRF24_TxPower  tx_power;       /**< TX power */
    uint8_t    is_rx_mode;         /**< 1 = RX mode, 0 = TX/standby */
    uint8_t    initialized;        /**< flag บอกว่า Init แล้ว */
} nRF24_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น nRF24L01
 * @param radio   ตัวแปร instance
 * @param pin_csn GPIO pin สำหรับ CSN
 * @param pin_ce  GPIO pin สำหรับ CE
 * @return NRF24_OK หรือ error code
 *
 * @note ต้องเรียก SPI_SimpleInit() ก่อน (SPI_MODE0, SPI_1MHZ)
 * @note ตั้งค่า default: CH=76, 1Mbps, 0dBm, CRC 2 bytes, payload=32
 * @example
 * SPI_SimpleInit(SPI_MODE0, SPI_1MHZ, SPI_PINS_DEFAULT);
 * nRF24_Init(&radio, PIN_PC4, PIN_PC3);
 */
nRF24_Status nRF24_Init(nRF24_Instance* radio, GPIO_Pin pin_csn, GPIO_Pin pin_ce);

/**
 * @brief ตั้ง RF Channel
 * @param radio   ตัวแปร instance
 * @param channel ช่อง (0-125) → ความถี่ = 2400 + channel MHz
 */
nRF24_Status nRF24_SetChannel(nRF24_Instance* radio, uint8_t channel);

/**
 * @brief ตั้ง Data Rate
 * @param radio ตัวแปร instance
 * @param dr    NRF24_DR_1MBPS / NRF24_DR_2MBPS / NRF24_DR_250KBPS
 */
nRF24_Status nRF24_SetDataRate(nRF24_Instance* radio, nRF24_DataRate dr);

/**
 * @brief ตั้ง TX Power
 * @param radio ตัวแปร instance
 * @param pwr   NRF24_PWR_M18DBM ถึง NRF24_PWR_0DBM
 */
nRF24_Status nRF24_SetTxPower(nRF24_Instance* radio, nRF24_TxPower pwr);

/**
 * @brief ตั้ง Payload Size
 * @param radio ตัวแปร instance
 * @param size  1-32 bytes
 */
nRF24_Status nRF24_SetPayloadSize(nRF24_Instance* radio, uint8_t size);

/**
 * @brief ตั้ง TX Address (5 bytes)
 * @param radio ตัวแปร instance
 * @param addr  array 5 bytes (NRF24_ADDR_WIDTH)
 *
 * @example
 * const uint8_t tx_addr[] = {0xE7,0xE7,0xE7,0xE7,0xE7};
 * nRF24_SetTxAddr(&radio, tx_addr);
 */
nRF24_Status nRF24_SetTxAddr(nRF24_Instance* radio, const uint8_t* addr);

/**
 * @brief ตั้ง RX Address สำหรับ pipe 0
 * @param radio ตัวแปร instance
 * @param addr  array 5 bytes
 *
 * @note สำหรับ TX พร้อม Auto-ACK ต้องตั้ง pipe 0 = TX address
 */
nRF24_Status nRF24_SetRxAddr(nRF24_Instance* radio, const uint8_t* addr);

/**
 * @brief ส่งข้อมูล
 * @param radio   ตัวแปร instance
 * @param data    pointer ข้อมูลที่ต้องการส่ง
 * @param len     ขนาดข้อมูล (จะ pad/trim ให้ตรง payload_size อัตโนมัติ)
 * @return NRF24_OK หรือ NRF24_ERROR_TX_FAIL (MAX_RT)
 *
 * @note blocking จนกว่า TX_DS หรือ MAX_RT
 * @example
 * uint8_t msg[] = {1, 2, 3, 4};
 * nRF24_Transmit(&radio, msg, 4);
 */
nRF24_Status nRF24_Transmit(nRF24_Instance* radio, const uint8_t* data, uint8_t len);

/**
 * @brief เข้า RX mode (รอรับข้อมูล)
 * @param radio ตัวแปร instance
 */
nRF24_Status nRF24_StartListening(nRF24_Instance* radio);

/**
 * @brief ออกจาก RX mode
 * @param radio ตัวแปร instance
 */
nRF24_Status nRF24_StopListening(nRF24_Instance* radio);

/**
 * @brief ตรวจสอบว่ามีข้อมูลรอรับหรือเปล่า
 * @param radio ตัวแปร instance
 * @return 1 = มีข้อมูล, 0 = ไม่มี
 */
uint8_t nRF24_Available(nRF24_Instance* radio);

/**
 * @brief รับข้อมูลจาก RX FIFO
 * @param radio ตัวแปร instance
 * @param buf   buffer รับข้อมูล (ต้องมีขนาดอย่างน้อย payload_size)
 * @return NRF24_OK หรือ NRF24_NO_DATA
 *
 * @example
 * if (nRF24_Available(&radio)) {
 *     uint8_t buf[32];
 *     nRF24_Receive(&radio, buf);
 *     printf("Received: %s\r\n", buf);
 * }
 */
nRF24_Status nRF24_Receive(nRF24_Instance* radio, uint8_t* buf);

/**
 * @brief อ่าน STATUS register
 * @param radio ตัวแปร instance
 * @return ค่า STATUS register
 */
uint8_t nRF24_GetStatus(nRF24_Instance* radio);

/**
 * @brief Power Down (ประหยัดไฟ)
 * @param radio ตัวแปร instance
 */
nRF24_Status nRF24_PowerDown(nRF24_Instance* radio);

/**
 * @brief Power Up
 * @param radio ตัวแปร instance
 */
nRF24_Status nRF24_PowerUp(nRF24_Instance* radio);

#ifdef __cplusplus
}
#endif

#endif /* __NRF24L01_H */
