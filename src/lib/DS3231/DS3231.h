/**
 * @file DS3231.h
 * @brief DS3231 Real-Time Clock Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่าน/ตั้งค่าวันเวลาด้วย DS3231 RTC ผ่าน I2C
 * DS3231 เป็น RTC ที่มี Temperature-Compensated Crystal Oscillator (TCXO)
 * ทำให้แม่นยำสูง (±2ppm หรือดีกว่า) และมี sensor วัดอุณหภูมิในตัว
 *
 * **คุณสมบัติ:**
 * - อ่าน/ตั้งค่า วินาที, นาที, ชั่วโมง, วันในสัปดาห์, วัน, เดือน, ปี
 * - รูปแบบ 12 ชั่วโมง (AM/PM) และ 24 ชั่วโมง
 * - Alarm 1 (วินาที, นาที, ชั่วโมง, วัน)
 * - Alarm 2 (นาที, ชั่วโมง, วัน)
 * - อ่านอุณหภูมิจาก sensor ในตัว (0.25°C resolution)
 * - 32kHz output สำหรับ timing reference
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          DS3231
 *                     +------+
 *   PC2 (SCL) ------> | SCL VCC| <----- 3.3V
 *   PC1 (SDA) ------> | SDA GND| ------> GND
 *   (optional)         | SQW    | ← Alarm interrupt output
 *   (optional)         | 32K    | ← 32kHz output
 *
 *   ต้องมี pull-up 4.7kΩ ที่ SDA และ SCL
 *   DS3231 มี battery backup (CR2032) สำหรับรักษาเวลาเมื่อไฟดับ
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "DS3231.h"
 *
 * DS3231_Instance rtc;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     DS3231_Init(&rtc);
 *
 *     // ตั้งเวลา: 2026-04-29, 14:30:00, วันพุธ
 *     DS3231_SetDateTime(&rtc, 2026, 4, 29, 14, 30, 0, DS3231_WEDNESDAY);
 *
 *     while (1) {
 *         DS3231_DateTime dt;
 *         DS3231_GetDateTime(&rtc, &dt);
 *         printf("%04d-%02d-%02d %02d:%02d:%02d\r\n",
 *                dt.year, dt.month, dt.day,
 *                dt.hour, dt.minute, dt.second);
 *         Delay_Ms(1000);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __DS3231_H
#define __DS3231_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/** @brief I2C Address ของ DS3231 (fixed, ไม่เปลี่ยน) */
#define DS3231_I2C_ADDR     0x68

/* Register addresses */
#define DS3231_REG_SECONDS  0x00
#define DS3231_REG_MINUTES  0x01
#define DS3231_REG_HOURS    0x02
#define DS3231_REG_DAY      0x03
#define DS3231_REG_DATE     0x04
#define DS3231_REG_MONTH    0x05
#define DS3231_REG_YEAR     0x06
#define DS3231_REG_A1_SEC   0x07
#define DS3231_REG_A1_MIN   0x08
#define DS3231_REG_A1_HR    0x09
#define DS3231_REG_A1_DAY   0x0A
#define DS3231_REG_A2_MIN   0x0B
#define DS3231_REG_A2_HR    0x0C
#define DS3231_REG_A2_DAY   0x0D
#define DS3231_REG_CONTROL  0x0E
#define DS3231_REG_STATUS   0x0F
#define DS3231_REG_AGING    0x10
#define DS3231_REG_TEMP_MSB 0x11
#define DS3231_REG_TEMP_LSB 0x12

/* ========== Type Definitions ========== */

/**
 * @brief วันในสัปดาห์
 */
typedef enum {
    DS3231_SUNDAY    = 1, /**< วันอาทิตย์ */
    DS3231_MONDAY    = 2, /**< วันจันทร์ */
    DS3231_TUESDAY   = 3, /**< วันอังคาร */
    DS3231_WEDNESDAY = 4, /**< วันพุธ */
    DS3231_THURSDAY  = 5, /**< วันพฤหัสบดี */
    DS3231_FRIDAY    = 6, /**< วันศุกร์ */
    DS3231_SATURDAY  = 7  /**< วันเสาร์ */
} DS3231_DayOfWeek;

/**
 * @brief รูปแบบเวลา
 */
typedef enum {
    DS3231_24H = 0, /**< 24 ชั่วโมง (default) */
    DS3231_12H = 1  /**< 12 ชั่วโมง (AM/PM) */
} DS3231_HourMode;

