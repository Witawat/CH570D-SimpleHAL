#include "hal_spi.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

/*********************************************************************
 * @brief   โครงสร้างข้อมูลภายในของ SPI
 */
struct hal_spi_obj {
    uint8_t  used;
    uint32_t clock_hz;
    uint8_t  mode;
    uint8_t  bit_order;
    hal_callback_t cb;
    void          *arg;
};

static struct hal_spi_obj spi_instances[HAL_SPI_MAX_INSTANCES];

/*********************************************************************
 * @fn      get_spi
 *
 * @brief   คืนพอยน์เตอร์ไปยัง instance SPI
 *
 * @return  พอยน์เตอร์ไปยัง instance SPI แรก
 */
static hal_spi_handle_t get_spi(void)
{
    return &spi_instances[0];
}

/*********************************************************************
 * @fn      spi_mode_to_wch
 *
 * @brief   แปลงโหมด SPI + bit order เป็นค่าที่ WCH library ใช้
 *
 * @param   mode - โหมด SPI
 * @param   order - ลำดับบิต
 *
 * @return  ค่า mode ที่ WCH library รับได้
 */
static uint8_t spi_mode_to_wch(hal_spi_mode_t mode, hal_spi_bit_order_t order)
{
    if (mode == HAL_SPI_MODE0 && order == HAL_SPI_LSB_FIRST) return Mode0_LowBitINFront;
    if (mode == HAL_SPI_MODE0 && order == HAL_SPI_MSB_FIRST) return Mode0_HighBitINFront;
    if (mode == HAL_SPI_MODE3 && order == HAL_SPI_LSB_FIRST) return Mode3_LowBitINFront;
    return Mode3_HighBitINFront;
}

/*********************************************************************
 * @fn      spi_set_clock
 *
 * @brief   คำนวณและตั้งค่า clock divider ของ SPI
 *
 * @param   h - handle ของ SPI
 */
static void spi_set_clock(hal_spi_handle_t h)
{
    uint32_t div = FREQ_SYS / (2 * h->clock_hz);
    if (div < 2) div = 2;
    if (div > 255) div = 255;
    SPI_CLKCfg((uint8_t)div);
}

/*********************************************************************
 * @fn      hal_spi_init
 *
 * @brief   เริ่มต้น SPI Master: ตั้งค่าเริ่มต้น, clock และ data mode
 *
 * @param   clock_hz - ความถี่ SPI
 * @param   mode - โหมด SPI
 * @param   order - ลำดับบิต
 *
 * @return  handle ของ SPI
 */
hal_spi_handle_t hal_spi_init(uint32_t clock_hz, hal_spi_mode_t mode, hal_spi_bit_order_t order)
{
    hal_spi_handle_t h = get_spi();
    if (h->used) return h;

    h->used      = 1;
    h->clock_hz  = clock_hz;
    h->mode      = (uint8_t)mode;
    h->bit_order = (uint8_t)order;
    h->cb        = NULL;
    h->arg       = NULL;

    SPI_MasterDefInit();
    spi_set_clock(h);
    SPI_DataMode(spi_mode_to_wch(mode, order));
    return h;
}

/*********************************************************************
 * @fn      hal_spi_transfer
 *
 * @brief   ส่ง-รับข้อมูลแบบ Full-duplex (tx=NULL=ส่ง 0xFF, rx=NULL=ไม่เก็บบทรับ)
 *
 * @param   h - handle ของ SPI
 * @param   tx - ข้อมูลที่จะส่ง (NULL = ส่ง 0xFF)
 * @param   rx - บัฟเฟอร์รับข้อมูล (NULL = ไม่รับ)
 * @param   len - จำนวนไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หากพารามิเตอร์ไม่ถูกต้อง
 */
hal_status_t hal_spi_transfer(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!len) return HAL_INVALID;

    for (uint16_t i = 0; i < len; i++) {
        uint8_t out = tx ? tx[i] : 0xFF;
        SPI_MasterSendByte(out);
        uint8_t in = SPI_MasterRecvByte();
        if (rx) rx[i] = in;
    }
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_spi_send_byte
 *
 * @brief   ส่งข้อมูล 1 ไบต์ โดยไม่รอรับ
 *
 * @param   h - handle ของ SPI
 * @param   data - ข้อมูล 1 ไบต์
 *
 * @return  HAL_OK หากสำเร็จ, HAL_INVALID หาก handle ไม่ถูกต้อง
 */
