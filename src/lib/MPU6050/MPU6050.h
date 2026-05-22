/**
 * @file MPU6050.h
 * @brief MPU6050 6-Axis IMU Library สำหรับ CH57x
 * @version 1.0
 * @date 2026-04-29
 *
 * @details
 * Library สำหรับอ่านข้อมูล Accelerometer + Gyroscope จาก MPU6050 ผ่าน I2C
 * MPU6050 เป็น 6-DoF IMU (Inertial Measurement Unit) ที่ใช้กันอย่างแพร่หลาย
 *
 * **คุณสมบัติ:**
 * - Accelerometer 3 แกน (X, Y, Z) — วัดความเร่ง ±2g/±4g/±8g/±16g
 * - Gyroscope 3 แกน (X, Y, Z)   — วัดความเร็วเชิงมุม ±250/±500/±1000/±2000 °/s
 * - Thermometer ในตัว (±1°C)
 * - Low Pass Filter (DLPF) ปรับได้
 * - Calibration offset สำหรับลด drift
 *
 * **Hardware Connection:**
 * ```
 *   CH57x          MPU6050
 *   PC2 (SCL) ------> SCL  + [4.7kΩ → 3.3V]
 *   PC1 (SDA) <-----> SDA  + [4.7kΩ → 3.3V]
 *   3.3V -----------> VCC
 *   GND ------------> GND
 *   GND ------------> AD0  (I2C address = 0x68)
 *   // หรือ VCC → AD0 เพื่อใช้ address 0x69
 * ```
 *
 * @example
 * #include "SimpleHAL.h"
 * #include "MPU6050.h"
 *
 * MPU6050_Instance imu;
 *
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     Timer_Init();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 *
 *     MPU6050_Init(&imu, MPU6050_ADDR_LOW);
 *
 *     while (1) {
 *         MPU6050_Data data;
 *         MPU6050_GetAll(&imu, &data);
 *         printf("Ax=%.2f Ay=%.2f Az=%.2f | Gx=%.2f Gy=%.2f Gz=%.2f\r\n",
 *                data.ax, data.ay, data.az,
 *                data.gx, data.gy, data.gz);
 *         Delay_Ms(100);
 *     }
 * }
 *
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../SimpleHAL/SimpleHAL.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== I2C Addresses ========== */
#define MPU6050_ADDR_LOW    0x68  /**< AD0 = GND (default) */
#define MPU6050_ADDR_HIGH   0x69  /**< AD0 = VCC */

/* ========== Registers ========== */
#define MPU6050_REG_SMPLRT_DIV    0x19
#define MPU6050_REG_CONFIG        0x1A
#define MPU6050_REG_GYRO_CONFIG   0x1B
#define MPU6050_REG_ACCEL_CONFIG  0x1C
#define MPU6050_REG_ACCEL_XOUT_H  0x3B
#define MPU6050_REG_TEMP_OUT_H    0x41
#define MPU6050_REG_GYRO_XOUT_H   0x43
#define MPU6050_REG_PWR_MGMT_1    0x6B
#define MPU6050_REG_WHO_AM_I      0x75

#define MPU6050_WHO_AM_I_VAL      0x68  /**< Expected value of WHO_AM_I */

/* ========== Type Definitions ========== */

/**
 * @brief Accelerometer Full-Scale Range
 */
typedef enum {
    MPU6050_ACCEL_2G  = 0, /**< ±2g  — LSB = 16384 count/g */
    MPU6050_ACCEL_4G  = 1, /**< ±4g  — LSB = 8192  count/g */
    MPU6050_ACCEL_8G  = 2, /**< ±8g  — LSB = 4096  count/g */
    MPU6050_ACCEL_16G = 3  /**< ±16g — LSB = 2048  count/g */
} MPU6050_AccelRange;

/**
 * @brief Gyroscope Full-Scale Range
 */
