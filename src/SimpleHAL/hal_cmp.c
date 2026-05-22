/*********************************************************************
 * @file    hal_cmp.c
 * @brief   SimpleHAL — ตัวขับ Comparator (CMP)
 *
 * ใช้งาน CMP ของ CH572 ผ่าน StdPeriphDriver CH57x_cmp.c
 * ซัพพอร์ตการอ่านค่าแบบ polling, interrupt, และ Timer capture
 */

#include "hal_cmp.h"
#include "CH57x_common.h"

/* โครงสร้างข้อมูลภายในของ CMP */
struct hal_cmp_obj {
    uint8_t  used;       /* flag บอกว่ากำลังใช้งานอยู่ */
    uint8_t  input;      /* ค่าการเลือกขาเข้า (CMPSwTypeDef) */
    uint8_t  vref;       /* ค่า VREF (CMPNrefLevelTypeDef) */
    hal_callback_t cb;   /* callback เมื่อเกิดขัดจังหวะ */
    void    *arg;        /* อาร์กิวเมนต์ที่ส่งให้ callback */
};

static struct hal_cmp_obj cmp_inst;

/*********************************************************************
 * @fn      hal_cmp_init
 *
 * @brief   เริ่มต้น Comparator: กำหนดขาเข้าและ VREF
 *
 * @param   input - การเลือกขา (+)/(-)
 * @param   vref  - ระดับแรงดันอ้างอิงภายใน
 *
 * @return  handle ของ CMP หรือ NULL หากมีข้อผิดพลาด
 */
hal_cmp_handle_t hal_cmp_init(hal_cmp_input_t input, hal_cmp_vref_t vref)
{
    struct hal_cmp_obj *h = &cmp_inst;

    if (h->used) return h;

    h->used  = 1;
    h->input = (uint8_t)input;
    h->vref  = (uint8_t)vref;
    h->cb    = 0;
    h->arg   = 0;

    CMP_Init((CMPSwTypeDef)input, (CMPNrefLevelTypeDef)vref);

    return h;
}

/*********************************************************************
 * @fn      hal_cmp_deinit
 *
 * @brief   ปิด CMP: ปิด interrupt และตัว Comparator
 *
 * @param   h - handle ของ CMP
 *
 * @return  ไม่มี
 */
void hal_cmp_deinit(hal_cmp_handle_t h)
{
    if (!h || !h->used) return;

    R8_CMP_CTRL_1 &= ~RB_CMP_IE;   /* ปิด interrupt */
    R8_CMP_CTRL_0 &= ~RB_CMP_EN;   /* ปิดตัว Comparator */

    h->used = 0;
    h->cb   = 0;
}

/*********************************************************************
 * @fn      hal_cmp_read
 *
 * @brief   อ่าน output ของ Comparator แบบทันทีทันใด
 *
 * @param   h - handle ของ CMP
 *
 * @return  1 ถ้าแรงดัน (+) > (-), 0 ถ้า (+) < (-)
 */
uint8_t hal_cmp_read(hal_cmp_handle_t h)
{
    if (!h || !h->used) return 0;

    CMP_Init((CMPSwTypeDef)h->input, (CMPNrefLevelTypeDef)h->vref);

    return CMP_ReadAPROut();
}

/*********************************************************************
 * @fn      hal_cmp_enable
 *
 * @brief   เปิดการทำงานของ Comparator
 *
 * @param   h - handle ของ CMP
 *
 * @return  ไม่มี
 */
void hal_cmp_enable(hal_cmp_handle_t h)
{
    if (!h || !h->used) return;
    CMP_Enable();
}

/*********************************************************************
 * @fn      hal_cmp_disable
 *
 * @brief   ปิดการทำงานของ Comparator
 *
 * @param   h - handle ของ CMP
 *
 * @return  ไม่มี
 */
void hal_cmp_disable(hal_cmp_handle_t h)
{
    if (!h || !h->used) return;
    CMP_Disable();
}

/*********************************************************************
 * @fn      hal_cmp_route_to_timer_cap
 *
 * @brief   เชื่อมต่อหรือตัดสัญญาณ output CMP ไปยัง Timer capture
 *
 * @param   h  - handle ของ CMP
 * @param   en - 1 = เชื่อมต่อ, 0 = ตัดการเชื่อมต่อ
 *
 * @return  ไม่มี
 */
void hal_cmp_route_to_timer_cap(hal_cmp_handle_t h, uint8_t en)
{
    if (!h || !h->used) return;
    CMP_OutToTIMCAPCfg(en ? ENABLE : DISABLE);
}

/*********************************************************************
 * @fn      hal_cmp_attach_irq
 *
 * @brief   ผูก callback และเปิด interrupt ของ CMP
 *
 * @param   h       - handle ของ CMP
 * @param   trigger - เงื่อนไข trigger
 * @param   cb      - ฟังก์ชัน callback
 * @param   arg     - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void hal_cmp_attach_irq(hal_cmp_handle_t h, hal_cmp_irq_t trigger,
                        hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;

    h->cb  = cb;
    h->arg = arg;

    CMP_INTCfg((CMPOutSelTypeDef)trigger, ENABLE);
}

/*********************************************************************
 * @fn      hal_cmp_int_handler
 *
 * @brief   ตัวจัดการขัดจังหวะ: เช็ค flag ถ้ามี callback ก็เรียก
 *
 * @return  ไม่มี
 */
void hal_cmp_int_handler(void)
{
    if (R8_CMP_CTRL_2 & RB_CMP_IF) {
        R8_CMP_CTRL_2 |= RB_CMP_IF;  /* ล้าง flag ขัดจังหวะ */

        if (cmp_inst.cb) {
            cmp_inst.cb(cmp_inst.arg);
        }
    }
}

/*********************************************************************
 * @fn      CMP_IRQHandler
 *
 * @brief   ISR จริงของ CMP: เรียก hal_cmp_int_handler
 *
 * @return  ไม่มี
 */
__INTERRUPT __HIGH_CODE void CMP_IRQHandler(void)
{
    hal_cmp_int_handler();
}
