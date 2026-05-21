#include "hal_rf.h"
#include "CH57x_common.h"
#include "CH572rf.h"

/*********************************************************************
 * @brief   ขนาด DMA buffer สำหรับรับ-ส่ง RF
 */
#define HAL_RF_DMA_BUF_SIZE 260

/*********************************************************************
 * @brief   โครงสร้างข้อมูลภายในของ RF
 */
struct hal_rf_obj {
    uint8_t        used;
    uint32_t       frequency_khz;
    uint8_t        phy_mode;
    int8_t         tx_power_dbm;
    volatile uint8_t rx_pending;
    volatile uint8_t tx_done;
    volatile uint8_t rx_timeout;
    volatile int8_t  rssi;
    volatile uint8_t crc_ok;
    uint8_t         *rx_user_buf;
    uint16_t         rx_user_max;
    uint16_t        *rx_user_len;
    hal_callback_t   rx_cb;
    void            *rx_arg;
    uint8_t          dma_buf[HAL_RF_DMA_BUF_SIZE] __attribute__((aligned(4)));
    uint32_t         access_address;
    uint8_t          access_address_ex;
    uint32_t         crc_init;
    uint32_t         crc_poly;
    uint8_t          white_channel;
};

static struct hal_rf_obj rf_instances[HAL_RF_MAX_INSTANCES];

/*********************************************************************
 * @fn      get_rf
 *
 * @brief   คืนพอยน์เตอร์ไปยัง instance RF
 *
 * @return  พอยน์เตอร์ไปยัง instance RF
 */
static hal_rf_handle_t get_rf(void)
{
    return &rf_instances[0];
}

/*********************************************************************
 * @fn      phy_to_wch
 *
 * @brief   แปลง PHY mode เป็นค่า WCH library
 *
 * @param   phy - PHY mode
 *
 * @return  ค่า PHY mode ของ WCH library
 */
static uint8_t phy_to_wch(uint8_t phy)
{
    switch (phy) {
    case HAL_RF_PHY_1M:  return PHY_MODE_PHY_1M;
    case HAL_RF_PHY_2M:  return PHY_MODE_PHY_2M;
    case HAL_RF_PHY_2G4:
    default:             return PHY_MODE_2G4;
    }
}

/*********************************************************************
 * @fn      rf_process_cb
 *
 * @brief   Callback กระบวนการ RF (TX/RX/timeout/CRC error)
 *
 * @param   status - สถานะของ RF
 * @param   id - ID ของ event
 *
 * @return  ไม่มี
 */
static void rf_process_cb(rfRole_States_t status, uint8_t id)
{
    hal_rf_handle_t h = get_rf();
    if (!h || !h->used) return;

    (void)id;

    switch (status) {
    case RF_STATE_TX_FINISH:
        h->tx_done = 1;
        break;

    case RF_STATE_RX: {
        uint8_t crc = RFIP_ReadCrc();
        if (crc) {
            h->crc_ok = 1;
            uint8_t len = h->dma_buf[0];
            if (h->rx_user_buf && h->rx_user_len) {
                uint16_t copy_len = (len < h->rx_user_max) ? len : h->rx_user_max;
                for (uint16_t i = 0; i < copy_len; i++) {
                    h->rx_user_buf[i] = h->dma_buf[1 + i];
                }
                *h->rx_user_len = copy_len;
            }
        } else {
            h->crc_ok = 0;
        }
        h->rssi = (int8_t)h->dma_buf[HAL_RF_DMA_BUF_SIZE - 1];
        h->rx_pending = 1;
        break;
    }

    case RF_STATE_TIMEOUT:
        h->rx_timeout = 1;
        break;

    case RF_STATE_RX_CRCERR:
        h->crc_ok = 0;
        h->rx_pending = 1;
        break;

    default:
        break;
    }
}

/*********************************************************************
 * @fn      hal_rf_init
 *
 * @brief   เริ่มต้น RF: กำหนดความถี่, PHY, กำลัง, ตั้งค่า role และเปิด IRQ
 *
 * @param   frequency_khz - ความถี่ในหน่วย kHz
 * @param   phy - โหมด PHY
 * @param   tx_power_dbm - กำลังส่งในหน่วย dBm
 *
 * @return  handle ของ RF
 */