/**
 * @brief Alarm 1 Match Mode
 */
typedef enum {
    DS3231_A1_EVERY_SECOND      = 0x0F, /**< ทุกวินาที */
    DS3231_A1_MATCH_SECONDS     = 0x0E, /**< ตรงวินาที */
    DS3231_A1_MATCH_MIN_SEC     = 0x0C, /**< ตรงนาที+วินาที */
    DS3231_A1_MATCH_HR_MIN_SEC  = 0x08, /**< ตรงชั่วโมง+นาที+วินาที */
    DS3231_A1_MATCH_DATE        = 0x00, /**< ตรงวันที่+ชั่วโมง+นาที+วินาที */
} DS3231_Alarm1Mode;

/**
 * @brief Alarm 2 Match Mode
 */
typedef enum {
    DS3231_A2_EVERY_MINUTE      = 0x07, /**< ทุกนาที */
    DS3231_A2_MATCH_MINUTES     = 0x06, /**< ตรงนาที */
    DS3231_A2_MATCH_HR_MIN      = 0x04, /**< ตรงชั่วโมง+นาที */
    DS3231_A2_MATCH_DATE        = 0x00, /**< ตรงวันที่+ชั่วโมง+นาที */
} DS3231_Alarm2Mode;

/**
 * @brief โครงสร้างวันเวลา
 */
typedef struct {
    uint16_t year;     /**< ปี ค.ศ. (2000-2099) */
    uint8_t  month;    /**< เดือน (1-12) */
    uint8_t  day;      /**< วัน (1-31) */
    uint8_t  hour;     /**< ชั่วโมง (0-23 หรือ 1-12) */
    uint8_t  minute;   /**< นาที (0-59) */
    uint8_t  second;   /**< วินาที (0-59) */
    DS3231_DayOfWeek day_of_week; /**< วันในสัปดาห์ */
    uint8_t  is_pm;    /**< 1=PM (เฉพาะ 12H mode) */
} DS3231_DateTime;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    DS3231_OK          = 0, /**< สำเร็จ */
    DS3231_ERROR_I2C   = 1, /**< I2C error */
    DS3231_ERROR_PARAM = 2  /**< Parameter ผิด */
} DS3231_Status;

/**
 * @brief DS3231 Instance
 */
typedef struct {
    DS3231_HourMode hour_mode; /**< รูปแบบเวลา (12H/24H) */
    uint8_t initialized;       /**< flag บอกว่า Init แล้ว */
} DS3231_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น DS3231
 * @param rtc ตัวแปร instance
 * @return DS3231_OK หรือ error code
 *
 * @note ต้องเรียก I2C_SimpleInit() ก่อน
 * @example DS3231_Init(&rtc);
 */
DS3231_Status DS3231_Init(DS3231_Instance* rtc);

/**
 * @brief ตั้งค่าวันเวลา
 * @param rtc   ตัวแปร instance
 * @param year  ปี ค.ศ. (2000-2099)
 * @param month เดือน (1-12)
 * @param day   วัน (1-31)
 * @param hour  ชั่วโมง (0-23 สำหรับ 24H)
 * @param min   นาที (0-59)
 * @param sec   วินาที (0-59)
 * @param dow   วันในสัปดาห์ (DS3231_SUNDAY ถึง DS3231_SATURDAY)
 * @return DS3231_OK หรือ error code
 *
 * @example
 * DS3231_SetDateTime(&rtc, 2026, 4, 29, 14, 30, 0, DS3231_WEDNESDAY);
 */
DS3231_Status DS3231_SetDateTime(DS3231_Instance* rtc,
                                  uint16_t year, uint8_t month, uint8_t day,
                                  uint8_t hour, uint8_t min, uint8_t sec,
                                  DS3231_DayOfWeek dow);

/**
 * @brief อ่านวันเวลาปัจจุบัน
 * @param rtc ตัวแปร instance
 * @param dt  pointer สำหรับรับวันเวลา
 * @return DS3231_OK หรือ error code
 *
 * @example
 * DS3231_DateTime dt;
 * DS3231_GetDateTime(&rtc, &dt);
 * printf("%04d-%02d-%02d %02d:%02d:%02d\r\n",
 *        dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
 */
DS3231_Status DS3231_GetDateTime(DS3231_Instance* rtc, DS3231_DateTime* dt);

