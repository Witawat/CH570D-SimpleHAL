/**
 * @file DHT.c
 * @brief DHT11/DHT22 Temperature & Humidity Sensor Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "DHT.h"

/* ========== Private Function Prototypes ========== */

/**
 * @brief รอให้ pin เปลี่ยนเป็นสถานะที่ต้องการ พร้อม timeout
 *
 * @param pin        GPIO pin ที่ต้องการตรวจสอบ
 * @param state      สถานะที่รอ (HIGH หรือ LOW)
 * @param timeout_us จำนวนสูงสุด (iterations ~1µs ต่อ iteration)
 * @return true ถ้า pin เปลี่ยนสถานะสำเร็จก่อน timeout
 */
static bool DHT_WaitForState(uint8_t pin, uint8_t state, uint16_t timeout_us);

/* ========== Private Functions ========== */

static bool DHT_WaitForState(uint8_t pin, uint8_t state, uint16_t timeout_us) {
    for (uint16_t i = 0; i < timeout_us; i++) {
        if (digitalRead(pin) == state) return true;
        Delay_Us(1);
    }
    return false;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น DHT sensor
 */
void DHT_Init(DHT_Instance* dht, uint8_t pin, DHT_Type type) {
    if (dht == NULL) return;

    dht->pin             = pin;
    dht->type            = type;
    dht->temperature     = 0.0f;
    dht->humidity        = 0.0f;
    dht->last_status     = DHT_OK;
    dht->last_read_time  = 0;
    dht->initialized     = 1;

    /* ตั้งค่า pin เป็น input + pull-up (idle state ของ DHT bus) */
    pinMode(pin, PIN_MODE_INPUT_PULLUP);
}

/**
 * @brief อ่านค่าอุณหภูมิและความชื้นจาก DHT sensor
 *
 * DHT Protocol (40-bit):
 *   [8 bit humidity int][8 bit humidity dec][8 bit temp int][8 bit temp dec][8 bit checksum]
 *
 * Timeline:
 *   Host: LOW ≥18ms (DHT11) / ≥1ms (DHT22) → release → HIGH 20-40µs
 *   DHT:  response LOW ~80µs → HIGH ~80µs
 *   Data: LOW ~50µs → HIGH 26µs(bit=0) / 70µs(bit=1)
 */
DHT_Status DHT_Read(DHT_Instance* dht) {
    if (dht == NULL || !dht->initialized) {
        return DHT_ERROR_NOT_INIT;
    }

    /* ตรวจสอบว่าถึงเวลาอ่านค่าหรือยัง */
    uint32_t now = Get_CurrentMs();
    if (dht->last_read_time != 0 &&
        (now - dht->last_read_time) < DHT_MIN_INTERVAL_MS) {
        dht->last_status = DHT_ERROR_NOT_READY;
        return DHT_ERROR_NOT_READY;
    }

    uint8_t data[5] = {0, 0, 0, 0, 0};

    /* ===== ขั้นตอนที่ 1: ส่ง Start Signal ===== */
    pinMode(dht->pin, PIN_MODE_OUTPUT);
    digitalWrite(dht->pin, LOW);

    if (dht->type == DHT_TYPE_DHT11) {
        Delay_Ms(20);   /* DHT11 ต้องการ ≥18ms */
    } else {
        Delay_Ms(2);    /* DHT22 ต้องการ ≥1ms */
    }

    /* ปล่อย bus กลับเป็น HIGH ผ่าน pull-up resistor */
    pinMode(dht->pin, PIN_MODE_INPUT_PULLUP);
    Delay_Us(40);   /* รอให้ bus stable ก่อน sensor ตอบสนอง */

    /* ===== ขั้นตอนที่ 2: ปิด Interrupt เพื่อ timing แม่นยำ ===== */
    __disable_irq();

    /* รอ DHT Response: ดึง bus LOW (~80µs) */
    if (!DHT_WaitForState(dht->pin, LOW, DHT_TIMEOUT_US)) {
        __enable_irq();
        dht->last_status = DHT_ERROR_TIMEOUT;
        return DHT_ERROR_TIMEOUT;
    }

    /* รอ DHT Response: ดึง bus HIGH (~80µs) */
    if (!DHT_WaitForState(dht->pin, HIGH, DHT_TIMEOUT_US)) {
        __enable_irq();
        dht->last_status = DHT_ERROR_TIMEOUT;
        return DHT_ERROR_TIMEOUT;
    }

    /* รอ DHT เริ่มส่ง data bit แรก (HIGH → LOW) */
    if (!DHT_WaitForState(dht->pin, LOW, DHT_TIMEOUT_US)) {
        __enable_irq();
        dht->last_status = DHT_ERROR_TIMEOUT;
        return DHT_ERROR_TIMEOUT;
    }

    /* ===== ขั้นตอนที่ 3: อ่าน 40 bits ===== */
    for (int i = 0; i < 40; i++) {
        /* รอ LOW → HIGH (เริ่ม bit pulse) */
        if (!DHT_WaitForState(dht->pin, HIGH, 75)) {
            __enable_irq();
            dht->last_status = DHT_ERROR_TIMEOUT;
            return DHT_ERROR_TIMEOUT;
        }

        /* รอ 40µs: ถ้า pin ยัง HIGH = bit '1' (70µs), ถ้า LOW แล้ว = bit '0' (26µs) */
        Delay_Us(40);

        data[i / 8] <<= 1;
        if (digitalRead(dht->pin) == HIGH) {
            data[i / 8] |= 1;  /* bit = 1 */
        }

        /* รอ HIGH → LOW (จบ bit pulse) */
        if (!DHT_WaitForState(dht->pin, LOW, 80)) {
            __enable_irq();
            dht->last_status = DHT_ERROR_TIMEOUT;
            return DHT_ERROR_TIMEOUT;
        }
    }

    __enable_irq();

    /* ===== ขั้นตอนที่ 4: ตรวจสอบ Checksum ===== */
    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        dht->last_status = DHT_ERROR_CHECKSUM;
        return DHT_ERROR_CHECKSUM;
    }

    /* ===== ขั้นตอนที่ 5: แปลงข้อมูล ===== */
    if (dht->type == DHT_TYPE_DHT11) {
        /* DHT11: integer only (data[1] และ data[3] มักเป็น 0) */
        dht->humidity    = (float)data[0];
        dht->temperature = (float)data[2];
    } else {
        /* DHT22: 16-bit unsigned humidity, 16-bit signed temperature */
        uint16_t raw_humidity = ((uint16_t)data[0] << 8) | data[1];
        uint16_t raw_temp     = ((uint16_t)data[2] << 8) | data[3];

        dht->humidity = (float)raw_humidity * 0.1f;

        /* Bit 15 ของ temperature = sign bit (1 = อุณหภูมิติดลบ) */
        if (raw_temp & 0x8000) {
            dht->temperature = -((float)(raw_temp & 0x7FFF) * 0.1f);
        } else {
            dht->temperature = (float)raw_temp * 0.1f;
        }
    }

    dht->last_read_time = Get_CurrentMs();
    dht->last_status    = DHT_OK;
    return DHT_OK;
}

