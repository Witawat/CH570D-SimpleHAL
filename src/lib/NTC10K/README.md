# NTC10K Library - คู่มือการใช้งาน

**Version:** 1.0
**Date:** 2025-12-22

## ภาพรวม

NTC10K Library เป็น library สำหรับวัดอุณหภูมิด้วย NTC 10K thermistor บน CH57x รองรับทั้ง Steinhart-Hart equation (แม่นยำสูง) และ Beta equation (เร็ว)

## คุณสมบัติ

- รองรับ NTC 10K (B-value 3950, 3435, หรือกำหนดเอง)
- Steinhart-Hart และ Beta equation
- Calibration system
- Averaging และ filtering
- Multi-sensor support (สูงสุด 4 sensors)
- Temperature monitoring พร้อม callback

## วงจร Voltage Divider

### แบบ NTC-to-GND (แนะนำ)
`
VCC (3.3V)
   |
 [R_series 10K]
   |
   +----> ADC Pin
   |
 [NTC 10K]
   |
  GND
`

**สูตรคำนวณ:**
- R_NTC = R_series × (ADC / (1023 - ADC))
- V_ADC = VCC × (R_NTC / (R_series + R_NTC))

### แบบ NTC-to-VCC
`
VCC (3.3V)
   |
 [NTC 10K]
   |
   +----> ADC Pin
   |
 [R_series 10K]
   |
  GND
`

**สูตรคำนวณ:**
- R_NTC = R_series × ((1023 - ADC) / ADC)

## การคำนวณอุณหภูมิ

### Beta Equation (เร็ว)
`
1/T = 1/T0 + (1/B) × ln(R/R0)
`
- T = อุณหภูมิ (Kelvin)
- T0 = อุณหภูมิอ้างอิง (298.15K = 25°C)
- B = B-value (3950K)
- R = ความต้านทาน NTC ปัจจุบัน
- R0 = ความต้านทาน NTC ที่ 25°C (10K)

### Steinhart-Hart Equation (แม่นยำ)
`
1/T = A + B×ln(R) + C×(ln(R))³
`
- A = 0.001129148
- B = 0.000234125
- C = 0.0000000876741

## Quick Start

`c
#include " NTC10K.h\

int main(void) {
 SystemCoreClockUpdate();
 
 // เริ่มต้น NTC
 NTC_Instance* ntc = NTC_Init(ADC_CH_PD2);
 
 while(1) {
 float temp = NTC_ReadTemperature(ntc);
 printf(\Temp: %.2f C\\r\\n\, temp);
 Delay_Ms(1000);
 }
}
`

## ตัวอย่างการใช้งาน

1. **01_Basic_Temperature.c** - อ่านอุณหภูมิพื้นฐาน
2. **02_With_Averaging.c** - ใช้ averaging ลด noise
3. **03_Calibration.c** - การ calibrate sensor
4. **04_Multi_Sensor.c** - ใช้หลาย sensors
5. **05_Temperature_Monitor.c** - ระบบเฝ้าระวัง
6. **06_Data_Logger.c** - บันทึกข้อมูล
7. **07_Project_ThermoController.c** - ควบคุมอุณหภูมิ

## API Reference

ดูรายละเอียดใน NTC10K.h

## Troubleshooting

- **อุณหภูมิผิดปกติ** → ตรวจสอบ R_series และ B-value
- **ADC ติด 0/1023** → ตรวจสอบการต่อวงจร
- **ค่ากระโดด** → ใช้ averaging หรือเพิ่ม capacitor


---

> **หมายเหตุ:** ไลบรารีนี้ถูกพอร์ตจากโปรเจกต์ CH32V003 ผ่าน Compatibility Shim  
> CH57x (CH570D) มีเฉพาะ PORTA (PA0–PA15) — ต้องปรับ pin mapping จาก PCx/PDx เป็น PAx  
> ดูรายละเอียดได้ที่ [compat_shim.md](../../SimpleHAL/docs/compat_shim.md)