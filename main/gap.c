///////////////////////////////////////////////////////////////////////////////////
// File: gap.c
//
// Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
//
// Author: Paweł Majewski
//
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// HEDER FILES
///////////////////////////////////////////////////////////////////////////////////

#include "gap.h"

///////////////////////////////////////////////////////////////////////////////////
// LOCAL DATA
///////////////////////////////////////////////////////////////////////////////////

static bool is_con = false;
static uint16_t spp_conn_id = 0;
static uint16_t spp_mtu_size = 23;
static uint16_t cmd = 0;
static uint16_t spp_srv_start_handle = 0;
static uint16_t spp_srv_end_handle = 0;
static char * notify_value_p = NULL;
static int notify_value_offset = 0;
static int notify_value_count = 0;
static esp_gattc_db_elem_t *db = NULL;
static QueueHandle_t cmd_reg_queue = NULL;
static uint16_t count = SPP_IDX_NB;
QueueHandle_t spp_uart_queue = NULL;

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static esp_ble_gap_cb_param_t scan_rst;

static const char device_name[] = "ESP_SPP_SERVER";

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static uint16_t spp_gattc_if = 0xff;

static esp_bt_uuid_t spp_service_uuid = {
    .len  = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_SPP_SERVICE_UUID,},
};

static void free_gattc_srv_db(void)
{
    is_con = false;
    spp_gattc_if = 0xff;
    spp_conn_id = 0;
    spp_mtu_size = 23;
    cmd = 0;
    spp_srv_start_handle = 0;
    spp_srv_end_handle = 0;
    notify_value_p = NULL;
    notify_value_offset = 0;
    notify_value_count = 0;
    if(db){
        free(db);
        db = NULL;
    }
}

