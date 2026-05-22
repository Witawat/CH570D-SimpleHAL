/**
 * @file DHT.h
 * @brief DHT11/DHT22 Temperature & Humidity Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านค่าอุณหภูมิและความชื้นจาก DHT11 และ DHT22 (AM2302)
 * ใช้ bit-banging เพื่อสร้าง timing ที่ถูกต้องตาม DHT protocol
 *
 * **คุณสมบัติ:**
 * - รองรับ DHT11 และ DHT22 / AM2302 ในไฟล์เดียว
 * - อ่านอุณหภูมิและความชื้นพร้อมกันในครั้งเดียว
 * - CRC checksum validation ป้องกันข้อมูลเสีย
 * - Status reporting ละเอียด (Timeout, Checksum, Not Ready)
 * - Multi-sensor support (สูงสุด 4 sensors)
 * - Non-blocking ready check ด้วย DHT_IsReady()
 *
 * **DHT11 Specifications:**
 * - อุณหภูมิ: 0-50°C (±2°C), ความละเอียด 1°C
 * - ความชื้น: 20-90%RH (±5%), ความละเอียด 1%
 * - ราคาถูก เหมาะกับงานทั่วไป
 *
 * **DHT22 / AM2302 Specifications:**
 * - อุณหภูมิ: -40-80°C (±0.5°C), ความละเอียด 0.1°C
 * - ความชื้น: 0-100%RH (±2-5%), ความละเอียด 0.1%
 * - แม่นยำกว่า เหมาะกับงานที่ต้องการความแม่นยำสูง
 *
 * **Hardware Connection:**
 * ```
 *   VCC (3.3V)
 *      |
 *    [10kΩ pull-up]
 *      |
 *   DHT Pin 2 (DATA) -----> GPIO Pin
 *   DHT Pin 1 (VCC)  -----> 3.3V
 *   DHT Pin 4 (GND)  -----> GND
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "DHT.h"
 *
 * DHT_Instance dht;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *
 *     DHT_Init(&dht, PC4, DHT_TYPE_DHT22);
 *     Delay_Ms(2000);  // รอ sensor stabilize ครั้งแรก
 *
 *     while (1) {
 *         if (DHT_Read(&dht) == DHT_OK) {
 *             printf("Temp: %.1f C, Hum: %.1f%%\r\n",
 *                    DHT_GetTemperature(&dht),
 *                    DHT_GetHumidity(&dht));
 *         } else {
 *             printf("Error: %s\r\n", DHT_StatusStr(dht.last_status));
 *         }
 *         Delay_Ms(2000);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __DHT_H
#define __DHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief จำนวน DHT instances สูงสุด
 */
#ifndef DHT_MAX_INSTANCES
#define DHT_MAX_INSTANCES       4
#endif

/**
 * @brief ช่วงเวลาน้อยสุดระหว่างการอ่านค่า (ms)
 * @note DHT11/DHT22 ต้องการ อย่างน้อย 1-2 วินาทีระหว่างการอ่าน
 */
#define DHT_MIN_INTERVAL_MS     2000

/**
 * @brief Timeout สำหรับรอ response ของ sensor (µs)
 */
#define DHT_TIMEOUT_US          200

/* ========== Type Definitions ========== */

/**
 * @brief ประเภทของ DHT sensor
 */
typedef enum {
    DHT_TYPE_DHT11  = 11,   /**< DHT11: 0-50°C, 20-90%RH, ความละเอียด 1 */
    DHT_TYPE_DHT22  = 22,   /**< DHT22: -40 ถึง 80°C, 0-100%RH, ความละเอียด 0.1 */
    DHT_TYPE_AM2302 = 22    /**< AM2302: เหมือนกับ DHT22 ทุกประการ */
} DHT_Type;

/**
 * @brief สถานะผลการอ่านค่า
 */
typedef enum {
    DHT_OK              = 0,    /**< อ่านค่าสำเร็จ */
    DHT_ERROR_TIMEOUT   = 1,    /**< Timeout - sensor ไม่ตอบสนอง ตรวจสอบการต่อ pull-up */
    DHT_ERROR_CHECKSUM  = 2,    /**< Checksum ไม่ตรง - ข้อมูลเสียหาย ตรวจสอบสาย */
    DHT_ERROR_NOT_READY = 3,    /**< ยังไม่ถึงเวลาอ่านค่าอีกครั้ง รอ 2 วินาที */
    DHT_ERROR_NOT_INIT  = 4     /**< ยังไม่ได้เรียก DHT_Init() */
} DHT_Status;

