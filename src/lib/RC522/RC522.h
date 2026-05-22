/**
 * @file RC522.h
 * @brief RC522 RFID Reader Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-30
 *
 * @details
 * Library สำหรับอ่าน UID จาก RFID card/tag ผ่าน MFRC522 module
 * ใช้ SPI communication และ manual CS control
 *
 * **Hardware Connection:**
 * ```
 *   CH57x (Default SPI)    RC522
 *   PC5 (SCK)  -------------> SCK
 *   PC6 (MOSI) -------------> MOSI
 *   PC7 (MISO) <------------- MISO
 *   CS pin     -------------> SDA (CS)
 *   RST pin    -------------> RST
 *   3.3V       -------------> 3.3V  (⚠️ RC522 ทำงานที่ 3.3V เท่านั้น)
 *   GND        -------------> GND
 * ```
 *
 * @note SPI ต้องเรียก SPI_SimpleInit() ก่อนใช้ RC522_Init()
 *
 * @example
 * RC522_Instance rfid;
 * SPI_SimpleInit(SPI_MODE0, SPI_4MHZ, SPI_PINS_DEFAULT);
 * RC522_Init(&rfid, PC4, PD2);   // CS=PC4, RST=PD2
 *
 * uint8_t uid[4];
 * uint8_t uid_len;
 * if (RC522_IsCardPresent(&rfid)) {
 *     if (RC522_ReadUID(&rfid, uid, &uid_len) == RC522_OK) {
 *         // ใช้ uid[0..uid_len-1]
 *     }
 *     RC522_Halt(&rfid);
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __RC522_H
#define __RC522_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== MFRC522 Register Map ========== */

