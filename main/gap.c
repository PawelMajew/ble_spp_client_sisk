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
// GLOBAL DATA
///////////////////////////////////////////////////////////////////////////////////

uint16_t esp_if_pm;
bool is_con = false;

///////////////////////////////////////////////////////////////////////////////////
// LOCAL DATA
///////////////////////////////////////////////////////////////////////////////////

static esp_ble_gap_cb_param_t scan_rst;

static const char device_name[] = "PM_SERVER";

///////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback function for handling Bluetooth Low Energy (BLE) GAP events.
 *
 * This function is invoked in response to various BLE GAP events. It switches
 * on the event type and performs corresponding actions or processing.
 *
 * @param event   The type of BLE GAP event.
 * @param param   A pointer to the parameters associated with the event.
 */
void esp_gap_handle(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
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
            esp_ble_gattc_open(esp_if_pm, scan_rst.scan_rst.bda, scan_rst.scan_rst.ble_addr_type, true);
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