static void notify_event_handler(esp_ble_gattc_cb_param_t * p_data)
{
    uint8_t handle = 0;
    if (p_data->notify.is_notify == true)
    {
        ESP_LOGI(GAP_TAG,"+NOTIFY:handle = %d,length = %d ", p_data->notify.handle, p_data->notify.value_len);
    }
    else
    {
        ESP_LOGI(GAP_TAG,"+INDICATE:handle = %d,length = %d ", p_data->notify.handle, p_data->notify.value_len);
    }
    handle = p_data->notify.handle;
    if (db == NULL) 
    {
        ESP_LOGE(GAP_TAG, " %s db is NULL", __func__);
        return;
    }
    if (handle == db[SPP_IDX_SPP_DATA_NTY_VAL].attribute_handle)
    {
        if ((p_data->notify.value[0] == '#') && (p_data->notify.value[1] == '#'))
        {
            if ((++notify_value_count) != p_data->notify.value[3])
            {
                if (notify_value_p != NULL)
                {
                    free(notify_value_p);
                }
                notify_value_count = 0;
                notify_value_p = NULL;
                notify_value_offset = 0;
                ESP_LOGE(GAP_TAG,"notify value count is not continuous,%s",__func__);
                return;
            }
            if (p_data->notify.value[3] == 1)
            {
                notify_value_p = (char *)malloc(((spp_mtu_size-7)*(p_data->notify.value[2]))*sizeof(char));
                if (notify_value_p == NULL)
                {
                    ESP_LOGE(GAP_TAG, "malloc failed,%s L#%d",__func__,__LINE__);
                    notify_value_count = 0;
                    return;
                }
                memcpy((notify_value_p + notify_value_offset),(p_data->notify.value + 4),(p_data->notify.value_len - 4));
                if (p_data->notify.value[2] == p_data->notify.value[3])
                {
                    uart_write_bytes(UART_NUM_0, (char *)(notify_value_p), (p_data->notify.value_len - 4 + notify_value_offset));
                    free(notify_value_p);
                    notify_value_p = NULL;
                    notify_value_offset = 0;
                    return;
                }
                notify_value_offset += (p_data->notify.value_len - 4);
            }
            else if(p_data->notify.value[3] <= p_data->notify.value[2])
            {
                memcpy((notify_value_p + notify_value_offset),(p_data->notify.value + 4),(p_data->notify.value_len - 4));
                if(p_data->notify.value[3] == p_data->notify.value[2])
                {
                    uart_write_bytes(UART_NUM_0, (char *)(notify_value_p), (p_data->notify.value_len - 4 + notify_value_offset));
                    free(notify_value_p);
                    notify_value_count = 0;
                    notify_value_p = NULL;
                    notify_value_offset = 0;
                    return;
                }
                notify_value_offset += (p_data->notify.value_len - 4);
            }
        }
        else
        {
            uart_write_bytes(UART_NUM_0, (char *)(p_data->notify.value), p_data->notify.value_len);
        }
    }
    else if (handle == ((db+SPP_IDX_SPP_STATUS_VAL)->attribute_handle))
    {
        esp_log_buffer_char(GAP_TAG, (char *)p_data->notify.value, p_data->notify.value_len);
    }
    else
    {
        esp_log_buffer_char(GAP_TAG, (char *)p_data->notify.value, p_data->notify.value_len);
    }
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GAP_TAG, "REG EVT, set scan params");
        esp_ble_gap_set_scan_params(&ble_scan_params);
        break;
    case ESP_GATTC_CONNECT_EVT:
        ESP_LOGI(GAP_TAG, "ESP_GATTC_CONNECT_EVT: conn_id=%d, gatt_if = %d", spp_conn_id, gattc_if);
        ESP_LOGI(GAP_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GAP_TAG, gl_profile_tab[APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        spp_gattc_if = gattc_if;
        is_con = true;
        spp_conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        esp_ble_gattc_search_service(spp_gattc_if, spp_conn_id, &spp_service_uuid);
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(GAP_TAG, "disconnect");
        free_gattc_srv_db();
        esp_ble_gap_start_scanning(SCAN_ALL_THE_TIME);
        break;
    case ESP_GATTC_SEARCH_RES_EVT:
        ESP_LOGI(GAP_TAG, "ESP_GATTC_SEARCH_RES_EVT: start_handle = %d, end_handle = %d, UUID:0x%04x",p_data->search_res.start_handle,p_data->search_res.end_handle,p_data->search_res.srvc_id.uuid.uuid.uuid16);
        spp_srv_start_handle = p_data->search_res.start_handle;
        spp_srv_end_handle = p_data->search_res.end_handle;
        break;
    case ESP_GATTC_SEARCH_CMPL_EVT:
        ESP_LOGI(GAP_TAG, "SEARCH_CMPL: conn_id = %x, status %d", spp_conn_id, p_data->search_cmpl.status);
        esp_ble_gattc_send_mtu_req(gattc_if, spp_conn_id);
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
        ESP_LOGI(GAP_TAG,"Index = %d,status = %d,handle = %d",cmd, p_data->reg_for_notify.status, p_data->reg_for_notify.handle);
        if(p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GAP_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT, status = %d", p_data->reg_for_notify.status);
            break;
        }
        uint16_t notify_en = 1;
        esp_ble_gattc_write_char_descr(
                spp_gattc_if,
                spp_conn_id,
                (db+cmd+1)->attribute_handle,
                sizeof(notify_en),
                (uint8_t *)&notify_en,
                ESP_GATT_WRITE_TYPE_RSP,
                ESP_GATT_AUTH_REQ_NONE);

        break;
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGI(GAP_TAG,"ESP_GATTC_NOTIFY_EVT");
        notify_event_handler(p_data);
        break;
    case ESP_GATTC_READ_CHAR_EVT:
        ESP_LOGI(GAP_TAG,"ESP_GATTC_READ_CHAR_EVT");
        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        ESP_LOGI(GAP_TAG,"ESP_GATTC_WRITE_CHAR_EVT:status = %d,handle = %d", param->write.status, param->write.handle);
        if(param->write.status != ESP_GATT_OK)
        {
            ESP_LOGE(GAP_TAG, "ESP_GATTC_WRITE_CHAR_EVT, error status = %d", p_data->write.status);
            break;
        }
        break;
    case ESP_GATTC_PREP_WRITE_EVT:
        break;
    case ESP_GATTC_EXEC_EVT:
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        ESP_LOGI(GAP_TAG,"ESP_GATTC_WRITE_DESCR_EVT: status =%d,handle = %d", p_data->write.status, p_data->write.handle);
        if(p_data->write.status != ESP_GATT_OK)
        {
            ESP_LOGE(GAP_TAG, "ESP_GATTC_WRITE_DESCR_EVT, error status = %d", p_data->write.status);
            break;
        }
        switch(cmd)
        {
        case SPP_IDX_SPP_DATA_NTY_VAL:
            cmd = SPP_IDX_SPP_STATUS_VAL;
            xQueueSend(cmd_reg_queue, &cmd, 10 / portTICK_PERIOD_MS);
            break;
        case SPP_IDX_SPP_STATUS_VAL:
            break;
        default:
            break;
        };
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if(p_data->cfg_mtu.status != ESP_OK){
            break;
        }
        ESP_LOGI(GAP_TAG,"+MTU:%d", p_data->cfg_mtu.mtu);
        spp_mtu_size = p_data->cfg_mtu.mtu;

        db = (esp_gattc_db_elem_t *)malloc(count*sizeof(esp_gattc_db_elem_t));
        if(db == NULL){
            break;
        }
        if(esp_ble_gattc_get_db(spp_gattc_if, spp_conn_id, spp_srv_start_handle, spp_srv_end_handle, db, &count) != ESP_GATT_OK){
            break;
        }
        if(count != SPP_IDX_NB){
            break;
        }
        cmd = SPP_IDX_SPP_DATA_NTY_VAL;
        xQueueSend(cmd_reg_queue, &cmd, 10/portTICK_PERIOD_MS);
        break;
    case ESP_GATTC_SRVC_CHG_EVT:
        break;
    default:
        break;
    }
}