/**
 * @brief โครงสร้างข้อมูล DHT instance
 *
 * @note สร้างตัวแปรแบบ static หรือ global ไม่ใช่ local
 */
typedef struct {
    uint8_t     pin;                /**< GPIO pin ที่ต่อกับ DATA ของ DHT */
    DHT_Type    type;               /**< ประเภท: DHT_TYPE_DHT11 หรือ DHT_TYPE_DHT22 */
    float       temperature;        /**< อุณหภูมิล่าสุดที่อ่านได้ (°C) */
    float       humidity;           /**< ความชื้นล่าสุดที่อ่านได้ (%RH) */
    DHT_Status  last_status;        /**< สถานะการอ่านครั้งล่าสุด */
    uint32_t    last_read_time;     /**< เวลาที่อ่านล่าสุด (ms จาก Timer_Init) */
    uint8_t     initialized;        /**< flag บอกว่า init แล้ว */
} DHT_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น DHT sensor
 *
 * @param dht   ตัวชี้ไปยัง DHT_Instance
 * @param pin   GPIO pin ที่ต่อกับสาย DATA ของ DHT (เช่น PC4, PD3)
 * @param type  ประเภท DHT (DHT_TYPE_DHT11 หรือ DHT_TYPE_DHT22)
 *
 * @example
 * DHT_Instance dht;
 * DHT_Init(&dht, PC4, DHT_TYPE_DHT22);
 */
void DHT_Init(DHT_Instance* dht, uint8_t pin, DHT_Type type);

/**
 * @brief อ่านค่าอุณหภูมิและความชื้นจาก DHT sensor
 *
 * @param dht  ตัวชี้ไปยัง DHT_Instance
 * @return DHT_Status สถานะผลการอ่าน
 *
 * @note ต้องรออย่างน้อย 2 วินาทีระหว่างการเรียกแต่ละครั้ง
 * @note ฟังก์ชันนี้จะ disable interrupts ชั่วคราวระหว่างอ่านข้อมูล (~5ms)
 * @note เรียก DHT_GetTemperature() และ DHT_GetHumidity() หลังจากนี้
 *
 * @example
 * if (DHT_Read(&dht) == DHT_OK) {
 *     float t = DHT_GetTemperature(&dht);
 *     float h = DHT_GetHumidity(&dht);
 * }
 */
DHT_Status DHT_Read(DHT_Instance* dht);

/**
 * @brief ดึงค่าอุณหภูมิล่าสุดที่อ่านได้
 *
 * @param dht  ตัวชี้ไปยัง DHT_Instance
 * @return อุณหภูมิ (°C) - DHT11: 0-50, DHT22: -40-80
 *
 * @note ต้องเรียก DHT_Read() ที่ให้ผล DHT_OK ก่อน
 */
float DHT_GetTemperature(DHT_Instance* dht);

/**
 * @brief ดึงค่าความชื้นล่าสุดที่อ่านได้
 *
 * @param dht  ตัวชี้ไปยัง DHT_Instance
 * @return ความชื้น (%RH) - DHT11: 20-90, DHT22: 0-100
 *
 * @note ต้องเรียก DHT_Read() ที่ให้ผล DHT_OK ก่อน
 */
float DHT_GetHumidity(DHT_Instance* dht);

/**
 * @brief ดึงสถานะการอ่านครั้งล่าสุด
 *
 * @param dht  ตัวชี้ไปยัง DHT_Instance
 * @return DHT_Status สถานะ
 */
DHT_Status DHT_GetStatus(DHT_Instance* dht);

/**
 * @brief ตรวจสอบว่า sensor พร้อมอ่านค่าหรือไม่ (non-blocking check)
 *
 * @param dht  ตัวชี้ไปยัง DHT_Instance
 * @return true ถ้าผ่านมาแล้ว 2 วินาที, false ถ้ายังรอไม่พอ
 *
 * @example
 * // ใช้ใน loop แทนการเรียก DHT_Read ตรงๆ
 * if (DHT_IsReady(&dht)) {
 *     DHT_Read(&dht);
 * }
 */
bool DHT_IsReady(DHT_Instance* dht);

/**
 * @brief แปลง DHT_Status เป็น string สำหรับ debug
 *
 * @param status สถานะที่ต้องการแปลง
 * @return string อธิบายสถานะ
 *
 * @example
 * printf("Status: %s\r\n", DHT_StatusStr(dht.last_status));
 */
const char* DHT_StatusStr(DHT_Status status);

#ifdef __cplusplus
}
#endif

#endif /* __DHT_H */
