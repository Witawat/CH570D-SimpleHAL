#include "hal_ble.h"
#include "CH57x_common.h"
#include "CH572BLEPeri_LIB.h"

#define HAL_BLE_MEMHEAP_SIZE  4096
#define HAL_BLE_TX_DATA_HANDLE 0x0014

__attribute__((aligned(4))) uint32_t hal_ble_mem[HAL_BLE_MEMHEAP_SIZE / 4];
static uint8_t hal_ble_mac[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x01};
uint32_t g_LLE_IRQLibHandlerLocation;
static tmosTaskID hal_ble_task_id;
static uint16_t hal_ble_conn_handle = 0xFFFF;

static hal_callback_t hal_ble_connect_cb = NULL;
static void *hal_ble_connect_arg = NULL;
static hal_callback_t hal_ble_disconnect_cb = NULL;
static void *hal_ble_disconnect_arg = NULL;
static hal_callback_t hal_ble_data_cb = NULL;
static void *hal_ble_data_arg = NULL;

struct hal_ble_obj {
    uint8_t  used;
    uint8_t  connected;
    char     name[HAL_BLE_DEVICE_NAME_MAX];
};

static struct hal_ble_obj ble_instances[HAL_BLE_MAX_INSTANCES];

static uint32_t ble_srand_cb(void) { return R8_CHIP_ID; }
static uint32_t ble_idle_cb(uint32_t t) { return t; }
static void ble_err_cb(uint8_t c, uint32_t s) { (void)c; (void)s; }

static void ble_state_cb(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    hal_ble_handle_t h = &ble_instances[0];
    if (!h || !h->used) return;

    switch (newState & GAPROLE_STATE_ADV_MASK) {
    case GAPROLE_CONNECTED:
        h->connected = 1;
        if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
            hal_ble_conn_handle = pEvent->linkCmpl.connectionHandle;
        if (hal_ble_connect_cb) hal_ble_connect_cb(hal_ble_connect_arg);
        break;
    case GAPROLE_CONNECTED_ADV:
        if (!h->connected) {
            h->connected = 1;
            if (hal_ble_connect_cb) hal_ble_connect_cb(hal_ble_connect_arg);
        }
        break;
    case GAPROLE_WAITING:
    case GAPROLE_INIT:
    case GAPROLE_STARTED:
        if (h->connected) {
            h->connected = 0;
            hal_ble_conn_handle = 0xFFFF;
            if (hal_ble_disconnect_cb) hal_ble_disconnect_cb(hal_ble_disconnect_arg);
        }
        break;
    default:
        break;
    }
}

static void ble_rssi_cb(uint16_t ch, int8_t r) { (void)ch; (void)r; }
static void ble_param_cb(uint16_t ch, uint16_t i, uint16_t l, uint16_t t) { (void)ch; (void)i; (void)l; (void)t; }

static uint16_t ble_app_process(uint8_t task_id, uint16_t events)
{
    (void)task_id;
    return 0;
}

