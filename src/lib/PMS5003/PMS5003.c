/**
 * @file PMS5003.c
 * @brief PMS5003 / PMS7003 PM2.5 Sensor Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "PMS5003.h"

/* ========== Protocol Constants ========== */

#define PMS5003_HEAD_1              0x42
#define PMS5003_HEAD_2              0x4D
#define PMS5003_FRAME_DATA_LEN      28
#define PMS5003_CMD_LEN             7

/* ========== Private Helpers ========== */

/**
 * @brief อ่าน big-endian 16-bit จาก buffer
 *
 * @param buf - pointer buffer
 * @param idx - index เริ่มต้น
 *
 * @return ค่า 16-bit (big-endian)
 *
 * @note ใช้สำหรับ PMS5003 frame protocol (big-endian)
 */
static uint16_t PMS5003_ReadBE16(const uint8_t* buf, uint8_t idx)
{
    return (uint16_t)(((uint16_t)buf[idx] << 8) | buf[idx + 1]);
}

/**
 * @brief ตรวจสอบ checksum ของ frame (sum bytes 0-29 == frame[30..31])
 *
 * @param frame - pointer 32-byte frame
 *
 * @return 1 = checksum OK, 0 = mismatch
 *
 * @note checksum = sum of bytes 0..29, stored big-endian at byte 30
 */
static uint8_t PMS5003_ValidateChecksum(const uint8_t* frame)
{
    uint16_t sum = 0;
    uint8_t i;

    for(i = 0; i < 30; i++) {
        sum = (uint16_t)(sum + frame[i]);
    }

    return (sum == PMS5003_ReadBE16(frame, 30)) ? 1 : 0;
}

/**
 * @brief แปลง raw frame 40 byte → PMS5003_Data struct
 *
 * @param out - [out] parsed data
 * @param frame - pointer 32-byte raw frame
 *
 * @note frame struct: header(2)+frame_len(2)+pm_std(6)+pm_atm(6)+cnt(12)+reserved(2)+checksum(2)
 */
static void PMS5003_ParseFrame(PMS5003_Data* out, const uint8_t* frame)
{
    out->frame_len  = PMS5003_ReadBE16(frame, 2);

    out->pm1_0_std  = PMS5003_ReadBE16(frame, 4);
    out->pm2_5_std  = PMS5003_ReadBE16(frame, 6);
    out->pm10_std   = PMS5003_ReadBE16(frame, 8);

    out->pm1_0_atm  = PMS5003_ReadBE16(frame, 10);
    out->pm2_5_atm  = PMS5003_ReadBE16(frame, 12);
    out->pm10_atm   = PMS5003_ReadBE16(frame, 14);

    out->cnt_0_3    = PMS5003_ReadBE16(frame, 16);
    out->cnt_0_5    = PMS5003_ReadBE16(frame, 18);
    out->cnt_1_0    = PMS5003_ReadBE16(frame, 20);
    out->cnt_2_5    = PMS5003_ReadBE16(frame, 22);
    out->cnt_5_0    = PMS5003_ReadBE16(frame, 24);
    out->cnt_10     = PMS5003_ReadBE16(frame, 26);

    out->reserved   = PMS5003_ReadBE16(frame, 28);
    out->checksum   = PMS5003_ReadBE16(frame, 30);
}

/**
 * @brief ส่ง command ไปยัง PMS5003 (7 bytes พร้อม checksum)
 *
 * @param sensor - instance
 * @param cmd - command byte
 * @param data_h - data byte high
 * @param data_l - data byte low
 *
 * @return PMS5003_OK หรือ PMS5003_ERROR_NOT_INIT
 *
 * @note frame = [0x42][0x4D][cmd][data_h][data_l][crc_h][crc_l]
 *       checksum = sum of bytes 0-4
 */
