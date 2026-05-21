/********************************** (C) COPYRIGHT ********************************
 * File Name          : main.c
 * Author             : SimpleHAL
 * Version            : V1.1
 * Date               : 2026/05/17
 * Description        : ตัวอย่างที่ 12 — KeyScan matrix keyboard
 *
 * สิ่งที่เรียนรู้:
 * - การ init KeyScan matrix keyboard
 * - การกำหนดขาที่ใช้เป็นแถวและคอลัมน์
 * - การตั้งค่าความถี่สแกนและโหมด repeat
 * - การใช้ callback เพื่อตรวจจับการกดปุ่ม
 *
 * KeyScan คืออะไร:
 * KeyScan เป็น hardware peripheral ของ CH572 ที่ใช้สแกน
 * matrix keyboard โดยอัตโนมัติ (ไม่ต้องใช้ CPU polling)
 * - รองรับ matrix สูงสุด 8×8 (64 ปุ่ม)
 * - มี noise filter ในตัว
 * - ตรวจจับ pressed/released อัตโนมัติ
 * - ทำงานในโหมด sleep ได้ (ประหยัดไฟ)
 *
 * การกำหนดขา:
 * - ใช้ bit mask เลือกขาที่เป็นแถว (row) และคอลัมน์ (col)
 * - CH572 ใช้พิน PA เป็นหลักสำหรับ KeyScan
 * - ปกติ: 4 ขาเป็น row, 4 ขาเป็น col → 16 ปุ่ม
 *
 * KEYSCAN_PA2 | KEYSCAN_PA3 | KEYSCAN_PA8 | KEYSCAN_PA10 | KEYSCAN_PA11:
 * = ใช้ 5 ขาเหล่านี้สำหรับ KeyScan (ทั้ง row/col)
 *******************************************************************************/

#include "simple_hal.h"

/*
 * ฟังก์ชัน callback — เมื่อมีการกด/ปล่อยปุ่ม
 *
 * ถูกเรียกโดย KeyScan hardware interrupt ทุกครั้งที่
 * ตรวจพบการเปลี่ยนแปลงของปุ่ม (กดหรือปล่อย)
 *
 * พารามิเตอร์:
 * - arg: ส่ง keyscan handle มาเพื่อให้อ่านค่าได้
 *
 * ข้อควรระวัง:
 * callback ทำงานใน interrupt context
 * ควรสั้นและรวดเร็ว
 * ไม่ควรใช้ hal_delay_ms()
 */
void key_cb(void *arg)
{
    /*
     * แคสต์ arg → hal_keyscan_handle_t
     * เพื่อเรียกใช้ฟังก์ชันอ่านค่าจาก KeyScan
     */
    hal_keyscan_handle_t ks = (hal_keyscan_handle_t)arg;

    /*
     * hal_keyscan_get_value(handle)
     * อ่านค่า bit mask ของปุ่มที่กำลังถูกกด
     * แต่ละบิตแทน 1 ปุ่ม (ปุ่มแรก = bit 0)
     * คืนค่า uint32_t (รองรับสูงสุด 32 ปุ่ม)
     *
     * hal_keyscan_get_count(handle)
     * อ่านจำนวนปุ่มที่ถูกกดในขณะนี้
     * (เช่น 0 = ไม่มี, 1 = 1 ปุ่ม, 2 = 2 ปุ่ม)
     */
    uint32_t val = hal_keyscan_get_value(ks);
    uint32_t cnt = hal_keyscan_get_count(ks);

    /*
     * ในตัวอย่างนี้ไม่ได้ใช้งาน val และ cnt จริงๆ
     * แต่ในโค้ดจริงควรตรวจสอบ:
     * - ถ้า cnt > 0: มีปุ่มถูกกด
     * - val: ระบุว่าปุ่มไหน (ดู bit position)
     *
     * (void)val / cnt: ป้องกัน compiler warning
     */
    (void)val;
    (void)cnt;
}

/*
 * จุดเริ่มต้นโปรแกรม
 */
int main()
{
    /* ตั้งค่านาฬิกา */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

    /*
     * เริ่มต้น KeyScan
     *
     * hal_keyscan_init(pin_mask, prescaler, repeat_mode)
     *
     * pin_mask: เลือกขาที่ใช้เป็น KeyScan (bit mask)
     * - KEYSCAN_PA2: ขา PA2
     * - KEYSCAN_PA3: ขา PA3
     * - KEYSCAN_PA8: ขา PA8
     * - KEYSCAN_PA9: ขา PA9
     * - KEYSCAN_PA10: ขา PA10
     * - KEYSCAN_PA11: ขา PA11
     *
     * การ OR (|) ขาเข้าด้วยกัน: เพื่อใช้หลายขาพร้อมกัน
     * ขาเหล่านี้จะถูกสลับเป็น input/output โดย KeyScan automation
     *
     * การจัดสรรร Row/Col:
     * ขาที่กำหนดจะถูกแบ่งเป็น row และ col โดย hardware
     * ควรใช้จำนวนขาเป็นเลขคู่ (4, 6, 8) เพื่อให้ row=col
     *
     * prescaler: KEYSCAN_DIV1 — ความถี่สแกน (หารจาก sysclk)
     * - KEYSCAN_DIV1: สแกนเร็วที่สุด
     * - KEYSCAN_DIV2: สแกนช้าลง (กัน noise)
     * - KEYSCAN_DIV4: สแกนช้าที่สุด (ประหยัดไฟ)
     *
     * repeat_mode: KEYSCAN_REP1 — โหมดตรวจจับซ้ำ
     * - KEYSCAN_NOREP: ไม่ repeat (ตรวจจับครั้งเดียว)
     * - KEYSCAN_REP1: repeat 1 ครั้ง (กันเด้ง)
     * - KEYSCAN_REP2: repeat 2 ครั้ง (กันเด้งมากขึ้น)
     */
    hal_keyscan_handle_t ks = hal_keyscan_init(
        KEYSCAN_PA2 | KEYSCAN_PA3 | KEYSCAN_PA8 | KEYSCAN_PA10 | KEYSCAN_PA11,
        KEYSCAN_DIV1, KEYSCAN_REP1);

    /*
     * แนบ callback function
     *
     * เมื่อ KeyScan ตรวจพบการกด/ปล่อยปุ่ม
     * ฟังก์ชัน key_cb จะถูกเรียก
     *
     * ส่ง ks handle ผ่าน arg เพื่อให้ callback
     * สามารถอ่านค่าได้ (hal_keyscan_get_value / get_count)
     */
    hal_keyscan_attach_cb(ks, key_cb, ks);

    /*
     * main loop
     *
     * ใช้ __WFI() ให้ CPU หลับรอ KeyScan interrupt
     * เมื่อมีการกดปุ่ม KeyScan HW จะ interrupt → เรียก key_cb
     * แล้ว CPU กลับมาหลับต่อ
     *
     * ข้อดี:
     * - KeyScan ทำงานโดย HW ทั้งหมด (ไม่ใช้ CPU)
     * - CPU หลับได้ตลอดเวลา → ประหยัดพลังงาน
     * - ตรวจจับปุ่มได้แม้ CPU หลับ (ถ้าเปิด KeyScan wake)
     */
    while (1) {
        __WFI();    // รอ KeyScan interrupt
    }
}
