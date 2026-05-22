# MPU6050 IMU Library

> **Library สำหรับใช้งาน MPU6050 Accelerometer + Gyroscope บน CH57x**

## 📋 สารบัญ

1. [ภาพรวม](#ภาพรวม)
2. [Hardware Setup](#hardware-setup)
3. [การใช้งานพื้นฐาน](#การใช้งานพื้นฐาน)
4. [Calibration](#calibration)
5. [การใช้งานขั้นสูง](#การใช้งานขั้นสูง)
6. [Troubleshooting](#troubleshooting)
7. [API Reference](#api-reference)

---

## ภาพรวม

MPU6050 เป็น **6-DoF IMU** (Inertial Measurement Unit) ที่มี Accelerometer 3 แกน + Gyroscope 3 แกนในชิปเดียว  
มี Digital Motion Processor (DMP) ในตัว และ temperature sensor

| คุณสมบัติ | ค่า |
|----------|-----|
| I2C Address | 0x68 (AD0=GND) หรือ 0x69 (AD0=VCC) |
| Accel Range | ±2g / ±4g / ±8g / ±16g |
| Gyro Range | ±250 / ±500 / ±1000 / ±2000 °/s |
| ADC Resolution | 16-bit |
| ไฟเลี้ยง | 3.3V–5V |

---

## Hardware Setup

```
CH57x (3.3V)        MPU6050 Module
                        +----------+
PC2 (SCL) -----------> | SCL  VCC | <----- 3.3V
PC1 (SDA) <----------> | SDA  GND | ------> GND
                        | AD0      | ------> GND (address 0x68)
                        | INT      | ------> GPIO (optional, interrupt)
                        +----------+

Pull-up:
  PC2 ──[4.7kΩ]── 3.3V
  PC1 ──[4.7kΩ]── 3.3V
```

### Axis Orientation

```
        Z ↑
        |
        |___→ X
       /
      Y
(ตาม ฉลาก บน module MPU6050)
```

---

## การใช้งานพื้นฐาน

```c
#include "SimpleHAL.h"
#include "MPU6050.h"

MPU6050_Instance imu;

int main(void) {
    SystemCoreClockUpdate();
    Timer_Init();
    I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);

    if (MPU6050_Init(&imu, MPU6050_ADDR_LOW) != MPU6050_OK) {
        printf("MPU6050 not found!\r\n");
        while (1) {}
    }
    printf("MPU6050 OK\r\n");

    while (1) {
        MPU6050_Data d;
        MPU6050_GetAll(&imu, &d);

        printf("A: %.2f %.2f %.2f g | G: %.1f %.1f %.1f dps | T: %.1f C\r\n",
               d.ax, d.ay, d.az,
               d.gx, d.gy, d.gz,
               d.temp);
        Delay_Ms(100);
    }
}
```

---

## Calibration

Gyroscope มี **drift** (ค่า offset ที่ทำให้ gyro ไม่ตรงศูนย์แม้ MPU หยุดนิ่ง)  
ต้อง **calibrate** เพื่อลด drift:

```c
// วาง MPU6050 ให้นิ่งบนพื้นราบ
printf("กรุณาวาง MPU6050 ให้นิ่ง...\r\n");
Delay_Ms(2000);

// Calibrate ด้วย 300 samples (~600ms)
MPU6050_CalibrateGyro(&imu, 300);
printf("Calibrate เสร็จแล้ว\r\n");

// หลัง calibrate ค่า gyro ควรใกล้ 0 เมื่อ MPU หยุดนิ่ง
```

---

## การใช้งานขั้นสูง

### เลือก Accel/Gyro Range

```c
// เพิ่ม range สำหรับการเคลื่อนที่เร็ว
MPU6050_SetAccelRange(&imu, MPU6050_ACCEL_8G);
MPU6050_SetGyroRange(&imu, MPU6050_GYRO_1000DPS);
```

### ตั้ง DLPF เพื่อลด noise

```c
// Filter มากขึ้น (noise น้อย แต่ delay มากขึ้น)
MPU6050_SetDLPF(&imu, MPU6050_DLPF_21HZ);

// สำหรับ drone/vibration ที่ต้องการ response เร็ว
MPU6050_SetDLPF(&imu, MPU6050_DLPF_94HZ);
```

### คำนวณมุม Tilt จาก Accelerometer

```c
#include <math.h>

float ax, ay, az;
MPU6050_GetAccel(&imu, &ax, &ay, &az);

float pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * (180.0f / 3.14159f);
float roll  = atan2f(ay, sqrtf(ax*ax + az*az)) * (180.0f / 3.14159f);

printf("Pitch: %.1f°  Roll: %.1f°\r\n", pitch, roll);
```

### Power Save Mode

```c
MPU6050_Sleep(&imu);
Delay_Ms(5000);
MPU6050_WakeUp(&imu);
```

---

## Troubleshooting

| ปัญหา | สาเหตุ | วิธีแก้ |
|-------|--------|---------|
| `MPU6050_ERROR_WHOAMI` | ชิปไม่ใช่ MPU6050 | ตรวจว่าต่อ VCC และ GND ถูกต้อง |
| `MPU6050_ERROR_I2C` | I2C ไม่เชื่อม | ตรวจ pull-up 4.7kΩ, ตรวจ address AD0 |
| Gyro drift สูง | ไม่ได้ calibrate | เรียก `MPU6050_CalibrateGyro()` |
| ค่า accel ผิด | range ไม่เหมาะ | ลอง `MPU6050_ACCEL_2G` (default) |

---

## API Reference

| Function | คำอธิบาย |
|----------|----------|
| `MPU6050_Init(imu, addr)` | Init — ตรวจ WHO_AM_I, ตั้งค่า default |
| `MPU6050_GetAll(imu, &data)` | อ่านทุกค่าพร้อมกัน (efficient สุด) |
| `MPU6050_GetAccel(imu, &ax, &ay, &az)` | อ่าน Accel (g) |
| `MPU6050_GetGyro(imu, &gx, &gy, &gz)` | อ่าน Gyro (°/s) |
| `MPU6050_GetTemp(imu, &temp)` | อ่านอุณหภูมิ (°C) |
| `MPU6050_GetRaw(imu, &raw)` | อ่านค่า raw 16-bit |
| `MPU6050_SetAccelRange(imu, range)` | ตั้ง Accel range |
| `MPU6050_SetGyroRange(imu, range)` | ตั้ง Gyro range |
| `MPU6050_SetDLPF(imu, dlpf)` | ตั้ง Low-Pass Filter |
| `MPU6050_CalibrateGyro(imu, samples)` | Calibrate Gyro offset |
| `MPU6050_Sleep(imu)` / `WakeUp(imu)` | Power management |

### Accel Range และ LSB

| Range | LSB (count/g) | ความละเอียด |
|-------|--------------|------------|
| `MPU6050_ACCEL_2G` | 16384 | 0.061 mg/LSB |
| `MPU6050_ACCEL_4G` | 8192 | 0.122 mg/LSB |
| `MPU6050_ACCEL_8G` | 4096 | 0.244 mg/LSB |
| `MPU6050_ACCEL_16G` | 2048 | 0.488 mg/LSB |

### Gyro Range และ LSB

| Range | LSB (count/°/s) |
|-------|----------------|
| `MPU6050_GYRO_250DPS` | 131.0 |
| `MPU6050_GYRO_500DPS` | 65.5 |
| `MPU6050_GYRO_1000DPS` | 32.8 |
| `MPU6050_GYRO_2000DPS` | 16.4 |

---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)