static PMS5003_Status PMS5003_SendCommand(PMS5003_Instance* sensor, uint8_t cmd, uint8_t data_h, uint8_t data_l)
{
    uint8_t frame[PMS5003_CMD_LEN];
    uint16_t sum;

    if(sensor == 0 || sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    frame[0] = PMS5003_HEAD_1;
    frame[1] = PMS5003_HEAD_2;
    frame[2] = cmd;
    frame[3] = data_h;
    frame[4] = data_l;

    sum = (uint16_t)(frame[0] + frame[1] + frame[2] + frame[3] + frame[4]);
    frame[5] = (uint8_t)(sum >> 8);
    frame[6] = (uint8_t)(sum & 0xFF);

    USART_WriteByte(frame[0]);
    USART_WriteByte(frame[1]);
    USART_WriteByte(frame[2]);
    USART_WriteByte(frame[3]);
    USART_WriteByte(frame[4]);
    USART_WriteByte(frame[5]);
    USART_WriteByte(frame[6]);

    return PMS5003_OK;
}

/* ========== Public Functions ========== */

/**
 * @brief เริ่มต้น PMS5003 sensor (9600 baud)
 *
 * @param sensor - instance (NULL → PMS5003_ERROR_PARAM)
 * @param pin_config - USART pin mapping
 *
 * @return PMS5003_OK หรือ PMS5003_ERROR_PARAM
 *
 * @note init USART ที่ 9600 baud
 *       flush UART buffer ทิ้ง
 *       ตั้งค่า frame_ready = 0
 */
PMS5003_Status PMS5003_Init(PMS5003_Instance* sensor, USART_PinConfig pin_config)
{
    if(sensor == 0) {
        return PMS5003_ERROR_PARAM;
    }

    sensor->pin_config = pin_config;
    sensor->rx_idx = 0;
    sensor->frame_ready = 0;
    sensor->initialized = 0;

    USART_SimpleInit(BAUD_9600, pin_config);
    USART_Flush();

    sensor->initialized = 1;
    return PMS5003_OK;
}

/**
 * @brief อ่าน UART และสะสม frame (non-blocking)
 *
 * @param sensor - instance (NULL → PMS5003_ERROR_PARAM)
 *
 * @return PMS5003_OK หรือ error
 *
 * @note ควรเรียกใน loop บ่อยๆ
 *       รอ header 0x42 0x4D
 *       ตรวจสอบ checksum อัตโนมัติ
 *       ถ้า frame สมบูรณ์ → ตั้ง frame_ready=1
 */
PMS5003_Status PMS5003_Update(PMS5003_Instance* sensor)
{
    uint8_t b;

    if(sensor == 0) {
        return PMS5003_ERROR_PARAM;
    }

    if(sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    while(USART_Available()) {
        b = USART_Read();

        if(sensor->rx_idx == 0) {
            if(b == PMS5003_HEAD_1) {
                sensor->rx_buf[0] = b;
                sensor->rx_idx = 1;
            }
            continue;
        }

        if(sensor->rx_idx == 1) {
            if(b == PMS5003_HEAD_2) {
                sensor->rx_buf[1] = b;
                sensor->rx_idx = 2;
            } else if(b == PMS5003_HEAD_1) {
                sensor->rx_buf[0] = b;
                sensor->rx_idx = 1;
            } else {
                sensor->rx_idx = 0;
            }
            continue;
        }

        sensor->rx_buf[sensor->rx_idx++] = b;

        if(sensor->rx_idx >= PMS5003_FRAME_LEN) {
            sensor->rx_idx = 0;

            if(PMS5003_ReadBE16(sensor->rx_buf, 2) != PMS5003_FRAME_DATA_LEN) {
                continue;
            }

            if(!PMS5003_ValidateChecksum(sensor->rx_buf)) {
                continue;
            }

            PMS5003_ParseFrame(&sensor->last_data, sensor->rx_buf);
            sensor->frame_ready = 1;
            return PMS5003_OK;
        }
    }

    return PMS5003_OK;
}

/**
 * @brief อ่านค่าจาก sensor (blocking, รอจนได้ frame หรือ timeout)
 *
 * @param sensor - instance (NULL หรือ out==NULL → PMS5003_ERROR_PARAM)
 * @param out - [out] ข้อมูลที่อ่านได้
 * @param timeout_ms - timeout (ms), 0 = ใช้ค่า default (2000ms)
 *
 * @return PMS5003_OK หรือ PMS5003_ERROR_TIMEOUT
 *
 * @note flush buffer + reset rx_idx ก่อนเริ่ม
 *       เรียก PMS5003_Update() ซ้ำใน loop
 *       sensor ส่ง frame ทุก ~1s ใน active mode
 */
PMS5003_Status PMS5003_Read(PMS5003_Instance* sensor, PMS5003_Data* out, uint32_t timeout_ms)
{
    uint32_t t0;
    PMS5003_Status st;

    if(sensor == 0 || out == 0) {
        return PMS5003_ERROR_PARAM;
    }

    if(sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    if(timeout_ms == 0) {
        timeout_ms = PMS5003_DEFAULT_TIMEOUT_MS;
    }

    sensor->frame_ready = 0;
    sensor->rx_idx = 0;
    USART_Flush();

    t0 = Get_CurrentMs();
    while((Get_CurrentMs() - t0) < timeout_ms) {
        st = PMS5003_Update(sensor);
        if(st != PMS5003_OK) {
            return st;
        }

        if(sensor->frame_ready) {
            *out = sensor->last_data;
            sensor->frame_ready = 0;
            return PMS5003_OK;
        }
    }

    return PMS5003_ERROR_TIMEOUT;
}

/**
 * @brief ตรวจสอบว่ามี frame พร้อมใช้งานหรือไม่
 *
 * @param sensor - instance
 *
 * @return 1 = มี data พร้อม, 0 = ยังไม่มี
 *
 * @note ใช้ PMS5003_GetData() เพื่ออ่านค่าหลังจากนี้
 */
uint8_t PMS5003_IsDataReady(PMS5003_Instance* sensor)
{
    if(sensor == 0 || sensor->initialized == 0) {
        return 0;
    }

    return sensor->frame_ready;
}

/**
 * @brief รีเซ็ต flag frame_ready
 *
 * @param sensor - instance
 *
 * @note ควรเรียกหลังจากอ่านข้อมูลแล้ว
 */
void PMS5003_ClearReady(PMS5003_Instance* sensor)
{
    if(sensor == 0 || sensor->initialized == 0) {
        return;
    }

    sensor->frame_ready = 0;
}

/**
 * @brief อ่านค่าล่าสุด (โดยไม่รอ frame ใหม่)
 *
 * @param sensor - instance (NULL หรือ out==NULL → PMS5003_ERROR_PARAM)
 * @param out - [out] ข้อมูลล่าสุด
 *
 * @return PMS5003_OK, PMS5003_ERROR_NOT_INIT, หรือ PMS5003_ERROR_TIMEOUT
 *
 * @note ต้องมี frame_ready=1 ก่อน (เรียก PMS5003_Update() จนกว่าจะพร้อม)
 */
PMS5003_Status PMS5003_GetData(PMS5003_Instance* sensor, PMS5003_Data* out)
{
    if(sensor == 0 || out == 0) {
        return PMS5003_ERROR_PARAM;
    }

    if(sensor->initialized == 0) {
        return PMS5003_ERROR_NOT_INIT;
    }

    if(sensor->frame_ready == 0) {
        return PMS5003_ERROR_TIMEOUT;
    }

    *out = sensor->last_data;
    return PMS5003_OK;
}

/**
 * @brief สั่ง PMS5003 เข้า sleep mode
 *
 * @param sensor - instance
 *
 * @return PMS5003_OK หรือ error
 *
 * @note ส่ง command 0xE4 data_h=0x00 data_l=0x00
 *       ใช้กระแส <1mA เมื่อ sleep
 *       ต้องใช้ PMS5003_Wakeup() เพื่อปลุก
 */
PMS5003_Status PMS5003_Sleep(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE4, 0x00, 0x00);
}

/**
 * @brief ปลุก PMS5003 จาก sleep
 *
 * @param sensor - instance
 *
 * @return PMS5003_OK หรือ error
 *
 * @note ส่ง command 0xE4 data_h=0x00 data_l=0x01
 *       sensor เริ่มส่งข้อมูลอีกครั้ง ~1s
 */
PMS5003_Status PMS5003_Wakeup(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE4, 0x00, 0x01);
}

/**
 * @brief ตั้งค่าเป็น passive mode
 *
 * @param sensor - instance
 *
 * @return PMS5003_OK หรือ error
 *
 * @note ใน passive mode sensor จะไม่ส่งข้อมูลเอง
 *       ต้องเรียก PMS5003_Request() เพื่อขอข้อมูล
 */
PMS5003_Status PMS5003_SetPassiveMode(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE1, 0x00, 0x00);
}

/**
 * @brief ตั้งค่าเป็น active mode (default)
 *
 * @param sensor - instance
 *
 * @return PMS5003_OK หรือ error
 *
 * @note ใน active mode sensor ส่งข้อมูลทุก ~1s อัตโนมัติ
 */
PMS5003_Status PMS5003_SetActiveMode(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE1, 0x00, 0x01);
}

/**
 * @brief ขอข้อมูล 1 frame (เฉพาะ passive mode)
 *
 * @param sensor - instance
 *
 * @return PMS5003_OK หรือ error
 *
 * @note ต้องอยู่ใน passive mode ก่อน
 *       sensor จะตอบกลับ 1 frame แล้วหยุด
 */
PMS5003_Status PMS5003_Request(PMS5003_Instance* sensor)
{
    return PMS5003_SendCommand(sensor, 0xE2, 0x00, 0x00);
}
