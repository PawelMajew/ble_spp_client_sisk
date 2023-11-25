///////////////////////////////////////////////////////////////////////////////////
// File: gatt.c
//
// Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
//
// Author: Paweł Majewski
//
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// HEDER FILES
///////////////////////////////////////////////////////////////////////////////////

#include "gatt.h"

///////////////////////////////////////////////////////////////////////////////////
// GLOBAL DATA
///////////////////////////////////////////////////////////////////////////////////

esp_bd_addr_t address_pm;

///////////////////////////////////////////////////////////////////////////////////
// LOCAL DATA
///////////////////////////////////////////////////////////////////////////////////

static uint16_t spp_conn_id = 0;
static uint16_t spp_mtu_size = 23;
static uint16_t cmd = 0;
static uint16_t spp_srv_start_handle = 0;
static uint16_t spp_srv_end_handle = 0;
static char *notify_value_p = NULL;
static int notify_value_offset = 0;
static int notify_value_count = 0;
static esp_gattc_db_elem_t *db = NULL;
static QueueHandle_t cmd_reg_queue = NULL;
static uint16_t count = SPP_IDX_NB;
QueueHandle_t spp_uart_queue = NULL;

/**
 * @brief BLE scan parameters.
 */
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static uint16_t spp_gattc_if = 0xff;

/**
 * @brief SPP service UUID definition.
 */
static esp_bt_uuid_t spp_service_uuid = {
    .len  = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_SPP_SERVICE_UUID,},
};

///////////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Free GATTC service database and reset related variables.
 */
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

/**
 * @brief Handler for GATTC notification events.
 *
 * @param p_data Pointer to the GATTC notification event data.
 */
