/**
 * @file ADS1115.h
 * @brief ADS1115 16-bit ADC Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านค่า ADC 16-bit 4-channel ด้วย ADS1115 ผ่าน I2C
 * ใช้เมื่อ MCU ADC (10-bit) ไม่ละเอียดพอ หรือต้องการ channel เพิ่ม
 *
 * **วงจร:**
 * ```
 *   CH57x           ADS1115
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VDD (2.0-5.5V)
 *   GND ────────────> GND
 *   GND ────────────> ADDR  (0x48)
 *   (VDD ──────────> ADDR   0x49)
 *   (SDA ──────────> ADDR   0x4A)
 *   (SCL ──────────> ADDR   0x4B)
 *                    AIN0-AIN3 → Analog inputs
 *                    ALERT/RDY → ไม่จำเป็นถ้าใช้ polling
 * ```
 *
 * @example
 * ADS1115_Instance adc;
 * ADS1115_Init(&adc, ADS1115_ADDR_GND);
 * ADS1115_SetGain(&adc, ADS1115_PGA_4096);
 *
 * int16_t raw = ADS1115_ReadRaw(&adc, ADS1115_CH_AIN0);
 * float v = ADS1115_ToVoltage(&adc, raw);
 * printf("CH0: %.4f V\r\n", v);
 *
 * @author CH57x Library Team
 */

#ifndef __ADS1115_H
#define __ADS1115_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Addresses ========== */
#define ADS1115_ADDR_GND  0x48  /**< ADDR = GND */
#define ADS1115_ADDR_VDD  0x49  /**< ADDR = VDD */
#define ADS1115_ADDR_SDA  0x4A  /**< ADDR = SDA */
#define ADS1115_ADDR_SCL  0x4B  /**< ADDR = SCL */

/* ========== Registers ========== */
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01
#define ADS1115_REG_LO_THRESH   0x02
#define ADS1115_REG_HI_THRESH   0x03

/* ========== Type Definitions ========== */

/**
 * @brief Input MUX — channel selection
 */
typedef enum {
    ADS1115_CH_DIFF_01 = 0x0000, /**< AIN0 - AIN1 (differential) */
    ADS1115_CH_DIFF_03 = 0x1000, /**< AIN0 - AIN3 (differential) */
    ADS1115_CH_DIFF_13 = 0x2000, /**< AIN1 - AIN3 (differential) */
    ADS1115_CH_DIFF_23 = 0x3000, /**< AIN2 - AIN3 (differential) */
    ADS1115_CH_AIN0    = 0x4000, /**< AIN0 vs GND (single-ended) */
    ADS1115_CH_AIN1    = 0x5000, /**< AIN1 vs GND */
    ADS1115_CH_AIN2    = 0x6000, /**< AIN2 vs GND */
    ADS1115_CH_AIN3    = 0x7000  /**< AIN3 vs GND */
} ADS1115_Channel;

/**
 * @brief PGA (Programmable Gain Amplifier) — Full-Scale Range
 */
typedef enum {
    ADS1115_PGA_6144 = 0x0000, /**< ±6.144V  LSB=187.5µV */
    ADS1115_PGA_4096 = 0x0200, /**< ±4.096V  LSB=125µV   */
    ADS1115_PGA_2048 = 0x0400, /**< ±2.048V  LSB=62.5µV  (default) */
    ADS1115_PGA_1024 = 0x0600, /**< ±1.024V  LSB=31.25µV */
    ADS1115_PGA_512  = 0x0800, /**< ±0.512V  LSB=15.625µV */
    ADS1115_PGA_256  = 0x0A00  /**< ±0.256V  LSB=7.8125µV */
} ADS1115_PGA;

/**
 * @brief Data Rate (samples per second)
 */
typedef enum {
    ADS1115_DR_8    = 0x0000, /**< 8 SPS    */
    ADS1115_DR_16   = 0x0020, /**< 16 SPS   */
    ADS1115_DR_32   = 0x0040, /**< 32 SPS   */
    ADS1115_DR_64   = 0x0060, /**< 64 SPS   */
    ADS1115_DR_128  = 0x0080, /**< 128 SPS (default) */
    ADS1115_DR_250  = 0x00A0, /**< 250 SPS  */
    ADS1115_DR_475  = 0x00C0, /**< 475 SPS  */
    ADS1115_DR_860  = 0x00E0  /**< 860 SPS  */
} ADS1115_DataRate;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    ADS1115_OK          = 0,
    ADS1115_ERROR_PARAM = 1,
    ADS1115_ERROR_I2C   = 2
} ADS1115_Status;

/**
 * @brief ADS1115 Instance
 */
typedef struct {
    uint8_t        i2c_addr;
    ADS1115_PGA    pga;        /**< PGA ปัจจุบัน */
    ADS1115_DataRate data_rate;
    float          fsr;        /**< Full-Scale Range (V) */
    uint8_t        initialized;
} ADS1115_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น ADS1115
 * @param ads  ตัวแปร instance
 * @param addr I2C address (ADS1115_ADDR_GND ถึง ADS1115_ADDR_SCL)
 * @return ADS1115_OK หรือ error code
 */
ADS1115_Status ADS1115_Init(ADS1115_Instance* ads, uint8_t addr);

/**
 * @brief ตั้ง PGA (gain / full-scale range)
 * @param ads ตัวแปร instance
 * @param pga ADS1115_PGA_6144 ถึง ADS1115_PGA_256
 */
void ADS1115_SetGain(ADS1115_Instance* ads, ADS1115_PGA pga);

/**
 * @brief ตั้ง Data Rate
 * @param ads ตัวแปร instance
 * @param dr  ADS1115_DR_8 ถึง ADS1115_DR_860
 */
void ADS1115_SetDataRate(ADS1115_Instance* ads, ADS1115_DataRate dr);

/**
 * @brief อ่านค่า ADC raw (Single-Shot mode)
 * @param ads ตัวแปร instance
 * @param ch  channel (ADS1115_CH_AIN0 ถึง ADS1115_CH_AIN3 หรือ differential)
 * @return ค่า signed 16-bit (-32768 ถึง 32767)
 *
 * @note blocking จนกว่า conversion เสร็จ (ตาม data rate)
 */
int16_t ADS1115_ReadRaw(ADS1115_Instance* ads, ADS1115_Channel ch);

/**
 * @brief แปลงค่า raw → แรงดัน (V)
 * @param ads ตัวแปร instance
 * @param raw ค่าจาก ReadRaw()
 * @return แรงดัน (V)
 */
float ADS1115_ToVoltage(ADS1115_Instance* ads, int16_t raw);

/**
 * @brief อ่านค่าแรงดัน (อ่าน raw + แปลงในขั้นตอนเดียว)
 * @param ads ตัวแปร instance
 * @param ch  channel
 * @return แรงดัน (V)
 */
float ADS1115_ReadVoltage(ADS1115_Instance* ads, ADS1115_Channel ch);

#ifdef __cplusplus
}
#endif

#endif /* __ADS1115_H */
