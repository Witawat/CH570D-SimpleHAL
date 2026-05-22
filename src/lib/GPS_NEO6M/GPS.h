/**
 * @file GPS.h
 * @brief NEO-6M GPS Module Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-05-01
 *
 * @details
 * Library สำหรับอ่านข้อมูลตำแหน่งจาก NEO-6M GPS Module ผ่าน UART
 * รองรับการ parse NMEA sentences ($GPGGA, $GPRMC)
 *
 * **หลักการทำงาน:**
 * - NEO-6M ส่ง NMEA sentences ออกมาทาง UART ทุก 1 วินาที (default 9600 baud)
 * - Library รับ buffer ทีละ character แล้ว parse เมื่อเจอ newline
 * - ดึงข้อมูล latitude, longitude, altitude, speed, satellites, date/time
 *
 * **Hardware Connection:**
 * ```
 *   NEO-6M            CH57x
 *   VCC  ---------->  3.3V
 *   GND  ---------->  GND
 *   TX   ---------->  USART1 RX pin
 *   RX   ---------->  (optional) USART1 TX pin
 * ```
 *
 * **⚠️ ข้อจำกัดบน CH57x:**
 * - CH57x มี USART1 เพียงช่องเดียว
 * - ถ้าใช้ GPS จะไม่สามารถใช้ USART1 สำหรับ debug printf ได้พร้อมกัน
 * - แนะนำให้ใช้ SWIO debug หรือสลับการใช้งาน
 * - USART RX buffer: 80 bytes (1 NMEA sentence + margin)
 *
 * @example
 * GPS_Instance gps;
 * GPS_Init(&gps, BAUD_9600, USART_PINS_DEFAULT);
 *
 * while(1) {
 *     GPS_Update(&gps);
 *     if (GPS_IsFixValid(&gps)) {
 *         float lat = GPS_GetLatitude(&gps);
 *         float lon = GPS_GetLongitude(&gps);
 *         // ใช้งานค่าพิกัด
 *     }
 *     Delay_Ms(1000);
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __GPS_H
#define __GPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== Configuration ========== */

/** @brief ขนาด buffer สำหรับ 1 NMEA sentence */
#ifndef GPS_BUFFER_SIZE
#define GPS_BUFFER_SIZE  80
#endif

/** @brief Default GPS update timeout (ms) */
#ifndef GPS_DEFAULT_TIMEOUT_MS
#define GPS_DEFAULT_TIMEOUT_MS  2000
#endif

/* ========== Type Definitions ========== */

/**
 * @brief GPS status codes
 */
typedef enum {
    GPS_OK              = 0,
    GPS_ERROR_PARAM     = 1,
    GPS_ERROR_TIMEOUT   = 2,
    GPS_ERROR_NOT_INIT  = 3
} GPS_Status;

/**
 * @brief GPS fix quality
 */
typedef enum {
    GPS_FIX_NONE    = 0,  /**< ไม่มี fix */
    GPS_FIX_GPS     = 1,  /**< GPS fix */
    GPS_FIX_DGPS    = 2   /**< DGPS fix */
} GPS_FixQuality;

/**
 * @brief GPS date/time structure
 */
typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  day;
    uint8_t  month;
    uint8_t  year;      /**< 2-digit year (e.g., 26 = 2026) */
} GPS_DateTime;

/**
 * @brief GPS instance
 */