hal_rf_handle_t hal_rf_init(uint32_t frequency_khz, hal_rf_phy_mode_t phy, int8_t tx_power_dbm)
{
    hal_rf_handle_t h = get_rf();
    if (h->used) return h;

    h->used             = 1;
    h->frequency_khz    = frequency_khz;
    h->phy_mode         = (uint8_t)phy;
    h->tx_power_dbm     = tx_power_dbm;
    h->rx_pending       = 0;
    h->tx_done          = 0;
    h->rx_timeout       = 0;
    h->crc_ok           = 0;
    h->rssi             = 0;
    h->rx_user_buf      = NULL;
    h->rx_user_max      = 0;
    h->rx_user_len      = NULL;
    h->rx_cb            = NULL;
    h->rx_arg           = NULL;
    h->access_address   = 0x94826E8E;
    h->access_address_ex = 0;
    h->crc_init         = 0xFFFF;
    h->crc_poly         = 0x8810;
    h->white_channel    = 0;

    rfRoleConfig_t roleConfig;
    roleConfig.processMask = RF_STATE_RX | RF_STATE_TIMEOUT | RF_STATE_RX_CRCERR | RF_STATE_TX_FINISH;
    roleConfig.rfProcessCB = rf_process_cb;
    RFRole_BasicInit(&roleConfig);

    RFIP_Calibration();

    PFIC_EnableIRQ(BLEB_IRQn);
    PFIC_EnableIRQ(BLEL_IRQn);

    return h;
}

/*********************************************************************
 * @fn      hal_rf_send
 *
 * @brief   ส่งข้อมูล: เตรียม DMA buffer, เรียก StartTx, รอจนเสร็จ
 *
 * @param   h - handle ของ RF
 * @param   data - ข้อมูลที่จะส่ง
 * @param   len - ความยาวข้อมูล
 *
 * @return  สถานะการส่ง
 */
hal_status_t hal_rf_send(hal_rf_handle_t h, const uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!data || !len || len > 251) return HAL_INVALID;

    h->tx_done = 0;

    h->dma_buf[0] = (uint8_t)len;
    for (uint16_t i = 0; i < len; i++) {
        h->dma_buf[1 + i] = data[i];
    }

    rfipTx_t tx_param;
    tx_param.accessAddress  = h->access_address;
    tx_param.accessAddressEx= h->access_address_ex;
    tx_param.crcInit        = h->crc_init;
    tx_param.frequency      = h->frequency_khz;
    tx_param.properties     = phy_to_wch(h->phy_mode);
    tx_param.txDMA          = (uint32_t)(uintptr_t)h->dma_buf;
    tx_param.whiteChannel   = h->white_channel;
    tx_param.txLen          = (uint8_t)len;
    tx_param.waitTime       = 50;
    tx_param.txPowerVal     = h->tx_power_dbm;
    tx_param.crcPoly        = h->crc_poly;

    RFIP_StartTx(&tx_param);

    while (!h->tx_done);

    return HAL_OK;
}

/*********************************************************************
 * @fn      hal_rf_recv
 *
 * @brief   รับข้อมูล: ตั้งค่า RX, รอรับหรือ timeout, คืนค่า RSSI
 *
 * @param   h - handle ของ RF
 * @param   buf - buffer สำหรับรับข้อมูล
 * @param   max_len - ขนาดสูงสุดของ buffer
 * @param   out_len - ตัวชี้สำหรับเก็บความยาวที่รับได้
 * @param   rssi - ตัวชี้สำหรับเก็บค่า RSSI
 * @param   timeout_us - timeout ในหน่วยไมโครวินาที
 *
 * @return  สถานะการรับ
 */