hal_status_t hal_spi_send_byte(hal_spi_handle_t h, uint8_t data)
{
    if (!h || !h->used) return HAL_INVALID;
    SPI_MasterSendByte(data);
    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_spi_recv_byte
 *
 * @brief   รับ 1 ไบต์: ส่ง 0xFF ไปก่อนเพื่อรับข้อมูลกลับ
 *
 * @param   h - handle ของ SPI
 *
 * @return  ข้อมูล 1 ไบต์ที่รับได้
 */
uint8_t hal_spi_recv_byte(hal_spi_handle_t h)
{
    if (!h || !h->used) return 0;
    SPI_MasterSendByte(0xFF);
    return SPI_MasterRecvByte();
}

/*********************************************************************
 * @fn      hal_spi_set_speed
 *
 * @brief   เปลี่ยนความถี่ SPI ขณะทำงาน
 *
 * @param   h - handle ของ SPI
 * @param   clock_hz - ความถี่ใหม่
 */
void hal_spi_set_speed(hal_spi_handle_t h, uint32_t clock_hz)
{
    if (!h || !h->used) return;
    h->clock_hz = clock_hz;
    spi_set_clock(h);
}

/*********************************************************************
 * @fn      hal_spi_chip_select
 *
 * @brief   ควบคุม Chip Select (PA15): 0=เลือก, 1=ไม่เลือก
 *
 * @param   h - handle ของ SPI
 * @param   level - ระดับสัญญาณ CS (0 = เลือก, 1 = ไม่เลือก)
 */
void hal_spi_chip_select(hal_spi_handle_t h, uint8_t level)
{
    (void)h;
    if (level)
        GPIOA_SetBits(GPIO_Pin_15);
    else
        GPIOA_ResetBits(GPIO_Pin_15);
}

/*********************************************************************
 * @fn      spi_dma_isr
 *
 * @brief   ISR สำหรับ DMA SPI: ล้าง flag, เรียก callback
 */
static void spi_dma_isr(void)
{
    hal_spi_handle_t h = get_spi();
    if (!h || !h->used) return;
    if (SPI_GetITFlag(SPI_IT_DMA_END)) {
        SPI_ClearITFlag(SPI_IT_DMA_END);
        SPI_ITCfg(DISABLE, SPI_IT_DMA_END);
        if (h->cb) {
            hal_callback_t cb = h->cb;
            void *arg = h->arg;
            h->cb = NULL;
            h->arg = NULL;
            cb(arg);
        }
    }
}

/*********************************************************************
 * @fn      hal_spi_transfer_dma
 *
 * @brief   ส่ง/รับแบบ DMA (tx หรือ rx อย่างใดอย่างหนึ่งต้องเป็น NULL)
 *
 * @param   h - handle ของ SPI
 * @param   tx - ข้อมูลที่จะส่ง (NULL หากต้องการรับ)
 * @param   rx - บัฟเฟอร์รับข้อมูล (NULL หากต้องการส่ง)
 * @param   len - จำนวนไบต์
 * @param   cb - callback เมื่อเสร็จสิ้น
 * @param   arg - อาร์กิวเมนต์ที่ส่งให้ callback
 *
 * @return  HAL_OK หากสำเร็จ, HAL_BUSY หาก tx และ rx ไม่เป็น NULL
 */
hal_status_t hal_spi_transfer_dma(hal_spi_handle_t h, const uint8_t *tx, uint8_t *rx, uint16_t len, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!len) return HAL_INVALID;

    h->cb  = cb;
    h->arg = arg;

    if (tx && !rx) {
        SPI_MasterDMATrans((uint8_t *)tx, len);
    } else if (!tx && rx) {
        SPI_MasterDMARecv(rx, len);
    } else {
        return HAL_BUSY;
    }

    SPI_ITCfg(ENABLE, SPI_IT_DMA_END);
    PFIC_EnableIRQ(SPI_IRQn);
    return HAL_OK;
}

/*********************************************************************
 * @fn      SPI_IRQHandler
 *
 * @brief   ISR ของ SPI: เรียก spi_dma_isr
 */
__INTERRUPT __HIGH_CODE void SPI_IRQHandler(void)
{
    spi_dma_isr();
}