typedef struct {
    USART_BaudRate  baud_rate;
    USART_PinConfig pin_config;

    char    rx_buf[GPS_BUFFER_SIZE];  /**< UART RX buffer */
    uint8_t rx_idx;                    /**< current buffer index */

    /* parsed data */
    float       latitude;       /**< Latitude (degrees, +N, -S) */
    float       longitude;      /**< Longitude (degrees, +E, -W) */
    float       altitude;       /**< Altitude (meters) */
    float       speed_kmh;      /**< Speed over ground (km/h) */
    uint8_t     satellites;     /**< Number of satellites in use */
    GPS_FixQuality fix_quality; /**< Fix quality */
    GPS_DateTime  datetime;     /**< Date/Time from GPS */
    uint8_t     fix_valid;      /**< 1 = valid fix, 0 = no fix */

    uint8_t     data_ready;     /**< 1 = new data available */
    uint8_t     initialized;    /**< Init flag */
} GPS_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น NEO-6M GPS Module
 * @param gps        pointer ไปยัง GPS_Instance
 * @param baud_rate  baud rate (ปกติ BAUD_9600 สำหรับ NEO-6M)
 * @param pin_config USART pin configuration (ปกติ USART_PINS_DEFAULT)
 * @return GPS_OK หรือ GPS_ERROR_PARAM
 *
 * @note ฟังก์ชันนี้จะยึด USART1 สำหรับ GPS — ไม่สามารถใช้ USART_Print พร้อมกันได้
 *
 * @example
 * GPS_Instance gps;
 * GPS_Init(&gps, BAUD_9600, USART_PINS_DEFAULT);
 */
GPS_Status GPS_Init(GPS_Instance* gps, USART_BaudRate baud_rate, USART_PinConfig pin_config);

/**
 * @brief อ่านข้อมูลจาก GPS buffer และ parse NMEA sentences
 * @param gps pointer ไปยัง GPS_Instance
 * @return GPS_OK หากมีข้อมูลใหม่, GPS_ERROR_TIMEOUT หากไม่มีข้อมูล
 *
 * @note ควรเรียกทุก loop iteration เพื่อไม่ให้ buffer ล้น
 *       ข้อมูล GPS มา ~1 ครั้ง/วินาที
 *
 * @example
 * while(1) {
 *     GPS_Update(&gps);
 *     if (gps.data_ready) {
 *         // process new data
 *         gps.data_ready = 0;
 *     }
 * }
 */
GPS_Status GPS_Update(GPS_Instance* gps);

/**
 * @brief ตรวจสอบว่ามี GPS fix หรือไม่
 * @param gps pointer ไปยัง GPS_Instance
 * @return 1 = มี fix, 0 = ไม่มี fix
 */
uint8_t GPS_IsFixValid(GPS_Instance* gps);

/**
 * @brief อ่านค่า latitude
 * @param gps pointer ไปยัง GPS_Instance
 * @return Latitude in degrees (+N, -S)
 */
float GPS_GetLatitude(GPS_Instance* gps);

/**
 * @brief อ่านค่า longitude
 * @param gps pointer ไปยัง GPS_Instance
 * @return Longitude in degrees (+E, -W)
 */
float GPS_GetLongitude(GPS_Instance* gps);

/**
 * @brief อ่านค่าความสูงจากระดับน้ำทะเล
 * @param gps pointer ไปยัง GPS_Instance
 * @return Altitude in meters
 */
float GPS_GetAltitude(GPS_Instance* gps);

/**
 * @brief อ่านความเร็วภาคพื้นดิน
 * @param gps pointer ไปยัง GPS_Instance
 * @return Speed in km/h
 */
float GPS_GetSpeed(GPS_Instance* gps);

/**
 * @brief อ่านจำนวนดาวเทียมที่ใช้
 * @param gps pointer ไปยัง GPS_Instance
 * @return Number of satellites
 */
uint8_t GPS_GetSatellites(GPS_Instance* gps);

/**
 * @brief อ่านวันที่/เวลาจาก GPS (UTC)
 * @param gps pointer ไปยัง GPS_Instance
 * @param dt  pointer ไปยัง GPS_DateTime ที่จะเขียนค่าลงไป
 *
 * @example
 * GPS_DateTime dt;
 * GPS_GetDateTime(&gps, &dt);
 * printf("UTC: %02d:%02d:%02d\r\n", dt.hour, dt.minute, dt.second);
 */
void GPS_GetDateTime(GPS_Instance* gps, GPS_DateTime* dt);

#ifdef __cplusplus
}
#endif

#endif /* __GPS_H */