typedef enum {
    MPU6050_GYRO_250DPS  = 0, /**< ±250 °/s  — LSB = 131.0 count/(°/s) */
    MPU6050_GYRO_500DPS  = 1, /**< ±500 °/s  — LSB = 65.5  count/(°/s) */
    MPU6050_GYRO_1000DPS = 2, /**< ±1000 °/s — LSB = 32.8  count/(°/s) */
    MPU6050_GYRO_2000DPS = 3  /**< ±2000 °/s — LSB = 16.4  count/(°/s) */
} MPU6050_GyroRange;

/**
 * @brief Digital Low-Pass Filter Bandwidth
 */
typedef enum {
    MPU6050_DLPF_260HZ = 0, /**< 260Hz Accel, 256Hz Gyro (no filter) */
    MPU6050_DLPF_184HZ = 1, /**< 184Hz */
    MPU6050_DLPF_94HZ  = 2, /**< 94Hz */
    MPU6050_DLPF_44HZ  = 3, /**< 44Hz (ดีสำหรับ motion detection ทั่วไป) */
    MPU6050_DLPF_21HZ  = 4, /**< 21Hz */
    MPU6050_DLPF_10HZ  = 5, /**< 10Hz */
    MPU6050_DLPF_5HZ   = 6  /**< 5Hz (filter มาก, delay สูง) */
} MPU6050_DLPF;

/**
 * @brief สถานะการทำงาน
 */
typedef enum {
    MPU6050_OK            = 0, /**< สำเร็จ */
    MPU6050_ERROR_I2C     = 1, /**< I2C error */
    MPU6050_ERROR_PARAM   = 2, /**< Parameter ผิด */
    MPU6050_ERROR_WHOAMI  = 3  /**< WHO_AM_I ผิด (ไม่ใช่ MPU6050) */
} MPU6050_Status;

/**
 * @brief ข้อมูล sensor ที่อ่านได้ (แปลงหน่วยแล้ว)
 */
typedef struct {
    float ax;   /**< Acceleration X (g) */
    float ay;   /**< Acceleration Y (g) */
    float az;   /**< Acceleration Z (g) */
    float gx;   /**< Gyro X (°/s) */
    float gy;   /**< Gyro Y (°/s) */
    float gz;   /**< Gyro Z (°/s) */
    float temp; /**< Temperature (°C) */
} MPU6050_Data;

/**
 * @brief ข้อมูล raw sensor (ยังไม่แปลงหน่วย)
 */
typedef struct {
    int16_t ax_raw, ay_raw, az_raw;
    int16_t gx_raw, gy_raw, gz_raw;
    int16_t temp_raw;
} MPU6050_RawData;

/**
 * @brief MPU6050 Instance
 */
typedef struct {
    uint8_t          i2c_addr;      /**< I2C address (0x68 หรือ 0x69) */
    MPU6050_AccelRange accel_range; /**< Accel range ที่ตั้งไว้ */
    MPU6050_GyroRange  gyro_range;  /**< Gyro range ที่ตั้งไว้ */
    float            accel_lsb;     /**< Accel scale factor (count/g) */
    float            gyro_lsb;      /**< Gyro scale factor (count/°/s) */
    int16_t          gyro_offset_x; /**< Gyro calibration offset X */
    int16_t          gyro_offset_y; /**< Gyro calibration offset Y */
    int16_t          gyro_offset_z; /**< Gyro calibration offset Z */
    uint8_t          initialized;   /**< flag บอกว่า Init แล้ว */
} MPU6050_Instance;

/* ========== Function Prototypes ========== */

/**
 * @brief เริ่มต้น MPU6050
 * @param imu      ตัวแปร instance
 * @param i2c_addr I2C address (MPU6050_ADDR_LOW หรือ MPU6050_ADDR_HIGH)
 * @return MPU6050_OK หรือ error code
 *
 * @note ตั้งค่า default: Accel ±2g, Gyro ±250°/s, DLPF 44Hz
 * @example MPU6050_Init(&imu, MPU6050_ADDR_LOW);
 */
MPU6050_Status MPU6050_Init(MPU6050_Instance* imu, uint8_t i2c_addr);