/**
 * @brief ดึงค่าอุณหภูมิล่าสุด
 */
float DHT_GetTemperature(DHT_Instance* dht) {
    if (dht == NULL) return 0.0f;
    return dht->temperature;
}

/**
 * @brief ดึงค่าความชื้นล่าสุด
 */
float DHT_GetHumidity(DHT_Instance* dht) {
    if (dht == NULL) return 0.0f;
    return dht->humidity;
}

/**
 * @brief ดึงสถานะการอ่านครั้งล่าสุด
 */
DHT_Status DHT_GetStatus(DHT_Instance* dht) {
    if (dht == NULL) return DHT_ERROR_NOT_INIT;
    return dht->last_status;
}

/**
 * @brief ตรวจสอบว่า sensor พร้อมอ่านค่าหรือไม่
 */
bool DHT_IsReady(DHT_Instance* dht) {
    if (dht == NULL || !dht->initialized) return false;
    if (dht->last_read_time == 0) return true;  /* ยังไม่เคยอ่านเลย */
    return (Get_CurrentMs() - dht->last_read_time) >= DHT_MIN_INTERVAL_MS;
}

/**
 * @brief แปลง DHT_Status เป็น string สำหรับ debug
 */
const char* DHT_StatusStr(DHT_Status status) {
    switch (status) {
        case DHT_OK:              return "OK";
        case DHT_ERROR_TIMEOUT:   return "TIMEOUT";
        case DHT_ERROR_CHECKSUM:  return "CHECKSUM_ERROR";
        case DHT_ERROR_NOT_READY: return "NOT_READY";
        case DHT_ERROR_NOT_INIT:  return "NOT_INITIALIZED";
        default:                  return "UNKNOWN";
    }
}
