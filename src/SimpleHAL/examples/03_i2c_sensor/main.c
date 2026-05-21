/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 3 — อ่าน/เขียน I2C sensor
 *
 * สิ่งที่เรียนรู้:
 * - การ init I2C ในโหมด master
 * - การอ่านค่าจาก sensor ผ่าน I2C protocol
 * - การแปลงค่าดิบ (raw) เป็นค่าจริง (องศา Celsius)
 * - RESTART (repeated start) แทน STOP+START เพื่อความเข้ากันได้ดีขึ้น
 *
 * การต่อวงจร: ต่อ LM75 (I2C temperature sensor) กับ I2C bus
 * - SCL = PA9
 * - SDA = PA8
 * - VCC = 3.3V, GND = GND
 *******************************************************************************/

#include "simple_hal.h"

/*
 * ค่าคงที่สำหรับ LM75 temperature sensor
 *
 * LM75_ADDR = 0x48 — ที่อยู่ I2C 7-bit (ถ้า A0/A1/A2 ต่อ GND ทั้งหมด)
 * หมายเหตุ: I2C address ต้องเป็น 7-bit (ไม่รวม bit ทิศทาง Read/Write)
 *
 * LM75_TEMP = 0x00 — register address สำหรับอ่านอุณหภูมิ
 * LM75 มี register ดังนี้:
 *   0x00 = Temperature register (อ่านอย่างเดียว, 2 ไบต์)
 *   0x01 = Configuration register (อ่าน/เขียน)
 *   0x02 = THYST register (อ่าน/เขียน)
 *   0x03 = TOS register (อ่าน/เขียน)
 */
#define LM75_ADDR   0x48
#define LM75_TEMP   0x00

int main()
{
    /*
     * temp_raw[2] — เก็บค่าดิบ 2 ไบต์ที่อ่านจาก LM75
     * temp — ตัวแปรเก็บค่าอุณหภูมิที่คำนวณแล้ว (หน่วย: 1/8 องศา)
     */
    uint8_t temp_raw[2];
    int16_t temp;

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น I2C ในโหมด master
     *
     * hal_i2c_init(clock_hz, role, own_addr)
     * - clock_hz: 400000 (400 kHz) — ความเร็ว I2C แบบ fast mode
     *   ค่ามาตรฐาน: 100000 (standard), 400000 (fast)
     * - role: HAL_I2C_MASTER — โหมด master (เป็นฝ่ายเริ่มต้นการสื่อสาร)
     * - own_addr: 0 — ที่อยู่ของตัวเอง (master ไม่จำเป็นต้องมี address)
     *
     * CH572 ใช้ขา I2C แบบฮาร์ดแวร์:
     * - SCL = PA9
     * - SDA = PA8
     */
    hal_i2c_handle_t i2c = hal_i2c_init(400000, HAL_I2C_MASTER, 0);

    /*
     * main loop: อ่านอุณหภูมิทุก 1 วินาที
     */
    while (1) {

        /*
         * hal_i2c_read(handle, dev_addr, reg, buffer, len)
         *
         * การทำงานภายใน (ใช้ RESTART — repeated start):
         *   1. START → ส่ง dev_addr(เขียน) → ส่ง reg → RESTART → ส่ง dev_addr(อ่าน)
         *      → อ่าน 2 ไบต์ → STOP
         *
         * ข้อดีของ RESTART: ไม่มีการปล่อย BUS ระหว่างคำสั่งเขียนและอ่าน
         * ทำให้อุปกรณ์บางประเภท (เช่น เซนเซอร์บางรุ่น) ทำงานได้ถูกต้อง
         *
         * พารามิเตอร์:
         * - dev_addr: LM75_ADDR (0x48) — ที่อยู่ LM75
         * - reg: LM75_TEMP (0x00) — register temperature
         * - temp_raw: บัฟเฟอร์สำหรับเก็บค่าที่อ่าน
         * - len: 2 — อ่าน 2 ไบต์ (LM75 ส่งอุณหภูมิเป็น 16-bit)
         */
        hal_i2c_read(i2c, LM75_ADDR, LM75_TEMP, temp_raw, 2);

        /*
         * แปลงค่าดิบเป็นอุณหภูมิ (องศา Celsius)
         *
         * LM75 ส่งข้อมูล 2 ไบต์: [D15 D14 D13 D12 D11 D10 D9 D8] [D7 X X X X X X X]
         * อุณหภูมิจริง = ค่า 11-bit (D15..D5) * 0.125 องศา
         * D15 เป็น sign bit (0=บวก, 1=ลบ)
         *
         * ขั้นตอนการแปลง:
         *   1. (temp_raw[0] << 8) | temp_raw[1] — รวม 2 ไบต์เป็น 16-bit
         *   2. >> 5 — เลื่อนขวา 5 บิตเพื่อให้เหลือ 11-bit (D15..D5)
         *      ทำให้ได้ค่าในหน่วย 1/8 องศา (เช่น 20 = 2.5°C)
         *
         * ตัวอย่าง: ถ้า temp_raw = {0x19, 0xA0}
         *   = 0x19A0 >> 5 = 0xCD = 205 → 205/8 = 25.625°C
         */
        temp = (int16_t)((temp_raw[0] << 8) | temp_raw[1]) >> 5;

        /*
         * รอ 1 วินาทีก่อนอ่านครั้งถัดไป
         * ในโปรแกรมจริงควรแสดงค่าผ่าน UART หรือ debug output
         * โดย temp/8 = อุณหภูมิในหน่วยองศา (เช่น 200/8 = 25°C)
         */
        hal_delay_ms(1000);
    }
}
