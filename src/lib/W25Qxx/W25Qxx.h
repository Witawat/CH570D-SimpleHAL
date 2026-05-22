/**
 * @file W25Qxx.h
 * @brief W25Qxx SPI Flash Memory Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่าน/เขียน SPI NOR Flash รุ่น W25Q series (Winbond)
 * รองรับ W25Q16 (2MB), W25Q32 (4MB), W25Q64 (8MB), W25Q128 (16MB)
 *
 * **โครงสร้าง Flash:**
 * ```
 * W25Q32 (4MB):
 *   ├── 8192 Pages   (256 bytes/page)
 *   ├── 512  Sectors (4096 bytes/sector = 16 pages)
 *   ├── 64   Blocks  (65536 bytes/block = 16 sectors)
 *   └── ต้อง Erase ก่อน Write เสมอ (1→0 เขียนได้, 0→1 ต้อง erase)
 * ```
 *
 * **วงจร:**
 * ```
 *   CH57x (SPI default)   W25Qxx
 *   PC5 (SCK) ──────────── > CLK
 *   PC6 (MOSI)──────────── > DI  (MOSI)
 *   PC7 (MISO)<──────────── DO  (MISO)
 *   GPIO (CS) ──────────── > /CS  (เช่น PC4)
 *   3.3V ─────────────────> VCC
 *   GND ──────────────────> GND
 *   VCC ─────────────────── /WP  (ไม่ใช้ Write Protect)
 *   VCC ─────────────────── /HOLD
 *   100nF capacitor: VCC-GND ใกล้ chip
 * ```
 *
 * @example
 * W25Qxx_Instance flash;
 * W25Qxx_Init(&flash, PIN_PC4);
 * printf("ID: %06lX\r\n", flash.jedec_id);
 *
 * // เขียนข้อมูล
 * W25Qxx_EraseSector(&flash, 0x000000);  // ล้าง sector 0
 * uint8_t data[] = "Hello Flash!";
 * W25Qxx_Write(&flash, 0x000000, data, sizeof(data));
 *
 * // อ่านข้อมูล
 * uint8_t buf[32];
 * W25Qxx_Read(&flash, 0x000000, buf, 32);
 *
 * @author CH57x Library Team
 */

#ifndef __W25QXX_H
#define __W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Commands ========== */
#define W25Q_CMD_WRITE_ENABLE   0x06
#define W25Q_CMD_WRITE_DISABLE  0x04
#define W25Q_CMD_READ_STATUS1   0x05
#define W25Q_CMD_READ_STATUS2   0x35
#define W25Q_CMD_WRITE_STATUS   0x01
#define W25Q_CMD_READ_DATA      0x03
#define W25Q_CMD_FAST_READ      0x0B
#define W25Q_CMD_PAGE_PROGRAM   0x02
#define W25Q_CMD_SECTOR_ERASE   0x20  /**< 4KB */
#define W25Q_CMD_BLOCK_ERASE32  0x52  /**< 32KB */
#define W25Q_CMD_BLOCK_ERASE64  0xD8  /**< 64KB */
#define W25Q_CMD_CHIP_ERASE     0xC7
#define W25Q_CMD_POWER_DOWN     0xB9
#define W25Q_CMD_RELEASE_PD     0xAB
#define W25Q_CMD_JEDEC_ID       0x9F
#define W25Q_CMD_DEVICE_ID      0x90

/* ========== Status Register bits ========== */
#define W25Q_STATUS_BUSY   0x01  /**< BUSY bit (WIP) */
#define W25Q_STATUS_WEL    0x02  /**< Write Enable Latch */

/* ========== Flash geometry ========== */
#define W25Q_PAGE_SIZE      256UL    /**< bytes per page */
#define W25Q_SECTOR_SIZE    4096UL   /**< bytes per sector */

/* ========== Timeouts ========== */
#define W25Q_TIMEOUT_WRITE_MS   5    /**< page program timeout */
#define W25Q_TIMEOUT_ERASE_MS   500  /**< sector erase timeout */
#define W25Q_TIMEOUT_CHIP_MS    30000