#define RC522_REG_COMMAND       0x01  /**< Starts and stops command execution */
#define RC522_REG_COM_IEN       0x02  /**< Enable/disable interrupt request control bits */
#define RC522_REG_DIV_IEN       0x03  /**< Enable/disable interrupt request control bits */
#define RC522_REG_COM_IRQ       0x04  /**< Interrupt request bits */
#define RC522_REG_DIV_IRQ       0x05  /**< Interrupt request bits */
#define RC522_REG_ERROR         0x06  /**< Error bits showing the error status */
#define RC522_REG_STATUS1       0x07  /**< Communication status bits */
#define RC522_REG_STATUS2       0x08  /**< Receiver and transmitter status bits */
#define RC522_REG_FIFO_DATA     0x09  /**< Input and output of 64 byte FIFO buffer */
#define RC522_REG_FIFO_LEVEL    0x0A  /**< Number of bytes stored in the FIFO buffer */
#define RC522_REG_WATER_LEVEL   0x0B  /**< Level for FIFO underflow and overflow warning */
#define RC522_REG_CONTROL       0x0C  /**< Miscellaneous control registers */
#define RC522_REG_BIT_FRAMING   0x0D  /**< Adjustments for bit-oriented frames */
#define RC522_REG_COLL          0x0E  /**< Bit position of the first bit-collision */
#define RC522_REG_MODE          0x11  /**< Defines general modes for transmitting and receiving */
#define RC522_REG_TX_MODE       0x12  /**< Defines transmission data rate and framing */
#define RC522_REG_RX_MODE       0x13  /**< Defines reception data rate and framing */
#define RC522_REG_TX_CONTROL    0x14  /**< Controls the logical behavior of the antenna driver pins TX1 and TX2 */
#define RC522_REG_TX_ASK        0x15  /**< Controls the setting of the transmission modulation */
#define RC522_REG_TX_SEL        0x16  /**< Selects the internal sources for the antenna driver */
#define RC522_REG_RX_SEL        0x17  /**< Selects internal receiver settings */
#define RC522_REG_RX_THRESHOLD  0x18  /**< Selects thresholds for the bit decoder */
#define RC522_REG_DEMOD         0x19  /**< Defines demodulator settings */
#define RC522_REG_MF_TX         0x1C  /**< Controls some MIFARE communication transmit parameters */
#define RC522_REG_MF_RX         0x1D  /**< Controls some MIFARE communication receive parameters */
#define RC522_REG_SERIAL_SPEED  0x1F  /**< Selects the speed of the serial UART interface */
#define RC522_REG_CRC_RESULT_H  0x21  /**< Shows the MSB values of the CRC calculation */
#define RC522_REG_CRC_RESULT_L  0x22  /**< Shows the LSB values of the CRC calculation */
#define RC522_REG_MOD_WIDTH     0x24  /**< Controls the ModWidth setting */
#define RC522_REG_RF_CFG        0x26  /**< Configures the receiver gain */
#define RC522_REG_GS_N          0x27  /**< Selects the conductance of the antenna driver pins TX1 and TX2 */
#define RC522_REG_CW_GS_P       0x28  /**< Defines the conductance of the p-driver output */
#define RC522_REG_MOD_GS_P      0x29  /**< Defines the conductance of the p-driver output */
#define RC522_REG_T_MODE        0x2A  /**< Defines settings for the internal timer */
#define RC522_REG_T_PRESCALER   0x2B  /**< Defines settings for the internal timer */
#define RC522_REG_T_RELOAD_H    0x2C  /**< Defines the 16-bit timer reload value (high byte) */
#define RC522_REG_T_RELOAD_L    0x2D  /**< Defines the 16-bit timer reload value (low byte) */
#define RC522_REG_T_COUNTER_H   0x2E  /**< Shows the 16-bit timer value (high byte) */
#define RC522_REG_T_COUNTER_L   0x2F  /**< Shows the 16-bit timer value (low byte) */
#define RC522_REG_TEST_SEL1     0x31  /**< General test signal configuration */
#define RC522_REG_TEST_SEL2     0x32  /**< General test signal configuration */
#define RC522_REG_TEST_PIN_EN   0x33  /**< Enables pin output driver on pins D1 to D7 */
#define RC522_REG_TEST_PIN_VAL  0x34  /**< Defines the values for D1 to D7 when it is used as an I/O bus */
#define RC522_REG_TEST_BUS      0x35  /**< Shows the status of the internal test bus */
#define RC522_REG_AUTO_TEST     0x36  /**< Controls the digital self-test */
#define RC522_REG_VERSION       0x37  /**< Shows the software version */
#define RC522_REG_ANALOG_TEST   0x38  /**< Controls the pins AUX1 and AUX2 */
#define RC522_REG_TEST_DAC1     0x39  /**< Defines the test value for TestDAC1 */
#define RC522_REG_TEST_DAC2     0x3A  /**< Defines the test value for TestDAC2 */
#define RC522_REG_TEST_ADC      0x3B  /**< Shows the value of ADC I and Q channels */

/* ========== MFRC522 Commands ========== */

#define RC522_CMD_IDLE          0x00  /**< No action */
#define RC522_CMD_MEM           0x01  /**< Stores 25 bytes into the internal buffer */
#define RC522_CMD_GENERATE_ID   0x02  /**< Generates a 10-byte random ID number */
#define RC522_CMD_CALC_CRC      0x03  /**< Activates the CRC coprocessor */
#define RC522_CMD_TRANSMIT      0x04  /**< Transmits data from the FIFO buffer */
#define RC522_CMD_NO_CMD_CHANGE 0x07  /**< No command change */
#define RC522_CMD_RECEIVE       0x08  /**< Activates the receiver circuits */
#define RC522_CMD_TRANSCEIVE    0x0C  /**< Transmits data from FIFO buffer to antenna */
#define RC522_CMD_MF_AUTHENT    0x0E  /**< Performs the MIFARE standard authentication */
#define RC522_CMD_SOFT_RESET    0x0F  /**< Resets the MFRC522 */

/* ========== PICC (Card) Commands ========== */

