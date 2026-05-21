#include "hal_spi.h"
#include "simple_hal_config.h"
#include "CH57x_common.h"

struct hal_spi_obj {
    uint8_t  used;
    uint32_t clock_hz;
    uint8_t  mode;
    uint8_t  bit_order;
    hal_callback_t cb;
    void          *arg;
};

static struct hal_spi_obj spi_instances[HAL_SPI_MAX_INSTANCES];

static hal_spi_handle_t get_spi(void)
{
    return &spi_instances[0];
}

static uint8_t spi_mode_to_wch(hal_spi_mode_t mode, hal_spi_bit_order_t order)
{
    if (mode == HAL_SPI_MODE0 && order == HAL_SPI_LSB_FIRST) return Mode0_LowBitINFront;
    if (mode == HAL_SPI_MODE0 && order == HAL_SPI_MSB_FIRST) return Mode0_HighBitINFront;
    if (mode == HAL_SPI_MODE3 && order == HAL_SPI_LSB_FIRST) return Mode3_LowBitINFront;
    return Mode3_HighBitINFront;
}

static void spi_set_clock(hal_spi_handle_t h)
{
    uint32_t div = FREQ_SYS / (2 * h->clock_hz);
    if (div < 2) div = 2;
    if (div > 255) div = 255;
    SPI_CLKCfg((uint8_t)div);
}

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

hal_status_t hal_spi_send_byte(hal_spi_handle_t h, uint8_t data)
{
    if (!h || !h->used) return HAL_INVALID;
    SPI_MasterSendByte(data);
    return HAL_OK;
}

uint8_t hal_spi_recv_byte(hal_spi_handle_t h)
{
    if (!h || !h->used) return 0;
    SPI_MasterSendByte(0xFF);
    return SPI_MasterRecvByte();
}

void hal_spi_set_speed(hal_spi_handle_t h, uint32_t clock_hz)
{
    if (!h || !h->used) return;
    h->clock_hz = clock_hz;
    spi_set_clock(h);
}

void hal_spi_chip_select(hal_spi_handle_t h, uint8_t level)
{
    (void)h;
    if (level)
        GPIOA_SetBits(GPIO_Pin_15);
    else
        GPIOA_ResetBits(GPIO_Pin_15);
}

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

__INTERRUPT __HIGH_CODE void SPI_IRQHandler(void)
{
    spi_dma_isr();
}