/**
 * @brief ตั้ง Accelerometer range
 * @param imu   ตัวแปร instance
 * @param range MPU6050_ACCEL_2G ถึง MPU6050_ACCEL_16G
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_SetAccelRange(MPU6050_Instance* imu, MPU6050_AccelRange range);

/**
 * @brief ตั้ง Gyroscope range
 * @param imu   ตัวแปร instance
 * @param range MPU6050_GYRO_250DPS ถึง MPU6050_GYRO_2000DPS
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_SetGyroRange(MPU6050_Instance* imu, MPU6050_GyroRange range);

/**
 * @brief ตั้ง Digital Low-Pass Filter
 * @param imu  ตัวแปร instance
 * @param dlpf bandwidth (MPU6050_DLPF_260HZ ถึง MPU6050_DLPF_5HZ)
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_SetDLPF(MPU6050_Instance* imu, MPU6050_DLPF dlpf);

/**
 * @brief อ่านข้อมูล raw (16-bit integers)
 * @param imu ตัวแปร instance
 * @param raw pointer รับข้อมูล raw
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_GetRaw(MPU6050_Instance* imu, MPU6050_RawData* raw);

/**
 * @brief อ่านข้อมูล Accelerometer (g units)
 * @param imu ตัวแปร instance
 * @param ax  pointer รับค่า X (g)
 * @param ay  pointer รับค่า Y (g)
 * @param az  pointer รับค่า Z (g)
 * @return MPU6050_OK หรือ error code
 *
 * @example
 * float ax, ay, az;
 * MPU6050_GetAccel(&imu, &ax, &ay, &az);
 * printf("Accel: %.3f %.3f %.3f g\r\n", ax, ay, az);
 */
MPU6050_Status MPU6050_GetAccel(MPU6050_Instance* imu, float* ax, float* ay, float* az);

/**
 * @brief อ่านข้อมูล Gyroscope (°/s)
 * @param imu ตัวแปร instance
 * @param gx  pointer รับค่า X (°/s)
 * @param gy  pointer รับค่า Y (°/s)
 * @param gz  pointer รับค่า Z (°/s)
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_GetGyro(MPU6050_Instance* imu, float* gx, float* gy, float* gz);

/**
 * @brief อ่านอุณหภูมิจาก sensor ในตัว
 * @param imu  ตัวแปร instance
 * @param temp pointer รับอุณหภูมิ (°C)
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_GetTemp(MPU6050_Instance* imu, float* temp);

/**
 * @brief อ่านข้อมูลทั้งหมดพร้อมกัน (accel + gyro + temp) — efficient กว่าเรียกแยก
 * @param imu  ตัวแปร instance
 * @param data pointer รับข้อมูลทั้งหมด
 * @return MPU6050_OK หรือ error code
 *
 * @example
 * MPU6050_Data d;
 * MPU6050_GetAll(&imu, &d);
 * printf("%.2f %.2f %.2f | %.2f %.2f %.2f | %.1f°C\r\n",
 *        d.ax, d.ay, d.az, d.gx, d.gy, d.gz, d.temp);
 */
MPU6050_Status MPU6050_GetAll(MPU6050_Instance* imu, MPU6050_Data* data);

/**
 * @brief Calibrate Gyroscope (ตั้งศูนย์ offset ขณะหยุดนิ่ง)
 * @param imu     ตัวแปร instance
 * @param samples จำนวน samples (แนะนำ 200-500)
 * @return MPU6050_OK หรือ error code
 *
 * @note ต้องวาง MPU6050 ให้นิ่งระหว่าง calibrate
 * @example MPU6050_CalibrateGyro(&imu, 300);
 */
MPU6050_Status MPU6050_CalibrateGyro(MPU6050_Instance* imu, uint16_t samples);

/**
 * @brief Wake up MPU6050 จาก sleep mode
 * @param imu ตัวแปร instance
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_WakeUp(MPU6050_Instance* imu);

/**
 * @brief ส่ง MPU6050 เข้า sleep mode (ประหยัดไฟ)
 * @param imu ตัวแปร instance
 * @return MPU6050_OK หรือ error code
 */
MPU6050_Status MPU6050_Sleep(MPU6050_Instance* imu);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
