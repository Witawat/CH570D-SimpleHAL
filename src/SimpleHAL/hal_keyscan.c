#include "hal_keyscan.h"
#include "CH57x_common.h"

/*********************************************************************
 * @brief   โครงสร้างข้อมูลภายในของ Key Scan
 */
struct hal_keyscan_obj {
    uint8_t        used;
    uint16_t       pins;
    hal_callback_t cb;
    void          *arg;
};

static struct hal_keyscan_obj keyscan_instances[HAL_KEYSCAN_MAX_INSTANCES];

/*********************************************************************
 * @fn      get_keyscan
 *
 * @brief   คืนพอยน์เตอร์ไปยัง instance Key Scan
 *
 * @return  พอยน์เตอร์ไปยัง instance Key Scan
 */
static hal_keyscan_handle_t get_keyscan(void)
{
    return &keyscan_instances[0];
}

/*********************************************************************
 * @fn      hal_keyscan_init
 *
 * @brief   เริ่มต้น Key Scan: ตั้งค่าขา, divider, repeat
 *
 * @param   pins - ขาที่ใช้สำหรับ Key Scan
 * @param   clk_div - ค่า clock divider
 * @param   repeat - ค่า repeat
 *
 * @return  handle ของ Key Scan
 */
hal_keyscan_handle_t hal_keyscan_init(uint16_t pins, uint8_t clk_div, uint8_t repeat)
{
    hal_keyscan_handle_t h = get_keyscan();
    if (h->used) return h;
    h->used = 1;
    h->pins = pins;
    h->cb   = NULL;
    h->arg  = NULL;

    KeyScan_Cfg(ENABLE, pins, clk_div, repeat);
    return h;
}

/*********************************************************************
 * @fn      hal_keyscan_get_value
 *
 * @brief   อ่านค่าปุ่มที่กดล่าสุดจาก global register KeyValue
 *
 * @param   h - handle ของ Key Scan
 *
 * @return  ค่าปุ่มที่กดล่าสุด
 */
uint32_t hal_keyscan_get_value(hal_keyscan_handle_t h)
{
    (void)h;
    return KeyValue;
}

/*********************************************************************
 * @fn      hal_keyscan_get_count
 *
 * @brief   อ่านจำนวนครั้งที่มีการกดปุ่ม
 *
 * @param   h - handle ของ Key Scan
 *
 * @return  จำนวนครั้งที่กดปุ่ม
 */
uint32_t hal_keyscan_get_count(hal_keyscan_handle_t h)
{
    (void)h;
    return KeyScan_Cnt;
}

/*********************************************************************
 * @fn      hal_keyscan_attach_cb
 *
 * @brief   ผูก callback และเปิด interrupt สำหรับ Key Scan
 *
 * @param   h - handle ของ Key Scan
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void hal_keyscan_attach_cb(hal_keyscan_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->cb  = cb;
    h->arg = arg;

    KeyScan_ITCfg(ENABLE, RB_KEY_PRESSED_IE | RB_SCAN_1END_IE);
    PFIC_EnableIRQ(KEYSCAN_IRQn);
}

/*********************************************************************
 * @fn      hal_keyscan_enable_wakeup
 *
 * @brief   เปิด/ปิดความสามารถในการปลุกจากปุ่มกด
 *
 * @param   h - handle ของ Key Scan
 * @param   enable - 1 = เปิด, 0 = ปิด
 *
 * @return  ไม่มี
 */
void hal_keyscan_enable_wakeup(hal_keyscan_handle_t h, uint8_t enable)
{
    (void)h;
    KeyPress_Wake(enable ? ENABLE : DISABLE);
}

/*********************************************************************
 * @fn      KEYSCAN_IRQHandler
 *
 * @brief   ISR ของ Key Scan: อ่าน flag, ล้าง, เรียก callback
 *
 * @return  ไม่มี
 */
__INTERRUPT __HIGH_CODE void KEYSCAN_IRQHandler(void)
{
    hal_keyscan_handle_t h = get_keyscan();
    if (!h || !h->used) return;

    uint8_t flags = KeyScan_GetITFlag(0xFF);
    KeyScan_ClearITFlag(flags);

    if (h->cb) {
        h->cb(h->arg);
    }
}
