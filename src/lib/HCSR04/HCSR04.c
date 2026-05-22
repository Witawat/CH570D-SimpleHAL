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
 * @param sensor  ตัวชี้ไปยัง HCSR04_Instance
 * @return ระยะเวลาของ ECHO pulse (µs) หรือ 0 ถ้า timeout
 *
 * หลักการทำงาน:
 *   1. ส่ง TRIG pulse 10µs
 *   2. รอ ECHO ขึ้น HIGH (sensor กำลังส่ง ultrasonic)
 *   3. วัดระยะเวลาที่ ECHO อยู่ HIGH (คลื่นเสียงเดินทาง round-trip)
 *   4. ระยะทาง = echo_µs / 58.0 (cm)  หรือ / 148.0 (inch)
 *      - ความเร็วเสียง ~340 m/s
 *      - round-trip = ×2 → 340 × 2 / 10000 = 58.82 ≈ 58
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
 * @brief วัดระยะทางหลายครั้งแล้วหาค่าเฉลี่ย
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
 */
float HCSR04_GetLastDistance(HCSR04_Instance* sensor) {
    if (sensor == NULL) return HCSR04_ERROR_VALUE;
    return sensor->last_distance_cm;
}

/**
 * @brief ตรวจสอบว่าวัตถุอยู่ใกล้กว่า threshold หรือไม่
 */
bool HCSR04_IsObjectNear(HCSR04_Instance* sensor, float threshold) {
    float distance = HCSR04_MeasureCm(sensor);
    if (distance == HCSR04_ERROR_VALUE) return false;
    return distance < threshold;
}