static void notify_event_handler(esp_ble_gattc_cb_param_t * p_data)
{
    uint8_t handle = 0;
    if (p_data->notify.is_notify == true)
    {
        ESP_LOGI(GATT_TAG,"+NOTIFY:handle = %d,length = %d ", p_data->notify.handle, p_data->notify.value_len);
    }
    else
    {
        ESP_LOGI(GATT_TAG,"+INDICATE:handle = %d,length = %d ", p_data->notify.handle, p_data->notify.value_len);
    }
    handle = p_data->notify.handle;
    if (db == NULL) 
    {
        ESP_LOGE(GATT_TAG, " %s db is NULL", __func__);
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
                ESP_LOGE(GATT_TAG,"notify value count is not continuous,%s",__func__);
                return;
            }
            if (p_data->notify.value[3] == 1)
            {
                notify_value_p = (char *)malloc(((spp_mtu_size-7)*(p_data->notify.value[2]))*sizeof(char));
                if (notify_value_p == NULL)
                {
                    ESP_LOGE(GATT_TAG, "malloc failed,%s L#%d",__func__,__LINE__);
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
        esp_log_buffer_char(GATT_TAG, (char *)p_data->notify.value, p_data->notify.value_len);
    }
    else
    {
        esp_log_buffer_char(GATT_TAG, (char *)p_data->notify.value, p_data->notify.value_len);
    }
}

/**
 * @brief UART task to handle UART events.
 *
 * @param pvParameters Task parameters.
 */
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
                        ESP_LOGE(GATT_TAG, "malloc failed,%s L#%d", __func__, __LINE__);
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

/**
 * @brief SPP client registration task to handle GATT client registration.
 *
 * @param arg Task parameters.
 */
static void spp_client_reg_task(void* arg)
{
    uint16_t cmd_id;
    for(;;) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if(xQueueReceive(cmd_reg_queue, &cmd_id, portMAX_DELAY)) {
            if(db != NULL) {
                if(cmd_id == SPP_IDX_SPP_DATA_NTY_VAL){
                    ESP_LOGI(GATT_TAG,"Index = %d,UUID = 0x%04x, handle = %d", cmd_id, (db+SPP_IDX_SPP_DATA_NTY_VAL)->uuid.uuid.uuid16, (db+SPP_IDX_SPP_DATA_NTY_VAL)->attribute_handle);
                    esp_ble_gattc_register_for_notify(spp_gattc_if, address_pm, (db+SPP_IDX_SPP_DATA_NTY_VAL)->attribute_handle);
                }else if(cmd_id == SPP_IDX_SPP_STATUS_VAL){
                    ESP_LOGI(GATT_TAG,"Index = %d,UUID = 0x%04x, handle = %d", cmd_id, (db+SPP_IDX_SPP_STATUS_VAL)->uuid.uuid.uuid16, (db+SPP_IDX_SPP_STATUS_VAL)->attribute_handle);
                    esp_ble_gattc_register_for_notify(spp_gattc_if, address_pm, (db+SPP_IDX_SPP_STATUS_VAL)->attribute_handle);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback function for handling Bluetooth Low Energy (BLE) GATT client (GATTC) events.
 *
 * This function is invoked in response to various BLE GATTC events. It logs event information,
 * processes different GATTC events, and performs corresponding actions based on the event type.
 *
 * @param event     The type of GATTC event.
 * @param gattc_if  The GATTC interface associated with the event.
 * @param param     A pointer to the parameters associated with the event.
 */
void esp_gattc_handle(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event)
    {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATT_TAG, "REG EVT, set scan params");
        if (param->reg.status == ESP_GATT_OK) {
            esp_if_pm = gattc_if;
        } else {
            ESP_LOGI(GATT_TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
        
        esp_ble_gap_set_scan_params(&ble_scan_params);
        break;
    case ESP_GATTC_UNREG_EVT:
        ESP_LOGI(GATT_TAG, "UNREG EVT");
        break;
    case ESP_GATTC_OPEN_EVT:
        ESP_LOGI(GATT_TAG, "OPEN EVT");
        break;
    case ESP_GATTC_READ_CHAR_EVT:
        ESP_LOGI(GATT_TAG,"ESP_GATTC_READ_CHAR_EVT");    
        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        ESP_LOGI(GATT_TAG,"ESP_GATTC_WRITE_CHAR_EVT:status = %d,handle = %d", param->write.status, param->write.handle);
        if(param->write.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATT_TAG, "ESP_GATTC_WRITE_CHAR_EVT, error status = %d", p_data->write.status);
            break;
        }
        break;
    case ESP_GATTC_CLOSE_EVT:
        ESP_LOGI(GATT_TAG, "CLOSE EVT");
        break;
    case ESP_GATTC_SEARCH_CMPL_EVT:
        ESP_LOGI(GATT_TAG, "SEARCH_CMPL: conn_id = %x, status %d", spp_conn_id, p_data->search_cmpl.status);
        esp_ble_gattc_send_mtu_req(gattc_if, spp_conn_id);    
        break;
    case ESP_GATTC_SEARCH_RES_EVT:
        ESP_LOGI(GATT_TAG, "ESP_GATTC_SEARCH_RES_EVT: start_handle = %d, end_handle = %d, UUID:0x%04x",p_data->search_res.start_handle,p_data->search_res.end_handle,p_data->search_res.srvc_id.uuid.uuid.uuid16);
        spp_srv_start_handle = p_data->search_res.start_handle;
        spp_srv_end_handle = p_data->search_res.end_handle;    
        break;    
    case ESP_GATTC_READ_DESCR_EVT:
        ESP_LOGI(GATT_TAG, "READ DESCR EVT");
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        ESP_LOGI(GATT_TAG,"ESP_GATTC_WRITE_DESCR_EVT: status =%d,handle = %d", p_data->write.status, p_data->write.handle);
        if(p_data->write.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATT_TAG, "ESP_GATTC_WRITE_DESCR_EVT, error status = %d", p_data->write.status);
            break;
        }
        switch(cmd)
        {
        case SPP_IDX_SPP_DATA_NTY_VAL:
            ESP_LOGI(GATT_TAG, "DATA NTY VAL EVT");
            cmd = SPP_IDX_SPP_STATUS_VAL;
            xQueueSend(cmd_reg_queue, &cmd, 10 / portTICK_PERIOD_MS);
            break;
        case SPP_IDX_SPP_STATUS_VAL:
            ESP_LOGI(GATT_TAG, "STATUS VAL EVT");
            break;
        default:
            break;
        };
        break;
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGI(GATT_TAG,"ESP_GATTC_NOTIFY_EVT");
        notify_event_handler(p_data);
        break;
    case ESP_GATTC_PREP_WRITE_EVT:
        ESP_LOGI(GATT_TAG, "PREP WRITE EVT");
        break;
    case ESP_GATTC_EXEC_EVT:
        ESP_LOGI(GATT_TAG, "EXEC EVT");
        break;
    case ESP_GATTC_ACL_EVT:
        ESP_LOGI(GATT_TAG, "ACL EVT");
        break; 
    case ESP_GATTC_CANCEL_OPEN_EVT:
        ESP_LOGI(GATT_TAG, "CANCEL OPEN EVT");
        break;
    case ESP_GATTC_SRVC_CHG_EVT:
        ESP_LOGI(GATT_TAG, "SRVC CHG EVT");
        break;
    case ESP_GATTC_ENC_CMPL_CB_EVT:
        ESP_LOGI(GATT_TAG, "ENC CMPL CB EVT");
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if(p_data->cfg_mtu.status != ESP_OK){
            break;
        }
        ESP_LOGI(GATT_TAG,"+MTU:%d", p_data->cfg_mtu.mtu);
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
    case ESP_GATTC_ADV_DATA_EVT:
    ESP_LOGI(GATT_TAG, "ADV DATA EVT");
        break;
    case ESP_GATTC_MULT_ADV_ENB_EVT:
    ESP_LOGI(GATT_TAG, "MULT ADV ENB EVT");
        break;    
    case ESP_GATTC_MULT_ADV_UPD_EVT:
    ESP_LOGI(GATT_TAG, "MULT ADV UPD EVT");
        break;
    case ESP_GATTC_MULT_ADV_DATA_EVT:
        ESP_LOGI(GATT_TAG, "MULT ADV DATA EVT");
        break;
    case ESP_GATTC_MULT_ADV_DIS_EVT:
        ESP_LOGI(GATT_TAG, "MULT ADV DIS EVT");
        break;
    case ESP_GATTC_CONGEST_EVT:
        ESP_LOGI(GATT_TAG, "CONGEST EVT");
        break;
    case ESP_GATTC_BTH_SCAN_ENB_EVT:
        ESP_LOGI(GATT_TAG, "BTH SCAN ENB EVT");
        break;
    case ESP_GATTC_BTH_SCAN_CFG_EVT:
        ESP_LOGI(GATT_TAG, "BTH SCAN CFG EVT");
        break; 
    case ESP_GATTC_BTH_SCAN_RD_EVT:
        ESP_LOGI(GATT_TAG, "BTH SCAN RD EVT");
        break;
    case ESP_GATTC_BTH_SCAN_THR_EVT:
        ESP_LOGI(GATT_TAG, "BTH SCN THR EVT");
        break;
    case ESP_GATTC_BTH_SCAN_PARAM_EVT:
        ESP_LOGI(GATT_TAG, "BTH SCAN PARAM EVT");
        break;
    case ESP_GATTC_BTH_SCAN_DIS_EVT:
        ESP_LOGI(GATT_TAG, "BTH SCAN DIS EVT");
        break;
    case ESP_GATTC_SCAN_FLT_CFG_EVT:
        ESP_LOGI(GATT_TAG, "SCAN FLT CFG EVT");
        break;
    case ESP_GATTC_SCAN_FLT_PARAM_EVT:
        ESP_LOGI(GATT_TAG, "SCAN FLT PARAM EVT");
        break;    
    case ESP_GATTC_SCAN_FLT_STATUS_EVT:
        ESP_LOGI(GATT_TAG, "SCAN FLT STATUS EVT");
        break;
    case ESP_GATTC_ADV_VSC_EVT:
        ESP_LOGI(GATT_TAG, "ADV VSC EVT");
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
        ESP_LOGI(GATT_TAG,"Index = %d,status = %d,handle = %d",cmd, p_data->reg_for_notify.status, p_data->reg_for_notify.handle);
        if(p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATT_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT, status = %d", p_data->reg_for_notify.status);
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
    case ESP_GATTC_UNREG_FOR_NOTIFY_EVT:
        ESP_LOGI(GATT_TAG, "UNREG FOR NOTIFY EVT");
        break;
    case ESP_GATTC_CONNECT_EVT:
        ESP_LOGI(GATT_TAG, "ESP_GATTC_CONNECT_EVT: conn_id=%d, gatt_if = %d", spp_conn_id, gattc_if);
        ESP_LOGI(GATT_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATT_TAG, address_pm, sizeof(esp_bd_addr_t));
        spp_gattc_if = gattc_if;
        is_con = true;
        spp_conn_id = p_data->connect.conn_id;
        memcpy(&address_pm, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        esp_ble_gattc_search_service(spp_gattc_if, spp_conn_id, &spp_service_uuid);    
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(GATT_TAG, "disconnect");
        free_gattc_srv_db();
        esp_ble_gap_start_scanning(SCAN_ALL_THE_TIME);    
        break; 
    case ESP_GATTC_READ_MULTIPLE_EVT:
        ESP_LOGI(GATT_TAG, "READ MULTIPLE EVT");
        break;
    case ESP_GATTC_QUEUE_FULL_EVT:
        ESP_LOGI(GATT_TAG, "QUEUE FULL EVT");
        break;
    case ESP_GATTC_SET_ASSOC_EVT:
        ESP_LOGI(GATT_TAG, "SET ASSOC EVT");
        break;
    case ESP_GATTC_GET_ADDR_LIST_EVT:
        ESP_LOGI(GATT_TAG, "GET ADDR LIST EVT");
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        ESP_LOGI(GATT_TAG, "DIS SRVC CMPL EVT");
        break;
    default:
        break;
    }
}

/**
 * @brief Initialize the Serial Port Profile (SPP) UART communication.
 *
 * This function sets up the necessary configurations for UART communication,
 * creates a command registration queue, and spawns tasks for SPP client registration
 * and UART communication.
 */
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