/* ========== Type Definitions ========== */

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    W25Q_OK          = 0,
    W25Q_ERROR_PARAM = 1,
    W25Q_ERROR_BUSY  = 2,  /**< Timeout รอ Flash */
    W25Q_ERROR_SPI   = 3
} W25Qxx_Status;

/**
 * @brief W25Qxx Instance
 */
typedef struct {
    GPIO_Pin pin_cs;    /**< GPIO pin สำหรับ /CS */
    uint32_t jedec_id;  /**< JEDEC ID (manufacturer + device) */
    uint32_t capacity;  /**< ขนาด Flash (bytes) จาก JEDEC ID */
    uint8_t  initialized;
} W25Qxx_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น W25Qxx Flash
 * @param flash  ตัวแปร instance
 * @param pin_cs GPIO pin สำหรับ /CS (active LOW)
 * @return W25Q_OK หรือ error code
 * @note ต้องเรียก SPI_SimpleInit() ก่อน (SPI_MODE0)
 *
 * @example
 * SPI_SimpleInit(SPI_MODE0, SPI_8MHZ, SPI_PINS_DEFAULT);
 * W25Qxx_Init(&flash, PIN_PC4);
 * printf("Flash: %lu KB\r\n", flash.capacity / 1024);
 */
W25Qxx_Status W25Qxx_Init(W25Qxx_Instance* flash, GPIO_Pin pin_cs);

/**
 * @brief อ่านข้อมูล
 * @param flash  ตัวแปร instance
 * @param addr   address 24-bit (0x000000 ...)
 * @param buf    buffer รับข้อมูล
 * @param len    จำนวน bytes
 * @return W25Q_OK หรือ error code
 *
 * @note ไม่มีข้อจำกัดด้าน address alignment สำหรับการอ่าน
 */
W25Qxx_Status W25Qxx_Read(W25Qxx_Instance* flash, uint32_t addr,
                            uint8_t* buf, uint32_t len);

/**
 * @brief เขียนข้อมูล (Page Program)
 * @param flash  ตัวแปร instance
 * @param addr   start address
 * @param data   ข้อมูลที่ต้องการเขียน
 * @param len    จำนวน bytes
 * @return W25Q_OK หรือ error code
 *
 * @note **ต้อง Erase sector ก่อนเขียน** (Flash เขียนได้เฉพาะ 1→0)
 * @note จัดการ page boundary อัตโนมัติ (ข้าม page ได้)
 */
W25Qxx_Status W25Qxx_Write(W25Qxx_Instance* flash, uint32_t addr,
                             const uint8_t* data, uint32_t len);

/**
 * @brief ลบ Sector (4KB)
 * @param flash ตัวแปร instance
 * @param addr  address ใดก็ได้ใน sector นั้น (จะ align เอง)
 * @return W25Q_OK หรือ error code
 *
 * @note ใช้เวลา ~100-400ms
 */
W25Qxx_Status W25Qxx_EraseSector(W25Qxx_Instance* flash, uint32_t addr);

/**
 * @brief ลบทั้ง Chip
 * @return W25Q_OK หรือ error code
 * @warning ใช้เวลานาน (W25Q32 ~15-30 วินาที)
 */
W25Qxx_Status W25Qxx_EraseChip(W25Qxx_Instance* flash);

/**
 * @brief รอจน Flash ไม่ busy
 * @param timeout_ms timeout (ms)
 */
W25Qxx_Status W25Qxx_WaitBusy(W25Qxx_Instance* flash, uint32_t timeout_ms);

/**
 * @brief อ่าน JEDEC ID
 */
uint32_t W25Qxx_ReadJedecID(W25Qxx_Instance* flash);

/**
 * @brief Power Down (ประหยัดไฟ ~1µA)
 */
W25Qxx_Status W25Qxx_PowerDown(W25Qxx_Instance* flash);

/**
 * @brief Wake up จาก Power Down
 */
W25Qxx_Status W25Qxx_WakeUp(W25Qxx_Instance* flash);

#ifdef __cplusplus
}
#endif

#endif /* __W25QXX_H */