hal_status_t hal_rf_recv(hal_rf_handle_t h, uint8_t *buf, uint16_t max_len, uint16_t *out_len, int8_t *rssi, uint32_t timeout_us)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!buf || !max_len) return HAL_INVALID;

    h->rx_pending  = 0;
    h->rx_timeout  = 0;
    h->rx_user_buf = buf;
    h->rx_user_max = max_len;
    h->rx_user_len = out_len;
    if (out_len) *out_len = 0;

    rfipRx_t rx_param;
    rx_param.accessAddress  = h->access_address;
    rx_param.accessAddressEx= h->access_address_ex;
    rx_param.crcInit        = h->crc_init;
    rx_param.frequency      = h->frequency_khz;
    rx_param.properties     = phy_to_wch(h->phy_mode);
    rx_param.rxDMA          = (uint32_t)(uintptr_t)h->dma_buf;
    rx_param.whiteChannel   = h->white_channel;
    rx_param.rxMaxLen       = max_len;
    rx_param.timeOut        = (uint16_t)(timeout_us / 2);
    rx_param.crcPoly        = h->crc_poly;

    RFIP_SetRx(&rx_param);

    while (!h->rx_pending && !h->rx_timeout);

    if (rssi) *rssi = h->rssi;

    if (h->rx_pending && h->crc_ok && out_len) {
        return HAL_OK;
    }

    h->rx_user_buf = NULL;
    h->rx_user_len = NULL;

    if (h->rx_timeout) return HAL_TIMEOUT;
    return HAL_ERROR;
}

/*********************************************************************
 * @fn      hal_rf_set_tx_power
 *
 * @brief   เปลี่ยนกำลังส่ง RF
 *
 * @param   h - handle ของ RF
 * @param   power_dbm - กำลังส่งในหน่วย dBm
 *
 * @return  ไม่มี
 */
void hal_rf_set_tx_power(hal_rf_handle_t h, int8_t power_dbm)
{
    if (!h || !h->used) return;
    h->tx_power_dbm = power_dbm;
}

/*********************************************************************
 * @fn      hal_rf_attach_rx_cb
 *
 * @brief   ผูก callback เมื่อได้รับข้อมูล RF
 *
 * @param   h - handle ของ RF
 * @param   cb - callback function
 * @param   arg - argument ที่ส่งให้ callback
 *
 * @return  ไม่มี
 */
void hal_rf_attach_rx_cb(hal_rf_handle_t h, hal_callback_t cb, void *arg)
{
    if (!h || !h->used) return;
    h->rx_cb  = cb;
    h->rx_arg = arg;
}

/*********************************************************************
 * @fn      hal_rf_stop
 *
 * @brief   หยุดและปิด RF
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void hal_rf_stop(hal_rf_handle_t h)
{
    if (!h || !h->used) return;
    RFRole_Stop();
    RFRole_Shut();
}

/*********************************************************************
 * @fn      hal_rf_sleep
 *
 * @brief   สั่ง RF เข้าโหมด sleep
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void hal_rf_sleep(hal_rf_handle_t h)
{
    (void)h;
    RFRole_Shut();
}

/*********************************************************************
 * @fn      hal_rf_wake
 *
 * @brief   ปลุก RF จาก sleep
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void hal_rf_wake(hal_rf_handle_t h)
{
    (void)h;
    RFIP_WakeUpRegInit();
    RFIP_Calibration();
}

/*********************************************************************
 * @fn      hal_rf_set_channel
 *
 * @brief   เปลี่ยนช่องความถี่ RF
 *
 * @param   h - handle ของ RF
 * @param   frequency_khz - ความถี่ในหน่วย kHz
 *
 * @return  ไม่มี
 */
void hal_rf_set_channel(hal_rf_handle_t h, uint32_t frequency_khz)
{
    if (!h || !h->used) return;
    h->frequency_khz = frequency_khz;
}

/*********************************************************************
 * @fn      hal_rf_calibrate
 *
 * @brief   Calibrate RF
 *
 * @param   h - handle ของ RF
 *
 * @return  ไม่มี
 */
void hal_rf_calibrate(hal_rf_handle_t h)
{
    (void)h;
    RFIP_Calibration();
}

/*********************************************************************
 * @fn      BB_IRQHandler
 *
 * @brief   ISR ของ Baseband (BB)
 *
 * @return  ไม่มี
 */
__INTERRUPT __HIGH_CODE void BB_IRQHandler(void)
{
    BB_LibIRQHandler();
}

/*********************************************************************
 * @fn      LLE_IRQHandler
 *
 * @brief   ISR ของ Link Layer (LLE)
 *
 * @return  ไม่มี
 */
__INTERRUPT __HIGH_CODE void LLE_IRQHandler(void)
{
    LLE_LibIRQHandler();
}
