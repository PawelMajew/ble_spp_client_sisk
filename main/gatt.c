// ///////////////////////////////////////////////////////////////////////////////////
// // File: gatt.c
// //
// // Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
// //
// // Author: Paweł Majewski
// //
// ///////////////////////////////////////////////////////////////////////////////////

// ///////////////////////////////////////////////////////////////////////////////////
// // HEDER FILES
// ///////////////////////////////////////////////////////////////////////////////////

// #include "gatt.h"

// ///////////////////////////////////////////////////////////////////////////////////
// // DEKLARATION LOCAL FUNCTIONS
// ///////////////////////////////////////////////////////////////////////////////////

// static void notify_event_handler(esp_ble_gattc_cb_param_t * p_data)
// {
//     uint8_t handle = 0;
//     if (p_data->notify.is_notify == true)
//     {
//         ESP_LOGI(GAP_TAG,"+NOTIFY:handle = %d,length = %d ", p_data->notify.handle, p_data->notify.value_len);
//     }
//     else
//     {
//         ESP_LOGI(GAP_TAG,"+INDICATE:handle = %d,length = %d ", p_data->notify.handle, p_data->notify.value_len);
//     }
//     handle = p_data->notify.handle;
//     if (db == NULL) 
//     {
//         ESP_LOGE(GAP_TAG, " %s db is NULL", __func__);
//         return;
//     }
//     if (handle == db[SPP_IDX_SPP_DATA_NTY_VAL].attribute_handle)
//     {
//         if ((p_data->notify.value[0] == '#') && (p_data->notify.value[1] == '#'))
//         {
//             if ((++notify_value_count) != p_data->notify.value[3])
//             {
//                 if (notify_value_p != NULL)
//                 {
//                     free(notify_value_p);
//                 }
//                 notify_value_count = 0;
//                 notify_value_p = NULL;
//                 notify_value_offset = 0;
//                 ESP_LOGE(GAP_TAG,"notify value count is not continuous,%s",__func__);
//                 return;
//             }
//             if (p_data->notify.value[3] == 1)
//             {
//                 notify_value_p = (char *)malloc(((spp_mtu_size-7)*(p_data->notify.value[2]))*sizeof(char));
//                 if (notify_value_p == NULL)
//                 {
//                     ESP_LOGE(GAP_TAG, "malloc failed,%s L#%d",__func__,__LINE__);
//                     notify_value_count = 0;
//                     return;
//                 }
//                 memcpy((notify_value_p + notify_value_offset),(p_data->notify.value + 4),(p_data->notify.value_len - 4));
//                 if (p_data->notify.value[2] == p_data->notify.value[3])
//                 {
//                     uart_write_bytes(UART_NUM_0, (char *)(notify_value_p), (p_data->notify.value_len - 4 + notify_value_offset));
//                     free(notify_value_p);
//                     notify_value_p = NULL;
//                     notify_value_offset = 0;
//                     return;
//                 }
//                 notify_value_offset += (p_data->notify.value_len - 4);
//             }
//             else if(p_data->notify.value[3] <= p_data->notify.value[2])
//             {
//                 memcpy((notify_value_p + notify_value_offset),(p_data->notify.value + 4),(p_data->notify.value_len - 4));
//                 if(p_data->notify.value[3] == p_data->notify.value[2])
//                 {
//                     uart_write_bytes(UART_NUM_0, (char *)(notify_value_p), (p_data->notify.value_len - 4 + notify_value_offset));
//                     free(notify_value_p);
//                     notify_value_count = 0;
//                     notify_value_p = NULL;
//                     notify_value_offset = 0;
//                     return;
//                 }
//                 notify_value_offset += (p_data->notify.value_len - 4);
//             }
//         }
//         else
//         {
//             uart_write_bytes(UART_NUM_0, (char *)(p_data->notify.value), p_data->notify.value_len);
//         }
//     }
//     else if (handle == ((db+SPP_IDX_SPP_STATUS_VAL)->attribute_handle))
//     {
//         esp_log_buffer_char(GAP_TAG, (char *)p_data->notify.value, p_data->notify.value_len);
//     }
//     else
//     {
//         esp_log_buffer_char(GAP_TAG, (char *)p_data->notify.value, p_data->notify.value_len);
//     }
// }

// void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
// {
//     esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

