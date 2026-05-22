/**
 * @file VL53L0X.h
 * @brief VL53L0X Time-of-Flight Distance Sensor Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับวัดระยะทางด้วย VL53L0X ToF (Time-of-Flight) sensor ผ่าน I2C
 * ช่วงวัด: 30mm - 2000mm (ขึ้นกับแสง), ความละเอียด ±3%
 * แม่นยำกว่า HC-SR04 (ultrasonic) และไม่รับกวนจากเสียง
 *
 * **วงจร:**
 * ```
 *   CH57x          VL53L0X
 *   PC2 (SCL) ──────> SCL
 *   PC1 (SDA) <─────> SDA
 *   3.3V ───────────> VDD (2.6-3.5V)
 *   GND ────────────> GND
 *   (GPIO ─────────> XSHUT  ลาก HIGH ถ้าไม่ใช้ shutdown)
 *   (GPIO <─────────GPIO1   interrupt pin, ไม่จำเป็น)
 * ```
 *
 * @example
 * VL53L0X_Instance tof;
 * VL53L0X_Init(&tof, VL53L0X_ADDR_DEFAULT);
 *
 * uint16_t mm = VL53L0X_ReadRangeMM(&tof);
 * printf("Distance: %d mm\r\n", mm);
 *
 * @author CH57x Library Team
 */

#ifndef __VL53L0X_H
#define __VL53L0X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>

/* ========== I2C Address ========== */
#define VL53L0X_ADDR_DEFAULT  0x29

/* ========== Key Registers ========== */
#define VL53L0X_REG_SYSRANGE_START              0x00
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS     0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS         0x14
#define VL53L0X_REG_CROSSTALK_COMP_ENABLE       0x20
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH     0x84
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR      0x0B
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID     0xC0  /**< = 0xEE */
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID  0xC2
#define VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV 0x89
#define VL53L0X_REG_MSRC_CONFIG_CONTROL         0x60
#define VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG      0x01
#define VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD 0x4E
#define VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET    0x4F
#define VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT   0xB6
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0    0xB0
#define VL53L0X_REG_OSC_CALIBRATE_VAL           0xF8
#define VL53L0X_REG_RESULT_FINAL_RANGE_MM       0x1E  /**< ผลลัพธ์ระยะ (2 bytes, BE) */

/* Range return value เมื่อ out of range */
#define VL53L0X_OUT_OF_RANGE  8190

/* ========== Type Definitions ========== */

/**
 * @brief โหมดการวัด
 */
typedef enum {
    VL53L0X_MODE_SINGLE    = 0, /**< วัดครั้งเดียว (default) */
    VL53L0X_MODE_CONTINUOUS = 1 /**< วัดต่อเนื่อง */
} VL53L0X_Mode;

/**
 * @brief Timing Budget — trade-off ระหว่างความแม่นยำ vs เวลา
 */
typedef enum {
    VL53L0X_BUDGET_20MS  = 20,  /**< เร็ว แต่ accuracy ต่ำ */
    VL53L0X_BUDGET_33MS  = 33,  /**< default (±3%) */
    VL53L0X_BUDGET_100MS = 100, /**< accuracy สูง */
    VL53L0X_BUDGET_200MS = 200  /**< accuracy สูงสุด */
} VL53L0X_Budget;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    VL53L0X_OK             = 0,
    VL53L0X_ERROR_PARAM    = 1,
    VL53L0X_ERROR_I2C      = 2,
    VL53L0X_ERROR_TIMEOUT  = 3,
    VL53L0X_ERROR_BOOT     = 4  /**< Init sequence failed */
} VL53L0X_Status;

/**
 * @brief VL53L0X Instance
 */
typedef struct {
    uint8_t  i2c_addr;
    uint8_t  stop_variable;   /**< ค่าสำหรับ stop sequence (จาก init) */
    uint8_t  mode;
    uint8_t  initialized;
} VL53L0X_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น VL53L0X (init sequence ตาม ST API)
 * @param tof  ตัวแปร instance
 * @param addr I2C address (VL53L0X_ADDR_DEFAULT = 0x29)
 * @return VL53L0X_OK หรือ error code
 */
VL53L0X_Status VL53L0X_Init(VL53L0X_Instance* tof, uint8_t addr);

/**
 * @brief วัดระยะ (single shot, blocking)
 * @param tof ตัวแปร instance
 * @return ระยะ (mm) หรือ VL53L0X_OUT_OF_RANGE (8190) ถ้าไกลเกิน
 */
uint16_t VL53L0X_ReadRangeMM(VL53L0X_Instance* tof);

/**
 * @brief เริ่ม Continuous mode
 * @param tof           ตัวแปร instance
 * @param period_ms     ระยะเวลาระหว่างการวัด (ms) 0=fastest
 */
VL53L0X_Status VL53L0X_StartContinuous(VL53L0X_Instance* tof, uint32_t period_ms);

/**
 * @brief หยุด Continuous mode
 */
VL53L0X_Status VL53L0X_StopContinuous(VL53L0X_Instance* tof);

/**
 * @brief อ่านค่าจาก Continuous mode (non-blocking)
 * @param tof     ตัวแปร instance
 * @param dist_mm ตัวแปรรับระยะ (mm)
 * @return VL53L0X_OK ถ้ามีข้อมูลใหม่, VL53L0X_ERROR_TIMEOUT ถ้ายังไม่พร้อม
 */
VL53L0X_Status VL53L0X_ReadContinuous(VL53L0X_Instance* tof, uint16_t* dist_mm);

/**
 * @brief เปลี่ยน I2C address (ใช้เมื่อมีหลาย sensor บน bus เดียวกัน)
 * @param tof      ตัวแปร instance
 * @param new_addr address ใหม่ (7-bit)
 * @note ต้องใช้ XSHUT pin ควบคุม เพื่อ enable ทีละตัว
 */
VL53L0X_Status VL53L0X_SetAddress(VL53L0X_Instance* tof, uint8_t new_addr);

#ifdef __cplusplus
}
#endif

#endif /* __VL53L0X_H */
