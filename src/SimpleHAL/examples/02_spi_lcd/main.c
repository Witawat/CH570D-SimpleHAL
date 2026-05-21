/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 2 — ส่งข้อมูลผ่าน SPI
 *
 * สิ่งที่เรียนรู้:
 * - การ init SPI ในโหมด master
 * - การเลือกอุปกรณ์ด้วย Chip Select (CS)
 * - การส่งและรับข้อมูลพร้อมกัน (full-duplex)
 *
 * การต่อวงจร: ต่อ SPI กับ LCD หรืออุปกรณ์ SPI อื่นๆ
 * - SCK  = PA13
 * - MOSI = PA15
 * - MISO = PA14
 * - CS   = PA15 (software control)
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /*
     * tx_data: ข้อมูลที่จะส่งผ่าน SPI
     * ในการสื่อสาร SPI ทุกครั้ง MCU จะส่งข้อมูลไปพร้อมกับรับข้อมูลกลับ
     * ค่า 0x90 มักเป็นคำสั่ง read register ของอุปกรณ์ SPI
     * ตามด้วย 0x00 3 ไบต์สำหรับ dummy byte (เพื่อรับข้อมูลกลับ)
     */
    uint8_t tx_data[] = {0x90, 0x00, 0x00, 0x00};
    uint8_t rx_data[4];  // บัฟเฟอร์รับข้อมูลที่ได้จาก SPI

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น SPI ในโหมด master
     *
     * hal_spi_init(clock_hz, mode, bit_order)
     * - clock_hz: 8000000 (8 MHz) — ความเร็วสัญญาณนาฬิกา SPI
     *   ความเร็วสูงสุดขึ้นกับ sysclk เช่น ที่ 100MHz ได้สูงสุด ~50MHz
     * - mode: HAL_SPI_MODE0
     *   - CPOL=0: SCK idle อยู่ที่ LOW
     *   - CPHA=0: ตัวอย่างข้อมูลบนขาขึ้น (rising edge)
     * - order: HAL_SPI_MSB_FIRST — ส่งบิตที่มีนัยสำคัญสูงสุดก่อน
     *   (เป็นค่ามาตรฐานของอุปกรณ์ SPI ส่วนใหญ่)
     *
     * CH572 ใช้ขา SPI แบบฮาร์ดแวร์:
     * - SCK  = PA13
     * - MOSI = PA12
     * - MISO = PA14
     * - CS   = PA15 (software control ผ่าน hal_spi_chip_select())
     */
    hal_spi_handle_t spi = hal_spi_init(8000000, HAL_SPI_MODE0, HAL_SPI_MSB_FIRST);

    /*
     * main loop: อ่านข้อมูลจากอุปกรณ์ SPI ทุก 1 วินาที
     */
    while (1) {

        /*
         * Chip Select (CS) — ใช้เลือกอุปกรณ์ SPI ที่ต้องการสื่อสาร
         * อุปกรณ์ SPI ส่วนใหญ่ใช้ active LOW: 0 = เลือก, 1 = ไม่เลือก
         * ต้องดึง CS ลงก่อนเริ่ม transmission และดึงขึ้นเมื่อเสร็จ
         */
        hal_spi_chip_select(spi, 0);   // เลือกอุปกรณ์ (CS = LOW)

        /*
         * hal_spi_transfer(handle, tx, rx, len) — ส่งและรับข้อมูลพร้อมกัน
         * - tx: ข้อมูลที่จะส่ง (4 ไบต์ = 0x90 + 3 dummy)
         * - rx: บัฟเฟอร์สำหรับรับข้อมูลกลับ (4 ไบต์)
         * - len: จำนวนไบต์ (4)
         *
         * SPI เป็น full-duplex: ทุกครั้งที่ส่ง 1 ไบต์ ก็จะรับ 1 ไบต์พร้อมกัน
         * ถ้าไม่ต้องการส่ง ให้ส่ง 0xFF (dummy), ถ้าไม่ต้องการรับ ให้ rx=NULL
         *
         * ฟังก์ชันนี้เป็นแบบ blocking — รอจนครบ 4 ไบต์ก่อนคืน
         */
        hal_spi_transfer(spi, tx_data, rx_data, 4);

        hal_spi_chip_select(spi, 1);   // ยกเลิกการเลือกอุปกรณ์ (CS = HIGH)

        /*
         * หน่วงเวลา 1 วินาที ก่อนอ่านครั้งถัดไป
         * ปกติแล้ว rx_data[1..3] จะมีค่าที่อ่านจากอุปกรณ์
         */
        hal_delay_ms(1000);
    }
}