#define PICC_CMD_REQA           0x26  /**< Request command, Type A */
#define PICC_CMD_WUPA           0x52  /**< Wake-UP command, Type A */
#define PICC_CMD_CT             0x88  /**< Cascade Tag */
#define PICC_CMD_SEL_CL1        0x93  /**< Anti collision/Select, Cascade Level 1 */
#define PICC_CMD_SEL_CL2        0x95  /**< Anti collision/Select, Cascade Level 2 */
#define PICC_CMD_SEL_CL3        0x97  /**< Anti collision/Select, Cascade Level 3 */
#define PICC_CMD_HLTA           0x50  /**< HaLT command, Type A */

/* ========== Constants ========== */

#define RC522_UID_MAX_LEN       10    /**< Maximum UID length (bytes) */
#define RC522_TIMEOUT_MS        25    /**< Timeout for card operations (ms) */

/* ========== Type Definitions ========== */

/**
 * @brief RC522 status codes
 */
typedef enum {
    RC522_OK              = 0,
    RC522_ERROR_PARAM     = 1,
    RC522_ERROR_NOCARD    = 2,
    RC522_ERROR_COLLISION = 3,
    RC522_ERROR_TIMEOUT   = 4,
    RC522_ERROR_CRC       = 5,
    RC522_ERROR_GENERAL   = 6
} RC522_Status;

/**
 * @brief RC522 instance
 */
typedef struct {
    uint8_t pin_cs;       /**< CS (SDA) pin */
    uint8_t pin_rst;      /**< RST pin */
    uint8_t initialized;  /**< Init flag */
} RC522_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น RC522 module
 * @param rfid    pointer ไปยัง RC522_Instance
 * @param pin_cs  GPIO pin สำหรับ CS (SDA)
 * @param pin_rst GPIO pin สำหรับ RST
 * @return RC522_OK หรือ RC522_ERROR_PARAM
 *
 * @note ต้องเรียก SPI_SimpleInit(SPI_MODE0, SPI_4MHZ, SPI_PINS_DEFAULT) ก่อน
 *
 * @example
 * SPI_SimpleInit(SPI_MODE0, SPI_4MHZ, SPI_PINS_DEFAULT);
 * RC522_Instance rfid;
 * RC522_Init(&rfid, PC4, PD2);
 */
RC522_Status RC522_Init(RC522_Instance* rfid, uint8_t pin_cs, uint8_t pin_rst);

/**
 * @brief ตรวจสอบว่ามี card อยู่ใกล้ antenna หรือไม่
 * @param rfid pointer ไปยัง RC522_Instance
 * @return 1 = มี card, 0 = ไม่มี
 */
uint8_t RC522_IsCardPresent(RC522_Instance* rfid);

/**
 * @brief อ่าน UID ของ card
 * @param rfid    pointer ไปยัง RC522_Instance
 * @param uid     buffer สำหรับเก็บ UID (ขนาด RC522_UID_MAX_LEN bytes)
 * @param uid_len pointer สำหรับรับจำนวน bytes ของ UID
 * @return RC522_OK หรือ error code
 *
 * @example
 * uint8_t uid[RC522_UID_MAX_LEN];
 * uint8_t len;
 * if (RC522_ReadUID(&rfid, uid, &len) == RC522_OK) {
 *     // uid[0..len-1] คือ UID ของ card
 * }
 */
RC522_Status RC522_ReadUID(RC522_Instance* rfid, uint8_t* uid, uint8_t* uid_len);

/**
 * @brief สั่งให้ card เข้า Halt state (หยุด polling card นั้น)
 * @param rfid pointer ไปยัง RC522_Instance
 */
void RC522_Halt(RC522_Instance* rfid);

/**
 * @brief Reset MFRC522 hardware
 * @param rfid pointer ไปยัง RC522_Instance
 */
void RC522_Reset(RC522_Instance* rfid);

/**
 * @brief อ่าน version register ของ MFRC522 (ใช้ตรวจสอบการเชื่อมต่อ)
 * @param rfid pointer ไปยัง RC522_Instance
 * @return version byte (0x91 = v1.0, 0x92 = v2.0, 0x00/0xFF = ไม่พบ)
 */
uint8_t RC522_GetVersion(RC522_Instance* rfid);

#ifdef __cplusplus
}
#endif

#endif  /* __RC522_H */