/**
 * @brief อ่านอุณหภูมิจาก sensor ในตัว
 * @param rtc  ตัวแปร instance
 * @param temp pointer สำหรับรับอุณหภูมิ (°C, ความละเอียด 0.25°C)
 * @return DS3231_OK หรือ error code
 *
 * @example
 * float temp;
 * DS3231_GetTemperature(&rtc, &temp);
 * printf("Temp: %.2f C\r\n", temp);
 */
DS3231_Status DS3231_GetTemperature(DS3231_Instance* rtc, float* temp);

/**
 * @brief ตั้ง Alarm 1
 * @param rtc   ตัวแปร instance
 * @param mode  โหมด alarm (DS3231_A1_MATCH_HR_MIN_SEC ฯลฯ)
 * @param day   วัน (1-31) หรือ 0 ถ้า mode ไม่ใช้วัน
 * @param hour  ชั่วโมง (0-23)
 * @param min   นาที (0-59)
 * @param sec   วินาที (0-59)
 * @return DS3231_OK หรือ error code
 *
 * @example
 * // Alarm ทุกวัน เวลา 07:00:00
 * DS3231_SetAlarm1(&rtc, DS3231_A1_MATCH_HR_MIN_SEC, 0, 7, 0, 0);
 * DS3231_EnableAlarm1(&rtc, 1);
 */
DS3231_Status DS3231_SetAlarm1(DS3231_Instance* rtc, DS3231_Alarm1Mode mode,
                                uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

/**
 * @brief ตั้ง Alarm 2
 * @param rtc   ตัวแปร instance
 * @param mode  โหมด alarm (DS3231_A2_MATCH_HR_MIN ฯลฯ)
 * @param day   วัน (1-31) หรือ 0 ถ้า mode ไม่ใช้วัน
 * @param hour  ชั่วโมง (0-23)
 * @param min   นาที (0-59)
 * @return DS3231_OK หรือ error code
 */
DS3231_Status DS3231_SetAlarm2(DS3231_Instance* rtc, DS3231_Alarm2Mode mode,
                                uint8_t day, uint8_t hour, uint8_t min);

/**
 * @brief เปิด/ปิด Alarm 1 interrupt บน SQW pin
 * @param rtc    ตัวแปร instance
 * @param enable 1=เปิด, 0=ปิด
 * @return DS3231_OK หรือ error code
 */
DS3231_Status DS3231_EnableAlarm1(DS3231_Instance* rtc, uint8_t enable);

/**
 * @brief เปิด/ปิด Alarm 2 interrupt บน SQW pin
 * @param rtc    ตัวแปร instance
 * @param enable 1=เปิด, 0=ปิด
 * @return DS3231_OK หรือ error code
 */
DS3231_Status DS3231_EnableAlarm2(DS3231_Instance* rtc, uint8_t enable);

/**
 * @brief ตรวจสอบว่า Alarm 1 ถูกตั้ง flag หรือยัง
 * @param rtc ตัวแปร instance
 * @return 1 = alarm fired, 0 = ยังไม่ fire
 */
uint8_t DS3231_IsAlarm1Fired(DS3231_Instance* rtc);

/**
 * @brief ตรวจสอบว่า Alarm 2 ถูกตั้ง flag หรือยัง
 * @param rtc ตัวแปร instance
 * @return 1 = alarm fired, 0 = ยังไม่ fire
 */
uint8_t DS3231_IsAlarm2Fired(DS3231_Instance* rtc);

/**
 * @brief ล้าง Alarm flag (ต้องล้างหลังจาก alarm fired เพื่อรับ alarm ครั้งถัดไป)
 * @param rtc      ตัวแปร instance
 * @param alarm_no 1 = clear alarm 1, 2 = clear alarm 2
 * @return DS3231_OK หรือ error code
 */
DS3231_Status DS3231_ClearAlarmFlag(DS3231_Instance* rtc, uint8_t alarm_no);

/**
 * @brief แปลง DS3231_DayOfWeek เป็น string ภาษาอังกฤษ
 * @param dow วันในสัปดาห์
 * @return เช่น "Monday", "Tuesday"
 */
const char* DS3231_DayOfWeekStr(DS3231_DayOfWeek dow);

/**
 * @brief แปลง DS3231_DayOfWeek เป็น string ภาษาไทย
 * @param dow วันในสัปดาห์
 * @return เช่น "จันทร์", "อังคาร"
 */
const char* DS3231_DayOfWeekStrTH(DS3231_DayOfWeek dow);

#ifdef __cplusplus
}
#endif

#endif /* __DS3231_H */