hal_ble_handle_t hal_ble_init(const char *device_name, uint8_t tx_power)
{
    hal_ble_handle_t h = &ble_instances[0];
    if (h->used) return h;

    h->used = 1;
    h->connected = 0;

    if (device_name) {
        uint8_t i;
        for (i = 0; i < HAL_BLE_DEVICE_NAME_MAX - 1 && device_name[i]; i++)
            h->name[i] = device_name[i];
        h->name[i] = 0;
    } else {
        const char *def = "SimpleHAL";
        uint8_t i;
        for (i = 0; i < HAL_BLE_DEVICE_NAME_MAX - 1 && def[i]; i++)
            h->name[i] = def[i];
        h->name[i] = 0;
    }

    if (tmos_memcmp(VER_LIB, VER_FILE, strlen(VER_FILE)) == FALSE) {
        while (1);
    }

    __SysTick_Config(SysTick_LOAD_RELOAD_Msk);
    sys_safe_access_enable();
    R32_MISC_CTRL = (R32_MISC_CTRL & ~(0x3f << 24)) | (0xe << 24);
    sys_safe_access_disable();
    g_LLE_IRQLibHandlerLocation = (uint32_t)LLE_IRQLibHandler;
    PFIC_SetPriority(BLEL_IRQn, 0xF0);

    bleConfig_t cfg;
    tmos_memset(&cfg, 0, sizeof(bleConfig_t));
    cfg.MEMAddr     = (uint32_t)(uintptr_t)hal_ble_mem;
    cfg.MEMLen      = HAL_BLE_MEMHEAP_SIZE;
    cfg.SNVAddr     = 0;
    cfg.SNVBlock    = 0;
    cfg.SNVNum      = 0;
    cfg.BufNumber   = 4;
    cfg.BufMaxLen   = 27;
    cfg.TxNumEvent  = 1;
    cfg.RxNumEvent  = 4;
    cfg.TxPower     = tx_power;
    cfg.WindowWidening = 0;
    cfg.WaitWindow  = 0;
    for (int i = 0; i < 6; i++) cfg.MacAddr[i] = hal_ble_mac[i];
    cfg.srandCB     = ble_srand_cb;
    cfg.idleCB      = ble_idle_cb;
    cfg.staCB       = ble_err_cb;
    cfg.readFlashCB = NULL;
    cfg.writeFlashCB = NULL;

    if (!cfg.MEMAddr || cfg.MEMLen < 3 * 1024) {
        while (1);
    }

    uint8_t lib_ret = BLE_LibInit(&cfg);
    if (lib_ret) {
        while (1);
    }

    hal_ble_task_id = TMOS_ProcessEventRegister(ble_app_process);

    GAPRole_PeripheralInit();

    static gapRolesCBs_t role_cbs;
    role_cbs.pfnStateChange  = ble_state_cb;
    role_cbs.pfnRssiRead     = ble_rssi_cb;
    role_cbs.pfnParamUpdate  = ble_param_cb;

    GAPRole_PeripheralStartDevice(hal_ble_task_id, NULL, &role_cbs);

    uint8_t name_len = 0;
    while (h->name[name_len]) name_len++;

    {
        uint8_t scan_rsp[31];
        uint8_t rsp_idx = 0;
        scan_rsp[rsp_idx++] = name_len + 1;
        scan_rsp[rsp_idx++] = 0x09;
        for (uint8_t i = 0; i < name_len; i++)
            scan_rsp[rsp_idx++] = h->name[i];
        scan_rsp[rsp_idx++] = 3;
        scan_rsp[rsp_idx++] = 0x19;
        scan_rsp[rsp_idx++] = 0x00;
        scan_rsp[rsp_idx++] = 0x00;

        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, rsp_idx, scan_rsp);
    }

    {
        uint8_t adv_data[31];
        uint8_t adv_idx = 0;
        adv_data[adv_idx++] = 2;
        adv_data[adv_idx++] = 0x01;
        adv_data[adv_idx++] = 0x06;

        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, adv_idx, adv_data);
    }

    {
        uint16_t adv_int = 80;
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, adv_int);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, adv_int);
    }

    return h;
}

void hal_ble_advertise_start(hal_ble_handle_t h, hal_ble_adv_mode_t mode)
{
    if (!h || !h->used) return;
    uint8_t enable = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &enable);
}

void hal_ble_advertise_stop(hal_ble_handle_t h)
{
    if (!h || !h->used) return;
    uint8_t disable = FALSE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &disable);
}

hal_status_t hal_ble_send(hal_ble_handle_t h, const uint8_t *data, uint16_t len)
{
    if (!h || !h->used) return HAL_INVALID;
    if (!h->connected) return HAL_ERROR;
    if (!data || !len || len > 20) return HAL_INVALID;

    attHandleValueNoti_t noti;
    noti.handle = HAL_BLE_TX_DATA_HANDLE;
    noti.len = len;
    noti.pValue = (uint8_t *)data;

    bStatus_t ret = GATT_Notification(hal_ble_conn_handle, &noti, FALSE);
    return (ret == 0) ? HAL_OK : HAL_ERROR;
}

void hal_ble_attach_connect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg)
{
    (void)h;
    hal_ble_connect_cb = cb;
    hal_ble_connect_arg = arg;
}

void hal_ble_attach_disconnect_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg)
{
    (void)h;
    hal_ble_disconnect_cb = cb;
    hal_ble_disconnect_arg = arg;
}

void hal_ble_attach_data_cb(hal_ble_handle_t h, hal_callback_t cb, void *arg)
{
    (void)h;
    hal_ble_data_cb = cb;
    hal_ble_data_arg = arg;
}

uint8_t hal_ble_is_connected(hal_ble_handle_t h)
{
    if (!h || !h->used) return 0;
    return h->connected;
}

void hal_ble_process(void)
{
    TMOS_SystemProcess();
}