//     switch (event) {
//     case ESP_GATTC_REG_EVT:
//         ESP_LOGI(GATT_TAG, "REG EVT, set scan params");
//         esp_ble_gap_set_scan_params(&ble_scan_params);
//         break;
//     case ESP_GATTC_CONNECT_EVT:
//         ESP_LOGI(GATT_TAG, "ESP_GATTC_CONNECT_EVT: conn_id=%d, gatt_if = %d", spp_conn_id, gattc_if);
//         ESP_LOGI(GATT_TAG, "REMOTE BDA:");
//         esp_log_buffer_hex(GATT_TAG, gl_profile_tab[APP_ID].remote_bda, sizeof(esp_bd_addr_t));
//         spp_gattc_if = gattc_if;
//         is_con = true;
//         spp_conn_id = p_data->connect.conn_id;
//         memcpy(gl_profile_tab[APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
//         esp_ble_gattc_search_service(spp_gattc_if, spp_conn_id, &spp_service_uuid);
//         break;
//     case ESP_GATTC_DISCONNECT_EVT:
//         ESP_LOGI(GATT_TAG, "disconnect");
//         free_gattc_srv_db();
//         esp_ble_gap_start_scanning(SCAN_ALL_THE_TIME);
//         break;
//     case ESP_GATTC_SEARCH_RES_EVT:
//         ESP_LOGI(GATT_TAG, "ESP_GATTC_SEARCH_RES_EVT: start_handle = %d, end_handle = %d, UUID:0x%04x",p_data->search_res.start_handle,p_data->search_res.end_handle,p_data->search_res.srvc_id.uuid.uuid.uuid16);
//         spp_srv_start_handle = p_data->search_res.start_handle;
//         spp_srv_end_handle = p_data->search_res.end_handle;
//         break;
//     case ESP_GATTC_SEARCH_CMPL_EVT:
//         ESP_LOGI(GATT_TAG, "SEARCH_CMPL: conn_id = %x, status %d", spp_conn_id, p_data->search_cmpl.status);
//         esp_ble_gattc_send_mtu_req(gattc_if, spp_conn_id);
//         break;
//     case ESP_GATTC_REG_FOR_NOTIFY_EVT:
//         ESP_LOGI(GATT_TAG,"Index = %d,status = %d,handle = %d",cmd, p_data->reg_for_notify.status, p_data->reg_for_notify.handle);
//         if(p_data->reg_for_notify.status != ESP_GATT_OK){
//             ESP_LOGE(GATT_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT, status = %d", p_data->reg_for_notify.status);
//             break;
//         }
//         uint16_t notify_en = 1;
//         esp_ble_gattc_write_char_descr(
//                 spp_gattc_if,
//                 spp_conn_id,
//                 (db+cmd+1)->attribute_handle,
//                 sizeof(notify_en),
//                 (uint8_t *)&notify_en,
//                 ESP_GATT_WRITE_TYPE_RSP,
//                 ESP_GATT_AUTH_REQ_NONE);

//         break;
//     case ESP_GATTC_NOTIFY_EVT:
//         ESP_LOGI(GATT_TAG,"ESP_GATTC_NOTIFY_EVT");
//         notify_event_handler(p_data);
//         break;
//     case ESP_GATTC_READ_CHAR_EVT:
//         ESP_LOGI(GATT_TAG,"ESP_GATTC_READ_CHAR_EVT");
//         break;
//     case ESP_GATTC_WRITE_CHAR_EVT:
//         ESP_LOGI(GATT_TAG,"ESP_GATTC_WRITE_CHAR_EVT:status = %d,handle = %d", param->write.status, param->write.handle);
//         if(param->write.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATT_TAG, "ESP_GATTC_WRITE_CHAR_EVT, error status = %d", p_data->write.status);
//             break;
//         }
//         break;
//     case ESP_GATTC_PREP_WRITE_EVT:
//         break;
//     case ESP_GATTC_EXEC_EVT:
//         break;
//     case ESP_GATTC_WRITE_DESCR_EVT:
//         ESP_LOGI(GATT_TAG,"ESP_GATTC_WRITE_DESCR_EVT: status =%d,handle = %d", p_data->write.status, p_data->write.handle);
//         if(p_data->write.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATT_TAG, "ESP_GATTC_WRITE_DESCR_EVT, error status = %d", p_data->write.status);
//             break;
//         }
//         switch(cmd)
//         {
//         case SPP_IDX_SPP_DATA_NTY_VAL:
//             cmd = SPP_IDX_SPP_STATUS_VAL;
//             xQueueSend(cmd_reg_queue, &cmd, 10 / portTICK_PERIOD_MS);
//             break;
//         case SPP_IDX_SPP_STATUS_VAL:
//             break;
//         default:
//             break;
//         };
//         break;
//     case ESP_GATTC_CFG_MTU_EVT:
//         if(p_data->cfg_mtu.status != ESP_OK){
//             break;
//         }
//         spp_mtu_size = p_data->cfg_mtu.mtu;

//         db = (esp_gattc_db_elem_t *)malloc(count*sizeof(esp_gattc_db_elem_t));
//         if(db == NULL){
//             break;
//         }
//         if(esp_ble_gattc_get_db(spp_gattc_if, spp_conn_id, spp_srv_start_handle, spp_srv_end_handle, db, &count) != ESP_GATT_OK){
//             break;
//         }
//         if(count != SPP_IDX_NB){
//             break;
//         }
//         cmd = SPP_IDX_SPP_DATA_NTY_VAL;
//         xQueueSend(cmd_reg_queue, &cmd, 10/portTICK_PERIOD_MS);
//         break;
//     case ESP_GATTC_SRVC_CHG_EVT:
//         break;
//     default:
//         break;
//     }  
// }