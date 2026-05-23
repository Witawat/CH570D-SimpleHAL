/**
 * @file HCSR04.c
 * @brief HC-SR04 Ultrasonic Distance Sensor Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "HCSR04.h"

/* ========== Private Functions ========== */

/**
 * @brief ส่ง TRIG pulse และวัด ECHO duration
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance ที่ init แล้ว
 * @return uint32_t - ระยะเวลาของ ECHO pulse (µs) หรือ 0 ถ้า timeout
 *
 * @note หลักการ: ส่ง TRIG pulse 10µs → รอ ECHO HIGH → วัด duration ที่ ECHO อยู่ HIGH
 * @note Timeout รอ ECHO start: 30ms (HCSR04_ECHO_TIMEOUT_US)
 * @note Timeout รอ ECHO end: 30ms (เกินระยะ ~5m)
 * @note ระยะทาง = echo_µs / 58.0 (cm) หรือ / 148.0 (inch)
 * @note การส่ง TRIG pulse: LOW 2µs → HIGH 10µs → LOW
 */
static uint32_t HCSR04_GetEchoDuration(HCSR04_Instance* sensor) {
    /* ส่ง TRIG pulse: LOW 2µs → HIGH 10µs → LOW */
    digitalWrite(sensor->pin_trig, LOW);
    Delay_Us(2);
    digitalWrite(sensor->pin_trig, HIGH);
    Delay_Us(10);
    digitalWrite(sensor->pin_trig, LOW);

    /* รอ ECHO ขึ้น HIGH (timeout 30ms) */
    uint32_t t_start = Get_CurrentUs();
    while (digitalRead(sensor->pin_echo) == LOW) {
        if ((Get_CurrentUs() - t_start) >= HCSR04_ECHO_TIMEOUT_US) {
            return 0;  /* Timeout รอ ECHO start */
        }
    }

    /* วัดระยะเวลาที่ ECHO อยู่ HIGH */
    uint32_t echo_start = Get_CurrentUs();
    while (digitalRead(sensor->pin_echo) == HIGH) {
        if ((Get_CurrentUs() - echo_start) >= HCSR04_ECHO_TIMEOUT_US) {
            return 0;  /* Timeout รอ ECHO end (วัตถุไกลเกินไป) */
        }
    }

    return Get_CurrentUs() - echo_start;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น HC-SR04 sensor
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance (NULL → return ทันที)
 * @param pin_trig - GPIO pin ที่ต่อกับ TRIG (Output)
 * @param pin_echo - GPIO pin ที่ต่อกับ ECHO (Input)
 *
 * @note ตั้งค่า pin_trig เป็น OUTPUT, pin_echo เป็น INPUT
 * @note ตั้ง TRIG pin เป็น LOW เริ่มต้น
 * @note ตั้งค่า initialized flag และ last_distance_cm = HCSR04_ERROR_VALUE
 */
void HCSR04_Init(HCSR04_Instance* sensor, uint8_t pin_trig, uint8_t pin_echo) {
    if (sensor == NULL) return;

    sensor->pin_trig          = pin_trig;
    sensor->pin_echo          = pin_echo;
    sensor->last_distance_cm  = HCSR04_ERROR_VALUE;
    sensor->last_measure_time = 0;
    sensor->initialized       = 1;

    /* ตั้งค่า GPIO */
    pinMode(pin_trig, PIN_MODE_OUTPUT);
    pinMode(pin_echo, PIN_MODE_INPUT);
    digitalWrite(pin_trig, LOW);
}

/**
 * @brief วัดระยะทางในหน่วยเซนติเมตร
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance (NULL หรือ !initialized → return HCSR04_ERROR_VALUE)
 * @return float - ระยะทาง (cm) ในช่วง 2-400 cm, หรือ HCSR04_ERROR_VALUE (-1.0) ถ้า timeout/นอกช่วง
 *
 * @note ใช้ HCSR04_GetEchoDuration() เพื่อวัด echo time
 * @note แปลง echo_µs → cm: distance = echo_us / 58.0
 * @note ตรวจสอบ range: 2.0 ≤ distance ≤ 400.0
 * @note อัปเดต last_distance_cm และ last_measure_time
 * @note ควรหน่วง ≥60ms ระหว่างการวัดแต่ละครั้ง
 */
float HCSR04_MeasureCm(HCSR04_Instance* sensor) {
    if (sensor == NULL || !sensor->initialized) return HCSR04_ERROR_VALUE;

    uint32_t echo_us = HCSR04_GetEchoDuration(sensor);

    if (echo_us == 0) {
        sensor->last_distance_cm = HCSR04_ERROR_VALUE;
        return HCSR04_ERROR_VALUE;
    }

    /* แปลงเวลา → ระยะทาง: distance = echo_µs / 58.0 */
    float distance = (float)echo_us / 58.0f;

    /* ตรวจสอบว่าอยู่ในช่วงวัดที่ถูกต้อง */
    if (distance < HCSR04_MIN_DISTANCE_CM || distance > HCSR04_MAX_DISTANCE_CM) {
        sensor->last_distance_cm = HCSR04_ERROR_VALUE;
        return HCSR04_ERROR_VALUE;
    }

    sensor->last_distance_cm  = distance;
    sensor->last_measure_time = Get_CurrentMs();
    return distance;
}

/**
 * @brief วัดระยะทางในหน่วยนิ้ว
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance (NULL หรือ !initialized → return HCSR04_ERROR_VALUE)
 * @return float - ระยะทาง (inch) หรือ HCSR04_ERROR_VALUE (-1.0) ถ้า timeout/นอกช่วง
 *
 * @note แปลง echo_µs → inch: distance = echo_us / 148.0
 * @note เก็บ last_distance_cm เป็น cm (inch × 2.54) เพื่อ consistency กับ MeasureCm
 * @note ตรวจสอบ range โดยแปลงเป็น cm ก่อน
 */
float HCSR04_MeasureInch(HCSR04_Instance* sensor) {
    if (sensor == NULL || !sensor->initialized) return HCSR04_ERROR_VALUE;

    uint32_t echo_us = HCSR04_GetEchoDuration(sensor);

    if (echo_us == 0) {
        sensor->last_distance_cm = HCSR04_ERROR_VALUE;
        return HCSR04_ERROR_VALUE;
    }

    /* แปลงเวลา → ระยะทาง: distance = echo_µs / 148.0 */
    float distance_inch = (float)echo_us / 148.0f;
    float distance_cm   = distance_inch * 2.54f;

    if (distance_cm < HCSR04_MIN_DISTANCE_CM || distance_cm > HCSR04_MAX_DISTANCE_CM) {
        sensor->last_distance_cm = HCSR04_ERROR_VALUE;
        return HCSR04_ERROR_VALUE;
    }

    sensor->last_distance_cm  = distance_cm;
    sensor->last_measure_time = Get_CurrentMs();
    return distance_inch;
}

/**
 * @brief วัดระยะทางหลายครั้งแล้วหาค่าเฉลี่ย (ลด noise)
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance (NULL หรือ !initialized → return HCSR04_ERROR_VALUE)
 * @param samples - จำนวนครั้งที่วัด (1-10, clamp อัตโนมัติ)
 * @return float - ค่าเฉลี่ย (cm) หรือ HCSR04_ERROR_VALUE ถ้า error ทุกครั้ง
 *
 * @note ใช้ HCSR04_MeasureCm() ภายใน loop
 * @note หน่วง 60ms ระหว่างการวัดแต่ละครั้ง (HC-SR04 requirement)
 * @note clamp samples: < 1 → 1, > 10 → 10
 * @note อัปเดต last_distance_cm เป็นค่าเฉลี่ย
 */
float HCSR04_MeasureAvgCm(HCSR04_Instance* sensor, uint8_t samples) {
    if (sensor == NULL || !sensor->initialized) return HCSR04_ERROR_VALUE;
    if (samples < 1)  samples = 1;
    if (samples > 10) samples = 10;

    float    sum   = 0.0f;
    uint8_t  valid = 0;

    for (uint8_t i = 0; i < samples; i++) {
        float d = HCSR04_MeasureCm(sensor);
        if (d != HCSR04_ERROR_VALUE) {
            sum += d;
            valid++;
        }
        /* รอ 60ms ก่อนวัดครั้งต่อไป (HC-SR04 requirement) */
        if (i < (uint8_t)(samples - 1)) {
            Delay_Ms(HCSR04_MIN_INTERVAL_MS);
        }
    }

    if (valid == 0) return HCSR04_ERROR_VALUE;

    float avg = sum / (float)valid;
    sensor->last_distance_cm = avg;
    return avg;
}

/**
 * @brief ดึงระยะทางล่าสุด (ไม่วัดใหม่)
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance (NULL → return HCSR04_ERROR_VALUE)
 * @return float - ระยะทางล่าสุด (cm) หรือ HCSR04_ERROR_VALUE (-1.0) ถ้ายังไม่เคยวัด
 *
 * @note ไม่มีการส่ง TRIG pulse — อ่านค่าจาก cache (last_distance_cm)
 * @note ใช้หลังจาก MeasureCm หรือ MeasureAvgCm เพื่ออ่านค่าโดยไม่ต้องวัดซ้ำ
 */
float HCSR04_GetLastDistance(HCSR04_Instance* sensor) {
    if (sensor == NULL) return HCSR04_ERROR_VALUE;
    return sensor->last_distance_cm;
}

/**
 * @brief ตรวจสอบว่าวัตถุอยู่ใกล้กว่าระยะที่กำหนดหรือไม่
 *
 * @param sensor - ตัวชี้ไปยัง HCSR04_Instance
 * @param threshold - ระยะ threshold (cm)
 * @return bool - true ถ้าวัตถุอยู่ใกล้กว่า threshold, false ถ้าไกลกว่าหรือ error
 *
 * @note เรียก HCSR04_MeasureCm() ภายใน — จะทำการวัดใหม่ทุกครั้ง
 * @note ถ้า MeasureCm คืน error → return false (safe default)
 */
bool HCSR04_IsObjectNear(HCSR04_Instance* sensor, float threshold) {
    float distance = HCSR04_MeasureCm(sensor);
    if (distance == HCSR04_ERROR_VALUE) return false;
    return distance < threshold;
}
