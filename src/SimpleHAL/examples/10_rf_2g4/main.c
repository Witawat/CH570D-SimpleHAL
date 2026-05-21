/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 10 — ส่ง/รับข้อมูลผ่าน RF 2.4GHz
 *
 * สิ่งที่เรียนรู้:
 * - การ init RF core 2.4GHz
 * - การเลือก PHY mode (ความเร็ว/โปรโตคอล)
 * - การส่งข้อมูลแบบไร้สาย
 * - การรับข้อมูลแบบมี timeout
 * - การอ่านค่า RSSI (ความแรงสัญญาณ)
 *
 * RF PHY mode:
 * - HAL_RF_PHY_1M: 1Mbps — nRF24L01 compatible
 * - HAL_RF_PHY_2M: 2Mbps — Enhanced ShockBurst
 * - HAL_RF_PHY_2G4: 2Mbps — WCH 2.4GHz proprietary
 *
 * การใช้งานจริง: ต้องมีอุปกรณ์ 2 ตัว (ตัวส่ง + ตัวรับ)
 * ตั้งความถี่และ PHY mode ให้ตรงกัน
 *******************************************************************************/

#include "simple_hal.h"

int main()
{
    /*
     * tx_buf[]: ข้อความที่จะส่ง ("Hello RF!")
     * rx_buf[32]: บัฟเฟอร์สำหรับรับข้อมูล (สูงสุด 251 ไบต์)
     * rx_len: จำนวนไบต์ที่รับได้จริง
     * rssi: ความแรงสัญญาณ (dBm, ค่าติดลบ)
     */
    uint8_t tx_buf[] = "Hello RF!";
    uint8_t rx_buf[32];
    uint16_t rx_len;
    int8_t rssi;

    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น RF 2.4GHz
     *
     * hal_rf_init(frequency_khz, phy_mode, tx_power_dbm)
     * - frequency_khz: 2402000 (2.402 GHz) — ความถี่ในการสื่อสาร
     *   ช่วงความถี่: 2400000-2483000 kHz (2.4-2.4835 GHz)
     *   ควรเว้นระยะห่างจาก WiFi/BLE เพื่อลด interference
     * - phy: HAL_RF_PHY_2G4 — โหมด 2.4Gbps proprietary
     *   ใช้ internal protocol ของ WCH (เข้ากันได้กับ nRF24L01)
     * - tx_power_dbm: 0 — กำลังส่ง 0 dBm (~1 mW)
     *   ค่ามาตรฐาน: -25 ถึง +7 dBm
     *   ยิ่งสูง: ส่งได้ไกลขึ้น แต่กินไฟมากขึ้น
     *
     * การเลือก PHY mode ที่ถูกต้อง:
     * โปรดสังเกตว่าค่า HAL_RF_PHY_* จะถูกแปลงเป็นค่า register
     * ของ WCH โดยอัตโนมัติภายใน hal_rf_init()
     */
    hal_rf_handle_t rf = hal_rf_init(2402000, HAL_RF_PHY_2G4, 0);

    /*
     * main loop: ส่ง → รอรับ → รอ 1 วินาที
     *
     * โปรแกรมนี้ทำทั้งส่งและรับในเครื่องเดียว
     * ในทางปฏิบัติ ควรมี 2 เครื่อง: เครื่องนึงส่ง, เครื่องนึงรับ
     */
    while (1) {

        /*
         * hal_rf_send(handle, data, len) — ส่งข้อมูลไร้สาย
         * - data: tx_buf (ข้อความ "Hello RF!")
         * - len: sizeof(tx_buf) (รวม null-terminator)
         *
         * การส่งเป็นแบบ blocking: รอจนส่งเสร็จ
         * ข้อมูลจะถูกส่งเป็น packet ทางอากาศ
         *
         * ข้อจำกัด: สูงสุด 251 ไบต์ต่อ packet
         */
        hal_rf_send(rf, tx_buf, sizeof(tx_buf));

        /*
         * hal_rf_recv(handle, buffer, max_len, &out_len, &rssi, timeout_us)
         *
         * รับข้อมูลแบบ blocking (แต่มี timeout):
         * - rx_buf: บัฟเฟอร์เก็บข้อมูลที่รับ
         * - sizeof(rx_buf): ขนาดสูงสุด
         * - &rx_len: พอยน์เตอร์รับจำนวนไบต์ที่รับได้
         * - &rssi: พอยน์เตอร์รับค่า RSSI (ความแรงสัญญาณ)
         * - 1000000: timeout 1 วินาที (1,000,000 µs)
         *
         * ค่าที่คืน:
         * - HAL_OK: มีข้อมูล — ตรวจสอบ rx_len และ rssi
         * - HAL_TIMEOUT: ไม่มีข้อมูลภายในเวลาที่กำหนด
         *
         * RSSI (Received Signal Strength Indicator):
         * - ค่าติดลบ: ยิ่งใกล้ 0 ยิ่งแรง
         *   เช่น -30 dBm = แรงมาก, -80 dBm = อ่อน
         * - ถ้าไม่ต้องการ RSSI ส่ง NULL แทน &rssi
         */
        if (hal_rf_recv(rf, rx_buf, sizeof(rx_buf), &rx_len, &rssi, 1000000)
            == HAL_OK) {
            /*
             * ถ้ามีข้อมูลเข้า:
             * - rx_buf[0..rx_len-1]: ข้อมูลที่รับมา
             * - rssi: ความแรงสัญญาณ (dBm)
             *
             * ในตัวอย่างนี้แค่ผ่านไป (no-op)
             * แต่โค้ดจริงควรส่งต่อ UART หรือประมวลผล
             */
            ;
        }

        /*
         * รอ 1 วินาที ก่อนส่งรอบถัดไป
         * เพื่อไม่ให้ส่งข้อมูลถี่เกินไป (สร้าง noise)
         */
        hal_delay_ms(1000);
    }
}