static void uart_task(void *pvParameters)
{
    uart_event_t event;
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(spp_uart_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            switch (event.type) {
            //Event of UART receving data
            case UART_DATA:
                if (event.size && (is_con == true) && (db != NULL) && ((db+SPP_IDX_SPP_DATA_RECV_VAL)->properties & (ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_WRITE))) {
                    uint8_t * temp = NULL;
                    temp = (uint8_t *)malloc(sizeof(uint8_t)*event.size);
                    if(temp == NULL){
                        ESP_LOGE(GATTC_TAG, "malloc failed,%s L#%d", __func__, __LINE__);
                        break;
                    }
                    memset(temp, 0x0, event.size);
                    uart_read_bytes(UART_NUM_0,temp,event.size,portMAX_DELAY);
                    esp_ble_gattc_write_char( spp_gattc_if,
                                              spp_conn_id,
                                              (db+SPP_IDX_SPP_DATA_RECV_VAL)->attribute_handle,
                                              event.size,
                                              temp,
                                              ESP_GATT_WRITE_TYPE_RSP,
                                              ESP_GATT_AUTH_REQ_NONE);
                    free(temp);
                }
                break;
            default:
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

static void spp_client_reg_task(void* arg)
{
    uint16_t cmd_id;
    for(;;) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if(xQueueReceive(cmd_reg_queue, &cmd_id, portMAX_DELAY)) {
            if(db != NULL) {
                if(cmd_id == SPP_IDX_SPP_DATA_NTY_VAL){
                    ESP_LOGI(GATTC_TAG,"Index = %d,UUID = 0x%04x, handle = %d", cmd_id, (db+SPP_IDX_SPP_DATA_NTY_VAL)->uuid.uuid.uuid16, (db+SPP_IDX_SPP_DATA_NTY_VAL)->attribute_handle);
                    esp_ble_gattc_register_for_notify(spp_gattc_if, gl_profile_tab[APP_ID].remote_bda, (db+SPP_IDX_SPP_DATA_NTY_VAL)->attribute_handle);
                }else if(cmd_id == SPP_IDX_SPP_STATUS_VAL){
                    ESP_LOGI(GATTC_TAG,"Index = %d,UUID = 0x%04x, handle = %d", cmd_id, (db+SPP_IDX_SPP_STATUS_VAL)->uuid.uuid.uuid16, (db+SPP_IDX_SPP_STATUS_VAL)->attribute_handle);
                    esp_ble_gattc_register_for_notify(spp_gattc_if, gl_profile_tab[APP_ID].remote_bda, (db+SPP_IDX_SPP_STATUS_VAL)->attribute_handle);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    esp_err_t error;

    ESP_LOGE(GAP_TAG, "GAP_EVT, event %d", event);

    switch(event){
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        error = param->scan_param_cmpl.status;
        if (error != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "Scan param set failed.");
            break;
        }
        esp_ble_gap_start_scanning(0xFFFF);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;

        ESP_LOGE(GAP_TAG, "GAP_EVT, ESP_GAP_BLE_SCAN_RESULT_EVT event: %d", scan_result->scan_rst.search_evt);
        switch (scan_result->scan_rst.search_evt) 
        {
            case ESP_GAP_SEARCH_INQ_RES_EVT:
                esp_log_buffer_hex(GAP_TAG, scan_result->scan_rst.bda, 6);
                ESP_LOGI(GAP_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
                adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
                ESP_LOGI(GAP_TAG, "Searched Device Name Len %d", adv_name_len);
                esp_log_buffer_char(GAP_TAG, adv_name, adv_name_len);
                ESP_LOGI(GAP_TAG, " ");
                if (adv_name != NULL) {
                    if (strncmp((char *)adv_name, device_name, adv_name_len) == 0) {
                        memcpy(&(scan_rst), scan_result, sizeof(esp_ble_gap_cb_param_t));
                        esp_ble_gap_stop_scanning();
                    }
                }
                break;
            case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                break;
            case ESP_GAP_SEARCH_DISC_RES_EVT:
                break;
            case ESP_GAP_SEARCH_DISC_BLE_RES_EVT:
                break;
            case ESP_GAP_SEARCH_DISC_CMPL_EVT:
                break;
            case ESP_GAP_SEARCH_DI_DISC_CMPL_EVT:
                break;
            case ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT:
                break;
            case ESP_GAP_SEARCH_INQ_DISCARD_NUM_EVT:
                break;
            default:
                break;
        }
        break;
    }   
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        error = param->scan_param_cmpl.status;
        if (error != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "Scan start failed.");
            break;
        }
        else
        {
            ESP_LOGE(GAP_TAG, "Scan start successed.");
        }
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        break;
    case ESP_GAP_BLE_KEY_EVT:
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        break;
    case ESP_GAP_BLE_OOB_REQ_EVT:
        break;
    case ESP_GAP_BLE_LOCAL_IR_EVT:
        break;
    case ESP_GAP_BLE_LOCAL_ER_EVT:
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        error = param->adv_stop_cmpl.status;
        if (error != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(GAP_TAG, "Stop adv failed");
        }
        else
        {
            ESP_LOGI(GAP_TAG, "Stop adv successfully");
        }
        break;
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        error = param->scan_param_cmpl.status;
        if (error != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "Scan stop failed.");
            break;
        }
        else
        {
            ESP_LOGE(GAP_TAG, "Scan stop successed.");
        }
        if (is_con == false) {
            ESP_LOGI(GAP_TAG, "Connect to the remote device.");
            esp_ble_gattc_open(gl_profile_tab[APP_ID].gattc_if, scan_rst.scan_rst.bda, scan_rst.scan_rst.ble_addr_type, true);
        }        
        break;
    case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT:
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        break;
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SET_CHANNELS_EVT:
        break;
    case ESP_GAP_BLE_READ_PHY_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SET_PREFERRED_DEFAULT_PHY_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SET_PREFERRED_PHY_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_SET_REMOVE_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_SET_CLEAR_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_DATA_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_START_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_STOP_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT:
        break;
    case ESP_GAP_BLE_EXT_ADV_REPORT_EVT:
        break;
    case ESP_GAP_BLE_SCAN_TIMEOUT_EVT:
        break;
    case ESP_GAP_BLE_ADV_TERMINATED_EVT:
        break;
    case ESP_GAP_BLE_SCAN_REQ_RECEIVED_EVT:
        break;
    case ESP_GAP_BLE_CHANNEL_SELECT_ALGORITHM_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_REPORT_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_SYNC_LOST_EVT:
        break;
    case ESP_GAP_BLE_PERIODIC_ADV_SYNC_ESTAB_EVT:
        break;
    case ESP_GAP_BLE_EVT_MAX:
        break;

    default:
        break;
    }
}

void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d", event, gattc_if);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }
    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}


void spp_uart_init(void)
{
    cmd_reg_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(spp_client_reg_task, "spp_client_reg_task", 2048, NULL, 10, NULL);

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_RTS,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    //Install UART driver, and get the queue.
    uart_driver_install(UART_NUM_0, 4096, 8192, 10, &spp_uart_queue, 0);
    //Set UART parameters
    uart_param_config(UART_NUM_0, &uart_config);
    //Set UART pins
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    xTaskCreate(uart_task, "uTask", 2048, (void*)UART_NUM_0, 8, NULL);
}
